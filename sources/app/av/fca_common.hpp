#ifndef __HAL_AGENT_H
#define __HAL_AGENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "odm_api_common.h"
#include "odm_api_video.h"
#include "odm_api_audio.h"
#include "odm_api_gpio.h"
#include "odm_api_ircut.h"
#include "odm_api_network.h"
#include "odm_api_event.h"
#include "odm_api_ptz.h"
#include "odm_api_ircut.h"
#include "odm_api_network.h"
#include "odm_api_system.h"
#include "odm_api_isp.h"
#include "odm_api_ota.h"

typedef enum {
	HAL_RET_SUCCESS = 0,
	HAL_RET_ERR_UNKNOWN,		 /* 1 Unknown Error                         */
	HAL_RET_ERR_ACCESS,			 /* 2 Permission denied                     */
	HAL_RET_ERR_NO_MEM,			 /* 3 Not enough memory                     */
	HAL_RET_ERR_NO_SPACE,		 /* 4 Not enough space                      */
	HAL_RET_ERR_IO,				 /* 5 Other I/O error                       */
	HAL_RET_ERR_ARG,			 /* 6 Invalid argument of some other kind   */
	HAL_RET_ERR_TIMEOUT,		 /* 7 Timeout                               */
	HAL_RET_ERR_NOT_INITIALIZED, /* 8 Internal resource not initialized     */
	HAL_RET_ERR_NOT_SUPPORTED,	 /* 9 Feature not supported                 */
} HAL_RET;

typedef enum {
	MAIN_VIDEO_CHANNEL = 0,
	MINOR_VIDEO_CHANNEL,
	VIDEO_CHANNEL_TOTAL,
} HAL_VIDEO_CHANNEL;

typedef enum {
	AUDIO_CHANNEL_ALAW = 0,
	AUDIO_CHANNEL_AAC,
	AUDIO_CHANNEL_TOTAL
} HAL_AUDIO_CHANNEL;

typedef enum {
	VIDEO_ENCODING_MODE_CBR	 = FCA_VIDEO_ENCODING_MODE_CBR,
	VIDEO_ENCODING_MODE_CVBR = FCA_VIDEO_ENCODING_MODE_AVBR,
	VIDEO_ENCODING_MODE_VBR	 = FCA_VIDEO_ENCODING_MODE_VBR,
} HAL_VIDEO_ENCODING_MODE;

typedef enum {
	VIDEO_CODEC_H264  = FCA_CODEC_VIDEO_H264,
	VIDEO_CODEC_H265  = FCA_CODEC_VIDEO_H265,
	VIDEO_CODEC_MJPEG = FCA_CODEC_VIDEO_MJPEG,
} HAL_VIDEO_CODEC;

typedef enum {
	VIDEO_QUALITY_VERY_HIGH,
	VIDEO_QUALITY_HIGH,
	VIDEO_QUALITY_EXCELLENT,
	VIDEO_QUALITY_BETTER,
	VIDEO_QUALITY_VERY_GOOD,
	VIDEO_QUALITY_GOOD,
	VIDEO_QUALITY_NORMAL,
	VIDEO_QUALITY_LOW,
	VIDEO_QUALITY_LOWER,
} HAL_VIDEO_QUALITY;

typedef enum {
	VIDEO_RESOLUTION_VGA,
	VIDEO_RESOLUTION_HD,
	VIDEO_RESOLUTION_FHD,
	VIDEO_RESOLUTION_3MP,
} HAL_VIDEO_RESOLUTION;

typedef enum {
	HAL_VIDEO_FRAME_TYPE_PBFRAME,
	HAL_VIDEO_FRAME_TYPE_IFRAME,
} HAL_VIDEO_FRAME_TYPE;

typedef enum {
	AUDIO_CODEC_AAC		  = FCA_AUDIO_CODEC_AAC,
	AUDIO_CODEC_G711_ALAW = FCA_AUDIO_CODEC_G711_ALAW,
	AUDIO_CODEC_G711_ULAW = FCA_AUDIO_CODEC_G711_ULAW,
	AUDIO_CODEC_PCM		  = FCA_AUDIO_CODEC_PCM,
} HAL_AUDIO_CODEC;

