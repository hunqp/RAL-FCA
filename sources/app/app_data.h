#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>
#include <stdbool.h>
#include <optional>

#include "fca_common.hpp"
#include "stream.hpp"
#include "helpers.hpp"
#include "mediabuffer.h"
#include "SdCard.h"
#include "utils.h"
#include "helpers.hpp"
#include "stream.hpp"
#include "dispatchqueue.hpp"
#include "fca_parameter.hpp"
#include "fca_common.hpp"
#include "fca_gpio.hpp"
#include "rtc/rtc.hpp"
#include "cpptime.h"
#include "mqtt.h"
#include "rtspd.h"


#include "fca_video.hpp"
#include "fca_audio.hpp"
#include "network_manager.h"
#include "sys_log.h"

using namespace rtc;

#define CHECK_TIME_WATCHDOG 0
#define TEST_USE_WEB_SOCKET

#define FCA_EQ_MAX_BANDS             (10)

#define RTSP_ACCOUNT_DEFAULT		"rtsp"
#ifdef RELEASE
#define ROOT_DEVICE_PASS_DEFAULT	"giDaK!7ecf$"
#else
#define ROOT_DEVICE_PASS_DEFAULT	"iot"
#endif
#define ROOT_DEVICE_PASS_LENGTH_MAX (32)
#define RTSP_PASSWORD_LENGTH_MAX	(17)

#define STREAM_RESERVE_BUFFER_INSEC (6)
#define STREAM_RESERVE_GOP_VIDEO	(13)
#define STREAM_RESERVE_GOP_AUDIO	(8)

#define DAY_NIGHT_TRANSITION_CHECK_INTERVAL (60)

#define SYSTEM_CONTROL_AMIC			   "AmicCapture"
#define SYSTEM_CONTROL_MASTER_PLAYBACK "MasterPlayback"
#define SYSTEM_CONTROL_AEC			   "AecNoise"
#define SYSTEM_CONTROL_SPEAKER_GAIN	   "SpeakerGain"
#define SYSTEM_CONTROL_BUTTON_CALL	   "ButtonCall"

#define SYSTEM_CONTROL_AUDIO_EQ			  "AudioEQ"
#define SYSTEM_CONTROL_NOISEREDUCTION	  "NoiseReduction"
#define SYSTEM_CONTROL_AGCCONTROL		  "AgcControl"
#define SYSTEM_CONTROL_ASLCCONTROL		  "AslcControl"
#define SYSTEM_CONTROL_AIGAIN			  "AIGain"
#define SYSTEM_CONTROL_EQCONTROL		  "EqControl"
#define SYSTEM_CONTROL_AUDIO_CONF_DEFAULT "AudioDefault"

#define NTP_UPDATE_STATUS_FILE "/var/run/ntpStatus"

typedef enum {
	JPEG_SNAP_TYPE_MOTION,
	JPEG_SNAP_TYPE_FACE,
	JPEG_SNAP_TYPE_HUMAN,
	JPEG_SNAP_TYPE_THUMBNAIL,
	JPEG_SNAP_TYPE_BABYCRY,
} JPEG_SNAP_TYPE_E;

enum {
	NET_SETUP_STATE_NONE,
	NET_SETUP_STATE_CONFIG,
	NET_SETUP_STATE_CONNECTING,
	NET_SETUP_STATE_CONNECTED,
	NET_SETUP_STATE_MQTT_CONNECTED,
};

typedef enum {
	STORAGE_ONLY_REG,
	STORAGE_ONLY_EVT,
	STORAGE_BOTH_OPT,
	STORAGE_NONE,
} STORAGE_SD_OPTION;

typedef struct {
	char path[50];
	bool bJPEG; /* JPEG or MP4 */
	JPEG_SNAP_TYPE_E type;
	uint32_t timestamp;
} S3Notification_t;

typedef struct {
	char clientId[30];
	char msg[200];
} DataChannelSend_t;

typedef struct {
	vector<string> arrStunServerUrl;
	vector<string> arrTurnServerUrl;

	void clear() {
		arrStunServerUrl.clear();
		arrTurnServerUrl.clear();
	}
} rtcServersConfig_t;

typedef struct {
	string account;
	string password;
	void clear() {
		account.clear();
		password.clear();
	}

	bool empty() {
		return account.empty() || password.empty();
	}
} rtspAuth_t;

