#ifndef __VIDEO_SOURCE_H__
#define __VIDEO_SOURCE_H__

#include "stream.hpp"

class VideoSource: public StreamSource {
private:
	void splitNalus(uint8_t *buffer, size_t length);
	void appendSample(uint8_t *buffer, size_t length);
	
public:
     VideoSource(VV_RB_MEDIA_HANDLE_T producer, uint64_t sampleDurationUs);
    ~VideoSource();

    void stop() override;
	void start() override;
	void loadNextSample() override;
    bool isEncoderH264();
	bool isSpsPpsIdrOrVpsSpsPpsIdr();
	rtc::binary getInitialNalus();

private:
	bool mIsEncoderH264;
	bool mSpsPpsIdrOrVpsSpsPpsIdr;
	rtc::binary mInitialNalus = {};
};

#endif /* __VIDEO_SOURCE_H__ */
