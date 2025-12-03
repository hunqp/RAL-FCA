#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <math.h>

#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "task_detect.h"
#include "app_data.h"
#include "SdCard.h"

#include "fca_motion.hpp"
#include "parser_json.h"

EventDetection eventDetection;

extern volatile bool bLocalEventsTriggered;
extern volatile bool bLocalEventsRecorderEnable;

static void onMOTIONDetected(fca_md_result_t result);

EventDetection::EventDetection() {
	mMutex = PTHREAD_MUTEX_INITIALIZER;
}

EventDetection::~EventDetection() {
	pthread_mutex_destroy(&mMutex);

	/* GIEC uninitialise */
	fca_md_set_onoff(0);
	fca_svp_humfil_set_onoff(0);
	fca_md_uninit_block();
	fca_svp_uninit_block();
}

int EventDetection::initialise() {
	int ret = 0;

	ret = fca_md_init(onMOTIONDetected);
	APP_DBG_MD("[%s] fca_md_init()\r\n", ret == 0 ? "SUCCESS" : "FAILED");
	if (ret != 0) {
		return -1;
	}

	fca_md_set_onoff(0);
	fca_svp_humfil_set_onoff(0);

	return ret;
}

int EventDetection::start() {
	int ret = -1;

	if (isPaused()) {
		APP_DBG_MD("Motion detection is paused\n");
		return -1;
	}

	pthread_mutex_lock(&mMutex);

	switch (mType) {
	case DETECT_TYPE_MOTION: {
		ret = fca_md_set_onoff(1);
		APP_DBG_MD("[%s] fca_md_set_onoff(1)\r\n", ret == 0 ? "SUCCESS" : "FAILED");
	} break;

	case DETECT_TYPE_HUMAN: {
		// fca_ivaHumanOnOff(1);
	} break;

	default:
		break;
	}

	mDetected = false;

	pthread_mutex_unlock(&mMutex);

	return ret;
}

bool EventDetection::isDetected() {
	pthread_mutex_lock(&mMutex);

	bool ret = mDetected;

	pthread_mutex_unlock(&mMutex);

	return ret;
}

int EventDetection::stop() {
	int ret = -1;

	pthread_mutex_lock(&mMutex);

	switch (mType) {
	case DETECT_TYPE_MOTION: {
		ret = fca_md_set_onoff(0);
		APP_DBG_MD("[%s] fca_md_set_onoff(0)\r\n", ret == 0 ? "SUCCESS" : "FAILED");
	} break;

	case DETECT_TYPE_HUMAN: {
		ret = fca_svp_humfil_set_onoff(0);
		APP_DBG_MD("[%s] fca_svp_humfil_set_onoff(0)\r\n", ret == 0 ? "SUCCESS" : "FAILED");
	} break;

	default:
		break;
	}

	pthread_mutex_unlock(&mMutex);

	return ret;
}

int EventDetection::validConfig(fca_motionSetting_t *config) {
	if (config->sensitive > MAX_LEVEL || config->sensitive <= LOW_LEVEL) {
		APP_DBG_MD("[INVALID] MD Sensitive [%d]\n", config->sensitive);
		return -1;
	}

	if (config->lenGuardZone > FCA_MAX_LENG_GUARDZONE || config->lenGuardZone < FCA_CONFIG_NONE) {
		APP_DBG_MD("[INVALID] Guardzone [%d]\n", config->sensitive);
		return -1;
	}

	if (config->interval <= FCA_CONFIG_NONE) {
		APP_DBG_MD("[INVALID] Interval [%d]\n", config->interval);
		return -1;
	}

	for (int i = 0; i < config->lenGuardZone; i++) {
		if (config->guardZone[i] >= FCA_MAX_LENG_GUARDZONE || config->guardZone[i] < 0) {
			APP_DBG_MD("[INVALID] Guardzone over array [%d] {limit: %d} \n", config->guardZone[i], FCA_MAX_LENG_GUARDZONE - 1);
			return -1;
		}
	}
	// if (config->humanSensitive > HIGH_LEVEL || config->humanSensitive < LOW_LEVEL) {
	// 	APP_DBG_MD("[INVALID] HD Sensitive [%d]\n", config->humanSensitive);
	// 	return -1;
	// }

	// if (config->svpThrld < 0 || config->svpThrld > 100) {
	// 	APP_DBG_MD("[INVALID] SVP Threshold [%d]\n", config->svpThrld);
	// 	return -1;
	// }

	// if (config->svpMdThrld < 0 || config->svpMdThrld > 100) {
	// 	APP_DBG_MD("[INVALID] SVP MD Threshold [%d]\n", config->svpMdThrld);
	// 	return -1;
	// }

	APP_DBG_MD("[VALID] Motion configuration\n");

	return 0;
}

