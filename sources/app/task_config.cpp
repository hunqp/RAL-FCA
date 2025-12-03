#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statfs.h>
#include <fstream>

#include "ak.h"
#include "timer.h"

#include "app.h"
#include "app_dbg.h"
#include "app_config.h"
#include "app_data.h"

#include "task_list.h"
#include "task_config.h"

#include "mqtt.h"
#include "parser_json.h"
#include "utils.h"

#include "fca_isp.hpp"

enum {
	/* general errors */
	FCA_SD_FAILURE = -1, /**< Generic failure code.   */
	FCA_SD_OK	   = 0,
};

#define LINUX_SD_DEV_FILE	  "/dev/mmcblk0p1"
#define LINUX_MOUNT_INFO_FILE "/proc/mounts"
#define CMD_FORMAT_DATA		  "mkfs.vfat"

q_msg_t gw_task_config_mailbox;

static void removeUserFile(const char *fileName);
static void printResetConf(fca_resetConfig_t *reset);
// static void printWifiConf(FCA_NET_WIFI_S *wifi);

void *gw_task_config_entry(void *) {
	wait_all_tasks_started();

	APP_DBG("[STARTED] gw_task_config_entry\n");

	ak_msg_t *msg = AK_MSG_NULL;

	timer_set(GW_TASK_CONFIG_ID, GW_CONFIG_CHECK_MOUNT_USR_CONFIG_REQ, GW_CONFIG_CHECK_MOUNT_USR_CONFIG_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_CONFIG_ID);

		switch (msg->header->sig) {
			// SET
		case GW_CONFIG_SET_RESET_CONFIG_REQ: {
			APP_DBG_SIG("GW_CONFIG_SET_RESET_CONFIG_REQ\n");
			int ret = APP_CONFIG_SUCCESS;
			try {
				json resetCfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				fca_resetConfig_t resetCfgTmp;
				memset(&resetCfgTmp, 0, sizeof(resetCfgTmp));
				if (fca_jsonGetReset(resetCfgJs, &resetCfgTmp)) {
					printResetConf(&resetCfgTmp);
					if (resetCfgTmp.bWatermark == true)
						removeUserFile(FCA_WATERMARK_FILE);
					if (resetCfgTmp.bMotion == true)
						removeUserFile(FCA_MOTION_FILE);
					if (resetCfgTmp.bMQTT == true)
						removeUserFile(FCA_NETWORK_MQTT_FILE);
					if (resetCfgTmp.bRTMP == true)
						removeUserFile(FCA_RTMP_FILE);
					if (resetCfgTmp.bWifi == true)
						removeUserFile(FCA_WIFI_FILE);
					if (resetCfgTmp.bEthernet == true)
						removeUserFile(FCA_ETHERNET_FILE);
					if (resetCfgTmp.bParam == true)
						removeUserFile(FCA_CAMERA_PARAM_FILE);
					if (resetCfgTmp.bEncode == true)
						removeUserFile(FCA_ENCODE_FILE);
					if (resetCfgTmp.bS3 == true)
						removeUserFile(FCA_S3_FILE);
					if (resetCfgTmp.bBucket == true)
						removeUserFile(FCA_BUCKET_SETTING_FILE);
					if (resetCfgTmp.bLedIndi == true)
						removeUserFile(FCA_IO_DRIVER_CONTROL_FILE);
					if (resetCfgTmp.bRainbow == true)
						removeUserFile(FCA_RAINBOW_FILE);
					if (resetCfgTmp.bMotor == true)
						removeUserFile(FCA_STEP_MOTOR_FILE);
					if (resetCfgTmp.bAlarm == true)
						removeUserFile(FCA_ALARM_CONTROL_FILE);
					if (resetCfgTmp.bRtc == true)
						removeUserFile(FCA_RTC_SERVERS_FILE);
					if (resetCfgTmp.bLogin == true)
						removeUserFile(FCA_DEVICE_LOGIN_INFO_FILE);
					if (resetCfgTmp.bSystem == true)
						removeUserFile(FCA_SYSTEM_CONTROLS_FILE);
					if (resetCfgTmp.bP2p == true)
						removeUserFile(FCA_P2P_FILE);
					if (resetCfgTmp.bOnvif == true)
						removeUserFile(FCA_ONVIF_FILE);
				}
				else {
					APP_PRINT("[ERR] data reset config from server invalid\n");
					ret = APP_CONFIG_ERROR_DATA_INVALID;
				}
			}
			catch (const exception &error) {
				APP_DBG("%s\n", error.what());
				ret = APP_CONFIG_ERROR_ANOTHER;
			}

			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_RESET, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}

		} break;

		case GW_CONFIG_SET_UPGRADE_STATUS_REQ: {
			APP_DBG_SIG("GW_CONFIG_SET_UPGRADE_STATUS_REQ\n");

			int ret = systemCmd("echo 0 > %s/%s", FCA_USER_CONF_PATH, FCA_OTA_STATUS);
			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_UPGRADE_STATUS, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}

		} break;

		case GW_CONFIG_SET_OWNER_CONFIG_REQ: {
			APP_DBG_SIG("GW_CONFIG_SET_OWNER_CONFIG_REQ\n");

			int ret = APP_CONFIG_ERROR_DATA_INVALID;
			try {
				json ownerCfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));

				bool status = ownerCfgJs["Status"].get<bool>();
				if (status) {
					systemCmd("echo 1 > %s/%s", FCA_USER_CONF_PATH, FCA_OWNER_STATUS);
				}
				else {
					systemCmd("rm -f %s/%s", FCA_USER_CONF_PATH, FCA_OWNER_STATUS);
				}
				ret = APP_CONFIG_SUCCESS;
			}
			catch (const exception &error) {
				APP_DBG("%s\n", error.what());
				ret = APP_CONFIG_ERROR_ANOTHER;
			}
			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_OWNER_STATUS, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_CONFIG_GET_SYSINFO_CONFIG_REQ: {
			APP_DBG_SIG("GW_CONFIG_GET_SYSINFO_CONFIG_REQ\n");
			fca_sysInfo_t sysInfo = {0};
			json dataJs			  = json::object();
			int ret				  = fca_configGetSysInfo(&sysInfo);
			if (ret == APP_CONFIG_SUCCESS) {
				if (!fca_jsonSetSysInfo(dataJs, &sysInfo)) {
					APP_DBG("[ERR] data sysInfo config invalid\n");
					ret = APP_CONFIG_ERROR_DATA_INVALID;
				}
			}
			json resJs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_SYSTEM_INFO, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}

		} break;

		case GW_CONFIG_CLOUD_SET_LED_INDICATOR_REQ: {
			APP_DBG_SIG("GW_CONFIG_CLOUD_SET_LED_INDICATOR_REQ\n");
			int8_t ret = APP_CONFIG_ERROR_DATA_INVALID;
			try {
				json data = json::parse(string((char *)msg->header->payload, msg->header->len));
				int ledSt = data["Status"].get<int>();
				ret		  = ledStatus.userSetState(ledSt);
			}
			catch (const exception &error) {
				APP_DBG("%s\n", error.what());
			}
			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_CONTROL_LED_INDICATOR, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_CONFIG_CLOUD_GET_LED_INDICATOR_REQ: {
			APP_DBG_SIG("GW_CONFIG_CLOUD_GET_LED_INDICATOR_REQ\n");
			int usrLedSt, ret;
			json dataJs = json::object();

			ret	   = ledStatus.getUserControlFromFile(&usrLedSt);
			dataJs = {
				{"Status", usrLedSt}
			};

			json resJs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_CONTROL_LED_INDICATOR, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ: {
			// APP_DBG_SIG("GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ\n");
			fca_cameraParam_t param = ispCtrl.CamParam;
			bool lastState			= gpiosHelpers.irLighting.getLightingState() == IR_Lighting::STATUS::ON ? true : false;
			int ctrlState			= ispCtrl.getDayNightState();
			if (ctrlState == DN_DAY_STATE) {
				ispCtrl.controlLighting(false);
				APP_DBG_ISP("\tlighting OFF\n");
			}
			else if (ctrlState == DN_NIGHT_STATE) {
				ispCtrl.controlLighting(true);
				APP_DBG_ISP("\tlighting ON\n");
			}
			bool curState = gpiosHelpers.irLighting.getLightingState() == IR_Lighting::STATUS::ON ? true : false;
			APP_DBG_ISP("lighting lastSt: %d, curSt: %d\n", lastState, curState);
			if (curState != lastState) {
				if (0 == param.dnParam.lockCntTh || 0 == param.dnParam.lockTime) {
					APP_DBG_ISP("disable lock feture in lighting mode\n");
					break;
				}

				uint32_t lockCntTh = param.dnParam.lockCntTh * 2;
				uint32_t lockTime  = param.dnParam.lockTime * 1000;

				if (++ispCtrl.lightingChangeCount == 1) {
					ispCtrl.lightingCheckStartTime = time(nullptr);
					APP_DBG_ISP("lighting start time check: %d\n", ispCtrl.lightingCheckStartTime);
				}
				else if (ispCtrl.lightingChangeCount >= lockCntTh) {	// If there are lockCntTh consecutive on/off times, increase the test interval
					uint32_t duration = (uint32_t)time(nullptr) - ispCtrl.lightingCheckStartTime;
					APP_DBG_ISP("duration: %d\n", duration);
					if (duration < DAY_NIGHT_TRANSITION_CHECK_INTERVAL) {	 // < 1min
						if (curState) {
							APP_DBG_ISP("prefer lighting OFF\n");
							ispCtrl.controlLighting(false);
						}
						APP_DBG_ISP("change timer check lighting\n");
						timer_remove_attr(GW_TASK_CONFIG_ID, GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ);
						APP_DBG_ISP("locktime: %d(ms)\n", lockTime);
						timer_set(GW_TASK_CONFIG_ID, GW_CONFIG_RESTART_AUTO_LIGHTING_CONTROL_REQ, lockTime, TIMER_ONE_SHOT);	// reset timer
					}
					ispCtrl.lightingChangeCount = 0;
				}
				APP_DBG_ISP("lighting change cout: %d\n", ispCtrl.lightingChangeCount);
			}
		} break;

		case GW_CONFIG_RESTART_AUTO_LIGHTING_CONTROL_REQ: {
			APP_DBG_SIG("GW_CONFIG_RESTART_AUTO_LIGHTING_CONTROL_REQ\n");
			ispCtrl.lightingChangeCount = 0;
			timer_remove_attr(GW_TASK_CONFIG_ID, GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ);
			timer_set(GW_TASK_CONFIG_ID, GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ, GW_CONFIG_AUTO_LIGHTING_CONTROL_INTERVAL, TIMER_PERIODIC);
			task_post_pure_msg(GW_TASK_CONFIG_ID, GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ);
		} break;

		case GW_CONFIG_CHECK_MOUNT_USR_CONFIG_REQ: {
			APP_DBG_SIG("GW_CONFIG_CHECK_MOUNT_USR_CONFIG_REQ\n");

			string conf_path = "/usr/fpt";
			string dev_blk	 = systemStrCmd("grep '\"USR_FPT\"' /proc/mtd | awk -F: '{print $1}' | sed 's/mtd/\\/dev\\/mtdblock/'");
			if (dev_blk.find("mtdblock") == string::npos) {
				APP_DBG("get partition name %s failed\n", conf_path.c_str());
				break;
			}
			APP_DBG("get partition name %s, mtd: %s success\n", conf_path.c_str(), dev_blk.data());

			std::string testFilePath = conf_path + "/mount_check.tmp";
			std::ofstream testFile(testFilePath);
			bool canWrite = testFile.is_open();
			testFile.close();

			if (!canWrite) {
				APP_DBG("retry mount %s\n", conf_path.c_str());
				systemCmd("sync");
				systemCmd("umount -l %s", conf_path.c_str());
				systemCmd("mount -t jffs2 %s %s", dev_blk.data(), conf_path.c_str());
				timer_set(GW_TASK_CONFIG_ID, GW_CONFIG_CHECK_MOUNT_USR_CONFIG_REQ, GW_CONFIG_CHECK_MOUNT_USR_CONFIG_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
				LOG_FILE_ERROR("mount %s failed\n", conf_path.c_str());
			}
			else {
				APP_DBG("mount %s ok\n", conf_path.c_str());
				std::remove(testFilePath.c_str());
			}
		} break;

		case GW_CONFIG_SET_TIMEZONE_REQ: {
			APP_DBG_SIG("GW_CONFIG_SET_TIMEZONE_REQ\n");
			int ret = APP_CONFIG_ERROR_ANOTHER;
			try {
				json cfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				string tz  = cfgJs["Timezone"].get<string>();
				ret		   = fca_configSetTZ(tz);
				if (ret == APP_CONFIG_SUCCESS) {
					string tzConvert = reverseUTCOffset(tz);
					APP_DBG("cloud set new timezone: %s, reversed: %s\n", tz.data(), tzConvert.data());
					setenv("TZ", tzConvert.data(), 1);
					tzset();
				}
			}
			catch (const exception &error) {
				APP_DBG("%s\n", error.what());
				ret = APP_CONFIG_ERROR_DATA_INVALID;
			}
			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_TIMEZONE, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_CONFIG_GET_TIMEZONE_REQ: {
			APP_DBG_SIG("GW_CONFIG_GET_TIMEZONE_REQ\n");
			int ret;
			string tz;
			json dataJs = json::object();
			ret			= fca_configGetTZ(tz);
			if (ret == APP_CONFIG_SUCCESS) {
				dataJs = {
					{"Timezone", tz}
				};
			}

			json resJs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_TIMEZONE, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		}
		break;

		case GW_CONFIG_WATCHDOG_PING_REQ: {
			// APP_DBG_SIG("GW_CONFIG_WATCHDOG_PING_REQ\n");
			task_post_pure_msg(GW_TASK_CONFIG_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
		}
		break;

		case GW_CONFIG_FORCE_MOTOR_STOP_REQ: {
			APP_DBG_SIG("GW_CONFIG_FORCE_MOTOR_STOP_REQ\n");

			int h, v;
			motors.run(FCA_PTZ_DIRECTION_STOP);
			motors.getPosition(&h, &v);
			nlohmann::json js = {
				{"Horizone", h},
				{"Vervical", v}
			};
			write_json_file(js, (FCA_VENDORS_FILE_LOCATE(FCA_STEP_MOTOR_FILE)));
		}
		break;

		default:
		break;
		}

		/* free message */
		ak_msg_free(msg);
	}

	return (void *)0;
}

void removeUserFile(const char *fileName) {
	string path = FCA_USER_CONF_PATH + string("/") + string(fileName);
	if (remove(path.c_str()) == 0) {
		APP_DBG("Delete file [%s] successed\n", path.data());
	}
	else {
		APP_DBG("Delete file [%s] failed\n", path.data());
	}
}

void printResetConf(fca_resetConfig_t *reset) {
	APP_DBG("[VAR] --------------------- [Reset] ---------------------\n");
	APP_DBG("[VAR] WatermarkSetting: %d\n", reset->bWatermark);
	APP_DBG("[VAR] MotionSetting: %d\n", reset->bMotion);
	APP_DBG("[VAR] MQTTSetting: %d\n", reset->bMQTT);
	APP_DBG("[VAR] RTMPSetting: %d\n", reset->bRTMP);
	APP_DBG("[VAR] WifiSetting: %d\n", reset->bWifi);
	APP_DBG("[VAR] EncodeSetting: %d\n", reset->bEncode);
}

// void printWifiConf(FCA_NET_WIFI_S *wifi) {
// 	APP_DBG("[VAR] --------------------- [Wifi] ---------------------\n");
// 	APP_DBG("[VAR] enable: %d\n", wifi->enable);
// 	APP_DBG("[VAR] ssid: %s\n", wifi->ssid);
// 	APP_DBG("[VAR] keys: %s\n", wifi->keys);
// 	APP_DBG("[VAR] keyType: %d\n", wifi->keyType);
// 	APP_DBG("[VAR] auth: %s\n", wifi->auth);
// 	APP_DBG("[VAR] channel: %d\n", wifi->channel);
// 	APP_DBG("[VAR] encrypType: %s\n", wifi->encrypType);
// 	APP_DBG("[VAR] gateWay: %s\n", wifi->gateWay);
// 	APP_DBG("[VAR] hostIP: %s\n", wifi->hostIP);
// 	APP_DBG("[VAR] submask: %s\n", wifi->submask);
// 	APP_DBG("[VAR] dhcp: %d\n", wifi->dhcp);
// }
