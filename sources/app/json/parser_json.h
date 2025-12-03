#ifndef __FCA_APP_JSON_H__
#define __FCA_APP_JSON_H__

#include "fca_parameter.hpp"

extern bool fca_setConfigJsonRes(json &json_in, const char *mess_type, int ret);
extern bool fca_getConfigJsonRes(json &json_in, json &json_data, const char *mess_type, int ret);
extern bool fca_jsonAlarmMotion(json &json_in, int channel, const char *startTime);
extern bool fca_jsonAlarmUploadFile(json &json_in, json &json_data);

extern bool fca_jsonSetNetMQTT(json &json_in, FCA_MQTT_CONN_S *mqtt);
extern bool fca_jsonGetNetMQTT(json &json_in, FCA_MQTT_CONN_S *param);

extern bool fca_jsonGetRtcServers(json &json_in, rtcServersConfig_t *rtcSvCfg);

extern bool fca_jsonSetMotion(json &json_in, fca_motionSetting_t *param);
extern int fca_jsonGetMotion(json &json_in, fca_motionSetting_t *param);

extern bool fca_jsonSetEncode(json &json_in, FCA_ENCODE_S *param);
extern bool fca_jsonGetEncode(json &json_in, FCA_ENCODE_S *param);

extern bool fca_jsonSetParam(json &json_in, fca_cameraParam_t *param);
extern int fca_jsonGetParam(json &json_in, fca_cameraParam_t *param);
extern bool fca_jsonGetParamWithType(json &json_in, fca_cameraParam_t *param, const string &type);

extern bool fca_jsonSetWifi(json &json_in, FCA_NET_WIFI_S *param);
extern bool fca_jsonGetWifi(json &json_in, FCA_NET_WIFI_S *param);
extern bool fca_jsonSetEthernet(json &json_in, fca_netEth_t *param);
extern bool fca_jsonGetEthernet(json &json_in, fca_netEth_t *param);

extern bool fca_jsonSetRTMP(json &json_in, fca_rtmp_t *param);
extern bool fca_jsonGetRTMP(json &json_in, fca_rtmp_t *param);

extern bool fca_jsonSetWatermark(json &json_in, FCA_OSD_S *param);
extern bool fca_jsonGetWatermark(json &json_in, FCA_OSD_S *param);

extern bool fca_jsonGetReset(json &json_in, fca_resetConfig_t *param);

extern bool fca_jsonSetStorage(json &json_in, int capacity, int usage, int free);
extern bool fca_jsonGetStorage(json &json_in, fca_storage_t *param);

extern bool fca_jsonSetS3(json &json_in, fca_s3_t *param);
extern bool fca_jsonGetS3(json &json_in, fca_s3_t *param);

extern bool fca_jsonSetAccount(json &json_in, fca_account_t *param);
extern bool fca_jsonGetAccount(json &json_in, fca_account_t *param);

bool fca_jsonSetNetInfo(json &json_in, fca_netIPAddress_t *lan, fca_netIPAddress_t *wifi);

extern bool fca_jsonGetSysInfo(json &json_in, fca_sysInfo_t *sysInfo);
extern bool fca_jsonSetSysInfo(json &json_in, fca_sysInfo_t *sysInfo);

extern bool fca_jsonSetAlarmControl(json &json_in, fca_alarmControl_t *param);
extern bool fca_jsonGetAlarmControl(json &json_in, fca_alarmControl_t *param);

extern bool fca_jsonSetDeviceAuth(json &json_in, devAuth_t *param);
extern bool fca_jsonGetDeviceAuth(json &json_in, devAuth_t *param);

extern bool fca_jsonSetNetAP(json &json_in, fca_netWifiStaScan_t *param);

extern bool fca_jsonSetSystemControls(json &json_in, systemCtrls_t *param);
extern int fca_jsonGetSystemControls(json &json_in, systemCtrls_t *param, bool isCheckMissData = true);

extern bool fca_jsonSetP2P(json &json_in, int *param);
extern bool fca_jsonGetP2P(json &json_in, int *param);

extern bool fca_getSnapshotJsonRes(json &json_in, json &json_data, int ret);

extern bool fca_jsonSetOnvif(json &json_in, bool *en);
extern bool fca_jsonGetOnvif(json &json_in, bool *en);

extern bool fca_jsonSetRecord(json &json_in, StorageSdSetting_t *storageSd);
extern bool fca_jsonGetRecord(json &json_in, StorageSdSetting_t *storageSd);

#endif	  // __FCA_JSON_H__
