#ifndef __FCA_PARAMETER_H__
#define __FCA_PARAMETER_H__

#include <string>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

#include "json.hpp"
#include "fca_assert.hpp"
#include "fca_common.hpp"

/******************************************************************************
 * Common define
 *******************************************************************************/
#define FCA_NAME_LEN		  64
#define FCA_NAME_PASSWORD_LEN 1024
#define FCA_CLIENT_ID_LEN	  64

/******************************************************************************
 * OSD, motion, video define
 *******************************************************************************/
#define FCA_MAX_STREAM_BITRATE	(10 * 1024)
#define FCA_MAX_STREAM_FPS		(30)
#define FCA_MAX_STREAM_QP		(50)
#define FCA_MAX_STREAM_GOP		(100)
#define FCA_SNAP_QUALITY	60
#define FCA_MOTION_INTERVAL 500	   //(ms)

#define FCA_JPEG_MOTION_TMP_PATH "/tmp/jpeg"
#define FCA_MAX_SENSITIVE		 5
#define FCA_MAX_LENG_GUARDZONE	 64

#define FCA_OSD_FONT_PATH_FILE "/conf/user/NimbusMono-Regular.otf"

enum FCA_VIDEO_CHANNEL_ID {
	VIDEO0_CHANNEL_ID,
	VIDEO1_CHANNEL_ID,
	VIDEON_CHANNEL_MAX,
};

enum fca_dayNightMode_e {
	FCA_FULL_COLOR_MODE,		   // Off
	FCA_NIGHT_VISION_AUTO_MODE,	   // Auto
	FCA_BLACK_WHITE_MODE,		   // On
	FCA_COLOR_NIGHT_MODE,
	FCA_BLACK_WHITE_IQ_MODE,
	FCA_DAYNIGHT_UNKNOWN_MODE
};

/*Capture compression mode*/
enum fca_captureComp_e {
	FCA_CAPTURE_COMP_H264,	   // H.264
	FCA_CAPTURE_COMP_H265,	   // H.265
	FCA_CAPTURE_COMP_MJPEG,	   // MJPEG
	FCA_CAPTURE_COMP_NONE	   //
};

enum {
	FCA_VIDEO_DIMENSION_VGA,
	FCA_VIDEO_DIMENSION_720P,
	FCA_VIDEO_DIMENSION_1080P,
	FCA_VIDEO_DIMENSION_2K,
	FCA_VIDEO_DIMENSION_UNKNOWN,
};

/*BitRate Control*/
enum fca_captureBrc_e {
	FCA_CAPTURE_BRC_CBR,
	FCA_CAPTURE_BRC_VBR,
	FCA_CAPTURE_BRC_MBR,
	FCA_CAPTURE_BRC_NR
};

enum fca_captureQuality_e {
	FCA_CAPTURE_QUALITY_LOWER = 0,
	FCA_CAPTURE_QUALITY_LOW,
	FCA_CAPTURE_QUALITY_NORMAL,
	FCA_CAPTURE_QUALITY_GOOD,
	FCA_CAPTURE_QUALITY_VERY_GOOD,
	FCA_CAPTURE_QUALITY_BETTER,
	FCA_CAPTURE_QUALITY_EXCELLENT,
	FCA_CAPTURE_QUALITY_HIGH,
	FCA_CAPTURE_QUALITY_VERY_HIGH,
	FCA_CAPTURE_QUALITY_NONE,
};

/*Light control*/
enum fca_lightCtrl_e {
	FCA_LIGHT_FULL_COLOR,	  // On
	FCA_LIGHT_BLACK_WHITE,	  // Off
	FCA_LIGHT_AUTO			  // Auto
};

/*Light control*/
enum fca_antiFlicker_e {
	FCA_ANTI_FLICKER_MODE_AUTO,
	FCA_ANTI_FLICKER_MODE_50_MHZ,
	FCA_ANTI_FLICKER_MODE_60_MHZ,
	FCA_ANTI_FLICKER_MODE_OFF
};

typedef struct {	
	int fps;
	int gop;
	int bitrate;
	int quality;
	int resolution;
	FCA_VIDEO_IN_CODEC_E codec;
	FCA_VIDEO_IN_ENCODING_MODE_E rcmode;
} FCA_VIDEO_FORMAT_S;

typedef struct {
	FCA_VIDEO_FORMAT_S mainStream;	   // main format for channel 0
	FCA_VIDEO_FORMAT_S minorStream;	   // extra format for channel 1
} FCA_ENCODE_S;

