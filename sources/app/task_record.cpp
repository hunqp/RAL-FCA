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
#include "videosource.hpp"
#include "audiosource.hpp"

#include "utils.h"

#define PREVIOUS_MP4_RECORD_DURATION_US (3 * 1000 * 1000)

#define VERIFY_RECORD_CHANNEL(ch) ((ch >= MAIN_VIDEO_CHANNEL && ch < VIDEO_CHANNEL_TOTAL) ? true : false)
#define VERIFY_RECORD_MODE(m) (((STORAGE_SD_OPTION)m >= STORAGE_ONLY_REG && (STORAGE_SD_OPTION)m <= (STORAGE_NONE)) ? true : false)

/* Extern variables ----------------------------------------------------------*/
q_msg_t gw_task_record_mailbox;

/* This variables take responsible for not duplicating post message */
volatile bool bLocalEventsTriggered = true;
volatile bool bLocalEventsRecorderEnable = true;
volatile StorageSdSetting_t storageSdSett;

/* Private variables ---------------------------------------------------------*/
static std::string containerFolder;
static MP4v2Writer recorder;
static pthread_t thCaptureSamplesId[2] = {0, 0};
static volatile bool enablePollingCaptureSamples = false;

/* Private function prototypes -----------------------------------------------*/
static void preparingBeforeCapturingSamples(void);
static void safeCollapseAllCapturingSamples(void);

static void *isolateSafeFormatExtDisk(void *args);
static void *recordingIncomeVideoSamples(void *args);
static void *recordingIncomeAudioSamples(void *args);

