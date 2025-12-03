#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include <stdint.h>
#include <string.h>

#include "ak.h"
#include "app.h"
#include "app_data.h"
#include "sys_dbg.h"

#include "json.hpp"

#include "fca_parameter.hpp"

using namespace std;
using json = nlohmann::json;

#define APP_CONFIG_SUCCESS			  (0)
#define APP_CONFIG_ERROR_FILE_OPEN	  (-1)
#define APP_CONFIG_ERROR_DATA_MALLOC  (-2)
#define APP_CONFIG_ERROR_DATA_INVALID (-3)
#define APP_CONFIG_ERROR_DATA_DIFF	  (-4)
#define APP_CONFIG_ERROR_TIMEOUT	  (-5)
#define APP_CONFIG_ERROR_BUSY		  (-6)
#define APP_CONFIG_ERROR_ANOTHER	  (-7)
#define APP_CONFIG_ERROR_DATA_MISSING (-8)

#define APP_CONFIG_REBOOT_LOG_FILE_NAME "reboot.log"
#define APP_CONFIG_SYS_LOG_FILE_NAME	  "system.log"
#define APP_CONFIG_INFO_LOG_FILE_NAME  "info.log"
#define APP_CONFIG_FATAL_LOG_FILE_NAME "fatal.log"
#define APP_CONFIG_ERROR_LOG_FILE_NAME "error.log"
#define APP_CONFIG_SERVICE_LOG_FILE_NAME "service.log"

#define APP_CONFIG_SYSLOG_REBOOT_FILE_NAME "reboot"
#define APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME "cpu-ram"

extern void logFile(const char *full_path, const uint32_t max_size, const uint8_t rotation_count, const char *fmt, ...);

#define LOG_FILE_REBOOT(...)  logFile(FCA_LOG_FLASH_PATH "/" APP_CONFIG_REBOOT_LOG_FILE_NAME, 2 * 1024, 0, __VA_ARGS__)
#define LOG_FILE_SYS(...)	  logFile(FCA_LOG_FLASH_PATH "/" APP_CONFIG_SYS_LOG_FILE_NAME, 2 * 1024, 0, __VA_ARGS__)
#define LOG_FILE_INFO(...)	  logFile(FCA_LOG_FLASH_PATH "/" APP_CONFIG_INFO_LOG_FILE_NAME, 4 * 1024, 0, __VA_ARGS__)
#define LOG_FILE_FATAL(...)	  logFile(FCA_LOG_FLASH_PATH "/" APP_CONFIG_FATAL_LOG_FILE_NAME, 1024, 0, __VA_ARGS__)
#define LOG_FILE_ERROR(...)	  logFile(FCA_LOG_FLASH_PATH "/" APP_CONFIG_ERROR_LOG_FILE_NAME, 1024, 0, __VA_ARGS__)
#define LOG_FILE_SERVICE(...) logFile(FCA_LOG_RAM_PATH "/" APP_CONFIG_SERVICE_LOG_FILE_NAME, 200 * 1024, 2, __VA_ARGS__)

/******************************************************************************
 * APP STRUCTURE CONFIGURE CLASS (BASE)
 ******************************************************************************/

/******************************************************************************
 * global config
 ******************************************************************************/
#define MESSAGE_TYPE_DEVINFO			   "DeviceInfo"
#define MESSAGE_TYPE_UPGRADE			   "UpgradeFirmware"
#define MESSAGE_TYPE_UPGRADE_STATUS		   "UpgradeStatus"
#define MESSAGE_TYPE_ALARM				   "AlarmInfo"
#define MESSAGE_TYPE_MQTT				   "MQTTSetting"
#define MESSAGE_TYPE_RTMP				   "RTMPSetting"
#define MESSAGE_TYPE_MOTION				   "MotionSetting"
#define MESSAGE_TYPE_SOUND_EVENT		   "SoundEventSetting"
#define MESSAGE_TYPE_MOTION_DETECT		   "MotionDetect"
#define MESSAGE_TYPE_HUMAN_DETECT		   "HumanDetect"
#define MESSAGE_TYPE_BABYCRYING_DETECT	   "BabyCryDetect"
#define MESSAGE_TYPE_ENCODE				   "EncodeSetting"
#define MESSAGE_TYPE_REBOOT				   "Reboot"
#define MESSAGE_TYPE_RESET				   "ResetSetting"
#define MESSAGE_TYPE_STORAGE_FORMAT		   "StorageFormat"
#define MESSAGE_TYPE_STORAGE_INFO		   "StorageInfo"
#define MESSAGE_TYPE_CAMERA_PARAM		   "ParamSetting"
#define MESSAGE_TYPE_WIFI				   "WifiSetting"
#define MESSAGE_TYPE_ETHERNET			   "EthernetSetting"
#define MESSAGE_TYPE_WATERMARK			   "WatermarkSetting"
#define MESSAGE_TYPE_S3					   "S3Setting"
#define MESSAGE_TYPE_BUCKET_SETTING		   "BucketSetting"
#define MESSAGE_TYPE_NETWORK_INFO		   "NetworkInfo"
#define MESSAGE_TYPE_SYSTEM_INFO		   "SystemInfo"
#define MESSAGE_TYPE_NETWORKAP			   "NetworkAp"
#define MESSAGE_TYPE_P2P				   "P2PSetting"
#define MESSAGE_TYPE_P2P_INFO			   "P2Pinfo"
#define MESSAGE_TYPE_PTZ				   "PTZ"
#define MESSAGE_TYPE_UPLOAD_FILE		   "UploadFile"
#define MESSAGE_TYPE_RECORD				   "RecordSetting"
#define MESSAGE_TYPE_SIGNALING			   "Signaling"
#define MESSAGE_TYPE_STUN_SERVER		   "StunServer"
#define MESSAGE_TYPE_TURN_SERVER		   "TurnServer"
#define MESSAGE_TYPE_WEB_SOCKET_SERVER	   "WebSocketServer"
#define MESSAGE_TYPE_CONTROL_LED_INDICATOR "LedIndicator"
#define MESSAGE_TYPE_OWNER_STATUS		   "OwnerStatus"
#define MESSAGE_TYPE_ALARM_CONTROL		   "AlarmControl"
#define MESSAGE_TYPE_LOGIN_INFO			   "LoginInfo"
#define MESSAGE_TYPE_RUNNING_CMDS		   "ExecutingCommands"
#define MESSAGE_TYPE_SYSTEM_CONTROLS	   "SystemControls"
#define MESSAGE_TYPE_TIMEZONE			   "TZSetting"
#define MESSAGE_TYPE_BUTTON_CALL		   "CallNotify"
#define MESSAGE_TYPE_BUTTON_CALL_CANCEL	   "CancelCall"
#define MESSAGE_TYPE_SNAPSHOT			   "Snapshot"
#define MESSAGE_TYPE_ONVIF				   "OnvifSetting"
#define MESSAGE_TYPE_DATA_REQUEST		   "DebugDataRequest"
#define MESSAGE_TYPE_SHARE_FILE			   "ShareFile"
#define MESSAGE_TYPE_AIMODEL_INFO		   "AIModelInfo"

