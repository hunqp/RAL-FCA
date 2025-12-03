#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/un.h>
#include <iostream>
#include <optional>

#include "ak.h"

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "task_list.h"
#include "task_record.h"
#include "SdCard.h"
#include "fca_common.hpp"
#include "parser_json.h"

#include "utils.h"

#define VERIFY_RECORD_CHANNEL(ch) ((ch >= MAIN_VIDEO_CHANNEL && ch < VIDEO_CHANNEL_TOTAL) ? true : false)
#define VERIFY_RECORD_MODE(m) (((STORAGE_SD_OPTION)m >= STORAGE_ONLY_REG && (STORAGE_SD_OPTION)m <= (STORAGE_NONE)) ? true : false)

/* Extern variables ----------------------------------------------------------*/
q_msg_t gw_task_record_mailbox;

/* This variables take responsible for not duplicating post message */
volatile bool bLocalEventsTriggered = true;
volatile bool bLocalEventsRecorderEnable = true;
volatile StorageSdSetting_t storageSdSett;

/* Private variables ---------------------------------------------------------*/
static SD_STATUS lastSdState = SD_STATE_REMOVED;

/* Private function prototypes -----------------------------------------------*/
static void *vThreadStorageManagement(void *);
static void *vThreadDedicatedFormatSd(void *);

