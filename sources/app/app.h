#ifndef __APP_H__
#define __APP_H__

#include "ak.h"
#include "timer.h"
#include <string>

using namespace std;

/*****************************************************************************/
/* task GW_SYS define.
 */
/*****************************************************************************/
/* define timer */
#define GW_SYS_WATCH_DOG_TIMEOUT_INTERVAL		  (120)			   // 120s watchdog
#define GW_SYS_WATCH_DOG_PING_TASK_REQ_INTERVAL	  (5000)		   // 5s reset watchdog
#define GW_SYS_STOP_RING_CALL_INTERVAL			  (60 * 1000)	   // NOTE: BA 60s
#define GW_SYS_RELEASE_BUTTON_CALL_INTERVAL		  (5 * 1000)	   // 5s
#define GW_SYS_CHECK_NTP_UPDATED_TIMEOUT_INTERVAL (1000)		   // 1s check ntp
#define GW_SYS_CLEAR_CACHE_TIMEOUT_INTERVAL		  (3600 * 1000)	   // 1h clear cache
#define GW_SYS_CHECK_RAM_USAGE_TIMEOUT_INTERVAL	  (10 * 1000)	   // 1min check RAM usage

/* define signal */
enum {
	GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ = AK_USER_DEFINE_SIG,
	GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES,
	GW_SYS_WATCH_DOG_PING_NEXT_TASK_TIMEOUT,
	GW_SYS_CLOUD_REBOOT_REQ,
	GW_SYS_REBOOT_REQ,
	GW_SYS_CMD_REBOOT_REQ,
	GW_SYS_START_WATCH_DOG_REQ,
	GW_SYS_STOP_WATCH_DOG_REQ,
	GW_SYS_SET_LOGIN_INFO_REQ,
	GW_SYS_GET_LOGIN_INFO_REQ,
	GW_SYS_RUNNING_CMDS_REQ,
	GW_SYS_LOAD_SYSTEM_CONTROLS_CONFIG_REQ,
	GW_SYS_SET_SYSTEM_CONTROLS_REQ,
	GW_SYS_GET_SYSTEM_CONTROLS_REQ,
	GW_SYS_LOAD_AUDIO_CONFIG_REQ,
	GW_SYS_LOAD_SPEAKER_MIC_CONFIG_REQ,
	GW_SYS_START_RTSPD_REQ,
	GW_SYS_STOP_RTSPD_REQ,
	GW_SYS_GET_SYSTEM_INFO_REQ,
	GW_SYS_GET_CA_FILE_REQ,
	GW_SYS_STOP_ONVIF_REQ,
};

/*****************************************************************************/
/*  task GW_CONSOLE define
 */
/*****************************************************************************/
/* define timer */

/* define signal */
enum {
	GW_CONSOLE_INTERNAL_LOGIN_CMD = AK_USER_DEFINE_SIG,
};

/*****************************************************************************/
/*  task GW_CLOUD define
 */
/*****************************************************************************/
/* define timer */
#define GW_CLOUD_MQTT_GEN_STATUS_TIMEOUT_INTERVAL			(3000)
#define GW_CLOUD_MQTT_CHECK_LOOP_TIMEOUT_INTERVAL			(5000)
#define GW_CLOUD_MQTT_TRY_CONNECT_TIMEOUT_INTERVAL			(5000)
#define GW_CLOUD_MQTT_SUB_TOPIC_STATUS_INTERVAL				(5000)
#define GW_CLOUD_MQTT_CHECK_SUB_TOPIC_TIMEOUT_INTERVAL		(15000)
#define GW_CLOUD_MQTT_CHECK_CONNECT_STATUS_INTERVAL			(10000)
#define GW_CLOUD_MQTT_TRY_CONNECT_AFTER_DISCONNECT_INTERVAL (20000)