extern int fca_readConfigUsrDfaulFileToJs(json &cfgJs, string &file);

extern int fca_configGetSerial(char *serial, size_t size);
extern int fca_configGetDeviceInfoStr(json &devInfoJs);
extern int fca_configGetUpgradeStatus(json &dataJs);

extern int fca_configSetMQTT(FCA_MQTT_CONN_S *mqttCfg);
extern int fca_configGetMQTT(FCA_MQTT_CONN_S *mqttCfg);

extern int fca_configSetRtcServers(const json &rtcSvCfg);
extern int fca_configGetRtcServers(rtcServersConfig_t *rtcServerCfg);

extern int fca_configSetMotion(fca_motionSetting_t *motionCfg);
extern int fca_configGetMotion(fca_motionSetting_t *motionCfg);

extern int fca_configSetEncode(FCA_ENCODE_S *encodeCfg);
extern int fca_configGetEncode(FCA_ENCODE_S *encodeCfg);

extern int fca_configSetParam(fca_cameraParam_t *paramCfg);
extern int fca_configGetParam(fca_cameraParam_t *paramCfg);

extern int fca_configSetWifi(FCA_NET_WIFI_S *wifiCfg);
extern int fca_configGetWifi(FCA_NET_WIFI_S *wifiCfg);
extern int fca_configSetEthernet(fca_netEth_t *ethCfg);
extern int fca_configGetEthernet(fca_netEth_t *ethCfg);

extern int fca_configSetRTMP(fca_rtmp_t *rtmpCfg);
extern int fca_configGetRTMP(fca_rtmp_t *rtmpCfg);

extern int fca_configSetWatermark(FCA_OSD_S *watermarkCfg);
extern int fca_configGetWatermark(FCA_OSD_S *watermarkCfg);

extern int fca_configSetS3(fca_s3_t *s3Cfg);
extern int fca_configGetS3(fca_s3_t *s3Cfg);

extern int fca_configSetS3BucketSetting(fca_s3_t *s3Cfg);
extern int fca_configGetS3BucketSetting(fca_s3_t *s3Cfg);

extern int fca_configSetAccount(fca_account_t *accountCfg);
extern int fca_configGetAccount(fca_account_t *accountCfg);

extern int fca_configGetSysInfo(fca_sysInfo_t *sysInfo);

extern int fca_configSetAlarmControl(fca_alarmControl_t *alarmCtl);
extern int fca_configGetAlarmControl(fca_alarmControl_t *alarmCtl);

extern int fca_configSetDeviceAuth(devAuth_t *info);
extern int fca_configGetDeviceAuth(devAuth_t *info);

extern int fca_configSetSystemControls(systemCtrls_t *sysCtrls);
extern int fca_configGetSystemControls(systemCtrls_t *sysCtrls);

extern int fca_configSetP2P(int *p2pCfg);
extern int fca_configGetP2P(int *p2pCfg);

extern int fca_configSetTZ(const string &timezone);
extern int fca_configGetTZ(string &timezone);

extern int fca_configSetOnvifSetting(fca_onvif_setting_t *onvifSetting);
extern int fca_configGetOnvifSetting(fca_onvif_setting_t *onvifSetting);

extern int fca_configSetOnvif(bool *enable);
extern int fca_configGetOnvif(bool *enable);

extern int fca_configSetRecord(StorageSdSetting_t *storageSd);
extern int fca_configGetRecord(StorageSdSetting_t *storageSd);

#endif	  //__APP_CONFIG_H__
