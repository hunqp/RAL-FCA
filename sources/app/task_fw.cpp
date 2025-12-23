#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ak.h"
#include "timer.h"
#include "utils.h"
#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "ota.h"
#include "firmware.h"
#include "task_list.h"
#include "task_fw.h"
#include "fca_gpio.hpp"

q_msg_t gw_task_fw_mailbox;

using namespace std;
using json = nlohmann::json;

#define TAG "[OTA] "

#define FIRMWARE_DOWNLOAD_TIMEOUT 300
#define FIRMWARE_FILE			  "/tmp/firmware.bin"

std::atomic<bool> isOtaRunning(false);

static bool getUrlResponseStatus(int status) {
	string msg;
	if (status == FCA_OTA_OK) {
		msg = "Success";
	}
	else {
		msg = "Fail";
	}
	try {
		nlohmann::json resJSon;
		resJSon = {
			{"Method"		, "SET"					},
			{"MessageType"	, MESSAGE_TYPE_UPGRADE	},
			{"Serial"		, fca_getSerialInfo()	},
			{"Data"			, {}					},
			{"Result"		, {
				{"Ret"		, status}, 
				{"Message"	, msg	}
			}},
			{"Timestamp"	,  time(nullptr)		},
		};

		task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJSon.dump().data(), resJSon.dump().length() + 1);
		return true;
	}
	catch (const exception &e) {
		APP_DBG_OTA(TAG "json error: %s\n", e.what());
	}
	return false;
}

void *gw_task_fw_entry(void *) {
	wait_all_tasks_started();

	APP_DBG_OTA(TAG "[STARTED] gw_task_upload_entry\n");
	ak_msg_t *msg;

	string url;
	string md5;

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_FW_ID);

		switch (msg->header->sig) {
		case GW_FW_GET_URL_REQ: {
			APP_DBG_SIG("GW_FW_GET_URL_REQ\n");

			if (isOtaRunning.load()) {
				APP_PRINT("ota is running\n");
				break;
			}

			int ret = FCA_OTA_FAIL;
			try {
				json message = json::parse(string((char *)msg->header->payload, msg->header->len));
				APP_DBG_OTA(TAG "[%s]\n", message.dump().data());

				url = message["FirmwareInfo"].get<string>().data();
				md5 = message["MD5"].get<string>().data();

				string str = "TEST-OTA";
				// #if BUILD_PLAY4
				// str = "P04L";
				// #elif BUILD_IQ4
				// str = "I04L";
				// #elif BUILD_SE3
				// str = "S04L";
				// #elif BUILD_SE3S
				// str = "S05L";
				// #endif
				APP_DBG_OTA("check model ota for: %s\n", str.c_str());
				ret = str.empty() || url.find(str) != std::string::npos ? FCA_OTA_OK : FCA_OTA_FAIL;
			}
			catch (const exception &error) {
				APP_DBG_OTA(TAG "%s\n", error.what());
			}

			getUrlResponseStatus(ret);

			if (ret == FCA_OTA_OK) {
				task_post_pure_msg(GW_TASK_FW_ID, GW_FW_DOWNLOAD_START_REQ);
				isOtaRunning.store(true);

				// stop onvif services
				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_STOP_ONVIF_REQ);
				// stop AI
				task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_DISABLE_AI_REQ);
				sleep(3); // wait for stop AI
			}
			else {
				APP_DBG_OTA(TAG "get url failed\n");
			}
		} break;

		case GW_FW_DOWNLOAD_START_REQ: {
			APP_DBG_SIG("GW_OTA_DOWNLOAD_START\n");
			MainThread.dispatch([url, md5]() {
				int ret = fca_otaDownloadFile(url, FIRMWARE_FILE, FIRMWARE_DOWNLOAD_TIMEOUT);
				if (ret == FCA_OTA_OK) {
					string crc = get_md5_file(FIRMWARE_FILE);
					if (strcmp(crc.data(), md5.data()) != 0) {
						APP_DBG_OTA(TAG "MD5sum firmware is incorrect, crc: %s, md5: %s\n", crc.data(), md5.data());
						ret = FCA_OTA_STATE_DL_FAIL;
					}
					else {
						APP_DBG_OTA(TAG "MD5sum firmware is correct\n");
						ret = FCA_OTA_STATE_DL_SUCCESS;
					}
				}
				else {
					APP_DBG_OTA(TAG "download firmware failed\n");
					ret = FCA_OTA_STATE_DL_FAIL;
				}
				systemCmd("echo %d > %s/%s", ret, vendorsConfigurationPath, FCA_OTA_STATUS);
				task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_GEN_MQTT_STATUS_REQ);
				
				if (ret != FCA_OTA_STATE_DL_SUCCESS) {
					systemCmd("rm -f %s", FIRMWARE_FILE);
					APP_DBG_OTA(TAG "download file firmware failed: %d\n", ret);
					task_post_pure_msg(GW_TASK_FW_ID, GW_FW_RELEASE_OTA_STATE_REQ);
					return;
				}
				sleep(1); // wait for response to MQTT server
				
				APP_DBG_OTA("OTA stop watchdog\n");
				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_STOP_WATCH_DOG_REQ);
				task_post_pure_msg(GW_TASK_FW_ID, GW_FW_UPGRADE_START_REQ);
				LOG_FILE_INFO("start OTA firmware\n");
			});
		} break;

		case GW_FW_UPGRADE_START_REQ: {
			APP_DBG_SIG("GW_FW_UPGRADE_START_REQ\n");

			LOG_FILE_INFO("start upgrade fw, url: %s, md5: %s", url.data(), md5.data());
			// ledStatus.controlLedEvent(Indicator::EVENT::UPDATE_FIRMWARE);
			int ret = fca_sys_fw_ota((char *)FIRMWARE_FILE);
			if (ret != 0) {
				LOG_FILE_INFO("OTA firmware failed: %d", ret);
				isOtaRunning.store(false);
				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_REBOOT_REQ);
			}
			else {
				systemCmd("echo 1 > %s/%s", vendorsConfigurationPath, FCA_OTA_STATUS);
				timer_set(GW_TASK_FW_ID, GW_FW_RELEASE_OTA_STATE_REQ, GW_FW_UPGRADE_FIRMWARE_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);	   // idle state OTA after 5min
			}

		} break;

		case GW_FW_RELEASE_OTA_STATE_REQ: {
			APP_DBG_SIG("GW_FW_RELEASE_OTA_STATE_REQ\n");
			isOtaRunning.store(false);
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_START_WATCH_DOG_REQ);
			task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_ENABLE_AI_REQ);
			LOG_FILE_INFO("stop OTA firmware\n");
		} break;

		case GW_FW_WATCHDOG_PING_REQ: {
			task_post_pure_msg(GW_TASK_FW_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
		} break;

		default:
			break;
		}

		/* free message */
		ak_msg_free(msg);
	}

	return (void *)0;
}
