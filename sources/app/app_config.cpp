#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include <string.h>

#include <iostream>
#include <cstring>

using namespace std;

#include "utils.h"

#include "app.h"
#include "app_dbg.h"
#include "app_config.h"
#include "parser_json.h"
#include "ota.h"
#include "fca_video.hpp"
/******************************************************************************
 * APP CONFIGURE CLASS (BASE)
 ******************************************************************************/

#include <vector>

static string xorKey = "fcamfssiot133099";

int fca_readConfigUsrDfaulFileToJs(json &cfgJs, string &file) {
	string filePath = FCA_VENDORS_FILE_LOCATE(file);
	if (!read_json_file(cfgJs, filePath)) {
		APP_DBG("Can not read: %s\n", filePath.data());
		filePath = FCA_DEFAULT_FILE_LOCATE(file);
		cfgJs.clear();
		if (!read_json_file(cfgJs, filePath)) {
			APP_DBG("Can not read: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	return APP_CONFIG_SUCCESS;
}

int fca_configGetSerial(char *serial, size_t size) {
	// example: 0e709-I04L-A01-C05OYYMMNNNNNNN
	int ret			  = APP_CONFIG_ERROR_ANOTHER;
	string serial_str = "";
	serial_str		  = systemStrCmd("fw_printenv | grep 'serial_number=' | cut -d '=' -f 2 | cut -d '-' -f 4 | awk 'length($0) > 4 { sub(/:.*/, \"\"); print tolower($0) }'");
	if (!serial_str.empty()) {
		APP_DBG("serial_number get: %s\n", serial_str.data());
		ret = APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("get serial number failed\n");
		serial_str = "unknown";
	}
	size_t len = serial_str.length() < size ? serial_str.length() + 1 : size;
	safe_strcpy(serial, serial_str.c_str(), len);
	APP_DBG("camera serial: %s\n", serial);
	return ret;
}

int fca_configGetDeviceInfoStr(json &devInfoJs) {
	json cfgJs;
	string file = FCA_DEVICE_INFO_PATH;

	if (!read_json_file(cfgJs, file)) {
		APP_DBG("Can not read: %s\n", file.data());
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	devInfoJs = {
		{"MessageType", string(MESSAGE_TYPE_DEVINFO)},
		{"Serial",	   string(fca_getSerialInfo()) },
		{"Data",		 cfgJs					   },
	};
	return APP_CONFIG_SUCCESS;
}

int fca_configGetUpgradeStatus(json &dataJs) {
	string content;
	string filePath = FCA_USER_CONF_PATH "/" FCA_OTA_STATUS;
	if (!read_raw_file(content, filePath)) {
		APP_DBG("Can not read: %s\n", filePath.data());
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	dataJs["Data"]["UpgradeStatus"] = fca_otaStatusUpgrade(stoi(content));

	return APP_CONFIG_SUCCESS;
}

int fca_configSetMQTT(FCA_MQTT_CONN_S *mqttCfg) {
	json mqttJs;
	if (fca_jsonSetNetMQTT(mqttJs, mqttCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_NETWORK_MQTT_FILE;
		if (write_json_file(mqttJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetMQTT(FCA_MQTT_CONN_S *mqttCfg) {
	json cfgJs;

	string fileName = FCA_NETWORK_MQTT_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetNetMQTT(cfgJs, mqttCfg)) {
		APP_DBG("MQTT config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetRtcServers(rtcServersConfig_t *rtcServerCfg) {
	json cfgJs;

	string fileName = FCA_RTC_SERVERS_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetRtcServers(cfgJs, rtcServerCfg)) {
		APP_DBG("rtc server config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetRtcServers(const json &rtcSvCfg) {
	string filePath = FCA_USER_CONF_PATH "/" FCA_RTC_SERVERS_FILE;
	if (write_json_file(rtcSvCfg, filePath))
		return APP_CONFIG_SUCCESS;
	else {
		APP_DBG("Can not write: %s\n", filePath.data());
		return APP_CONFIG_ERROR_FILE_OPEN;
	}
}

int fca_configSetMotion(fca_motionSetting_t *motionCfg) {
	json motionJs;
	if (fca_jsonSetMotion(motionJs, motionCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_MOTION_FILE;
		if (write_json_file(motionJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetMotion(fca_motionSetting_t *motionCfg) {
	int ret = APP_CONFIG_ERROR_ANOTHER;
	json usrCfg;
	string fileName = FCA_MOTION_FILE;
	string filePath = FCA_USER_CONF_PATH + string("/") + fileName;
	if (read_json_file(usrCfg, filePath)) {
		ret = fca_jsonGetMotion(usrCfg, motionCfg);
		if (ret == APP_CONFIG_SUCCESS) {
			APP_DBG("User param config: %s\n", usrCfg.dump().data());
		}
	}

	if (ret != APP_CONFIG_SUCCESS) {
		json defCfg;
		filePath = FCA_DFAUL_CONF_PATH + string("/") + fileName;
		if (read_json_file(defCfg, filePath)) {
			if (fca_jsonGetMotion(defCfg, motionCfg) == APP_CONFIG_SUCCESS) {
				// TODO
				ret = APP_CONFIG_SUCCESS;
			}
		}
	}

	return ret;
}

int fca_configSetEncode(FCA_ENCODE_S *encodeCfg) {
	json encodeJs;
	if (fca_jsonSetEncode(encodeJs, encodeCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_ENCODE_FILE;
		if (write_json_file(encodeJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetEncode(FCA_ENCODE_S *encodeCfg) {
	json cfgJs;

	string fileName = FCA_ENCODE_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetEncode(cfgJs, encodeCfg)) {
		// APP_DBG("Encode config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetParam(fca_cameraParam_t *paramCfg) {
	json paramJs;
	if (fca_jsonSetParam(paramJs, paramCfg)) {
		string file = FCA_USER_CONF_PATH "/" FCA_CAMERA_PARAM_FILE;
		if (write_json_file(paramJs, file))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", file.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetParam(fca_cameraParam_t *paramCfg) {
	int ret = APP_CONFIG_ERROR_ANOTHER;
	json usrCfg;
	string fileName = FCA_CAMERA_PARAM_FILE;
	string filePath = FCA_USER_CONF_PATH + string("/") + fileName;
	if (read_json_file(usrCfg, filePath)) {
		ret = fca_jsonGetParam(usrCfg, paramCfg);
		if (ret == APP_CONFIG_SUCCESS) {
			APP_DBG("User param config: %s\n", usrCfg.dump().data());
		}
	}

	if (ret != APP_CONFIG_SUCCESS) {
		json defCfg;
		filePath = FCA_DFAUL_CONF_PATH + string("/") + fileName;
		if (read_json_file(defCfg, filePath)) {
			// APP_DBG("Default param config: %s\n", defCfg.dump().data());
			if (fca_jsonGetParam(defCfg, paramCfg) == APP_CONFIG_SUCCESS) {	   // WRD02: v1.1.0 - v1.2.0 missing json key
				if (ret == APP_CONFIG_ERROR_DATA_MISSING) {
					APP_DBG("user config missing data\n");

					usrCfg["AwbNightDisMax"]		 = paramCfg->dnParam.awbNightDisMax;
					usrCfg["DayCheckFrameNum"]		 = paramCfg->dnParam.dayCheckFrameNum;
					usrCfg["NightCheckFrameNum"]	 = paramCfg->dnParam.nightCheckFrameNum;
					usrCfg["ExpStableCheckFrameNum"] = paramCfg->dnParam.expStableCheckFrameNum;
					usrCfg["StableWaitTimeoutMs"]	 = paramCfg->dnParam.stableWaitTimeoutMs;
					usrCfg["GainR"]					 = paramCfg->dnParam.gainR;
					usrCfg["OffsetR"]				 = paramCfg->dnParam.offsetR;
					usrCfg["GainB"]					 = paramCfg->dnParam.gainB;
					usrCfg["OffsetB"]				 = paramCfg->dnParam.offsetB;
					usrCfg["GainRb"]				 = paramCfg->dnParam.gainRb;
					usrCfg["OffsetRb"]				 = paramCfg->dnParam.offsetRb;
					usrCfg["BlkCheckFlag"]			 = paramCfg->dnParam.blkCheckFlag;
					usrCfg["LowLumiTh"]				 = paramCfg->dnParam.lowLumiTh;
					usrCfg["HighLumiTh"]			 = paramCfg->dnParam.highLumiTh;
					usrCfg["NonInfraredRatioTh"]	 = paramCfg->dnParam.nonInfraredRatioTh;
					usrCfg["HighlightLumiTh"]		 = paramCfg->dnParam.highlightLumiTh;
					usrCfg["HighlightBlkCntMax"]	 = paramCfg->dnParam.highlightBlkCntMax;
					usrCfg["LockCntTh"]				 = paramCfg->dnParam.lockCntTh;
					usrCfg["LockTime"]				 = paramCfg->dnParam.lockTime;

					filePath = FCA_USER_CONF_PATH + string("/") + fileName;
					write_json_file(usrCfg, filePath);
					fca_jsonGetParam(usrCfg, paramCfg);
				}
				ret = APP_CONFIG_SUCCESS;
			}
		}
	}

	return ret;
}

int fca_configSetWifi(FCA_NET_WIFI_S *wifiCfg) {
	json wifiJs;
	if (fca_jsonSetWifi(wifiJs, wifiCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_WIFI_FILE;
		if (write_json_file(wifiJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetWifi(FCA_NET_WIFI_S *wifiCfg) {
	json cfgJs;

	string fileName = FCA_WIFI_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetWifi(cfgJs, wifiCfg)) {
		APP_DBG("Wifi config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetEthernet(fca_netEth_t *ethCfg) {
	json wifiJs;
	if (fca_jsonSetEthernet(wifiJs, ethCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_ETHERNET_FILE;
		if (write_json_file(wifiJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetEthernet(fca_netEth_t *ethCfg) {
	json cfgJs;

	string fileName = FCA_ETHERNET_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetEthernet(cfgJs, ethCfg)) {
		APP_DBG("Eth config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetRTMP(fca_rtmp_t *rtmpCfg) {
	json rtmpJs;
	if (fca_jsonSetRTMP(rtmpJs, rtmpCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_RTMP_FILE;
		if (write_json_file(rtmpJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetRTMP(fca_rtmp_t *rtmpCfg) {
	json cfgJs;

	string fileName = FCA_RTMP_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetRTMP(cfgJs, rtmpCfg)) {
		APP_DBG("RTMP config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetWatermark(FCA_OSD_S *watermarkCfg) {
	json watermarkJs;
	if (fca_jsonSetWatermark(watermarkJs, watermarkCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_WATERMARK_FILE;
		if (write_json_file(watermarkJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetWatermark(FCA_OSD_S *watermarkCfg) {
	json cfgJs;

	string fileName = FCA_WATERMARK_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetWatermark(cfgJs, watermarkCfg)) {
		APP_DBG("Watermark config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetS3(fca_s3_t *s3Cfg) {
	json s3Js;
	if (fca_jsonSetS3(s3Js, s3Cfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_S3_FILE;
		if (write_json_file(s3Js, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetS3(fca_s3_t *s3Cfg) {
	json cfgJs;
	std::string fileName = FCA_S3_FILE;

	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetS3(cfgJs, s3Cfg)) {
		APP_DBG("S3 config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetS3BucketSetting(fca_s3_t *s3Cfg) {
	json s3Js;
	if (fca_jsonSetS3(s3Js, s3Cfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_BUCKET_SETTING_FILE;
		if (write_json_file(s3Js, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetS3BucketSetting(fca_s3_t *s3Cfg) {
	json cfgJs;
	std::string fileName = FCA_BUCKET_SETTING_FILE;

	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetS3(cfgJs, s3Cfg)) {
		APP_DBG("S3 config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetAccount(fca_account_t *accountCfg) {
	json accountJs;
	if (fca_jsonSetAccount(accountJs, accountCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_ACCOUNT_FILE;
		if (write_json_file(accountJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetAccount(fca_account_t *accountCfg) {
	json cfgJs;

	string fileName = FCA_ACCOUNT_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetAccount(cfgJs, accountCfg)) {
		APP_DBG("Account config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetSysInfo(fca_sysInfo_t *sysInfo) {
	json cfgJs;
	string file = FCA_DEVICE_INFO_PATH;
	if (!read_json_file(cfgJs, file)) {
		APP_DBG("Can not read: %s\n", file.data());
		return APP_CONFIG_ERROR_FILE_OPEN;
	}
	if (fca_jsonGetSysInfo(cfgJs, sysInfo)) {
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", file.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetAlarmControl(fca_alarmControl_t *alarmCtl) {
	json alarmJs;
	if (fca_jsonSetAlarmControl(alarmJs, alarmCtl)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_ALARM_CONTROL_FILE;
		if (write_json_file(alarmJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetAlarmControl(fca_alarmControl_t *alarmCtl) {
	json cfgJs;

	string fileName = FCA_ALARM_CONTROL_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetAlarmControl(cfgJs, alarmCtl)) {
		APP_DBG("Record config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

void logInfoEncryptDecrypt(devAuth_t *in, devAuth_t *out, const string &key) {
	if (!in->rtspInfo.empty()) {
		out->rtspInfo.account  = xorEncryptDecrypt(in->rtspInfo.account, key);
		out->rtspInfo.password = xorEncryptDecrypt(in->rtspInfo.password, key);
	}
	if (!in->rootPass.empty()) {
		out->rootPass = xorEncryptDecrypt(in->rootPass, key);
	}
	if (!in->ubootPass.empty()) {
		out->ubootPass = xorEncryptDecrypt(in->ubootPass, key);
	}
}

int fca_configSetDeviceAuth(devAuth_t *info) {
	json cfgJs;
	/* encrypt */
	devAuth_t enInfo;
	logInfoEncryptDecrypt(info, &enInfo, xorKey);
	if (fca_jsonSetDeviceAuth(cfgJs, &enInfo)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_DEVICE_LOGIN_INFO_FILE;
		if (write_json_file(cfgJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetDeviceAuth(devAuth_t *info) {
	json cfgJs;

	string filePath = FCA_USER_CONF_PATH + string("/") + FCA_DEVICE_LOGIN_INFO_FILE;
	if (!read_json_file(cfgJs, filePath)) {
		APP_DBG("Can not read: %s\n", filePath.data());
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	devAuth_t enInfo;
	if (fca_jsonGetDeviceAuth(cfgJs, &enInfo)) {
		APP_DBG("Record config: %s\n", cfgJs.dump().data());
		/* decrypt */
		logInfoEncryptDecrypt(&enInfo, info, xorKey);
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", filePath.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetSystemControls(systemCtrls_t *sysCtrls) {
	json sysCtrlsJs;
	if (fca_jsonSetSystemControls(sysCtrlsJs, sysCtrls)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_SYSTEM_CONTROLS_FILE;
		if (write_json_file(sysCtrlsJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetSystemControls(systemCtrls_t *sysCtrls) {
	int ret = APP_CONFIG_ERROR_ANOTHER;
	// json usrCfg;
	// string fileName = FCA_SYSTEM_CONTROLS_FILE;
	// string filePath = FCA_USER_CONF_PATH + string("/") + fileName;
	// if (read_json_file(usrCfg, filePath)) {
	// 	ret = fca_jsonGetSystemControls(usrCfg, sysCtrls);
	// 	if (ret == APP_CONFIG_SUCCESS) {
	// 		APP_DBG("User param config: %s\n", usrCfg.dump().data());
	// 	}
	// }

	// if (ret != APP_CONFIG_SUCCESS) {
	// 	json defCfg;
	// 	filePath = FCA_DFAUL_CONF_PATH + string("/") + fileName;
	// 	if (read_json_file(defCfg, filePath)) {
	// 		APP_DBG("Default param config: %s\n", defCfg.dump().c_str());
	// 		int res = fca_jsonGetSystemControls(defCfg, sysCtrls);
	// 		if (res == APP_CONFIG_SUCCESS) {
	// 			if (ret == APP_CONFIG_ERROR_DATA_MISSING) {
	// 				APP_DBG("user config missing data\n");
	// 				json audioEQJson = {
	// 					{"Default",						sysCtrls->audioEq.applyDefault																						  },
	// 					{SYSTEM_CONTROL_AIGAIN,			sysCtrls->audioEq.aiGain																								},
	// 					{SYSTEM_CONTROL_AGCCONTROL,
	// 					 {{"Enable", sysCtrls->audioEq.agcControl.enable},
	// 					  {"AgcLevel", round(sysCtrls->audioEq.agcControl.agc_level * 10.0) / 10.0},
	// 					  {"AgcMaxGain", sysCtrls->audioEq.agcControl.agc_max_gain},
	// 					  {"AgcMinGain", round(sysCtrls->audioEq.agcControl.agc_min_gain * 10.0) / 10.0},
	// 					  {"AgcNearSensitivity", sysCtrls->audioEq.agcControl.agc_near_sensitivity}}																			},
	// 					{SYSTEM_CONTROL_ASLCCONTROL,	 {{"AslcDb", sysCtrls->audioEq.aslcControl.aslc_db}, {"Limit", round(sysCtrls->audioEq.aslcControl.limit * 10.0) / 10.0}}},
	// 					{SYSTEM_CONTROL_EQCONTROL,
	// 					 {{"PreGain", sysCtrls->audioEq.eqControl.pre_gain},
	// 					  {"Bands", sysCtrls->audioEq.eqControl.bands},
	// 					  {"Enable", sysCtrls->audioEq.eqControl.enable},
	// 					  {"BandFreqs", json::array()},
	// 					  {"BandGains", json::array()},
	// 					  {"BandQ", json::array()},
	// 					  {"BandTypes", json::array()},
	// 					  {"BandEnable", json::array()}}																														},
	// 					{SYSTEM_CONTROL_NOISEREDUCTION,
	// 					 {{"Enable", sysCtrls->audioEq.noiseReduction.enable}, {"NoiseSuppressDbNr", sysCtrls->audioEq.noiseReduction.noiseSuppressDbNr}}						 }
	// 				  };

	// 				for (unsigned long i = 0; i < FCA_EQ_MAX_BANDS; i++) {
	// 					float q = (i < sysCtrls->audioEq.eqControl.bands) ? sysCtrls->audioEq.eqControl.bandQ[i] : 0.0;
	// 					audioEQJson["EqControl"]["BandFreqs"].push_back((i < sysCtrls->audioEq.eqControl.bands) ? sysCtrls->audioEq.eqControl.bandfreqs[i] : 0);
	// 					audioEQJson["EqControl"]["BandGains"].push_back((i < sysCtrls->audioEq.eqControl.bands) ? sysCtrls->audioEq.eqControl.bandgains[i] : 0);
	// 					audioEQJson["EqControl"]["BandQ"].push_back(round(q * 10.0) / 10.0);
	// 					audioEQJson["EqControl"]["BandTypes"].push_back((i < sysCtrls->audioEq.eqControl.bands) ? sysCtrls->audioEq.eqControl.band_types[i] : FCA_NO);
	// 					audioEQJson["EqControl"]["BandEnable"].push_back((i < sysCtrls->audioEq.eqControl.bands) ? sysCtrls->audioEq.eqControl.band_enable[i] : 0);
	// 				}

	// 				usrCfg[SYSTEM_CONTROL_AUDIO_EQ] = audioEQJson;

	// 				filePath = FCA_USER_CONF_PATH + string("/") + fileName;
	// 				APP_DBG_AUDIO("User param config: %s\n", usrCfg.dump().data());
	// 				write_json_file(usrCfg, filePath);
	// 				fca_jsonGetSystemControls(usrCfg, sysCtrls);
	// 			}
	// 			ret = APP_CONFIG_SUCCESS;
	// 		}
	// 		APP_DBG("Result code get system controls [%d]\n", res);
	// 	}
	// }

	return ret;
}

int fca_configSetP2P(int *p2pCfg) {
	json cfgJs;
	if (fca_jsonSetP2P(cfgJs, p2pCfg)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_P2P_FILE;
		if (write_json_file(cfgJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetP2P(int *p2pCfg) {
	json cfgJs;
	string fileName = FCA_P2P_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetP2P(cfgJs, p2pCfg)) {
		APP_DBG("p2p config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetTZ(const string &timezone) {
	string filePath = FCA_USER_CONF_PATH "/" FCA_TIMEZONE_FILE;
	if (!write_raw_file(timezone, filePath)) {
		APP_DBG("set TZ config failed\n");
		return APP_CONFIG_ERROR_FILE_OPEN;
	}
	return APP_CONFIG_SUCCESS;
}

int fca_configGetTZ(string &timezone) {
	string filePath = FCA_USER_CONF_PATH "/" FCA_TIMEZONE_FILE;
	if (!read_raw_file(timezone, filePath)) {
		APP_DBG("read %s failed\n", filePath.data());
		filePath = FCA_DFAUL_CONF_PATH "/" FCA_TIMEZONE_FILE;
		if (!read_raw_file(timezone, filePath)) {
			APP_DBG("read %s failed\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	timezone.erase(timezone.find_last_not_of("\n") + 1);
	return APP_CONFIG_SUCCESS;
}

int fca_configSetOnvif(bool *enable) {
	json cfgJs;
	if (fca_jsonSetOnvif(cfgJs, enable)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_ONVIF_FILE;
		if (write_json_file(cfgJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetOnvif(bool *enable) {
	json cfgJs;
	string fileName = FCA_ONVIF_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetOnvif(cfgJs, enable)) {
		APP_DBG("onvif config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configSetOnvifSetting(fca_onvif_setting_t *onvifSetting) {
	json Js;
	fca_sysInfo_t sysInfo;
	FCA_ENCODE_S videEncode;

	fca_configGetSysInfo(&sysInfo);
	fca_configGetEncode(&videEncode);

	Js["Port"]		 = onvifSetting->port;
	Js["Username"]	 = onvifSetting->username;
	Js["Password"]	 = onvifSetting->password;
	Js["Interfaces"] = onvifSetting->interfaces;
	Js["Location"]	 = onvifSetting->location;
	Js["Name"]		 = onvifSetting->name;

	/* Non-Modification Configuration */
	Js["SN"]					 = fca_getSerialInfo();
	Js["Fw"]					 = sysInfo.softVersion;
	Js["Hw"]					 = "1.0.0";
	Js["Model"]					 = "IPC";
	Js["Manufacturer"]			 = "FCA";
	Js["AdvancedFaultIfUnknown"] = 0;
	Js["AdvancedSynologyNvr"]	 = 0;

	Js["Events"][0]["Input"] = "/tmp/onvif_noti_motion";
	Js["Events"][0]["Name"]	 = "VideoSourceConfigurationToken";
	Js["Events"][0]["Topic"] = "tns1:VideoSource/MotionDetected";
	Js["Events"][0]["Value"] = "VideoSourceToken";
	Js["Events"][1]["Input"] = "/tmp/onvif_noti_human";
	Js["Events"][1]["Name"]	 = "VideoSourceConfigurationToken";
	Js["Events"][1]["Topic"] = "tns1:VideoSource/HumanDetected";
	Js["Events"][1]["Value"] = "VideoSourceToken";

	Js["Profiles"][0]["Decoder"] = "G711";
	Js["Profiles"][0]["Type"]	 = "H264";
	Js["Profiles"][0]["Height"]	 = 1296;
	Js["Profiles"][0]["Width"]	 = 2304;
	Js["Profiles"][0]["SnapURL"] = "http://%s/cgi-bin/snapshot.sh";
	Js["Profiles"][0]["URL"]	 = "rtsp://%s/profile1";
	Js["Profiles"][0]["Name"]	 = "Profile_0";

	Js["Profiles"][1]["Decoder"] = "G711";
	Js["Profiles"][1]["Type"]	 = "H264";
	Js["Profiles"][1]["Height"]	 = 720;
	Js["Profiles"][1]["Width"]	 = 1280;
	Js["Profiles"][1]["SnapURL"] = "http://%s/cgi-bin/snapshot.sh";
	Js["Profiles"][1]["URL"]	 = "rtsp://%s/profile2";
	Js["Profiles"][1]["Name"]	 = "Profile_1";

	char scope[96];
	memset(scope, 0, sizeof(scope));
	sprintf(scope, "onvif://www.onvif.org/name/%s", onvifSetting->name);
	Js["Scopes"][0] = std::string(scope);

	memset(scope, 0, sizeof(scope));
	sprintf(scope, "onvif://www.onvif.org/location/%s", onvifSetting->location);
	Js["Scopes"][1] = std::string(scope);

	Js["Scopes"][2] = "onvif://www.onvif.org/Profile/Streaming";
	Js["Scopes"][3] = "onvif://www.onvif.org/Profile/T";

	if (write_json_file(Js, FCA_ONVIF_SETTING_PATH)) {
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not write: %s\n", FCA_ONVIF_SETTING_PATH);
		return APP_CONFIG_ERROR_FILE_OPEN;
	}
}

int fca_configGetOnvifSetting(fca_onvif_setting_t *onvifSetting) {
	json cfgJs;

	if (read_json_file(cfgJs, FCA_ONVIF_SETTING_PATH)) {
		memset(onvifSetting, 0, sizeof(fca_onvif_setting_t));

		try {
			std::string username   = cfgJs["Username"].get<std::string>();
			std::string password   = cfgJs["Password"].get<std::string>();
			std::string interfaces = cfgJs["Interfaces"].get<std::string>();
			std::string location   = cfgJs["Location"].get<std::string>();
			std::string name	   = cfgJs["Name"].get<std::string>();
			onvifSetting->port	   = cfgJs["Port"].get<int>();

			strncpy(onvifSetting->username, username.c_str(), (username.length() > sizeof(onvifSetting->username) ? sizeof(onvifSetting->username) : username.length()));
			strncpy(onvifSetting->password, password.c_str(), (password.length() > sizeof(onvifSetting->password) ? sizeof(onvifSetting->password) : password.length()));
			strncpy(onvifSetting->interfaces, interfaces.c_str(),
					(interfaces.length() > sizeof(onvifSetting->interfaces) ? sizeof(onvifSetting->interfaces) : interfaces.length()));
			strncpy(onvifSetting->location, location.c_str(), (location.length() > sizeof(onvifSetting->location) ? sizeof(onvifSetting->location) : location.length()));
			strncpy(onvifSetting->name, name.c_str(), (name.length() > sizeof(onvifSetting->name) ? sizeof(onvifSetting->name) : name.length()));

			return APP_CONFIG_SUCCESS;
		}
		catch (...) {
		}
	}

	return APP_CONFIG_ERROR_DATA_INVALID;
}

int fca_configSetRecord(StorageSdSetting_t *storageSd) {
	json recordJs;
	if (fca_jsonSetRecord(recordJs, storageSd)) {
		string filePath = FCA_USER_CONF_PATH "/" FCA_RECORD_FILE;
		if (write_json_file(recordJs, filePath))
			return APP_CONFIG_SUCCESS;
		else {
			APP_DBG("Can not write: %s\n", filePath.data());
			return APP_CONFIG_ERROR_FILE_OPEN;
		}
	}
	else {
		APP_DBG("Cann't convet struct to json\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}

int fca_configGetRecord(StorageSdSetting_t *storageSd) {
	json cfgJs;
	string fileName = FCA_RECORD_FILE;
	if (fca_readConfigUsrDfaulFileToJs(cfgJs, fileName)) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (fca_jsonGetRecord(cfgJs, storageSd)) {
		APP_DBG("Record config: %s\n", cfgJs.dump().data());
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("Can not convert: %s\n", fileName.data());
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
}