/*Night Vision*/
typedef struct {
	int mode;
	bool ircutSwap;
} fca_nightVison_t;

/*Flip Mirror*/
typedef struct {
	bool picFlip;
	bool picMirror;
} fca_flipMirror_t;

typedef struct t_fca_dnParam {
	int nightVisionMode;
	int lightingMode;
	bool lightingEnable;
	unsigned int day2NightThreshold;
	unsigned int night2DayThreshold;
	unsigned int awbNightDisMax;
	unsigned int dayCheckFrameNum;
	unsigned int nightCheckFrameNum;
	unsigned int expStableCheckFrameNum;
	unsigned int stableWaitTimeoutMs;
	unsigned int gainR;
	int offsetR;
	unsigned int gainB;
	int offsetB;
	unsigned int gainRb;
	int offsetRb;
	unsigned int blkCheckFlag;
	unsigned int lowLumiTh;
	unsigned int highLumiTh;
	unsigned int nonInfraredRatioTh;
	unsigned int highlightLumiTh;
	unsigned int highlightBlkCntMax;
	unsigned int lockCntTh;
	unsigned int lockTime;
} fca_dayNightParam_t;

/*Camera Param*/
typedef struct {
	bool pictureFlip;
	bool pictureMirror;
	int antiFlickerMode;
	int brightness;
	int contrast;
	int saturation;
	int sharpness;
	int irBrightness;
	int irContrast;
	int irSaturation;
	int irSharpness;
	fca_dayNightParam_t dnParam;
} fca_cameraParam_t;

/*Motion control*/
typedef struct {
	bool setSound;
	bool setLighting;
	int durationInSecs;
} fca_alarmControl_t;

/*Motion Setting*/
typedef struct {
	int sensitive;
	int interval;
	int duration;
	bool enable;
	bool videoAtt;
	bool pictureAtt;
	bool humanDetAtt;
	bool humanFocus;
	bool humanDraw;
	bool guardZoneAtt;
	int lenGuardZone;
	int guardZone[FCA_MAX_LENG_GUARDZONE];
} fca_motionSetting_t;

/* OSD Setting*/
#define WATERMARK_LENGTH_MAX (32)
typedef struct {
	std::string name;
	bool timeAtt;
	bool nameAtt;
} FCA_OSD_S;

/* Sound event setting */
typedef struct {
	bool enable;
	int interval;
	float confidence;
} fca_SoundEventSetting_t;
/******************************************************************************
 * Record define
 *******************************************************************************/
#define MOUNT_FOLDER		"/tmp/sd/" /**Default mountpoint */
#define FCA_AUDIO_G711A_EXT ".g711a"
#define FCA_VIDEO_H264_EXT	".h264"
#define FCA_VIDEO_H265_EXT	".h265"
#define FCA_SD_CAPACITY_USE 0.85	// 85% SD capacity used

typedef struct {
	int channel;
	int mode;
	int dayStorage;
	int durationInSecs;
} fca_record_t;

enum fca_mode_e {
	FCA_MODE_FULL,
	FCA_MODE_MOTION,
	FCA_MODE_MAX
};

/******************************************************************************
 * Config define
 *******************************************************************************/
#define FCA_CONFIG_NONE 0

/*Account */
typedef struct {
	char username[32];
	char password[32];
	bool reserved;
	bool shareable;
	int len;
	char authoList[2][32];
} fca_account_t;

/*Reset Setting*/
typedef struct {
	bool bWatermark;
	bool bMotion;
	bool bMQTT;
	bool bRTMP;
	bool bWifi;
	bool bEthernet;
	bool bParam;
	bool bEncode;
	bool bS3;
	bool bBucket;
	bool bLedIndi;
	bool bRainbow;
	bool bMotor;
	bool bAlarm;
	bool bRtc;
	bool bLogin;
	bool bSystem;
	bool bP2p;
	bool bOnvif;
} fca_resetConfig_t;

/*Storage*/
typedef struct {
	bool bIsFornat;
} fca_storage_t;

/* ONVIF Setting */
typedef struct {
	int port;
	char username[64];
	char password[64];
	char interfaces[8];
	char location[64];
	char name[64];
} fca_onvif_setting_t;

/*******************************************************************************
 * Camera Info Define
 ********************************************************************************/
typedef struct {
	char serial[64];
} fca_deviceInfo_t;