typedef struct {
	string ubootPass;
	string rootPass;
	rtspAuth_t rtspInfo;

	void clear() {
		ubootPass.clear();
		rootPass.clear();
		rtspInfo.clear();
	}
} devAuth_t;

typedef struct {
	struct {
		bool enable;
		int volume;
	} amicCapture;
	struct {
		int volume;
	} masterPlayback;
	struct {
		int enable;
		int aecEn;
		int noiseEn;
		int aecThr;
		int aecScale;
	} aecNoise;
	struct {
		bool enable;
		int amplifyFactor;
	} speakerGain;
	struct {
		bool enable;
	} buttonCall;
	struct {
		bool applyDefault;
		int aiGain;
		struct {
			signed short noiseSuppressDbNr;
			int enable;
		} noiseReduction;
		struct {
			float agc_level;
			signed short agc_max_gain;
			float agc_min_gain;
			signed short agc_near_sensitivity;
			bool enable;
		} agcControl;
		struct {
			float limit;
			signed long aslc_db;
		} aslcControl;
		struct {
			signed short pre_gain;
			unsigned long bands;
			unsigned long bandfreqs[FCA_EQ_MAX_BANDS];
			signed short bandgains[FCA_EQ_MAX_BANDS];
			float bandQ[FCA_EQ_MAX_BANDS];
			unsigned short band_types[FCA_EQ_MAX_BANDS];
			unsigned char enable;
			unsigned char band_enable[FCA_EQ_MAX_BANDS];
		} eqControl;
	} audioEq;
} systemCtrls_t;

typedef struct {
	bool bSound;
	bool bLight;
	int elapsedSecond;
	bool bRunning;
} AlarmTrigger_t;

typedef struct {
	int channel;
	int maxDayStorage;
	int regDurationInSec;
	STORAGE_SD_OPTION opt;
} StorageSdSetting_t;

typedef struct {
	RECORDER_TYPE type;
	uint32_t timestamp;
} RecorderSdTrigger_t;

/* S3 Setting*/
typedef struct {
	std::string accessKey;
	std::string endPoint;

	void clear() {
		accessKey.clear();
		endPoint.clear();
	}
} fca_s3_t;

extern std::string camIpPublic;
extern GPIOHelpers gpiosHelpers;
extern DispatchQueue sysThread;
extern DispatchQueue voiceGuideThread;
extern DispatchQueue pushToTalkThread;
extern optional<shared_ptr<Stream>> avStream;
extern std::atomic<bool> isOtaRunning;
// extern SEDTrigger BbCryDetect; TODO

extern std::optional<shared_ptr<Stream>> avStream;
extern unordered_map<string, shared_ptr<Client>> clients;
// extern RTSPServer rtspServer;

extern int netSetupState;
extern bool isFistTimeStartModeNetwork;
extern systemCtrls_t sysCtrls;
extern std::string caSslPath;
extern CppTime::Timer systemTimer;
extern OfflineEventManager offlineEventManager;

extern void sendMsgRespondProcess(const string &otaProcess);
extern void sendMsgControlDataChannel(const string &id, const string &msg);
extern void lockMutexListClients();
extern void unlockMutexListClients();
extern string fca_getSerialInfo();
extern void genDefaultPasswordRtsp(string &user, string &pass);
extern string getMqttConnectStatus();
extern void netClearDns(const char *ifname, string fileDNS);
extern int systemSetup(const string &type, systemCtrls_t *sysCtrls);
extern int startOnvifServices();
extern int stopOnvifServices();
// extern std::string getPayloadMQTTSignaling(uint32_t time, JPEG_SNAP_TYPE_E triggerType, uint32_t duration, std::string typeStr, s3_object_t *s3_obj);
extern int getNtpStatus();


#define FCA_TEMPLATE_SET_CONFIG_RESPONSE(mt,rc) 	templatesResponseToCloud("SET", mt, nlohmann::json{}, rc)
#define FCA_TEMPLATE_GET_CONFIG_RESPONSE(mt,dt,rc) 	templatesResponseToCloud("GET", mt, dt, rc)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern AudioHelpers audioHelpers;
extern VideoHelpers videoHelpers;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern std::string templatesResponseToCloud(std::string method, std::string messageType, nlohmann::json js, int rc);

#endif /* __APP_DATA_H__ */
