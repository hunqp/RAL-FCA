#include <signal.h>
#include "rtspd.h"
#include "fca_video.hpp"
#include "fca_audio.hpp"

RTSPD::RTSPD() : RTSPD555() {
    signal(SIGPIPE, SIG_IGN);
}

RTSPD::~RTSPD() {
    
}

bool RTSPD::begin(int usPortnum) {
    mConsumerGettingVIDEO0 = ringBufferCreateConsumer(VideoHelpers::VIDEO0_PRODUCER, 250 * 1024);
    mConsumerGettingVIDEO1 = ringBufferCreateConsumer(VideoHelpers::VIDEO1_PRODUCER, 250 * 1024);
    mConsumerGettingAUDIO0 = ringBufferCreateConsumer(AudioHelpers::AUDIO0_PRODUCER, 320);
    return RTSPD555::begin(usPortnum);
}

void RTSPD::addMediaSession0(std::string streamName, std::string description, 
    int videoEncoder, int audioEncoder, int channels, int bitsamples, int samplerate)
{
    RSTPD555_MEDIA_SESSION_S ms;
    ms.streamName.assign(streamName);
    ms.description.assign(description);
    ms.audio.channels = channels;
    ms.audio.bitsamples = bitsamples;
    ms.audio.samplerate = samplerate;
    ms.audio.encoder = audioEncoder;
    ms.video.encoder = videoEncoder;

    ms.audio.doGettingFramed = [&](RTSPD555_FRAMER_S *Frame) -> int {
        int rc = -1;
        VV_RB_MEDIA_FRAMED_S resource;
        rc = ringBufferReadFrom(mConsumerGettingAUDIO0, &resource);
        if (rc == 0) {
            Frame->sample.clear();
            Frame->bIdr = false;
            Frame->timestamp = resource.timestamp;
            Frame->encoder = (resource.encoder == VV_RB_MEDIA_ENCODER_FRAME_ALAW) ? RTSPD555_AUDIO_ENCODER_ALAW_ID : RTSPD555_AUDIO_ENCODER_ULAW_ID;
            Frame->framePerSeconds = resource.framePerSeconds;
            Frame->sample.insert(Frame->sample.end(), 
                reinterpret_cast<uint8_t*>(resource.pData), 
                reinterpret_cast<uint8_t*>(resource.pData) + resource.dataLen);
            return Frame->sample.size();
        }
        return 0;
    };

    ms.video.doGettingFramed = [&](RTSPD555_FRAMER_S *Frame) -> int {
        int rc = -1;
        static uint32_t lastSeqId = 0;
        VV_RB_MEDIA_FRAMED_S resource;
        rc = ringBufferReadFrom(mConsumerGettingVIDEO0, &resource);
        if (rc == 0) {
            if (resource.id > (lastSeqId + 1)) {
                int lost = resource.id - lastSeqId;
			    printf("-> Video 0 samples sequence (newest: %d, oldest: %d) has missed (lost %d)\r\n", resource.id, lastSeqId, lost);
            }
            lastSeqId = resource.id;

            Frame->sample.clear();
            Frame->seqId = resource.id;
            Frame->bIdr = (resource.type == VV_RB_MEDIA_FRAME_HDR_TYPE_I) ? true : false;
            Frame->timestamp = resource.timestamp;
            Frame->encoder = (resource.encoder == VV_RB_MEDIA_ENCODER_FRAME_H264) ? RTSPD555_VIDEO_ENCODER_H264_ID : RTSPD555_VIDEO_ENCODER_H265_ID;
            Frame->framePerSeconds = resource.framePerSeconds;
            Frame->sample.insert(Frame->sample.end(), 
                reinterpret_cast<uint8_t*>(resource.pData), 
                reinterpret_cast<uint8_t*>(resource.pData) + resource.dataLen);
            return Frame->sample.size();
        }
        return 0;
    };

    RTSPD555::addMediaSubsession(ms);
}

void RTSPD::addMediaSession1(std::string streamName, std::string description, 
    int videoEncoder, int audioEncoder, int channels, int bitsamples, int samplerate)
{
    RSTPD555_MEDIA_SESSION_S ms;
    ms.streamName.assign(streamName);
    ms.description.assign(description);
    ms.audio.channels = channels;
    ms.audio.bitsamples = bitsamples;
    ms.audio.samplerate = samplerate;
    ms.audio.encoder = audioEncoder;
    ms.video.encoder = videoEncoder;

    ms.audio.doGettingFramed = [&](RTSPD555_FRAMER_S *Frame) -> int {
        int rc = -1;
        VV_RB_MEDIA_FRAMED_S resource;
        rc = ringBufferReadFrom(mConsumerGettingAUDIO0, &resource);
        if (rc == 0) {
            static uint32_t lastSeqId = 0;
            if (resource.id > (lastSeqId + 1)) {
                int lost = resource.id - lastSeqId;
			    printf("-> Audio 0 samples sequence (newest: %d, oldest: %d) has missed (lost %d)\r\n", resource.id, lastSeqId, lost);
            }
            // printf("Audio seqId: %5d, size: %5d, timestamp: %lld\r\n", resource.id, resource.dataLen, resource.timestamp);
            lastSeqId = resource.id;
            Frame->sample.clear();
            Frame->bIdr = false;
            Frame->timestamp = resource.timestamp;
            Frame->encoder = (resource.encoder == VV_RB_MEDIA_ENCODER_FRAME_ALAW) ? RTSPD555_AUDIO_ENCODER_ALAW_ID : RTSPD555_AUDIO_ENCODER_ULAW_ID;
            Frame->framePerSeconds = resource.framePerSeconds;
            Frame->sample.insert(Frame->sample.end(), 
                reinterpret_cast<uint8_t*>(resource.pData), 
                reinterpret_cast<uint8_t*>(resource.pData) + resource.dataLen);
            return Frame->sample.size();
        }
        return 0;
    };

    ms.video.doGettingFramed = [&](RTSPD555_FRAMER_S *Frame) -> int {
        int rc = -1;
        VV_RB_MEDIA_FRAMED_S resource;
        rc = ringBufferReadFrom(mConsumerGettingVIDEO1, &resource);
        if (rc == 0) {
            Frame->sample.clear();
            Frame->seqId = resource.id;
            Frame->bIdr = (resource.type == VV_RB_MEDIA_FRAME_HDR_TYPE_I) ? true : false;
            Frame->timestamp = resource.timestamp;
            Frame->encoder = (resource.encoder == VV_RB_MEDIA_ENCODER_FRAME_H264) ? RTSPD555_VIDEO_ENCODER_H264_ID : RTSPD555_VIDEO_ENCODER_H265_ID;
            Frame->framePerSeconds = resource.framePerSeconds;
            Frame->sample.insert(Frame->sample.end(), 
                reinterpret_cast<uint8_t*>(resource.pData), 
                reinterpret_cast<uint8_t*>(resource.pData) + resource.dataLen);
            return Frame->sample.size();
        }
        return 0;
    };

    RTSPD555::addMediaSubsession(ms);
}