/* define signal */
enum {
	GW_CLOUD_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_CLOUD_MQTT_INIT_REQ = AK_USER_DEFINE_SIG,
	GW_CLOUD_MQTT_TRY_CONNECT_REQ,
	GW_CLOUD_MQTT_CHECK_CONNECT_STATUS_REQ,
	GW_CLOUD_PROCESS_INCOME_DATA,
	GW_CLOUD_MQTT_ON_CONNECTED,
	GW_CLOUD_MQTT_ON_DISCONNECTED,
	GW_CLOUD_SET_MQTT_CONFIG_REQ,
	GW_CLOUD_GET_MQTT_CONFIG_REQ,
	GW_CLOUD_CAMERA_STATUS_RES,
	GW_CLOUD_CAMERA_CONFIG_RES,
	GW_CLOUD_CAMERA_ALARM_RES,
	GW_CLOUD_SIGNALING_MQTT_RES,
	GW_CLOUD_MESSAGE_LENGTH_OUT_OF_RANGE_REP,
	GW_CLOUD_CHECK_BROKER_STATUS_REQ,
	GW_CLOUD_GEN_MQTT_STATUS_REQ,
	GW_CLOUD_MQTT_SUB_TOPIC_REP,
	GW_CLOUD_MQTT_CHECK_SUB_TOPIC_REQ,
	GW_CLOUD_MQTT_CHECK_MQTT_LOOP_REQ,
	GW_CLOUD_MQTT_RELEASE_RESOURCES_REQ,
};

/*****************************************************************************/
/* task GW_AV define
 */
/*****************************************************************************/
/* define timer */
#define GW_AV_CHECK_VIDEO_STREAM_RUNNING_LONG_INTERVAL	(60000)
#define GW_AV_CHECK_VIDEO_STREAM_RUNNING_SHORT_INTERVAL (10000)
#define GW_AV_CHECK_CAPTURE_PROCESS_TIMEOUT_INTERVAL	(10000)
#define GW_AV_AUTO_DAYNIGHT_CONTROL_TIMEOUT_INTERVAL	(5000)
#define GW_AV_RELOAD_PICTURE_TIMEOUT_INTERVAL			(5000)
#define GW_AV_OSD_UPDATE_TIME_TEXT_TIMEOUT_INTERVAL		(1000)
/* define signal */

enum {
	GW_AV_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_AV_INIT_REQ			= AK_USER_DEFINE_SIG,
	GW_AV_SET_CAMERAPARAM_REQ,
	GW_AV_GET_CAMERAPARAM_REQ,
	GW_AV_SET_ENCODE_REQ,
	GW_AV_GET_ENCODE_REQ,
	GW_AV_SET_WATERMARK_REQ,
	GW_AV_GET_WATERMARK_REQ,
	GW_AV_ADJUST_IMAGE_WITH_DAYNIGHT_STATE_REQ,
	GW_AV_RELOAD_PICTURE_REQ
};

/*****************************************************************************/
/* task GW_CONFIG define
 */
/*****************************************************************************/
/* define timer */
#define GW_CONFIG_AUTO_LIGHTING_CONTROL_INTERVAL		  (5000)
#define GW_CONFIG_CHECK_MOUNT_USR_CONFIG_TIMEOUT_INTERVAL (5000)

/* define signal */

enum {
	// SET
	GW_CONFIG_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_CONFIG_SET_CONFIG		= AK_USER_DEFINE_SIG,
	GW_CONFIG_SET_RESET_CONFIG_REQ,
	GW_CONFIG_SET_UPGRADE_REQ,
	GW_CONFIG_SET_UPGRADE_STATUS_REQ,
	GW_CONFIG_SET_REBOOT_REQ,
	GW_CONFIG_SET_RECORD_CONFIG_REQ,
	GW_CONFIG_SET_OWNER_CONFIG_REQ,
	GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ,
	GW_CONFIG_RESTART_AUTO_LIGHTING_CONTROL_REQ,
	// GET
	GW_CONFIG_GET_RESET_CONFIG_REQ,
	GW_CONFIG_GET_SYSINFO_CONFIG_REQ,
	GW_CONFIG_GET_RECORD_CONFIG_REQ,
	GW_CONFIG_GET_NETAP_CONFIG_REQ,
	GW_CONFIG_CLOUD_SET_LED_INDICATOR_REQ,
	GW_CONFIG_CLOUD_GET_LED_INDICATOR_REQ,
	GW_CONFIG_SET_TIMEZONE_REQ,
	GW_CONFIG_GET_TIMEZONE_REQ,
	GW_CONFIG_FORCE_MOTOR_STOP_REQ,
	GW_CONFIG_CHECK_MOUNT_USR_CONFIG_REQ
};