int EventDetection::setConfig(fca_motionSetting_t *config) {
	int ret = -1;

	pthread_mutex_lock(&mMutex);

	memcpy(&mConfig, config, sizeof(fca_motionSetting_t));

	if (mConfig.enable == false) {
		mType = DETECT_TYPE_NONE;
	}
	else {
		mType = (mConfig.humanDetAtt ? DETECT_TYPE_HUMAN : DETECT_TYPE_MOTION);
	}

	/* -- Set ROI -- */
	fca_roi_t roi;
	memset(&roi, 0, sizeof(fca_roi_t));
	if (mConfig.guardZoneAtt) {
		for (int id = 0; id < mConfig.lenGuardZone; id++) {
			if (mConfig.guardZone[id] >= 0 && mConfig.guardZone[id] < 64) {
				int row = (mConfig.guardZone[id] / 8);
				int col = (mConfig.guardZone[id] % 8);
				roi.roi_grid[row] |= (1 << (7 - col));
			}
		}
#if 0	 // test roid grid
		roi.roi_grid[0] = 0x0F;
		roi.roi_grid[1] = 0x0F;
		roi.roi_grid[2] = 0x0F;
		roi.roi_grid[3] = 0x0F;
		roi.roi_grid[4] = 0x0F;
		roi.roi_grid[5] = 0x0F;
		roi.roi_grid[6] = 0x0F;
		roi.roi_grid[7] = 0x0F;
#endif
	}
	else {
		for (uint8_t id = 0; id < sizeof(roi.roi_grid); ++id) {
			roi.roi_grid[id] = 0xFF;
		}
	}

#ifndef RELEASE
	for (uint8_t id = 0; id < 8; ++id) {
		APP_DBG_MD("%X\r\n", roi.roi_grid[id]);
	}
#endif

	ret = fca_md_set_roi_grid(&roi);
	APP_DBG_MD("[%s] fca_md_set_roi_grid()\r\n", ret == 0 ? "SUCCESS" : "FAILED");

	if (mConfig.humanDetAtt == true) {
		if ((ret = fca_svp_alarmzone_set_onoff((int)mConfig.humanDraw)) != 0) {
			APP_DBG_MD("fca_svp_alarmzone_set_onoff() [%d] -> failed\n", ret);
		}
		if ((ret = fca_svp_draw_enable(1, (int)mConfig.humanDraw)) != 0) {	  // sub channel
			APP_DBG_MD("fca_svp_draw_enable() [%d] -> failed\n", ret);
		}
		if ((ret = fca_svp_draw_enable(0, (int)mConfig.humanDraw)) != 0) {	  // main channel
			APP_DBG_MD("fca_svp_draw_enable() [%d] -> failed\n", ret);
		}
		if ((ret = fca_svp_humfocus_set_onoff((int)mConfig.humanFocus)) != 0) {
			APP_DBG_MD("fca_svp_humfocus_set_onoff() [%d] -> failed\n", ret);
		}
	}

	/* -- Set Sensitivity -- */
	// NOTE: call API fca_md_set_sensitivity() after set roi
	ret = fca_md_set_sensitivity(mConfig.sensitive);
	// APP_DBG_MD("[%s] md_sensitivity: %d, hd_sensitivity: %d \r\n", ret == 0 ? "success" : "failed", mConfig.sensitive, mConfig.humanSensitive);

	pthread_mutex_unlock(&mMutex);

	return ret;
}

