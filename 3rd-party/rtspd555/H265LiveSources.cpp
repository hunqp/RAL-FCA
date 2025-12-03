#include <assert.h>
#include "BasicUsageEnvironment.hh"
#include "H265VideoRTPSink.hh"
#include "SimpleRTPSink.hh"
#include "MPEG4GenericRTPSink.hh"
#include "liveMedia.hh"
#include "H265LiveSources.hh"
#include "common.hh"

H265FramedSource *H265FramedSource::createNew(UsageEnvironment &env, char const *filename) {
	return new H265FramedSource(env, filename);
}

H265FramedSource::H265FramedSource(UsageEnvironment &env, const char *filename) : FramedSource(env) {
    fState = True;
}

H265FramedSource::~H265FramedSource(void) {
	fState = False;
}

void H265FramedSource::addDoGettingFramed(std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame) {
    fDoGettingFramed = doGettingFrame;
}

Boolean H265FramedSource::isVpsSpsPpsIdrFrames(uint8_t *dataPtr) {
    H265NalUnitHeader *hdr = (H265NalUnitHeader *)(dataPtr + H265_NALU_STARTCODE_LEN);
    return (hdr->unitType() == 32) ? True : False;
}

void H265FramedSource::doGetNextFrame() {
    fFrameSize = 0;

    /* Fetch a new frame sample if none is available */
    if (fFrameBuffer.sample.empty()) {
        size_t readBytes = fDoGettingFramed(&fFrameBuffer);
        if (readBytes <= H265_NALU_STARTCODE_LEN) {
            fFrameBuffer.sample = {};
            envir().taskScheduler().scheduleDelayedTask(4000, (TaskFunc*)FramedSource::afterGetting, this);
            return;
        }
        fOffset = 0;
    }

    /* Wait IDR frame for first entrance */
    if (fWaitIdr) {
        if (fFrameBuffer.bIdr) {
            fWaitIdr = False;
        }
		else {
            fFrameBuffer.sample = {};
            envir().taskScheduler().scheduleDelayedTask(4000, (TaskFunc*)FramedSource::afterGetting, this);
            return;
        }
    }

    /* Fetch one nalu from buffers */
    NaluUnit nalu;
	unsigned int naluLen;
	uint64_t timestamp = fFrameBuffer.timestamp;
    
    naluLen = readOneNaluFromBuffer(fFrameBuffer.sample.data(), fFrameBuffer.sample.size(), fOffset, nalu);
    if (naluLen == 0) {
        fFrameBuffer.sample = {};
        envir().taskScheduler().scheduleDelayedTask(4000, (TaskFunc*)FramedSource::afterGetting, this);
        return;
    }

    /* We ignore start code */
    naluLen -= H265_NALU_STARTCODE_LEN;

    if (naluLen > fMaxSize) {
        fNumTruncatedBytes = naluLen - fMaxSize;
        fFrameSize = fMaxSize;
        envir() << "Truncate bytes (fMaxSize: " << fMaxSize << ", DataLen: " << naluLen << ")\r\n";
    }
	else {
        fFrameSize = naluLen;
        fNumTruncatedBytes = 0;
    }

    gettimeofday(&fPresentationTime, nullptr);
    memcpy(fTo, nalu.data, fFrameSize);
    fOffset += naluLen;

    if (fFirstTimeStamp == 0) {
        fFirstTimeStamp = 1;
        fLastPlayTime = timestamp;
        fDurationInMicroseconds = 1000000 / fFrameBuffer.framePerSeconds;
    } else {
        fDurationInMicroseconds = timestamp - fLastPlayTime;
        fLastPlayTime = timestamp;
    }

    afterGetting(this);
}

VideoH265Subsession *VideoH265Subsession::createNew(UsageEnvironment &env, const char *filename, 
    Boolean reuseFirstSource, std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame)
{
	return new VideoH265Subsession(env, filename, reuseFirstSource, doGettingFrame);
}

VideoH265Subsession::VideoH265Subsession(UsageEnvironment &env, const char *filename, 
    Boolean reuseFirstSource, std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame) 
    : FileServerMediaSubsession(env, filename, reuseFirstSource)
{
	fDoGettingFramed = doGettingFrame;
}

FramedSource *VideoH265Subsession::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate) {
	estBitrate = 500;
	H265FramedSource *streamSource = H265FramedSource::createNew(envir(), fFileName);
	if (streamSource) {
        streamSource->addDoGettingFramed(fDoGettingFramed);
		return H265VideoStreamDiscreteFramer::createNew(envir(), streamSource, False, False);
    }

	return NULL;
}

RTPSink *VideoH265Subsession::createNewRTPSink(Groupsock *rtpGroupsock, 
    unsigned char rtpPayloadTypeIfDynamic, FramedSource *inputSource) 
{
	OutPacketBuffer::maxSize = 300000;
	return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
