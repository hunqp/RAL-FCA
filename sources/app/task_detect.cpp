#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ak.h"
#include "timer.h"
#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "task_list.h"
#include "task_detect.h"
#include "parser_json.h"
#include "fca_motion.hpp"
#include "fca_video.hpp"
#include "fca_audio.hpp"
#include "fca_isp.hpp"
#include "fca_gpio.hpp"

q_msg_t gw_task_detect_mailbox;

static int retrySnapshot = 0;
static fca_alarmControl_t alarmControlSetting;
static S3Notification_t notificationS3;
static AlarmTrigger_t alarmTrigger;
bool bSoundTriggerStatus = false;
bool bLightTriggerStatus = false;

static void triggerSoundAlarm(bool bSound);
static void triggerLightAlarm(bool bLight);
static void reportAlarmTriggerStatus();

void *gw_task_detect_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;
	wait_all_tasks_started();
	APP_DBG_MD("[STARTED] Task Motion Detect\n");

	// MP4Handle.onOpened([&]() {
	// 	videoStreamBufferChannel1.addRegisterOldest((void **)&MP4Handle.videoTrackSamples);
	// 	audioStreamBufferChannel1.addRegisterOldest((void **)&MP4Handle.audioTrackSamples);
	// });

	// MP4Handle.onClosed([&]() {
	// 	videoStreamBufferChannel1.unregister((void **)&MP4Handle.videoTrackSamples);
	// 	audioStreamBufferChannel1.unregister((void **)&MP4Handle.audioTrackSamples);
	// 	MP4Handle.videoTrackSamples = NULL;
	// 	MP4Handle.audioTrackSamples = NULL;
	// });

	// MP4Handle.onWriteFrame([&](bool bIsVideoFrame) {
	// 	DataSource *track = (bIsVideoFrame) ? (DataSource *)MP4Handle.videoTrackSamples : (DataSource *)MP4Handle.audioTrackSamples;
	// 	if (bIsVideoFrame) {
	// 		MP4Handle.writeFrame(track->fData, track->fDataLen, (track->fMicroseconds / 1000), true);
	// 		videoStreamBufferChannel1.nextFramedRegister((void **)&MP4Handle.videoTrackSamples);
	// 	}
	// 	else {
	// 		MP4Handle.writeFrame(track->fData, track->fDataLen, (track->fMicroseconds / 1000), false);
	// 		audioStreamBufferChannel1.nextFramedRegister((void **)&MP4Handle.audioTrackSamples);
	// 	}
	// });

#ifndef RELEASE
	std::string motionJpegPath = FCA_MEDIA_JPEG_PATH + std::string("/") + FCA_IMAGE_S3_MOTION_FILE;
	std::string motionMp4Path  = FCA_MEDIA_MP4_PATH + std::string("/") + FCA_RECORD_S3_MOTION_FILE;

	if (doesFileExist(motionJpegPath.c_str())) {
		remove(motionJpegPath.c_str());
	}
	if (doesFileExist(motionMp4Path.c_str())) {
		remove(motionMp4Path.c_str());
	}