typedef enum {
	AUDIO_SAMPLE_RATE_8_KHZ	   = FCA_AUDIO_SAMPLE_RATE_8_KHZ,
	AUDIO_SAMPLE_RATE_16_KHZ   = FCA_AUDIO_SAMPLE_RATE_16_KHZ,
	AUDIO_SAMPLE_RATE_32_KHZ   = FCA_AUDIO_SAMPLE_RATE_32_KHZ,
	AUDIO_SAMPLE_RATE_44_1_KHZ = FCA_AUDIO_SAMPLE_RATE_44_1_KHZ,
	AUDIO_SAMPLE_RATE_48_KHZ   = FCA_AUDIO_SAMPLE_RATE_48_KHZ,
} HAL_AUDIO_SAMPLE_RATE;

typedef enum {
	DAY_NIGHT_MODE_AUTO,
	DAY_MODE,
	NIGHT_MODE,
} HAL_DAY_NIGHT_MODE;

typedef enum {
	DAY_VISION,
	NIGHT_VISION,
} HAL_DAY_NIGHT_VISION;

typedef enum {
	MOTOR_STEP_PAN_LEFT,
	MOTOR_STEP_PAN_RIGHT,
	MOTOR_STEP_TILT_UP,
	MOTOR_STEP_TILT_DOWN,
} HAL_MOTOR_STEP;

typedef struct {
	int width;
	int height;
	int bitrate;
	int fps;
	int gop;
	HAL_VIDEO_CODEC codec;
	HAL_VIDEO_ENCODING_MODE encode;
	HAL_VIDEO_RESOLUTION resolution;
	HAL_VIDEO_QUALITY quality;
} HAL_VideoSetting_t;

typedef struct {
	int channel;
	int format;
	HAL_AUDIO_CODEC codec;
	HAL_AUDIO_SAMPLE_RATE samplerate;
} HAL_AudioSetting_t;

typedef enum {
	OSD_TOP_LEFT	 = FCA_OSD_TOP_LEFT,
	OSD_BOTTOM_LEFT	 = FCA_OSD_BOTTOM_LEFT,
	OSD_TOP_RIGHT	 = FCA_OSD_TOP_RIGHT,
	OSD_BOTTOM_RIGHT = FCA_OSD_BOTTOM_RIGHT,
	OSD_DISABLE		 = FCA_OSD_DISABLE,
} HAL_OSD_POSITION_E;

typedef struct {
	bool flipped;
	bool mirrored;
	int brightness;
	int contrast;
	int saturation;
	int sharpness;
	HAL_DAY_NIGHT_MODE dnmode;
	HAL_DAY_NIGHT_VISION dnvision;
} HAL_ISPSetting_t;

typedef struct {
	struct {
		int pan;
		int tilt;
	} motors;
	struct {
		int State;
	} led;
} HAL_Controller_t;

typedef enum {
	NET_ETH_STATE_UNPLUG  = FCA_NET_ETH_STATE_UNPLUG,
	NET_ETH_STATE_PLUG	  = FCA_NET_ETH_STATE_PLUG,
	NET_ETH_STATE_UNKNOWN = FCA_NET_ETH_STATE_UNKNOWN,
} HAL_NET_ETH_STATE_PORT_E;

typedef enum{
	ISP_FLICKER_TYPE_AUTO = FCA_ISP_FLICKER_TYPE_AUTO,  // Automatic flicker detection
	ISP_FLICKER_TYPE_50HZ = FCA_ISP_FLICKER_TYPE_50HZ,  // 50 Hz flicker (e.g., for some regions)
	ISP_FLICKER_TYPE_60HZ = FCA_ISP_FLICKER_TYPE_60HZ,  // 60 Hz flicker (e.g., for other regions)
	ISP_FLICKER_TYPE_OFF = FCA_ISP_FLICKER_TYPE_OFF    // Flicker reduction disabled
} ISP_FLICKER_TYPE_E;

extern uint64_t nowInUs();

#ifdef __cplusplus
}
#endif

#endif /* __HAL_AGENT_H */