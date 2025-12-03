#ifndef _H264_CUSTOMIZE_HH_
#define _H264_CUSTOMIZE_HH_

#include "Media.hh"
#include "rtspd555.hh"
#include "FramedSource.hh"
#include "FileServerMediaSubsession.hh"

#define H264_NALU_STARTCODE_LEN	(4)

class H264FramedSource: public FramedSource {
public:
	static H264FramedSource *createNew(UsageEnvironment &env, char const *filename);
	virtual void doGetNextFrame(void);
	void addDoGettingFramed(std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame);

protected:
	H264FramedSource(UsageEnvironment &env, const char *filename);
	virtual ~H264FramedSource(void);

private:
    Boolean fState = False;
	Boolean fWaitIdr = True;
	char *fFilename = NULL;
	uint64_t fLastPlayTime = 0;
	uint64_t fFirstTimeStamp = 0;
	uint32_t fOffset;
	RTSPD555_FRAMER_S fFrameBuffer;
	std::function<int(RTSPD555_FRAMER_S*)> fDoGettingFramed;

	Boolean isSpsPpSIdrFrames(uint8_t *pFrame);
};

class VideoH264Subsession: public FileServerMediaSubsession {
public:
	static VideoH264Subsession *createNew(UsageEnvironment &env, 
		const char *filename, Boolean reuseFirstSource,
		std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame);

protected:
	virtual ~VideoH264Subsession(void) {};
	VideoH264Subsession(UsageEnvironment &env, 
		const char *filename, Boolean reuseFirstSource, 
		std::function<int(RTSPD555_FRAMER_S*)> doGettingFrame);

protected:
	virtual FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate);
	virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock, 
		unsigned char rtpPayloadTypeIfDynamic, 
		FramedSource *inputSource);

private:
	std::function<int(RTSPD555_FRAMER_S*)> fDoGettingFramed;
};

#endif /* _H264_CUSTOMIZE_HH_ */