/* Function implementation ---------------------------------------------------*/
void *gw_task_record_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;

	wait_all_tasks_started();

	APP_PRINT("[STARTED] GW_TASK_RECORD_ID Entry\n");

	/* Setup SD/EMMC */
	SDCARD.onStatusChange([](SD_STATUS state) {
		switch (state) {
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
		APP_PRINT("Ext-Disk (EMMC/SD) has \"%s\"\r\n", (state == SD_STATE_REMOVED) ? "removed" : (state == SD_STATE_INSERTED) ? "inserted" : "mounted");
	});
	SDCARD.ForceUmount();

	/* Setup recorder */
	recorder.onOpened([](const char *filename) {
		APP_DBG("Create \'%s\' \r\n", filename);
	});
	recorder.onClosed([](RECORDER_INDEX_S *desc) {
		APP_DBG("Closes \'%s\' \r\n", desc->name);
		task_post_dynamic_msg(GW_TASK_RECORD_ID, GW_RECORD_UPDATE_INDEX_DATABASE, (uint8_t*)desc, sizeof(RECORDER_INDEX_S));
		// task_post_pure_msg(GW_TASK_AI_ID, GW_AI_DETECTION_POLL_REFRESH);
	});
	recorder.onFailed([](const char *errors) {
		APP_ERROR("%s", errors);
	});

	FCA_ENCODE_S encoder = videoHelpers.getEncoders();
	recorder.initialise(1280, 720, encoder.minorStream.fps, 8000);

	bLocalEventsTriggered = false;

	// Load configurations
	fca_configGetRecord((StorageSdSetting_t*)&storageSdSett);
	if (storageSdSett.opt == STORAGE_ONLY_REG || storageSdSett.opt == STORAGE_NONE) {
		bLocalEventsRecorderEnable = false;
	}

	while (1) {
		msg = ak_msg_rev(GW_TASK_RECORD_ID);

		switch (msg->header->sig) {
		case GW_RECORD_INIT: {
			APP_DBG_SIG("GW_RECORD_INIT\r\n");

			SDCARD.initialise();
		}
		break;

		case GW_RECORD_ON_MOUNTED: {
			APP_DBG_SIG("GW_RECORD_ON_MOUNTED\r\n");

			task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_PREPARE);
			RAM_SYSI("Ext-Disk has attached\r\n");
		}
		break;

		case GW_RECORD_ON_REMOVED: {
			APP_DBG_SIG("GW_RECORD_ON_REMOVED\r\n");

			SDCARD.ForceUmount();
			safeCollapseAllCapturingSamples();
			timer_remove_attr(GW_TASK_RECORD_ID, GW_RECORD_CLOSE);
			RAM_SYSW("Ext-Disk has detached\r\n");
		}
		break;

		case GW_RECORD_PREPARE: {
			APP_DBG_SIG("GW_RECORD_PREPARE\r\n");

			/* 	Close all current threads are running
				and reopen again. Purpose is if user change
				recording on another channels, this will 
				select corresponding channels
			*/
			safeCollapseAllCapturingSamples();
			preparingBeforeCapturingSamples();
			task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_START);
		}
		break;

		case GW_RECORD_START: {
			APP_DBG_SIG("GW_RECORD_START\r\n");

			containerFolder = SDCARD.prepareContainerFolder();
			timer_set(GW_TASK_RECORD_ID, GW_RECORD_CLOSE, GW_RECORD_MAX_RECORDING_INTERVAL, TIMER_ONE_SHOT);
		}
		break;

		case GW_RECORD_CLOSE: {
			APP_DBG_SIG("GW_RECORD_CLOSE\r\n");

			recorder.setPromiseSafeClosure();
			task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_START);
		}
		break;

		case GW_RECORD_FORMAT_CARD_REQ: {
			APP_DBG_SIG("GW_RECORD_FORMAT_CARD_REQ\r\n");

			/* 	Because operate format may take time, 
				so we MUST do it in separate thread 
			*/
			pthread_t isolateThreadId;
			pthread_create(&isolateThreadId, NULL, isolateSafeFormatExtDisk, NULL);
			pthread_detach(isolateThreadId);
		}
		break;

		case GW_RECORD_GET_CAPACITY_REQ: {
			APP_DBG_SIG("GW_RECORD_GET_CAPACITY_REQ\r\n");

			uint64_t total, free, used;
			SDCARD.GetCapacity(&total, &free, &used);

			APP_PRINT("[EMMC/SD] Total: %lld, Used: %lld, Free: %lld\n", total, used, free);

			nlohmann::json js, sjs;

			sjs = {
				{"Capacity", total},
				{"Usage",	 used },
			};

			if (fca_getConfigJsonRes(js, sjs, MESSAGE_TYPE_STORAGE_INFO, APP_CONFIG_SUCCESS)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)js.dump().data(), js.dump().length() + 1);
			}
		}
		break;

		case GW_RECORD_ADD_METADATA: {
			APP_DBG_SIG("GW_RECORD_ADD_METADATA\r\n");
			
			// EventsPolling_t *evtStatus = (EventsPolling_t*)msg->header->payload;

			// recorder.addMetadata(evtStatus->type, evtStatus->beginTs, evtStatus->endTs);
			// APP_PRINT("Recorder add metadata type %d {%d, %d}\r\n", evtStatus->type, evtStatus->beginTs, evtStatus->endTs);
		}
		break;
		
		case GW_RECORD_UPDATE_INDEX_DATABASE: {
			APP_DBG_SIG("GW_RECORD_UPDATE_INDEX_DATABASE\r\n");

			RECORDER_INDEX_S *desc = (RECORDER_INDEX_S*)msg->header->payload;
			SDCARD.updateIndexDatabase(desc);
			APP_DBG("[UPDATE] Database records add \"%s\", start %d, end %d\r\n", desc->name, desc->startTs, desc->closeTs);
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

			// 	reqSett.opt = reqJs["Mode"].get<STORAGE_SD_OPTION>();
			// 	reqSett.channel = reqJs["Channel"].get<int>();
			// 	if (reqJs.contains("DayStorage")) {
			// 		reqSett.maxDayStorage = reqJs["DayStorage"].get<int>();
			// 	}
			// 	else {
			// 		reqSett.maxDayStorage = 3; /* Default storage at least 3 days */
			// 	}

			// 	if (reqJs.contains("Duration")) {
			// 		reqSett.regDurationInSec 	= reqJs["Duration"].get<int>();
			// 		if (reqSett.regDurationInSec < SDCARD_RECORD_DURATION_MIN || reqSett.regDurationInSec > SDCARD_RECORD_DURATION_MAX) {
			// 			APP_DBG("Setting record time is error: [Duration] - %d", reqSett.regDurationInSec);
			// 			throw runtime_error("Error setting");
			// 		}
			// 	}
			// 	else {
			// 		reqSett.regDurationInSec = SDCARD_RECORD_DURATION_DEFAULT; /* Default 900s */
			// 	}

			// 	if (VERIFY_RECORD_CHANNEL(reqSett.channel) && VERIFY_RECORD_MODE(reqSett.opt)) {
			// 		memcpy((void*)&storageSdSett, &reqSett, sizeof(storageSdSett));
			// 		fca_configSetRecord(&reqSett);
			// 		ret = APP_CONFIG_SUCCESS;

			// 		/* Set local variables */
			// 		if (storageSdSett.opt == STORAGE_ONLY_REG || storageSdSett.opt == STORAGE_NONE) {
			// 			bLocalEventsRecorderEnable = false;
			// 		}
			// 		else bLocalEventsRecorderEnable = true;
			// 	}
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

void preparingBeforeCapturingSamples(void) {
	enablePollingCaptureSamples = true;

	for (uint8_t id = 0; id < 2; ++id) {
		if (!thCaptureSamplesId[id]) {
			pthread_create(
				&thCaptureSamplesId[0], 
				NULL, 
				(id == 0) ? recordingIncomeVideoSamples : recordingIncomeAudioSamples, 
				NULL);
		}
	}
}

void safeCollapseAllCapturingSamples(void) {
	enablePollingCaptureSamples = false;
	for (uint8_t id = 0; id < 2; ++id) {
		if (thCaptureSamplesId[id]) {
			pthread_join(thCaptureSamplesId[id], NULL);
			thCaptureSamplesId[id] = (pthread_t)NULL;
		}
	}
	recorder.close();
}


void *isolateSafeFormatExtDisk(void *) {
	SDCARD.deinitialse();
	safeCollapseAllCapturingSamples();
	sleep(1);

	int rc = SDCARD.ForceFormat();

	sleep(1);
	task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_INIT);
	SYSI("Format Ext-Disk has %s\r\n", (rc == 0) ? "success" : "errors");

	/* Response via data channel */
	nlohmann::json js = {
		{"Id",	   	deviceSerialNumber	},
		{"Type",	"Report"			},
		{"Command", "FormatStorage"		},
		{"Content",
			{{"Success", rc == 0 ? true : false}}
		}
	};

	try {
		pthread_mutex_lock(&Client::mtxClientsProtect);
		for (auto it : clients) {
			auto dc = it.second->dataChannel.value();
			try {
				if (dc->isOpen()) {
					dc->send(js.dump());
				}
			}
			catch (const exception &error) {
				APP_DBG("%s\n", error.what());
			}
		}
		pthread_mutex_unlock(&Client::mtxClientsProtect);
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	/* Public MQTT data */
	json resJs, dataJs;
	if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_STORAGE_FORMAT, APP_CONFIG_SUCCESS)) {
		task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
	}

	return NULL;
}