void EventDetection::setStatus(bool boolean) {
	pthread_mutex_lock(&mMutex);
	mDetected = boolean;
	pthread_mutex_unlock(&mMutex);
}

void EventDetection::setPaused(bool paused) {
	pthread_mutex_lock(&mMutex);
	mPaused = paused;
	pthread_mutex_unlock(&mMutex);
}

bool EventDetection::isPaused() {
	pthread_mutex_lock(&mMutex);
	bool ret = mPaused;
	pthread_mutex_unlock(&mMutex);
	return ret;
}

void EventDetection::enableDectection() {
	APP_DBG_MD("[motion] enable detection\n");
	setPaused(false);
	task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_START_DETECT_REQ);
}

void EventDetection::disableDetection() {
	APP_DBG_MD("[motion] disable detection\n");
	setPaused(true);
	stop();
}

void EventDetection::print() {
	APP_DBG_MD("Enable: %d\n", mConfig.enable);
	APP_DBG_MD("Sensitive: %d\n", mConfig.sensitive);
	APP_DBG_MD("Interval: %d\n", mConfig.interval);
	APP_DBG_MD("Duration: %d\n", mConfig.duration);
	APP_DBG_MD("Video attribute: %d\n", mConfig.videoAtt);
	APP_DBG_MD("Picture attribute: %d\n", mConfig.pictureAtt);
	APP_DBG_MD("Human attribute: %d\n", mConfig.humanDetAtt);
	APP_DBG_MD("Human focus: %d\n", mConfig.humanFocus);
	APP_DBG_MD("Human draw: %d\n", mConfig.humanDraw);
	APP_DBG_MD("Guard zone attribute: %d\n", mConfig.guardZoneAtt);
}

void onMOTIONDetected(fca_md_result_t result) {
	(void)result;

	uint32_t ts = (uint32_t)time(NULL);

	/* Trigger storage cloud */
	if (eventDetection.isDetected() == false) {
		eventDetection.setStatus(true);
		/* Stop motion for saving CPU, because after duration setting, the event triggers again */
		eventDetection.stop();
		task_post_common_msg(GW_TASK_DETECT_ID, GW_DETECT_ALARM_MOTION_DETECTED, (uint8_t *)&ts, sizeof(ts));
	}

	/* Trigger storage Sd */
	if (sdCard.currentState() == SD_STATE_MOUNTED && bLocalEventsRecorderEnable) {
		if (!bLocalEventsTriggered) {
			RecorderSdTrigger_t trigger;
			trigger.type	  = RECORD_TYPE_MDT;
			trigger.timestamp = ts;
			task_post_common_msg(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_EVENTS, (uint8_t *)&trigger, sizeof(trigger));
			bLocalEventsTriggered = true;

			uint32_t duration = eventDetection.config->duration + 5;
			timer_set(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR, duration * 1000, TIMER_ONE_SHOT);
		}
	}
}

void onIVAHumanDetect() {
	uint32_t ts = (uint32_t)time(NULL);

	/* Trigger storage cloud */
	if (eventDetection.isDetected() == false) {
		eventDetection.setStatus(true);
		/* Stop motion for saving CPU, because after duration setting, the event triggers again */
		eventDetection.stop();
		task_post_common_msg(GW_TASK_DETECT_ID, GW_DETECT_ALARM_HUMAN_DETECTED, (uint8_t *)&ts, sizeof(ts));
	}

	/* Trigger storage Sd */
	if (sdCard.currentState() == SD_STATE_MOUNTED && bLocalEventsRecorderEnable) {
		if (!bLocalEventsTriggered) {
			RecorderSdTrigger_t trigger;
			trigger.type	  = RECORD_TYPE_HMD;
			trigger.timestamp = ts;
			task_post_common_msg(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_EVENTS, (uint8_t *)&trigger, sizeof(trigger));
			bLocalEventsTriggered = true;

			uint32_t duration = eventDetection.config->duration + 5;
			timer_set(GW_TASK_RECORD_ID, GW_RECORD_STORAGE_LOCAL_TYPE_REGULAR, duration * 1000, TIMER_ONE_SHOT);
		}
	}
}