/*****************************************************************************/
/* task GW_DETECT define
 */
/*****************************************************************************/
/* define timer */
#define GW_DETECT_INIT_TIMEOUT_INTERVAL				  (5000)
#define GW_DETECT_LIGHT_ALERT_BLINKING_INTERVAL		  (1000)
#define GW_DETECT_HANDLING_ALARM_INTERVAL			  (300)
#define GW_DETECT_POLLING_UNDETECTED_TIMEOUT_INTERVAL (5000)
#define GW_DETECT_ALARM_TRIGGER_STATUS_REP_INTERVAL	  (1000)
/* define signal */

enum {
	GW_DETECT_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_DETECT_INIT_REQ			= AK_USER_DEFINE_SIG,
	GW_DETECT_SET_MOTION_CONFIG_REQ,
	GW_DETECT_GET_MOTION_CONFIG_REQ,
	GW_DETECT_SET_ALARM_CONTROL_REQ,
	GW_DETECT_GET_ALARM_CONTROL_REQ,
	GW_DETECT_SET_SOUND_EVENT_CONFIG_REQ,
	GW_DETECT_GET_SOUND_EVENT_CONFIG_REQ,
	GW_DETECT_START_DETECT_REQ,
	GW_DETECT_ALARM_HUMAN_DETECTED,
	GW_DETECT_ALARM_MOTION_DETECTED,
	GW_DETECT_ALARM_BABY_CRYING_DETECTED,
	GW_DETECT_SNAP_IMAGE_REQ,
	GW_DETECT_PUSH_MQTT_ALARM_NOTIFICATION,
	GW_DETECT_HANDLING_ALARM,
	GW_DETECT_INTERVAL_ALARM_TIMEOUT,
	GW_DETECT_DURATION_ALARM_TIMEOUT,

	GW_DETECT_PERIODIC_ALERT_LIGHTING,

	GW_DETECT_START_TRIGGER_ALARM_REQ,
	GW_DETECT_STOP_TRIGGER_ALARM_REQ,
	GW_DETECT_ALARM_TRIGGER_STATUS_REP,

	GW_DETECT_PENDING_TRIGGER_SOUND_ALARM_REQ,
	GW_DETECT_CONTINUE_TRIGGER_SOUND_ALARM_REQ,
	GW_DETECT_IVA_AI_GET_VERSION_REQ,

	GW_DETECT_START_BABYCRY_REQ,
	GW_DETECT_STOP_BABYCRY_REQ,
	GW_DETECT_ENABLE_AI_REQ,
	GW_DETECT_DISABLE_AI_REQ,
	
};

/*****************************************************************************/
/* task GW_LED define
 */
/*****************************************************************************/
/* define timer */
#define GW_LED_BLINK_1S_INTERVAL	(10000)
#define GW_LED_BLINK_500MS_INTERVAL (5000)
/* define signal */

enum {
	GW_LED_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_LED_WHITE_BLINK		 = AK_USER_DEFINE_SIG,
	GW_LED_WHITE_ON,
	GW_LED_WHITE_OFF,
	GW_LED_ORANGE_BLINK,
	GW_LED_ORANGE_ON,
	GW_LED_ORANGE_OFF,
};

/*****************************************************************************/
/* task GW_UPLOAD define
 */
/*****************************************************************************/
/* define timer */
#define GW_UPLOAD_FILE_JPEG_REQ_INTERVAL (2500)
#define GW_UPLOAD_FILE_MP4_REQ_INTERVAL	 (3500)

#define GW_UPLOAD_PUT_BITRATE_LIMIT (5 * 1024)	  // 5 MB/s

/* define signal */

