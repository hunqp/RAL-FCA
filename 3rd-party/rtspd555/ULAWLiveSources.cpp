#include <assert.h>
#include "BasicUsageEnvironment.hh"
#include "SimpleRTPSink.hh"
#include "MPEG4GenericRTPSink.hh"
#include "liveMedia.hh"
#include "ULAWLiveSources.hh"

ULAWFrameSource *ULAWFrameSource::createNew(UsageEnvironment &env, char const *filename, 
	unsigned numChannels, unsigned samplingFrequency, unsigned bitPerSample) 
{
	ULAWFrameSource* newSource = new ULAWFrameSource(env, filename, numChannels, samplingFrequency, bitPerSample);
    if (newSource != NULL && newSource->bitsPerSample() == 0) {
        Medium::close(newSource);
    }
    return newSource;
}

ULAWFrameSource::ULAWFrameSource(UsageEnvironment &env, const char *filename, 
	unsigned numChannels, unsigned samplingFrequency, unsigned bitPerSample) 
	: FramedSource(env) 
{
	fState = True;
    fNumChannels = numChannels;
	fBitsPerSample = bitPerSample;
    fSamplingFrequency = samplingFrequency;
    unsigned bytesPerSample = (fNumChannels * fBitsPerSample) / 8;
    if (bytesPerSample == 0) {
        bytesPerSample = 1;
    }

	fPlayTimePerSample = 1e6 / (double)fSamplingFrequency;
	unsigned maxSamplesPerFrame = (1400 * 8) / (fNumChannels * fBitsPerSample);
	unsigned desiredSamplesPerFrame = (unsigned)(0.04 * fSamplingFrequency);
	unsigned samplesPerFrame = desiredSamplesPerFrame < maxSamplesPerFrame ? desiredSamplesPerFrame : maxSamplesPerFrame;
	fPreferredFrameSize = (samplesPerFrame * fNumChannels * fBitsPerSample);
}

unsigned ULAWFrameSource::bitsPerSample() { 
    return fBitsPerSample; 
}

unsigned ULAWFrameSource::numChannels() { 
    return fNumChannels; 
}

unsigned ULAWFrameSource::samplingFrequency() { 
    return fSamplingFrequency; 
}

void ULAWFrameSource::addDoGettingFramed(std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame) {
	fDoGettingFramed = doGettingFrame;
}

ULAWFrameSource::~ULAWFrameSource(void) {
	fState = False;
}

void ULAWFrameSource::doGetNextFrame(void) {
	size_t readBytes = 0;

	fFrameSize = 0;
	readBytes = fDoGettingFramed(&fFrameBuffer);
	
	/* Control stream input if fLimitNumBytesToStream on */
	if (fLimitNumBytesToStream && fNumBytesToStream < fMaxSize) {
		fMaxSize = fNumBytesToStream;
	}
	if (fPreferredFrameSize < fMaxSize) {
		fMaxSize = fPreferredFrameSize;
	}
	
	if (readBytes > 0) {
		unsigned int len = readBytes;
		uint64_t u64ts = fFrameBuffer.timestamp;
		
		if (len > fMaxSize) {
			fNumTruncatedBytes = len - fMaxSize;
			fFrameSize = fMaxSize;
		}
		else {
			fFrameSize = len;
			fNumTruncatedBytes = 0;
		}

		memcpy(fTo, fFrameBuffer.sample.data(), fFrameSize);
		gettimeofday(&fPresentationTime, NULL);

		if (fFirstTimeStamp == 0) {
			fFirstTimeStamp = 1;
			fLastPlayTime = u64ts;
			unsigned bytesPerSample = (fNumChannels * fBitsPerSample) / 8;
			if (bytesPerSample == 0) {
                bytesPerSample = 1;
            }
			fDurationInMicroseconds = (unsigned)((fPlayTimePerSample * fFrameSize) / bytesPerSample);
		} 
		else {
			fDurationInMicroseconds = u64ts - fLastPlayTime;
			fLastPlayTime = u64ts;
		}
	}

	envir().taskScheduler().scheduleDelayedTask(4000, (TaskFunc*)FramedSource::afterGetting, this);
}

AudioULAWSubsession *AudioULAWSubsession::createNew(UsageEnvironment &env, 
	const char *filename, Boolean reuseFirstSource,
	std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame)
{
	return new AudioULAWSubsession(env, filename, reuseFirstSource, doGettingFrame);
}

AudioULAWSubsession::AudioULAWSubsession(UsageEnvironment &env, 
	const char *filename, Boolean reuseFirstSource,
	std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame) 
	: FileServerMediaSubsession(env, filename, reuseFirstSource)
{
    fDoGettingFramed = doGettingFrame;
}

void AudioULAWSubsession::setParameters(unsigned numChannels, unsigned samplingFrequency, unsigned bitPerSample) {
    this->fNumChannels = numChannels;
    this->fBitsPerSample = bitPerSample;
    this->fSamplingFrequency = samplingFrequency;
}

FramedSource *AudioULAWSubsession::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate) {
	FramedSource* resultSource = NULL;
	do {
		ULAWFrameSource *streamSource = ULAWFrameSource::createNew(envir(), fFileName, fNumChannels, fSamplingFrequency, fBitsPerSample);
		if (streamSource == NULL) {
            break;
        }
		streamSource->addDoGettingFramed(fDoGettingFramed);

		/* Get attributes of the audio source: */
		fBitsPerSample = streamSource->bitsPerSample();
		if (!(fBitsPerSample == 4 || fBitsPerSample == 8 || fBitsPerSample == 16)) {
			envir() << "The input file contains " << fBitsPerSample << " bit-per-sample	audio, which we don't handle\n";
			break;
		}
		fSamplingFrequency = streamSource->samplingFrequency();
		fNumChannels = streamSource->numChannels();
		unsigned bitsPerSecond = fSamplingFrequency * fBitsPerSample * fNumChannels;

		resultSource = streamSource;

		estBitrate = (bitsPerSecond + 500) / 1000; // kbps
		return resultSource;
	} while (0);

	Medium::close(resultSource);
	return NULL;
}

RTPSink *AudioULAWSubsession::createNewRTPSink(Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource *inputSource) {
	ULAWFrameSource* ULAWSource = (ULAWFrameSource*)inputSource;
	const char *mimeType = (const char*)"PCMU";
	unsigned char payloadFormatCode;

	if (fSamplingFrequency == 8000 && fNumChannels == 1) {
		payloadFormatCode = 0; /* A static RTP payload type */
	}
    else {
		payloadFormatCode = rtpPayloadTypeIfDynamic;
	}
	return SimpleRTPSink::createNew(envir(), rtpGroupsock, payloadFormatCode, fSamplingFrequency, "audio", mimeType, fNumChannels);
}
