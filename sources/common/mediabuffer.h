#ifndef VVC_RING_BUFFER_H
#define VVC_RING_BUFFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VV_RB_MEDIA_PRODUCER = 0x00,
	VV_RB_MEDIA_CONSUMER = 0x01,
} VV_RB_MEDIA_ROLES;

typedef enum {
    VV_RB_MEDIA_ENCODER_FRAME_H264 = 0x00,
    VV_RB_MEDIA_ENCODER_FRAME_H265,
    VV_RB_MEDIA_ENCODER_FRAME_ALAW,
    VV_RB_MEDIA_ENCODER_FRAME_ULAW,
    VV_RB_MEDIA_ENCODER_FRAME_AAC,
    VV_RB_MEDIA_ENCODER_FRAME_PCM,
} VV_RB_MEDIA_ENCODE_TYPE;

typedef enum {
    VV_RB_MEDIA_FRAME_HDR_TYPE_I = 0x00,
    VV_RB_MEDIA_FRAME_HDR_TYPE_PB,
    VV_RB_MEDIA_FRAME_HDR_TYPE_AUDIO,
} VV_RB_MEDIA_FRAME_HDR_TYPE;

typedef struct {
    uint32_t id;
    uint8_t *pData;
    uint32_t dataLen;
    uint64_t timestamp;
    uint32_t framePerSeconds;
    VV_RB_MEDIA_FRAME_HDR_TYPE type;
    VV_RB_MEDIA_ENCODE_TYPE encoder;
} VV_RB_MEDIA_FRAMED_S;

typedef void * VV_RB_MEDIA_HANDLE_T;

extern VV_RB_MEDIA_HANDLE_T ringBufferCreateProducer(uint32_t wantedSize);
extern VV_RB_MEDIA_HANDLE_T ringBufferCreateConsumer(VV_RB_MEDIA_HANDLE_T pHdl, uint32_t preallocateSize);
extern void ringBufferDelete(VV_RB_MEDIA_HANDLE_T pHdl);
extern int ringBufferSendTo(VV_RB_MEDIA_HANDLE_T pHdl, VV_RB_MEDIA_FRAMED_S *pFramer);
extern int ringBufferReadFrom(VV_RB_MEDIA_HANDLE_T pHdl, VV_RB_MEDIA_FRAMED_S *pFramer);
extern int ringBufferSeekTo(VV_RB_MEDIA_HANDLE_T pHdl, uint64_t timestamp);

#ifdef __cplusplus
}
#endif

#endif /* VVC_RING_BUFFER_H */