#endif

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_DETECT_ID);

		switch (msg->header->sig) {
		// case GW_DETECT_INIT_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_INIT_REQ\n");

		// 	if (fca_configGetAlarmControl(&alarmControlSetting) != APP_CONFIG_SUCCESS) {
		// 		alarmControlSetting.setSound	   = false;
		// 		alarmControlSetting.setLighting	   = false;
		// 		alarmControlSetting.durationInSecs = 30;
		// 	}

		// 	APP_DBG_MD("[ALARM] Sound   : %d\r\n", alarmControlSetting.setSound);
		// 	APP_DBG_MD("[ALARM] Light   : %d\r\n", alarmControlSetting.setLighting);
		// 	APP_DBG_MD("[ALARM] Duration: %d\r\n", alarmControlSetting.durationInSecs);

		// 	if (eventDetection.initialise() != 0) {
		// 		APP_DBG_MD("[FAILED] Event detection initialise\n");
		// 		break;
		// 	}
		// 	else {
		// 		APP_DBG_MD("[SUCCESS] Event detection initialise\n");
		// 	}

		// 	fca_motionSetting_t config;
		// 	memset(&config, 0, sizeof(config));
		// 	if (fca_configGetMotion(&config) == APP_CONFIG_SUCCESS) {
		// 		APP_DBG_MD("[SUCCESS] Event detection get configuration\n");
		// 		if (eventDetection.validConfig(&config) == APP_CONFIG_SUCCESS) {
		// 			eventDetection.setConfig(&config);
		// 			eventDetection.print();
		// 			task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_START_DETECT_REQ);
		// 		}
		// 		else {
		// 			APP_DBG_MD("[INVALID] Event detection configuration\n");
		// 		}
		// 	}
		// 	else {
		// 		APP_DBG_MD("[FAILED] Event detection get configuration\n");
		// 	}
		// } break;

		// case GW_DETECT_SET_MOTION_CONFIG_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_SET_MOTION_CONFIG_REQ\n");

		// 	int ret = APP_CONFIG_ERROR_ANOTHER;

		// 	fca_motionSetting_t motionCfg = {0};

		// 	/*get old config*/
		// 	if (fca_configGetMotion(&motionCfg) != APP_CONFIG_SUCCESS) {
		// 		memset(&motionCfg, 0, sizeof(motionCfg));
		// 	}

		// 	try {
		// 		json motionCfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));

		// 		ret = fca_jsonGetMotion(motionCfgJs, &motionCfg);
		// 		if ((ret == APP_CONFIG_ERROR_DATA_MISSING || ret == APP_CONFIG_SUCCESS) && eventDetection.validConfig(&motionCfg) == 0) {
		// 			fca_configSetMotion(&motionCfg);	// set new config
		// 			eventDetection.stop();
		// 			ret = (eventDetection.setConfig(&motionCfg) == 0) ? APP_CONFIG_SUCCESS : APP_CONFIG_ERROR_DATA_INVALID;
		// 			task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_START_DETECT_REQ);

		// 			// SYSLOG_LOG(
		// 			// 	LOG_INFO,
		// 			// 	"logf=%s [MOTION] set motion config - enable: %d, interval: %d, humanAttr: %d, humanFocus: %d, humanDraw: %d, md_send: %d, hd_send: %d, clipAttr: %d",
		// 			// 	APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, motionCfg.enable, motionCfg.interval, motionCfg.humanDetAtt, motionCfg.humanFocus, motionCfg.humanDraw,
		// 			// 	motionCfg.sensitive, motionCfg.humanSensitive, motionCfg.videoAtt);
		// 		}
		// 	}
		// 	catch (const exception &error) {
		// 		APP_DBG_MD("%s\n", error.what());
		// 		ret = APP_CONFIG_ERROR_DATA_INVALID;
		// 	}

		// 	/* response to mqtt server */
		// 	json resJs;
		// 	if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_MOTION, ret)) {
		// 		task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
		// 	}
		// 	APP_DBG_MD("\n");

		// } break;

		// case GW_DETECT_GET_MOTION_CONFIG_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_GET_MOTION_CONFIG_REQ\n");

		// 	json resJs, dataJs = json::object();
		// 	bool ret = APP_CONFIG_SUCCESS;
		// 	if (!fca_jsonSetMotion(dataJs, eventDetection.config)) {
		// 		ret = APP_CONFIG_ERROR_FILE_OPEN;
		// 	}

		// 	if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_MOTION, ret)) {
		// 		task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
		// 	}
		// } break;

		// case GW_DETECT_SET_ALARM_CONTROL_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_SET_ALARM_CONTROL_REQ\n");

		// 	int ret = APP_CONFIG_ERROR_DATA_INVALID;

		// 	try {
		// 		fca_alarmControl_t newSett;
		// 		json dataJs = json::parse(string((char *)msg->header->payload, msg->header->len));
		// 		fca_jsonGetAlarmControl(dataJs, &newSett);

		// 		if (newSett.durationInSecs > 0) {
		// 			memcpy(&alarmControlSetting, &newSett, sizeof(fca_alarmControl_t));
		// 			fca_configSetAlarmControl(&alarmControlSetting);
		// 			ret = APP_CONFIG_SUCCESS;
		// 		}
		// 	}
		// 	catch (const exception &error) {
		// 		APP_DBG_MD("%s\n", error.what());
		// 		ret = APP_CONFIG_ERROR_ANOTHER;
		// 	}

		// 	json resJs;
		// 	if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_ALARM_CONTROL, ret)) {
		// 		task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
		// 	}
		// } break;

		// case GW_DETECT_GET_ALARM_CONTROL_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_GET_ALARM_CONTROL_REQ\n");

		// 	json resJs, dataJs = json::object();
		// 	int ret = (fca_jsonSetAlarmControl(dataJs, &alarmControlSetting) ? APP_CONFIG_SUCCESS : APP_CONFIG_ERROR_DATA_INVALID);

		// 	if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_ALARM_CONTROL, ret)) {
		// 		task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
		// 	}
		// } break;

		// case GW_DETECT_START_DETECT_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_START_DETECT_REQ\n");

		// 	/* 	Do not start a new record if mp4 is recording or mp4 is existed (uploading),
		// 		waiting for mp4 completed and timeout to apply new setting
		// 	*/
		// 	std::string s3MP4 = FCA_MEDIA_MP4_PATH "/" FCA_RECORD_S3_MOTION_FILE;
		// 	if (MP4Handle.isOpened() || doesFileExist(s3MP4.c_str())) {
		// 		break;
		// 	}
		// 	eventDetection.start();
		// 	IF_SYS_LOG_DBG {
		// 		SYSLOG_LOG(LOG_INFO, "logf=%s <======== start detect =========>", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME);
		// 		timer_remove_attr(GW_TASK_SYS_ID, GW_SYS_GET_SYSTEM_INFO_REQ);
		// 		timer_set(GW_TASK_SYS_ID, GW_SYS_GET_SYSTEM_INFO_REQ, 5000, TIMER_PERIODIC);
		// 	}
		// } break;

		// case GW_DETECT_ENABLE_AI_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_ENABLE_AI_REQ\n");

		// 	eventDetection.enableDectection();
		// } break;

		// case GW_DETECT_DISABLE_AI_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_DISABLE_AI_REQ\n");

		// 	eventDetection.disableDetection();

		// 	if (MP4Handle.isOpened()) {
		// 		MP4Handle.close();
		// 	}
		// 	if (doesFileExist((FCA_MEDIA_MP4_PATH "/" FCA_RECORD_S3_MOTION_FILE))) {
		// 		remove((FCA_MEDIA_MP4_PATH "/" FCA_RECORD_S3_MOTION_FILE));
		// 	}
		// } break;

		// case GW_DETECT_ALARM_HUMAN_DETECTED: {
		// 	APP_DBG_SIG("GW_DETECT_ALARM_HUMAN_DETECTED\n");

		// 	APP_DBG_MD("\n");
		// 	APP_DBG_MD("+ + + + + + + + + + +\n");
		// 	APP_DBG_MD("+ HUMAN DETECTED\n");
		// 	APP_DBG_MD("+ + + + + + + + + + +\n");
		// 	APP_DBG_MD("\n");

		// 	uint32_t *ts  = (uint32_t *)(msg->header->payload);
		// 	retrySnapshot = 0;

		// 	ledStatus.controlLedEvent(Indicator::EVENT::MOTION, Indicator::STATUS::START);

		// 	/* Upload JPEG to S3 before publish to MQTT for app can get thumbnail */
		// 	memset(&notificationS3, 0, sizeof(S3Notification_t));
		// 	notificationS3.timestamp = *ts;
		// 	notificationS3.type		 = JPEG_SNAP_TYPE_HUMAN;
		// 	if (eventDetection.config->pictureAtt) {
		// 		task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_SNAP_IMAGE_REQ);
		// 	}

		// 	task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_HANDLING_ALARM);
		// } break;

		// case GW_DETECT_ALARM_MOTION_DETECTED: {
		// 	APP_DBG_SIG("GW_DETECT_ALARM_MOTION_DETECTED\n");

		// 	APP_DBG_MD("\n");
		// 	APP_DBG_MD("+ + + + + + + + + + + \n");
		// 	APP_DBG_MD("+ MOTION DETECTED\n");
		// 	APP_DBG_MD("+ + + + + + + + + + + \n");
		// 	APP_DBG_MD("\n");

		// 	uint32_t *ts  = (uint32_t *)(msg->header->payload);
		// 	retrySnapshot = 0;

		// 	ledStatus.controlLedEvent(Indicator::EVENT::MOTION, Indicator::STATUS::START);

		// 	/* Upload JPEG to S3 before publish to MQTT for app can get thumbnail */
		// 	memset(&notificationS3, 0, sizeof(S3Notification_t));
		// 	notificationS3.timestamp = *ts;
		// 	notificationS3.type		 = JPEG_SNAP_TYPE_MOTION;
		// 	if (eventDetection.config->pictureAtt) {
		// 		task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_SNAP_IMAGE_REQ);
		// 	}

		// 	task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_HANDLING_ALARM);
		// } break;

		// case GW_DETECT_SNAP_IMAGE_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_SNAP_IMAGE_REQ\n");

		// 	notificationS3.bJPEG = true;
		// 	memset(notificationS3.path, 0, sizeof(notificationS3.path));
		// 	sprintf(notificationS3.path, "%s/%s", FCA_MEDIA_JPEG_PATH, FCA_IMAGE_S3_MOTION_FILE);
		// 	int ret = HAL_VIDEO_Snapshot(notificationS3.path);
		// 	if (ret != HAL_RET_SUCCESS) {
		// 		LOG_FILE_SERVICE("Snapshot failed return %d\r\n", ret);
		// 		if ((retrySnapshot++) < 2) {
		// 			timer_set(GW_TASK_DETECT_ID, GW_DETECT_SNAP_IMAGE_REQ, 1000, TIMER_ONE_SHOT);
		// 			break;
		// 		}
		// 	}
		// 	else {
		// 		task_post_common_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_SET_INFO_UPLOAD, (uint8_t *)&notificationS3, sizeof(S3Notification_t));
		// 	}
		// } break;

		// case GW_DETECT_PUSH_MQTT_ALARM_NOTIFICATION: {
		// 	APP_DBG_SIG("GW_DETECT_PUSH_MQTT_ALARM_NOTIFICATION\n");

		// 	s3_object_t s3_obj;
		// 	memcpy(&s3_obj, msg->header->payload, sizeof(s3_object_t));

		// 	std::string rep = getPayloadMQTTSignaling(notificationS3.timestamp, notificationS3.type, 0, "jpeg", &s3_obj);
		// 	task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_ALARM_RES, (uint8_t *)rep.c_str(), strlen(rep.c_str()));
		// } break;

		// case GW_DETECT_HANDLING_ALARM: {
		// 	APP_DBG_SIG("GW_DETECT_HANDLING_ALARM\n");

		// 	if (eventDetection.isPaused()) {
		// 		APP_DBG_MD("Motion detection is paused\n");
		// 		break;
		// 	}

		// 	AlarmTrigger_t trigger;
		// 	trigger.bRunning	  = false;
		// 	trigger.bSound		  = alarmControlSetting.setSound;
		// 	trigger.bLight		  = alarmControlSetting.setLighting;
		// 	trigger.elapsedSecond = alarmControlSetting.durationInSecs;

		// 	/*  Ignore alarm detection when user is using emergency alarm
		// 		Only enable report status if sound or light is set true
		// 	*/
		// 	if (alarmTrigger.bRunning == false) {
		// 		if (trigger.bSound == true || trigger.bLight == true) {
		// 			task_post_common_msg(GW_TASK_DETECT_ID, GW_DETECT_START_TRIGGER_ALARM_REQ, (uint8_t *)&trigger, sizeof(AlarmTrigger_t));
		// 		}
		// 	}

		// 	/* Open session recording S3 Moment
		// 	 */
		// 	if (eventDetection.config->videoAtt) {
		// 		if (!MP4Handle.isOpened()) {
		// 			std::string s3MP4 = FCA_MEDIA_MP4_PATH "/" FCA_RECORD_S3_MOTION_FILE;

		// 			extern VideoCtrl videoCtrl;
		// 			FCA_ENCODE_S vEncode = videoCtrl.videoEncodeConfig();
		// 			int ret				 = MP4Handle.open(s3MP4.c_str(), 1280, 720, vEncode.minorStream.format.FPS, 8000);
		// 			if (0 == ret) {
		// 				LOG_FILE_INFO("[RECORD] open file record motion failed\n");
		// 			}
		// 		}
		// 	}

		// 	/* Set duration timeout for moment
		// 	 */
		// 	uint32_t duration = eventDetection.config->duration + 5;
		// 	timer_set(GW_TASK_DETECT_ID, GW_DETECT_DURATION_ALARM_TIMEOUT, duration * 1000, TIMER_ONE_SHOT);

		// 	IF_SYS_LOG_DBG {
		// 		SYSLOG_LOG(LOG_INFO, "logf=%s <======== start record motion =========>", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME);
		// 		timer_remove_attr(GW_TASK_SYS_ID, GW_SYS_GET_SYSTEM_INFO_REQ);
		// 		timer_set(GW_TASK_SYS_ID, GW_SYS_GET_SYSTEM_INFO_REQ, 30000, TIMER_PERIODIC);
		// 	}
		// } break;

		// case GW_DETECT_INTERVAL_ALARM_TIMEOUT: {
		// 	APP_DBG_SIG("GW_DETECT_INTERVAL_ALARM_TIMEOUT\n");

		// 	std::string motionJpegPath = FCA_MEDIA_JPEG_PATH + std::string("/") + FCA_IMAGE_S3_MOTION_FILE;
		// 	std::string motionMp4Path  = FCA_MEDIA_MP4_PATH + std::string("/") + FCA_RECORD_S3_MOTION_FILE;

		// 	if (doesFileExist(motionJpegPath.c_str())) {
		// 		remove(motionJpegPath.c_str());
		// 	}
		// 	if (doesFileExist(motionMp4Path.c_str())) {
		// 		remove(motionMp4Path.c_str());
		// 	}
		// 	timer_remove_attr(GW_TASK_UPLOAD_ID, GW_UPLOAD_FILE_JPEG_REQ);
		// 	timer_remove_attr(GW_TASK_UPLOAD_ID, GW_UPLOAD_FILE_MP4_REQ);

		// 	task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_START_DETECT_REQ);
		// } break;

		// case GW_DETECT_DURATION_ALARM_TIMEOUT: {
		// 	APP_DBG_SIG("GW_DETECT_DURATION_ALARM_TIMEOUT\n");
		// 	/* Close MP4 Record if it has openned */
		// 	if (MP4Handle.isOpened()) {
		// 		MP4Handle.close();
		// 		notificationS3.bJPEG = false;
		// 		memset(notificationS3.path, 0, sizeof(notificationS3.path));
		// 		sprintf(notificationS3.path, "%s/%s", FCA_MEDIA_MP4_PATH, FCA_RECORD_S3_MOTION_FILE);
		// 		task_post_common_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_SET_INFO_UPLOAD, (uint8_t *)&notificationS3, sizeof(S3Notification_t));
		// 	}

		// 	uint32_t secs = (eventDetection.config->interval > eventDetection.config->duration) ? (eventDetection.config->interval - eventDetection.config->duration)
		// 																						: eventDetection.config->interval;
		// 	timer_set(GW_TASK_DETECT_ID, GW_DETECT_INTERVAL_ALARM_TIMEOUT, secs * 1000, TIMER_ONE_SHOT);
		// } break;

		// case GW_DETECT_START_TRIGGER_ALARM_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_START_TRIGGER_ALARM_REQ\n");

		// 	memcpy(&alarmTrigger, (AlarmTrigger_t *)(msg->header->payload), sizeof(AlarmTrigger_t));

		// 	timer_set(GW_TASK_DETECT_ID, GW_DETECT_ALARM_TRIGGER_STATUS_REP, GW_DETECT_ALARM_TRIGGER_STATUS_REP_INTERVAL, TIMER_PERIODIC);

		// 	alarmTrigger.bRunning = true;
		// 	triggerSoundAlarm(alarmTrigger.bSound);
		// 	triggerLightAlarm(alarmTrigger.bLight);
		// } break;

		// case GW_DETECT_STOP_TRIGGER_ALARM_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_STOP_TRIGGER_ALARM_REQ\n");

		// 	if (!alarmTrigger.bRunning) {
		// 		break;
		// 	}

		// 	timer_remove_attr(GW_TASK_DETECT_ID, GW_DETECT_ALARM_TRIGGER_STATUS_REP);
		// 	triggerSoundAlarm(false);
		// 	triggerLightAlarm(false);
		// 	alarmTrigger.bRunning = false;
		// 	reportAlarmTriggerStatus();
		// } break;

		// case GW_DETECT_ALARM_TRIGGER_STATUS_REP: {
		// 	APP_DBG_SIG("GW_DETECT_ALARM_TRIGGER_STATUS_REP\n");

		// 	/* Do nothing if user is talking */
		// 	if (!Client::currentClientPushToTalk.empty()) {
		// 		break;
		// 	}

		// 	--(alarmTrigger.elapsedSecond);
		// 	reportAlarmTriggerStatus();

		// 	APP_DBG_MD("Trigger alarm has elapsed %d second [%d, %d]\n", alarmTrigger.elapsedSecond, bSoundTriggerStatus, bLightTriggerStatus);

		// 	/* Continue play alarm sound to the end
		// 		TODO: Play file
		// 	*/
		// 	if (AudioCtrl::bIsSpeakerPlaying.load() == false && bSoundTriggerStatus == true) {
		// 		AudioCtrl::audioPlayFileG711(FCA_DEVICE_SOUND_PATH "/" FCA_SOUND_MOTION_ALARM_FILE);
		// 	}

		// 	if (alarmTrigger.elapsedSecond == 0) {
		// 		task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_STOP_TRIGGER_ALARM_REQ);
		// 	}
		// } break;

		// case GW_DETECT_PERIODIC_ALERT_LIGHTING: {
		// 	APP_DBG_SIG("GW_DETECT_PERIODIC_ALERT_LIGHTING\n");

		// 	static bool boolean = true;

		// 	if (bLightTriggerStatus) {
		// 		gpiosHelpers.irLighting.setLighting(boolean ? IR_Lighting::STATUS::ON : IR_Lighting::STATUS::OFF);
		// 		boolean = !boolean;
		// 	}
		// 	else {
		// 		boolean = true;
		// 		gpiosHelpers.irLighting.setLighting(IR_Lighting::STATUS::OFF);
		// 		timer_remove_attr(GW_TASK_DETECT_ID, GW_DETECT_PERIODIC_ALERT_LIGHTING);
		// 		fca_cameraParam_t bkupParam;
		// 		fca_configGetParam(&bkupParam);
		// 		ispCtrl.CamParam.dnParam.lightingMode = bkupParam.dnParam.lightingMode;
		// 		ispCtrl.controlSmartLight(ispCtrl.CamParam.dnParam.lightingMode);
		// 	}
		// } break;

		// case GW_DETECT_PENDING_TRIGGER_SOUND_ALARM_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_PENDING_TRIGGER_SOUND_ALARM_REQ\n");

		// 	timer_remove_attr(GW_TASK_DETECT_ID, GW_DETECT_ALARM_TRIGGER_STATUS_REP);
		// 	if (AudioCtrl::bIsSpeakerPlaying.load() == true) {
		// 		AudioCtrl::setStopPlayFile(true);
		// 	}
		// } break;

		// case GW_DETECT_CONTINUE_TRIGGER_SOUND_ALARM_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_CONTINUE_TRIGGER_SOUND_ALARM_REQ\n");

		// 	if (alarmTrigger.bRunning) {
		// 		timer_set(GW_TASK_DETECT_ID, GW_DETECT_ALARM_TRIGGER_STATUS_REP, GW_DETECT_ALARM_TRIGGER_STATUS_REP_INTERVAL, TIMER_PERIODIC);
		// 	}
		// } break;

		// case GW_DETECT_IVA_AI_GET_VERSION_REQ: {
		// 	APP_DBG_SIG("GW_DETECT_IVA_AI_GET_VERSION_REQ\n");
		// 	int ret = APP_CONFIG_ERROR_DATA_INVALID;

		// 	json resJs, dataJs, data_res = json::object();
		// 	try {
		// 		json dataJs = json::parse(string((char *)msg->header->payload, msg->header->len));

		// 		string model_type = dataJs["Type"].get<string>();
		// 		printf("[model_type] ==================== %s\n", model_type.c_str());
		// 	}
		// 	catch (const exception &error) {
		// 		APP_DBG_MD("%s\n", error.what());
		// 		ret = APP_CONFIG_ERROR_DATA_INVALID;
		// 	}

		// 	if (fca_getConfigJsonRes(resJs, data_res, MESSAGE_TYPE_AIMODEL_INFO, ret)) {
		// 		task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
		// 	}
		// 	APP_DBG_MD("\n");
		// } break;

		case GW_DETECT_WATCHDOG_PING_REQ: {
			// APP_DBG_SIG("GW_DETECT_WATCHDOG_PING_REQ\n");
			task_post_pure_msg(GW_TASK_DETECT_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
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

void reportAlarmTriggerStatus() {
	json repJS = {
		{"Id",	   fca_getSerialInfo()},
		{"Type",	 "Report"			 },
		{"Command", "EmergencyAlarm"	},
		{"Content",
		 {
			 {"Sound", alarmTrigger.bSound},
			 {"Lighting", alarmTrigger.bLight},
			 {"Enable", alarmTrigger.bRunning},
			 {"ElapsedSecond", alarmTrigger.elapsedSecond},
		 }							 },
	};

	pthread_mutex_lock(&Client::mtxClientsProtect);
	for (auto it : clients) {
		auto dc = it.second->dataChannel.value();
		try {
			if (dc->isOpen()) {
				dc->send(repJS.dump());
			}
		}
		catch (const exception &error) {
			APP_DBG_MD("%s\n", error.what());
		}
	}
	pthread_mutex_unlock(&Client::mtxClientsProtect);
}

void triggerSoundAlarm(bool bSound) {
	if (bSound == true && bSoundTriggerStatus == false) {
		APP_DBG_MD("[ALARM] TURN ON SOUND\n");
		bSoundTriggerStatus = true;
		// audioHelpers.silent();
		// audioHelpers.playSiren();
		// AudioCtrl::audioPlayFileG711(FCA_SOUND_MOTION_ALARM_FILE);
	}
	else if (bSound == false && bSoundTriggerStatus == true) {
		APP_DBG_MD("[ALARM] TURN OFF SOUND\n");
		bSoundTriggerStatus = false;
		// AudioCtrl::setStopPlayFile(true);
	}
}

void triggerLightAlarm(bool bLight) {
	if (bLight == true && bLightTriggerStatus == false) {
		APP_DBG_MD("[ALARM] TURN ON LIGHTING\n");

		bLightTriggerStatus					  = true;
		ispCtrl.CamParam.dnParam.lightingMode = FCA_LIGHT_BLACK_WHITE;
		ispCtrl.controlSmartLight(ispCtrl.CamParam.dnParam.lightingMode);
		timer_set(GW_TASK_DETECT_ID, GW_DETECT_PERIODIC_ALERT_LIGHTING, GW_DETECT_LIGHT_ALERT_BLINKING_INTERVAL, TIMER_PERIODIC);
	}
	else if (bLight == false && bLightTriggerStatus == true) {
		APP_DBG_MD("[ALARM] TURN OFF LIGHTING\n");
		bLightTriggerStatus = false;
	}
}