/* Function implementation ---------------------------------------------------*/
void *gw_task_record_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;

	wait_all_tasks_started();

	APP_PRINT("[STARTED] GW_TASK_RECORD_ID Entry\n");

	// MUST-BE wait for a few seconds avoid system startup
	// and send notification HD/MD to record immediately
	// when all variables hasn't initialized.
	sleep(5);
	bLocalEventsTriggered = false;

	// Load configurations
	fca_configGetRecord((StorageSdSetting_t*)&storageSdSett);
	if (storageSdSett.opt == STORAGE_ONLY_REG || storageSdSett.opt == STORAGE_NONE) {
		bLocalEventsRecorderEnable = false;
	}

	/// Create thread dedicated check Sd status

	// TODO
	// pthread_t createId;
	// pthread_create(&createId, NULL, vThreadStorageManagement, NULL);

	while (1) {
		msg = ak_msg_rev(GW_TASK_RECORD_ID);

		switch (msg->header->sig) {
		case GW_RECORD_ON_MOUNTED: {
			APP_DBG_SIG("GW_RECORD_ON_MOUNTED\r\n");

			sdCard.loadingStatistic();
			task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR);
		}
		break;

		case GW_RECORD_ON_REMOVED: {
			APP_DBG_SIG("GW_RECORD_ON_REMOVED\r\n");

			sdCard.erasingStatistic();
			sdCard.doOPER_CloseRecord();
			timer_remove_attr(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR);
		}
		break;

		case GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR: {
			APP_DBG_SIG("GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR\r\n");

			/* PARSER ATTRIBUTES: "local_record_rgl_enable" */
			bool rglEnable = true;

			if (storageSdSett.opt == STORAGE_ONLY_EVT || storageSdSett.opt == STORAGE_NONE) {
				rglEnable = false;
			}

			/* -- BEGIN RECORDING TYPE REGULAR -- */
			timer_remove_attr(GW_TASK_RECORD_ID, GW_RECORD_FORCE_CLOSE_STORAGE_LOCAL_EVENTS);

			sdCard.doOPER_CloseRecord();
			if (rglEnable) {
				sdCard.doOPER_BeginRecord(RECORD_TYPE_REG);
				uint32_t durationInSec = (storageSdSett.regDurationInSec < 60) ? SDCARD_RECORD_DURATION_DEFAULT : storageSdSett.regDurationInSec;
				timer_set(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR, durationInSec * 1000, TIMER_ONE_SHOT);
			}
			bLocalEventsTriggered = false;
		}
		break;

		case GW_RECORD_STORAGE_LOCAL_TYPE_EVENTS: {
			APP_DBG_SIG("GW_RECORD_STORAGE_LOCAL_TYPE_EVENTS\r\n");

			RecorderSdTrigger_t *trigger = (RecorderSdTrigger_t *)msg->header->payload;

			// AVOID: Duplicate signal
			if (sdCard.recorderCurType() == trigger->type) {
				APP_PRINT("The current record is type events\r\n");
				break;
			}

			// If current regular recorder duration > 15s, let close it and open event recorder
			// Else if current regular recorder duration < 15s, let change type from regular to
			// event and continue saving on this file.
			uint32_t curTsInSecs = (uint32_t)time(NULL);
			uint32_t durationElapsedInSecs = curTsInSecs - sdCard.recorderStartTs();

			if (durationElapsedInSecs > 15) {
				sdCard.doOPER_CloseRecord();
				sdCard.doOPER_BeginRecord(trigger->type, trigger->timestamp);
			}
			else {
				sdCard.recorderChangeType(trigger->type);
			}
		}
		break;

		case GW_RECORD_FORCE_CLOSE_STORAGE_LOCAL_EVENTS: {
			APP_DBG_SIG("GW_RECORD_FORCE_CLOSE_STORAGE_LOCAL_EVENTS\r\n");

			sdCard.doOPER_CloseRecord();
			timer_remove_attr(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR);
			/* Begin recorder type regular immediately */
			task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR);
			bLocalEventsTriggered = false;
		}
		break;

		case GW_RECORD_FORMAT_CARD_REQ: {
			APP_DBG_SIG("GW_RECORD_FORMAT_CARD_REQ\r\n");

			pthread_t fmtSdId;
			pthread_create(&fmtSdId, NULL, vThreadDedicatedFormatSd, NULL);
		}
		break;

		case GW_RECORD_GET_CAPACITY_REQ: {
			APP_DBG_SIG("GW_RECORD_GET_CAPACITY_REQ\r\n");

			uint64_t total, free, used;
			sdCard.doOPER_GetCapacity(&total, &free, &used);

			APP_PRINT("[EMMC/SD] Total: %lld, Used: %lld, Free: %lld\n", total, used, free);

			json resJs, dataJs;

			dataJs = {
				{"Capacity", total},
				{"Usage",	 used },
			};

			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_STORAGE_INFO, APP_CONFIG_SUCCESS)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		}
		break;

		case GW_RECORD_WATCHDOG_PING_REQ: {
			APP_DBG_SIG("GW_RECORD_WATCHDOG_PING_REQ\n");
			task_post_pure_msg(GW_TASK_RECORD_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
		}
		break;

		case GW_RECORD_GET_RECORD_REQ: {
			APP_DBG_SIG("GW_RECORD_GET_RECORD_REQ\n");

			json dataJs;
			dataJs["Mode"] = (int)storageSdSett.opt;
			dataJs["Channel"] = (int)storageSdSett.channel;
			dataJs["Duration"] = (int)storageSdSett.regDurationInSec;
			dataJs["DayStorage"] = (int)storageSdSett.maxDayStorage;

			json resJs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_RECORD, APP_CONFIG_SUCCESS)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		}
		break;

		case GW_RECORD_SET_RECORD_REQ: {
			APP_DBG_SIG("GW_RECORD_SET_RECORD_REQ\n");

			int ret = APP_CONFIG_ERROR_DATA_INVALID;
			StorageSdSetting_t reqSett;

			try {
				json reqJs = json::parse(string((char *)msg->header->payload, msg->header->len));

				reqSett.opt = reqJs["Mode"].get<STORAGE_SD_OPTION>();
				reqSett.channel = reqJs["Channel"].get<int>();
				if (reqJs.contains("DayStorage")) {
					reqSett.maxDayStorage = reqJs["DayStorage"].get<int>();
				}
				else {
					reqSett.maxDayStorage = 3; /* Default storage at least 3 days */
				}

				if (reqJs.contains("Duration")) {
					reqSett.regDurationInSec 	= reqJs["Duration"].get<int>();
					if (reqSett.regDurationInSec < SDCARD_RECORD_DURATION_MIN || reqSett.regDurationInSec > SDCARD_RECORD_DURATION_MAX) {
						APP_DBG("Setting record time is error: [Duration] - %d", reqSett.regDurationInSec);
						throw runtime_error("Error setting");
					}
				}
				else {
					reqSett.regDurationInSec = SDCARD_RECORD_DURATION_DEFAULT; /* Default 900s */
				}

				if (VERIFY_RECORD_CHANNEL(reqSett.channel) && VERIFY_RECORD_MODE(reqSett.opt)) {
					memcpy((void*)&storageSdSett, &reqSett, sizeof(storageSdSett));
					fca_configSetRecord(&reqSett);
					ret = APP_CONFIG_SUCCESS;

					/* Set local variables */
					if (storageSdSett.opt == STORAGE_ONLY_REG || storageSdSett.opt == STORAGE_NONE) {
						bLocalEventsRecorderEnable = false;
					}
					else bLocalEventsRecorderEnable = true;
				}
			}
			catch (const exception &e) {
				std::cout << e.what() << std::endl;
			}

			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_RECORD, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		}
		break;

		default:
		break;
		}

		ak_msg_free(msg);
	}

	return (void *)0;
}

