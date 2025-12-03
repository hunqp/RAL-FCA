/************************************************************************
 **
 ** HungPNQ
 ** 
 ** 14/07/2025
 ** 
 ** RTSPD555
 **
 ** Description: RTSP Server base on LIVE555
 **
 ************************************************************************
 */
#ifndef RTSPD555_HH
#define RTSPD555_HH

#include <vector>
#include <string>
#include <stdint.h>
#include <stdbool.h>
#include <functional>

enum {
    RTSPD555_MEDIA_ENCODER_NONE_ID = 0,
    RTSPD555_VIDEO_ENCODER_H264_ID,
    RTSPD555_VIDEO_ENCODER_H265_ID,
    RTSPD555_AUDIO_ENCODER_ALAW_ID,
    RTSPD555_AUDIO_ENCODER_ULAW_ID,
};

typedef struct {
    bool bIdr;
    int encoder;
    uint32_t seqId;
    int framePerSeconds;
    std::vector<uint8_t> sample;
    uint64_t timestamp;
} RTSPD555_FRAMER_S;

typedef struct {
    struct {
        int encoder;
        std::function<int(RTSPD555_FRAMER_S*)> doGettingFramed;
    } video;
    struct {
        int encoder;
        int channels;
        int bitsamples;
        int samplerate;
        std::function<int(RTSPD555_FRAMER_S*)> doGettingFramed;
    } audio;
    std::string streamName;
    std::string description;
} RSTPD555_MEDIA_SESSION_S;

class RTSPD555 {
public:
     RTSPD555();
    ~RTSPD555();

    virtual bool begin(int usPortnum);
    virtual void close();
    virtual void run();
    virtual void addMediaSubsession(RSTPD555_MEDIA_SESSION_S &ms);
    virtual void onOpened(std::function<void(void)> callback);
    virtual void onClosed(std::function<void(void)> callback);

private:
    void *mInstance = NULL;

};

#endif /* RTSPD555_HH */