typedef struct {
	char buildTime[32];
	char softVersion[16];
} fca_sysInfo_t;

extern int fca_contextInit();

/*******************************************************************************
 * MQTT Define
 ********************************************************************************/
#define MAX_MQTT_MSG_LENGTH 150000	  // Byte

enum {
	FCA_MQTT_RESPONE_SUCCESS = 100,
	FCA_MQTT_RESPONE_FAILED	 = 102,
	FCA_MQTT_RESPONE_TIMEOUT = 101,
	FCA_MQTT_RESPONE_DENY	 = 103
};

typedef struct {
	int bEnable; /* 1: Enable; 0: Disable */
	int bTLSEnable;
	char clientID[FCA_CLIENT_ID_LEN];
	char username[FCA_NAME_LEN];
	char password[FCA_NAME_PASSWORD_LEN];
	char host[FCA_NAME_LEN];
	char TLSVersion[8];
	int keepAlive;
	int QOS;
	int retain;
	int port;
	char protocol[16];	  // IPCP_MQTT_PROTOCOL_TYPE_E
} FCA_MQTT_CONN_S;

typedef struct {
	char topicStatus[128];
	char topicRequest[128];
	char topicResponse[128];
	char topicSignalingRequest[128];
	char topicSignalingResponse[128];
	char topicAlarm[128];
} mqttTopicCfg_t;

/*******************************************************************************
 * task fw Define
 ********************************************************************************/

typedef struct {
	char fileName[16];
	char putUrl[128];
} fca_upload_file_t;

/*******************************************************************************
 * task network Define
 ********************************************************************************/
#define MAX_MAC_LEN (17)

enum fca_net_status_e {
	FCA_NET_UNKNOWN,
	FCA_NET_CONNECTED,
	FCA_NET_DISCONNECT,
};

typedef struct {
	char gateWay[16];
	char hostIP[16];
	char submask[16];
	char preferredDns[16];
	char alternateDns[16];
} fca_netConf_t;

typedef struct {
	char mac[32];
	char hostIP[16];
	char subnet[16];
	char gateway[16];
	char dns[16];
} fca_netIPAddress_t;

typedef struct {
	bool enable;
	char ssid[64];
	char keys[64];
	int keyType;
	char auth[32];
	int channel;
	char encrypType[16];
	fca_netConf_t detail;
	bool dhcp;
} FCA_NET_WIFI_S;

typedef struct {
	bool enable;
	fca_netConf_t detail;
	bool dhcp;
} fca_netEth_t;

#define FCA_WIFI_SCAN_HOST_AP_NUM_MAX 10
typedef struct {
	char mac[20];
	char flag[128];
	char ssid[64];
	int signal;
	int freq;
} netItem_t;

typedef struct {
	int number;
	netItem_t item[FCA_WIFI_SCAN_HOST_AP_NUM_MAX];
} fca_netWifiStaScan_t;

/*******************************************************************************
 * task RTMP Define
 ********************************************************************************/
/* RTMP Setting*/
typedef struct {
	char host[64];
	bool enable;
	int port;
	char token[64];
	char node[64];
	int streamType;
} fca_rtmp_t;

/*******************************************************************************
 * PTZ Define
 ********************************************************************************/
/* PTZ*/
typedef struct {
	char direction[16];
	int step;
	int x;
	int y;
} fca_ptz_t;

/*******************************************************************************
 * Nightvision Define
 ********************************************************************************/
#define MAX_DAY_NIGHT_SWITCHES (6)

enum DAYNIGHT_STATE_E {
	DN_STATE_UNKNOWN,
	DN_DAY_STATE,
	DN_NIGHT_STATE
};

enum CHECK_DAYNIGHT_TYPE_E {
	UNKNOWN,
	SLS,
	ALS,
};

extern bool FCA_ParserUserFiles(nlohmann::json &js, std::string filename);
extern bool FCA_UpdateUserFiles(nlohmann::json &js, std::string filename);

extern bool FCA_ParserParams(nlohmann::json &js, FCA_ENCODE_S *encoders);
extern bool FCA_UpdateParams(nlohmann::json &js, FCA_ENCODE_S *encoders);

extern bool FCA_ParserParams(nlohmann::json &js, FCA_OSD_S *osds);
extern bool FCA_UpdateParams(nlohmann::json &js, FCA_OSD_S *osds);

#endif	  // __FCA_PARAMETER_H__