static void waitForNTPSynchronized() {
	int ntp = 0;

	while (ntp != 1) {
		ntp = getNtpStatus();
		sleep(5);
	}
}

void *vThreadStorageManagement(void *) {
	uint32_t nowMs = 0;

	/* Wait for NTP run completed avoid recording timestamp since 2000 */
	waitForNTPSynchronized();

	for (;;) {
		sdCard.scan();
		SD_STATUS state = sdCard.currentState();

		if (state != lastSdState) {
			lastSdState = state;

			switch (lastSdState) {
			case SD_STATE_REMOVED: {
				task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_ON_REMOVED);
			}
			break;

			case SD_STATE_INSERTED: {
			}
			break;

			case SD_STATE_MOUNTED: {
				task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_ON_MOUNTED);
			}
			break;

			default:
			break;
			}
		}
		APP_PRINT("[SCAN] Detect Sd %s\r\n", lastSdState == SD_STATE_REMOVED ? "REMOVED" : lastSdState == SD_STATE_INSERTED ? "INSERTED" : "MOUNTED");

		if (time(NULL) - nowMs > 60) {
			nowMs = time(NULL);
			/* -- Periodic maintain capacity at least 20% free space -- */
			if (lastSdState == SD_STATE_MOUNTED) {
				uint64_t total, free, used;
				if (sdCard.doOPER_GetCapacity(&total, &free, &used) == SD_OPER_SUCCESS) {
					int percent = (int)(((double)used / (double)total) * 100.0);

					APP_PRINT("Sd Total: %llu\r\n", total);
					APP_PRINT("Sd Used : %llu\r\n", used);
					APP_PRINT("Sd Free : %llu\r\n", free);

					if (percent > 80) {
						sdCard.eraseOldestFolder();
					}
				}

				if (1) {
					sdCard.clean();
				}
			}
		}

		sleep(5);
	}

	return 0;
}

void *vThreadDedicatedFormatSd(void *) {
	sdCard.doOPER_CloseRecord();
	int ret = sdCard.doOPER_Format();
	APP_PRINT("[FORMAT] Sd return %d\r\n", ret);
	lastSdState = SD_STATE_REMOVED;

	/* Respond via data channel */
	json repJS = {
		{"Id",	   	fca_getSerialInfo()},
		{"Type",	"Report"},
		{"Command", "FormatStorage"},
		{"Content",
			{{"Success", ret == SD_OPER_SUCCESS ? true : false}}
		}
	};

	try {
		lockMutexListClients();
		for (auto it : clients) {
			auto dc = it.second->dataChannel.value();
			try {
				if (dc->isOpen()) {
					dc->send(repJS.dump());
				}
			}
			catch (const exception &error) {
				APP_DBG("%s\n", error.what());
			}
		}
		unlockMutexListClients();
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	pthread_detach(pthread_self());
	return NULL;
}