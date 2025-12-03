#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>

#include "fca_common.hpp"
#include "fca_osd.hpp"

OSDCtrl::OSDCtrl() {
	APP_DBG_OSD("[OSD] Init class OSDCtrl\n");
}

OSDCtrl::~OSDCtrl() {
	APP_DBG_OSD("[OSD] Release class OSDCtrl\n");
	// fca_osd_uninit();
}

int OSDCtrl::loadConfigFromFile(FCA_OSD_S *watermark) {
	// int err;

	// err = fca_configGetWatermark(watermark);
	// if (err != 0) {
	// 	APP_DBG_OSD("get config from to file error\n");
	// 	return err;
	// }

	// err = verifyConfig(watermark);
	// if (err != 0) {
	// 	APP_DBG_OSD("get config from to file error\n");
	// 	return err;
	// }

	return 0;
}

int OSDCtrl::verifyConfig(FCA_OSD_S *watermark) {
	// if (strlen(watermark->name) > WATERMARK_LENGTH_MAX - 2 || strlen(watermark->name) <= 0) {
	// 	APP_DBG_OSD("name set error leng: %d , [leng: 0-->%d character]\n", strlen(watermark->name), WATERMARK_LENGTH_MAX - 2);
	// 	return -1;
	// }
	// else {
	// 	APP_DBG_OSD("+++ Config OSD +++\n");
	// 	APP_DBG_OSD("		- set name text: [%s]\n", watermark->name);
	// 	APP_DBG_OSD("		- show name: %s\n", (watermark->nameAtt == true) ? "Enable" : "Disable");
	// 	APP_DBG_OSD("		- show time: %s\n", (watermark->timeAtt == true) ? "Enable" : "Disable");
	// 	APP_DBG_OSD("\n");
	// }

	return 0;
}

int OSDCtrl::updateWatermark(FCA_OSD_S *watermark) {
	// FCA_OSD_POSITION_E pos;
	// if (watermark->timeAtt) {
	// 	APP_DBG_OSD("enable osd time\n");
	// 	pos = FCA_OSD_TOP_RIGHT;
	// }
	// else {
	// 	APP_DBG_OSD("disable osd time\n");	  
	// 	pos = FCA_OSD_DISABLE;
	// }

	// if (fca_isp_set_osd_time(pos) != 0) {
	// 	APP_DBG_OSD("set position time failed\n");
	// 	return -1;
	// }
	// APP_DBG_OSD("set position time success\n");

	// if (watermark->nameAtt) {
	// 	APP_DBG_OSD("enable watermark\n");
	// 	pos = FCA_OSD_BOTTOM_LEFT;
	// }
	// else {
	// 	APP_DBG_OSD("disable watermark\n");	   
	// 	pos = FCA_OSD_DISABLE;
	// }

	// if (fca_isp_set_osd_text(pos, watermark->name) != 0) {
	// 	APP_DBG_OSD("set watermark failed\n");
	// 	return -1;
	// }
	// APP_DBG_OSD("set watermark success\n");

	return 0;
}

int OSDCtrl::getWatermarkConfJs(json &json_in) {
	// if (!fca_jsonSetWatermark(json_in, &mWatermarkConf)) {
	// 	APP_DBG_OSD("[ERR] data watermark config invalid\n");
	// 	return APP_CONFIG_ERROR_DATA_INVALID;
	// }
	return APP_CONFIG_SUCCESS;
}

FCA_OSD_S OSDCtrl::watermarkConf() const {
	return mWatermarkConf;
}

void OSDCtrl::setWatermarkConf(const FCA_OSD_S *newWatermarkConf) {
	// memcpy(&mWatermarkConf, newWatermarkConf, sizeof(mWatermarkConf));
}

void OSDCtrl::printWatermarkConfig() {
	APP_DBG_OSD("[VAR] --------------------- [WaterMaks] ---------------------\n");
	APP_DBG_OSD("[VAR] NameTitle: %s\n", mWatermarkConf.name);
	APP_DBG_OSD("[VAR] TimeTitleAttribute: %d\n", mWatermarkConf.timeAtt);
	APP_DBG_OSD("[VAR] NameTitleAttribute: %d\n", mWatermarkConf.nameAtt);
}