enum {
	GW_UPLOAD_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_UPLOAD_INIT_REQ			= AK_USER_DEFINE_SIG,
	GW_UPLOAD_SET_S3_CONFIG_REQ,
	GW_UPLOAD_GET_S3_CONFIG_REQ,
	GW_UPLOAD_SET_BUCKET_SETTING_CONFIG_REQ,
	GW_UPLOAD_GET_BUCKET_SETTING_CONFIG_REQ,
	GW_UPLOAD_SET_INFO_UPLOAD,
	GW_UPLOAD_POST_FILE_REQ,
	GW_UPLOAD_FILE_JPEG_REQ,
	GW_UPLOAD_FILE_MP4_REQ,
	GW_UPLOAD_SNAPSHOT_REQ,
};

/*****************************************************************************/
/* task GW_NETWORK define
 */
/*****************************************************************************/
/* define timer */
#define GW_NET_RESTART_NETWORK_SERVICES_TIMEROUT_INTERVAL (240000)	 /* 4min */
#define GW_NET_RELOAD_WIFI_DRIVER_TIMEROUT_INTERVAL		  (900000)	 /* 15min */
#define GW_NET_REBOOT_ERROR_NETWORK_TIMEROUT_INTERVAL	  (14400000) /* 4h */
#define GW_NET_RETRY_CONNECT_MQTT_INTERVAL				  (5000)	 /* 5000ms */
#define GW_NET_START_AP_MODE_TIMEOUT_INTERVAL			  (5000)	 /* 5000ms */
#define GW_NET_CHECK_CONNECT_ROUTER_INTERVAL			  (1000)	 /* 1s */
#define GW_NET_CHECK_STATE_PLUG_LAN_INTERVAL			  (500)		 /* 500ms */
#define GW_NET_UNLOCK_SWITCH_AP_MODE_INTERVAL			  (20000)	 /* 20s */
#define GW_NET_TRY_GET_LAN_IP_TIMEROUT_INTERVAL			  (2000)

/* define signal */
enum {
	/* wifi station interface */
	GW_NET_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_NET_NETWORK_INIT_REQ	 = AK_USER_DEFINE_SIG,

	GW_NET_WIFI_MQTT_SET_CONF_REQ,
	GW_NET_WIFI_MQTT_GET_CONF_REQ,
	GW_NET_ETHERNET_MQTT_SET_CONF_REQ,
	GW_NET_ETHERNET_MQTT_GET_CONF_REQ,
	GW_NET_WIFI_MQTT_GET_LIST_AP_REQ,
	GW_NET_MQTT_GET_NETWORK_INFO_REQ,

	GW_NET_WIFI_DO_CONNECT,
	GW_NET_ETHERNET_CONTROL_WITH_CONFIG_REQ,
	GW_NET_RUN_HOSTAPD,

	GW_NET_RUN_HOSTBLE,
	GW_NET_UNLOCK_SWITCH_AP_MODE_REQ,
	GW_NET_SWITCH_AP_MODE_START_REQ,
	GW_NET_RESET_BUTTON_REQ,

	GW_NET_WIFI_STA_CONNECT_TO_ROUTER,
	GW_NET_WIFI_STA_CONNECTED_TO_ROUTER,

	GW_NET_INTERNET_UPDATED,
	GW_NET_GET_STATE_LAN_REQ,
	GW_NET_TRY_GET_LAN_IP_PRIO_ONVIF_REQ,

	GW_NET_RESET_WIFI_REQ,
	GW_NET_PLAY_VOICE_NETWORK_CONNECTED_REQ,
	GW_NET_RESTART_UDHCPC_NETWORK_INTERFACES_REQ,
	GW_NET_RELOAD_WIFI_DRIVER_REQ,
	GW_NET_REBOOT_ERROR_NETWORK_REQ,
};

/*****************************************************************************/
/* task GW_FW_TASK define
 */
/*****************************************************************************/
/* define timer */
#define GW_FW_UPGRADE_FIRMWARE_TIMEOUT_INTERVAL (300000) /* 5min */

