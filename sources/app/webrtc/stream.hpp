#ifndef STREAM_HPP
#define STREAM_HPP

#include <pthread.h>
#include "rtc/rtc.hpp"
#include "mediabuffer.h"

#define VIDEO_FPS           (13)
#define AUDIO_SAMPLERATE    (8000)

#define VIDEO_LIVESTREAM_FPS				(13)
#define VIDEO_LIVESTREAM_SAMPLE_DURATION_US  (VIDEO_LIVESTREAM_FPS / VIDEO_LIVESTREAM_FPS)
#define AUDIO_LIVESTREAM_SAMPLE_DURATION_US	(40000)
#define VIDEO_PLAYBACK_DURATION_US			(1000000 / VIDEO_LIVESTREAM_FPS)
#define AUDIO_PLAYBACK_DURATION_US		    (40000)
#define AUDIO_PLAYBACK_SAMPLE_SIZE          (320)

typedef enum {
    PB_CTL_PLAY,
    PB_CTL_PAUSE,
    PB_CTL_STOP,
    PB_CTL_RESUME,
    PB_CTL_SEEK,
    PB_CTL_SPEED,
} PLAYBACK_CONTROL;

typedef enum {
    PB_STATE_PLAYING,
    PB_STATE_PAUSING,
    PB_STATE_STOPPED,
    PB_STATE_RESUMING,
    PB_STATE_SEEKING,
    PB_STATE_SPEEDING,
    PB_STATE_DONE,
    PB_STATE_ERROR,
} PLAYBACK_STATUS;

#define PB_SPEED_SLOW05  (0)
#define PB_SPEED_NORMAL  (1)
#define PB_SPEED_FASTx2  (2)
#define PB_SPEED_FASTx4  (3)
#define PB_SPEED_FASTx8  (4)
#define PB_SPEED_FASTx16 (5)

class StreamSource {
protected:
    rtc::binary sample = {};
    uint64_t sampleTime_us = 0;
	uint64_t sampleDuration_us = 0;
    VV_RB_MEDIA_HANDLE_T consumer = NULL;

public:
     StreamSource(VV_RB_MEDIA_HANDLE_T producer, uint32_t preallocateBufferSize);
	~StreamSource();

    virtual void stop();
    virtual void start();
    virtual void loadNextSample();
    virtual rtc::binary getSample();
    virtual uint64_t getSampleTime_us();
	virtual uint64_t getSampleDuration_us();
};

class Stream: public std::enable_shared_from_this<Stream> {
public:
    enum class StreamSourceType {
        Audio,
        Video
    };

    std::shared_ptr<StreamSource> audio0 = nullptr;
	std::shared_ptr<StreamSource> video0 = nullptr;
	std::shared_ptr<StreamSource> video1 = nullptr;

     Stream(std::shared_ptr<StreamSource> video0, std::shared_ptr<StreamSource> video1, std::shared_ptr<StreamSource> audio0);
	~Stream();

    void start();
    void stop();

private:
	pthread_mutex_t mMutex;
};

extern uint64_t currentTimeInMicroSeconds();

#endif /* __STREAM_HPP */

/*
-------------------------------------------------------------------------------------
| Design for smooth audio:
| Calculate sample duration in microsecond for each 1024 Bytes audio codec sent.
-------------------------------------------------------------------------------------
Audio ALaw Parameters:
	-Sample rate: 8000 (Hz)
	-Bit format : 16 Bit

	8000(Bytes)		--->	1000000(us)
	1024 (Bytes)	--->	X (us)

	X = (1024 * 1000000) / 8000 = 128000(us)
*/
