#ifndef __AUDIO_SOURCE_H__
#define __AUDIO_SOURCE_H__

#include "stream.hpp"

class AudioSource : public StreamSource {
public:
	 AudioSource(VV_RB_MEDIA_HANDLE_T producer, uint64_t sampleDurationUs);
	~AudioSource();

	void stop() override;
	void start() override;
	void loadNextSample() override;
};

#endif /* __AUDIO_SOURCE_H__ */