/* define signal */
enum {
	/* wifi station interface */
	GW_FW_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_FW_GET_URL_REQ		= AK_USER_DEFINE_SIG,
	GW_FW_DOWNLOAD_START_REQ,
	GW_FW_UPGRADE_START_REQ,
	GW_FW_RELEASE_OTA_STATE_REQ,
};

/*****************************************************************************/
/*  task GW_TASK_WEBRTC define
 */
/*****************************************************************************/
/* define timer */
#define GW_WEBRTC_CLOSE_SIGNALING_SOCKET_TIMEOUT_INTERVAL	(300000) /* 5min */
#define GW_WEBRTC_ERASE_CLIENT_NO_ANSWER_TIMEOUT_INTERVAL	(40000)	 /* 40s */
#define GW_WEBRTC_TRY_CONNECT_SOCKET_INTERVAL				(10000)	 /* 10s */
#define GW_WEBRTC_TRY_GET_EXTERNAL_IP_INTERVAL				(7000)	 /* 7s */
#define GW_WEBRTC_TRY_UPDATE_STUN_IP_TO_HOSTS_FILE_INTERVAL (7000)	 /* 7s */
#define GW_WEBRTC_WAIT_DOWNLOAD_RESPONSE_TIMEOUT_INTERVAL	(20000)	 /* 20s */
#define GW_WEBRTC_CLIENT_SEND_PING_INTERVAL					(10000)	 /* 10s */
#define GW_WEBRTC_ERASE_CLIENT_PING_PONG_TIMEOUT_INTERVAL	(20000)	 /* 20s */
#define GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK_INTERVAL		(3000)	 /* 3s */

/* define signal */
enum {
	GW_WEBRTC_WATCHDOG_PING_REQ	 = AK_USER_WATCHDOG_SIG,
	GW_WEBRTC_SIGNALING_MQTT_REQ = AK_USER_DEFINE_SIG,
// #ifdef TEST_USE_WEB_SOCKET
	GW_WEBRTC_TRY_CONNECT_SOCKET_REQ,
	GW_WEBRTC_SIGNALING_SOCKET_REQ,
// #endif
	GW_WEBRTC_SET_SIGNLING_SERVER_REQ,
	GW_WEBRTC_GET_SIGNLING_SERVER_REQ,
	GW_WEBRTC_CHECK_CLIENT_CONNECTED_REQ,
	GW_WEBRTC_ERASE_CLIENT_REQ,
	GW_WEBRTC_DBG_IPC_SEND_MESSAGE_REQ,
	GW_WEBRTC_ON_MESSAGE_CONTROL_DATACHANNEL_REQ,
	GW_WEBRTC_DATACHANNEL_DOWNLOAD_RELEASE_REQ,
	GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK,
	GW_WEBRTC_TRY_GET_STUN_EXTERNAL_IP_REQ,
	GW_WEBRTC_CLOSE_SIGNALING_SOCKET_REQ,
	GW_WEBRTC_SET_CLIENT_MAX_REQ,
	GW_WEBRTC_GET_CLIENT_MAX_REQ,
	GW_WEBRTC_GET_CURRENT_CLIENTS_TOTAL_REQ,
};

/*****************************************************************************/
/*  task GW_TASK_RTMP define
 */
/*****************************************************************************/
/* define timer */
#define GW_RTMP_RECONNECT_INTERVAL		(30000) /* 30s */
#define GW_RTMP_RECONNECT_INTERVAL_FAST (5000)	/* 5s */

/* define signal */
enum {
	GW_RTMP_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_RTMP_START_REQ		  = AK_USER_DEFINE_SIG,
	GW_RTMP_STOP_REQ,
	GW_RTMP_SET_CONFIG_REQ,
	GW_RTMP_GET_CONFIG_REQ,
	GW_RTMP_LOAD_RTMP_CONTROL_REQ,
	GW_RTMP_TRY_CONNECT_REQ,
};

/*****************************************************************************/
/* task GW_TASK_RECORD define
 */