void *recordingIncomeVideoSamples(void *args) {
	VV_RB_MEDIA_FRAMED_S Frame = {0};
	VV_RB_MEDIA_HANDLE_T consumer = NULL;

	consumer = ringBufferCreateConsumer(VideoHelpers::VIDEO1_PRODUCER, 250 * 1024);
	if (!consumer) {
		RAM_SYSW("Can't video consumer for recording\r\n");
		return NULL;
	}
	APP_DBG("BEGIN -> Recording video local storage\r\n");

	while (enablePollingCaptureSamples) {
		if (ringBufferReadFrom(consumer, &Frame) != 0) {
			usleep(10000);
            continue;
		}

		if (SDCARD.currentState() != SD_STATE_MOUNTED) {
			continue;
		}

		if (Frame.type == VV_RB_MEDIA_FRAME_HDR_TYPE_I) {
			APP_DBG("Frame per seconds: %d\r\n", Frame.framePerSeconds);

			if (recorder.getPromiseSafeClosure()) {
				recorder.close();
			}
			if (!recorder.isOpening()) {
				recorder.begin(containerFolder.c_str(), (uint32_t)time(NULL));
			}
		}

		recorder.addVideoFrame(
			Frame.pData, 
			Frame.dataLen, 
			Frame.framePerSeconds,
			0,
			(Frame.encoder == VV_RB_MEDIA_ENCODER_FRAME_H264) ? MP4V2_MP4_VIDEO_H264 : MP4V2_MP4_VIDEO_H265
		);

	}
	
	APP_DBG("CLOSE -> Recording video local storage\r\n");
	ringBufferDelete(consumer);

	return NULL;
}

void *recordingIncomeAudioSamples(void *args) {
	VV_RB_MEDIA_FRAMED_S Frame = {0};
	VV_RB_MEDIA_HANDLE_T consumer = NULL;
	
	consumer = ringBufferCreateConsumer(AudioHelpers::AUDIO0_PRODUCER, 312);
	if (!consumer) {
		RAM_SYSW("Can't audio consumer for recording\r\n");
		return NULL;
	}
	APP_DBG("BEGIN -> Recording audio local storage\r\n");

	while (enablePollingCaptureSamples) {
		if (ringBufferReadFrom(consumer, &Frame) != 0) {
			usleep(10000);
            continue;
		}

		if (SDCARD.currentState() != SD_STATE_MOUNTED) {
			continue;
		}

		recorder.addAudioFrame(
			Frame.pData, 
			Frame.dataLen, 
			Frame.timestamp,
			MP4V2_MP4_AUDIO_ALAW
		);
	}

	APP_DBG("CLOSE -> Recording audio local storage\r\n");
	ringBufferDelete(consumer);

	return NULL;
}