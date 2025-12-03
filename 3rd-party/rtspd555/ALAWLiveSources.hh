#ifndef _ALAW_CUSTOMIZE_HH_
#define _ALAW_CUSTOMIZE_HH_

#include "Media.hh"
#include "rtspd555.hh"
#include "FramedSource.hh"
#include "FileServerMediaSubsession.hh"

class ALAWFrameSource: public FramedSource {
public:
	static ALAWFrameSource *createNew(UsageEnvironment &env, char const *filename, 
		unsigned numChannels, unsigned samplingFrequency, unsigned bitPerSample);
	unsigned numChannels();
    unsigned bitsPerSample();
	unsigned samplingFrequency();
	void addDoGettingFramed(std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame);

protected:
	ALAWFrameSource(UsageEnvironment &env, const char *filename, 
		unsigned numChannels, unsigned samplingFrequency, 
		unsigned bitPerSample);
	virtual ~ALAWFrameSource(void);

private:
	virtual void doGetNextFrame(void);

private:
    Boolean fState = False;
	char *fFilename = NULL;
	Boolean fLimitNumBytesToStream = False;
	unsigned fNumBytesToStream = 0;
	unsigned fNumChannels = 0;
	unsigned fSamplingFrequency = 0;
	unsigned fBitsPerSample = 0;
	unsigned fPreferredFrameSize = 0;
	unsigned fLastPlayTime = 0;
	unsigned fPlayTimePerSample = 0;
	uint64_t fFirstTimeStamp = 0;
	RTSPD555_FRAMER_S fFrameBuffer;
	std::function<int(RTSPD555_FRAMER_S*)> fDoGettingFramed;
};

class AudioALAWSubsession: public FileServerMediaSubsession {
public:
	static AudioALAWSubsession *createNew(UsageEnvironment &env, 
		const char *filename, Boolean reuseFirstSource, 
		std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame);

public:
	void setParameters(unsigned numChannels, unsigned samplingFrequency, 
		unsigned bitPerSample);

protected:
	virtual ~AudioALAWSubsession(void) {};
	AudioALAWSubsession(UsageEnvironment &env, 
		const char *filename, Boolean reuseFirstSource, 
		std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame);

protected:
	virtual FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate);
	virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock, 
		unsigned char rtpPayloadTypeIfDynamic, 
		FramedSource *inputSource);

protected:
	unsigned fNumChannels = 0;
	unsigned fSamplingFrequency = 0;
	unsigned char fBitsPerSample = 0;

private:
	std::function<int(RTSPD555_FRAMER_S*)> fDoGettingFramed;
};

#endif /* _ALAW_CUSTOMIZE_HH_ */