/*****************************************************************************/
/* define timer */
#define GW_RECORD_REGULAR_MAX_INTERVAL	 (60000 * 15)	 // 15 minutes
#define GW_RECORD_EVENTS_BUFFER_ITNERVAL (30000)		 // 30 seconds
#define GW_RECORD_EVENTS_MAX_INTERVAL	 (60000 * 5)	 // 05 mitutes
/* define signal */
enum {
	GW_RECORD_WATCHDOG_PING_REQ = AK_USER_WATCHDOG_SIG,
	GW_RECORD_ON_MOUNTED		= AK_USER_DEFINE_SIG,
	GW_RECORD_ON_REMOVED,
	GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR,
	GW_RECORD_STORAGE_LOCAL_TYPE_EVENTS,
	GW_RECORD_FORCE_CLOSE_STORAGE_LOCAL_EVENTS,
	GW_RECORD_FORMAT_CARD_REQ,
	GW_RECORD_GET_CAPACITY_REQ,
	GW_RECORD_GET_RECORD_REQ,
	GW_RECORD_SET_RECORD_REQ,
};

/*****************************************************************************/
/*  global define variable
 */
/*****************************************************************************/
#define APP_OK (0x00)
#define APP_NG (0x01)

#define APP_FLAG_OFF (0x00)
#define APP_FLAG_ON	 (0x01)

/* define file name */
#define FCA_SERIAL_FILE			   "Serial"
#define FCA_ACCOUNT_FILE		   "Account"
#define FCA_P2P_FILE			   "P2P"
#define FCA_ENCODE_FILE			   "Encode"
#define FCA_DEVINFO_FILE		   "DeviceInfo"
#define FCA_SOUND_EVENT_FILE	   "SoundEvent"
#define FCA_MOTION_FILE			   "Motion"
#define FCA_WIFI_FILE			   "Wifi"
#define FCA_ETHERNET_FILE		   "Ethernet"
#define FCA_WATERMARK_FILE		   "Watermark"
#define FCA_S3_FILE				   "S3"
#define FCA_BUCKET_SETTING_FILE	   "BucketSetting"
#define FCA_RTMP_FILE			   "RTMP"
#define FCA_NETWORK_CA_FILE		   "fcam_ca_root.cert"
#define FCA_NETWORK_MQTT_FILE	   "MQTTService"
#define FCA_NETWORK_WPA_FILE	   "wpa_supplicant.conf"
#define FCA_CAMERA_PARAM_FILE	   "CameraParam"
#define FCA_PTZ_FILE			   "PTZ"
#define FCA_RTC_SERVERS_FILE	   "RtcServersConfig"
#define FCA_STORAGE_FILE		   "Storage"
#define FCA_RECORD_FILE			   "Record"
#define FCA_CHECKSUM_FILE		   "SdChecksumFile"
#define FCA_OTA_STATUS			   "OTAStatus"
#define FCA_IO_DRIVER_CONTROL_FILE "IoControl"
#define FCA_RAINBOW_FILE		   "rainbow.json"
#define FCA_OWNER_STATUS		   "OwnerStatus"
#define FCA_ALARM_CONTROL_FILE	   "AlarmControl"
#define FCA_STEP_MOTOR_FILE		   "StepMotors"
#define FCA_DEVICE_LOGIN_INFO_FILE "LoginInfo"
#define FCA_SYSTEM_CONTROLS_FILE   "SystemControls"
#define FCA_TIMEZONE_FILE		   "TZ"
#define FCA_ONVIF_FILE			   "Onvif"
#define FCA_LINKS_FILE			   "Links"
#define FCA_MAC_DEFAULT			   "00:E0:4C:35:00:00"

/* define path name */
#define FCA_DEVICE_BIN_PATH			  "/app/bin"
#define FCA_DEVICE_SOUND_PATH		  "/tmp/nfs/config/sound"
#define FCA_DEVICE_INFO_PATH		  "/tmp/nfs/envir/version"
// #define vendorsConfigurationPath			  "/tmp/envir/vendors"
// #define defaultConfigurationPath			  "/tmp/envir/default"
#define FCA_MEDIA_JPEG_PATH			  "/tmp/jpeg"
#define FCA_MEDIA_MP4_PATH			  "/tmp/mp4"
#define FCA_SDCARD_CHECKSUM_FILE_PATH "/tmp/sdFileInfo"
#define FCA_RTSP_KEY_GEN_FILE_PATH	  "/tmp/rtspKeyGen"
#define FCA_BLE_SERVICE_UUID_PATH	  "ble_userconfig.json"
// #define FCA_ONVIF_SETTING_PATH		  vendorsConfigurationPath "/onvif_config.json"
#define FCA_LOG_FLASH_PATH			  
#define FCA_LOG_RAM_PATH			  "/tmp/log"

/* use for wifi and network */
#define FCA_NET_WIRED_IF_NAME		 "eth0"
#define FCA_NET_WIFI_STA_IF_NAME	 "wlan0"
#define FCA_NET_WIFI_AP_IF_NAME		 FCA_NET_WIFI_STA_IF_NAME
#define FCA_UDHCPC_SCRIPT			 FCA_DEVICE_BIN_PATH "/udhcpc.sh"
#define FCA_NTP_UPDATE_SCRIPT		 "fcam_ntpdate.sh"
#define FCA_NET_UDHCPC_PID_FILE		 "/var/run/%s.pid"
#define FCA_NET_WIFI_AP_UDHCPD_FILE	 "/tmp/udhcpd.conf"
#define FCA_NET_WIFI_AP_HOSTADP_FILE "/tmp/hostapd.conf"
#define FCA_NET_WIFI_STA_WPA_FILE	 "/tmp/wpa_supplicant.conf"
#define FCA_NET_HOSTS_FILE			 "/etc/hosts"
#define FCA_DNS_FILE				 "/etc/resolv.conf"
#define FCA_ONVIF_SCRIPT			 "fca_onvif.sh"
#define FCA_ONVIF_WSD_SERVER		 "wsd_services"
#define FCA_ONVIF_LIGHTTPD_SERVER	 "lighttpd"
#define FCA_ONVIF_LWRTSPD_SERVER	 "lwrtspd"

#define FCA_MQTT_KEEPALIVE	90
#define FCA_SSL_VERIFY_NONE 0
#define FCA_SSL_VERIFY_PEER 1

#define FCA_SOUND_REBOOT_DEVICE_FILE	   "reboot_device.pcm"
#define FCA_SOUND_WAIT_CONNECT_FILE		   "waiting_for_connection.pcm"
#define FCA_SOUND_HELLO_DEVICE_FILE		   "hello_fpt_camera.pcm"
#define FCA_SOUND_NET_CONNECT_SUCCESS_FILE "connection_success.pcm"
#define FCA_SOUND_MOTION_ALARM_FILE		   "motion_alarm.pcm"

#define FCA_SOUND_CHANGE_WIFI_MODE_FILE	   "wifi_mode.pcm"
#define FCA_SOUND_CHANGE_BLE_MODE_FILE	   "ble_mode.pcm"
#define FCA_SOUND_BLE_CONNECTED_FILE	   "ble_connected.pcm"
#define FCA_SOUND_BLE_DISCONNECTED_FILE	   "ble_disconnected.pcm"
#define FCA_SOUND_WAIT_SWITCH_MODE_FILE	   "switch_connection_method.pcm"

#define FCA_RECORD_S3_MOTION_FILE "motion.mp4"
#define FCA_IMAGE_S3_MOTION_FILE  "motion.jpeg"

extern std::string deviceSerialNumber;
extern std::string defaultConfigurationPath;
extern std::string vendorsConfigurationPath;

extern bool selectedBluetoothMode;
extern std::string vendorsHostapdSsid;
extern std::string vendorsHostapdPssk;

inline std::string FCA_DEFAULT_FILE_LOCATE(const std::string filename) {
    return std::string(defaultConfigurationPath + "/default/" + filename);
}

inline std::string FCA_DEFAULT_SOUND_LOCATE(const std::string filename) {
    return std::string(defaultConfigurationPath + "/sound/" + filename);
}

inline std::string FCA_VENDORS_FILE_LOCATE(const std::string filename) {
    return std::string(vendorsConfigurationPath + "/" + filename);
}

#endif	  // __APP_H__
