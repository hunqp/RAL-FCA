
#include "app.h"
#include "app_data.h"
#include "app_config.h"
#include "task_list.h"
#include "parser_json.h"

#include "fca_common.hpp"
#include "fca_isp.hpp"

using namespace std;
using json = nlohmann::json;

IspCtrl ispCtrl;

void dayNightCallback(FCA_DAY_NIGHT_STATE_E state, const void *user_data) {
	(void)user_data;
	ispCtrl.dayNightState.store(state == FCA_NIGHT_STATE ? DN_NIGHT_STATE : DN_DAY_STATE);
	APP_DBG_ISP("DayNight status is: %s\n", ispCtrl.dayNightState.load() == DN_NIGHT_STATE ? "Night" : "Day");
	task_post_pure_msg(GW_TASK_AV_ID, GW_AV_ADJUST_IMAGE_WITH_DAYNIGHT_STATE_REQ);
}

IspCtrl::IspCtrl() {
	lightingChangeCount = 0;
	dayNightState.store(DN_STATE_UNKNOWN);
}

IspCtrl::~IspCtrl() {
	APP_DBG_ISP("~IspCtrl()\n");
	/* day night release */
	fca_dn_mode_resource_release();
}

int IspCtrl::loadConfigFromFile(fca_cameraParam_t *camParam) {
	// if (fca_configGetParam(camParam) == APP_CONFIG_SUCCESS) {
	// 	bool updateUserFile = false;
	// 	// WRD02: fix data invalid v1.1.0 - v1.2.0
	// 	if (abs(camParam->irContrast) > COLOR_ADJUSTMENT_MAX_THRESHOLD) {
	// 		APP_DBG("fix color threshold default\n");
	// 		camParam->brightness   = ISP_DAY_BRIGHTNESS_DEFAULT;
	// 		camParam->contrast	   = ISP_DAY_CONTRAST_DEFAULT;
	// 		camParam->saturation   = ISP_DAY_SATURATION_DEFAULT;
	// 		camParam->sharpness	   = ISP_DAY_SHARPNESS_DEFAULT;
	// 		camParam->irBrightness = ISP_NIGHT_BRIGHTNESS_DEFAULT;
	// 		camParam->irContrast   = ISP_NIGHT_CONTRAST_DEFAULT;
	// 		camParam->irSaturation = ISP_NIGHT_SATURATION_DEFAULT;
	// 		camParam->irSharpness  = ISP_NIGHT_SHARPNESS_DEFAULT;
	// 		updateUserFile		   = true;
	// 	}

	// 	if (camParam->dnParam.day2NightThreshold == 0) {
	// 		APP_DBG("fix day night threshold default\n");
	// 		camParam->dnParam.day2NightThreshold = ISP_DAY_2_NIGHT_THRESHOLD_DEFAULT;
	// 		camParam->dnParam.night2DayThreshold = ISP_NIGHT_2_DAY_THRESHOLD_DEFAULT;
	// 		updateUserFile						 = true;
	// 	}

	// 	if (updateUserFile) {
	// 		APP_DBG("update new param for user config\n");
	// 		fca_configSetParam(camParam);
	// 	}

	// 	return verifyConfig(camParam);
	// }
	return APP_CONFIG_ERROR_DATA_INVALID;
}

int IspCtrl::setDayNightConfig(const fca_dayNightParam_t &conf) {
	int ret				 = -1;
	// fca_dn_attr_t dnConf = {0};

	// dnConf.u32lock_time			  = conf.lockTime;
	// dnConf.u32trigger_times		  = conf.lockCntTh;
	// dnConf.u32trigger_interval	  = DAY_NIGHT_TRIGGER_INTERVAL;
	// dnConf.s32day2night_threshold = conf.day2NightThreshold;
	// dnConf.s32night2day_threshold = conf.night2DayThreshold;
	// dnConf.e32init_mode = conf.nightVisionMode == FCA_BLACK_WHITE_MODE ? FCA_DN_MODE_NIGHT : conf.nightVisionMode == FCA_FULL_COLOR_MODE ? FCA_DN_MODE_DAY : FCA_DN_MODE_AUTO;
	// dnConf.cb			= dayNightCallback;
	// dnConf.user_data	= NULL;

	// dnConf.u32awb_night_dis_max			 = conf.awbNightDisMax;
	// dnConf.u32day_check_frame_num		 = conf.dayCheckFrameNum;
	// dnConf.u32night_check_frame_num		 = conf.nightCheckFrameNum;
	// dnConf.u32exp_stable_check_frame_num = conf.expStableCheckFrameNum;
	// dnConf.u32stable_wait_timeout_ms	 = conf.stableWaitTimeoutMs;
	// /* exp: tmp = dis_value * 1024 / threshold.gain_r; new gain_r = non_infrared_ratio_th/tmp * 1024 */
	// dnConf.u32gain_r				= conf.gainR;
	// dnConf.s32offset_r				= conf.offsetR;
	// dnConf.u32gain_b				= conf.gainB;
	// dnConf.s32offset_b				= conf.offsetB;
	// dnConf.u32gain_rb				= conf.gainRb;
	// dnConf.s32offset_rb				= conf.offsetRb;
	// dnConf.u32blk_check_flag		= conf.blkCheckFlag;
	// dnConf.u32low_lumi_th			= conf.lowLumiTh;
	// dnConf.u32high_lumi_th			= conf.highLumiTh;
	// dnConf.u32non_infrared_ratio_th = conf.nonInfraredRatioTh;
	// /* During the day, the highlight will take up a small proportion of the photo, so highlight_lumi_th is the threshold to know if it is a highlight to count the number of
	//  * highlight_cnt. If highlight_cnt < highlight_blk_cnt_max then it is truly daytime, the camera will turn off the spotlight. */
	// dnConf.u32highlight_lumi_th		= conf.highlightLumiTh;
	// dnConf.u32highlight_blk_cnt_max = conf.highlightBlkCntMax;

	// ret = fca_dn_switch_set_config(&dnConf);
	// if (ret == 0) {
	// 	APP_DBG_ISP("set config dn success\n");
	// 	APP_DBG_ISP("\tu32lock_time: %d\n", dnConf.u32lock_time);
	// 	APP_DBG_ISP("\tu32trigger_interval: %d\n", dnConf.u32trigger_interval);
	// 	APP_DBG_ISP("\tu32trigger_times: %d\n", dnConf.u32trigger_times);
	// 	APP_DBG_ISP("\ts32day2night_threshold: %d\n", dnConf.s32day2night_threshold);
	// 	APP_DBG_ISP("\ts32night2day_threshold: %d\n", dnConf.s32night2day_threshold);
	// 	APP_DBG_ISP("\t32init_mode: %d\n", dnConf.e32init_mode);
	// 	SYSLOG_LOG(LOG_INFO, "logf=%s [PARAM] set dn mode: %d", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, conf.nightVisionMode);
	// }
	// else {
	// 	APP_DBG_ISP("set config dn failed\n");
	// }

	return ret;
}

int IspCtrl::controlAntiFlickerMode(const int &mode) {
	FCA_ISP_FLICKER_TYPE_E modeSet, modeGet;

	/* convert to api giec */
	modeSet = mode == FCA_ANTI_FLICKER_MODE_50_MHZ	 ? FCA_ISP_FLICKER_TYPE_50HZ
			  : mode == FCA_ANTI_FLICKER_MODE_60_MHZ ? FCA_ISP_FLICKER_TYPE_60HZ
			  : mode == FCA_ANTI_FLICKER_MODE_AUTO	 ? FCA_ISP_FLICKER_TYPE_AUTO
													 : FCA_ISP_FLICKER_TYPE_OFF;

	if (fca_isp_set_antiflicker(modeSet) != 0) {
		APP_DBG_ISP("set AntiFlicket error\n");
		return -1;
	}

	fca_isp_get_antiflicker(&modeGet);
	if (modeGet != modeSet) {
		APP_DBG_ISP("diff set-get AntiFlicker mode, set error\n");
		return -1;
	}
	APP_DBG_ISP("set antiflicker type: %d success\n", modeSet);
	return 0;
}

int IspCtrl::controlFlipMirror(bool bFlip, bool bMirror) {
	FCA_ISP_MIRROR_FLIP_E typeSet, typeGet;
	#if SUPPORT_ETH
	typeSet = bFlip && bMirror ? FCA_ISP_MIRROR_FLIP_NONE : FCA_ISP_MIRROR_AND_FLIP;
	#else
	typeSet = bFlip && bMirror ? FCA_ISP_MIRROR_AND_FLIP : FCA_ISP_MIRROR_FLIP_NONE;
	#endif

#ifndef RELEASE
	if (typeSet == FCA_ISP_MIRROR_AND_FLIP) {
		APP_DBG_ISP("on flip-mirror\n");
	}
	else {
		APP_DBG_ISP("off flip-mirror\n");
	}
#endif

	if (fca_isp_set_mirror_flip(typeSet) != 0) {
		APP_DBG_ISP("flip/mirror set config failed\n");
		return -1;
	}

	fca_isp_get_mirror_flip(&typeGet);
	if (typeGet != typeSet) {
		APP_DBG_ISP("diff set-get flip/mirror, set error\n");
		return -1;
	}
	APP_DBG_ISP("flip/mirror set config type: %d success\n", typeSet);
	return 0;
}

int IspCtrl::imageAdjustments(int brightness, int contrast, int saturation, int sharpness) {
	int err;
	fca_isp_config_t ispConf;
	ispConf.brightness = brightness;
	ispConf.contrast   = contrast;
	ispConf.saturation = saturation;
	ispConf.sharpness  = sharpness;

	APP_DBG_ISP("set config isp brightness: %d, contrast: %d, saturatuon: %d, sharpness: %d\n", ispConf.brightness, ispConf.contrast, ispConf.saturation, ispConf.sharpness);

	err = fca_isp_set_config(&ispConf);
	if (err != 0) {
		APP_DBG_ISP("image adjustments error [%d]\n", err);
		return -1;
	}
	APP_DBG_ISP("image adjustments success\n");
	return 0;
}

fca_cameraParam_t IspCtrl::camParam() const {
	return mCamParam;
}

void IspCtrl::setCamParam(const fca_cameraParam_t *newCamParam) {
	memcpy(&mCamParam, newCamParam, sizeof(mCamParam));
}

void IspCtrl::printCameraparamConfig() {
	APP_DBG_ISP("--------------------- [Camera parameter] ---------------------\n");
	APP_DBG_ISP("pictureFlip: %d\n", mCamParam.pictureFlip);
	APP_DBG_ISP("pictureMirror: %d\n", mCamParam.pictureMirror);
	APP_DBG_ISP("anti-flicker: %d\n\n", mCamParam.antiFlickerMode);
	APP_DBG_ISP("brightness: %d\n", mCamParam.brightness);
	APP_DBG_ISP("contrast: %d\n", mCamParam.contrast);
	APP_DBG_ISP("saturation: %d\n", mCamParam.saturation);
	APP_DBG_ISP("sharpness: %d\n", mCamParam.sharpness);
	APP_DBG_ISP("ir brightness: %d\n", mCamParam.irBrightness);
	APP_DBG_ISP("ir contrast: %d\n", mCamParam.irContrast);
	APP_DBG_ISP("ir saturation: %d\n", mCamParam.irSaturation);
	APP_DBG_ISP("ir sharpness: %d\n\n", mCamParam.irSharpness);
	APP_DBG_ISP("nightVisionMode: %d\n", mCamParam.dnParam.nightVisionMode);
	APP_DBG_ISP("lighting enable: %d\n", mCamParam.dnParam.lightingEnable);
	APP_DBG_ISP("lighting mode: %d\n", mCamParam.dnParam.lightingMode);
	APP_DBG_ISP("d2n threshold: %d, n2d threshold: %d\n", mCamParam.dnParam.day2NightThreshold, mCamParam.dnParam.night2DayThreshold);
	APP_DBG_ISP("dis max: %d\n", mCamParam.dnParam.awbNightDisMax);
	APP_DBG_ISP("day check frame: %d, night check frame: %d\n", mCamParam.dnParam.dayCheckFrameNum, mCamParam.dnParam.nightCheckFrameNum);
	APP_DBG_ISP("exp stable check frames: %d, stable wait timeout: %d\n", mCamParam.dnParam.expStableCheckFrameNum, mCamParam.dnParam.stableWaitTimeoutMs);
	APP_DBG_ISP("gain: [R: %d, B: %d, Rb: %d], offset: [R: %d, B: %d, Rb: %d] \n", mCamParam.dnParam.gainR, mCamParam.dnParam.gainB, mCamParam.dnParam.gainRb,
				mCamParam.dnParam.offsetR, mCamParam.dnParam.offsetB, mCamParam.dnParam.offsetRb);
	APP_DBG_ISP("block check flag: %d, lowLumiTh: %d, highLumiTh: %d, non infrared ratio: %d \n", mCamParam.dnParam.blkCheckFlag, mCamParam.dnParam.lowLumiTh,
				mCamParam.dnParam.highLumiTh, mCamParam.dnParam.nonInfraredRatioTh);
	APP_DBG_ISP("highlight lumi th: %d, highlight block cnt: %d\n", mCamParam.dnParam.highlightLumiTh, mCamParam.dnParam.highlightBlkCntMax);
	APP_DBG_ISP("lock cnt: %d, lock time: %d(s)\n", mCamParam.dnParam.lockCntTh, mCamParam.dnParam.lockTime);
}

int IspCtrl::verifyConfig(fca_cameraParam_t *camParam) {
	if (camParam->dnParam.nightVisionMode > FCA_BLACK_WHITE_MODE || camParam->dnParam.nightVisionMode < FCA_FULL_COLOR_MODE) {
		APP_DBG_ISP("nightVision mode [%d] error\n", camParam->dnParam.nightVisionMode);
	}
	else if (camParam->antiFlickerMode > FCA_ANTI_FLICKER_MODE_60_MHZ || camParam->antiFlickerMode < FCA_ANTI_FLICKER_MODE_AUTO) {
		APP_DBG_ISP("anti-flicker mode [%d] error\n", camParam->antiFlickerMode);
	}
	else if (camParam->dnParam.lightingMode > FCA_LIGHT_AUTO || camParam->dnParam.lightingMode < FCA_LIGHT_FULL_COLOR) {
		APP_DBG_ISP("lighting mode [%d] error\n", camParam->dnParam.lightingMode);
	}
	else if (abs(camParam->brightness) > COLOR_ADJUSTMENT_MAX_THRESHOLD || abs(camParam->irBrightness) > COLOR_ADJUSTMENT_MAX_THRESHOLD) {
		APP_DBG_ISP("brightness [%d], ir brightness [%d] error\n", camParam->brightness, camParam->irBrightness);
	}
	else if (abs(camParam->contrast) > COLOR_ADJUSTMENT_MAX_THRESHOLD || abs(camParam->irContrast) > COLOR_ADJUSTMENT_MAX_THRESHOLD) {
		APP_DBG_ISP("contrast [%d], ir contrast [%d] error\n", camParam->contrast, camParam->irContrast);
	}
	else if (abs(camParam->saturation) > COLOR_ADJUSTMENT_MAX_THRESHOLD || abs(camParam->irSaturation) > COLOR_ADJUSTMENT_MAX_THRESHOLD) {
		APP_DBG_ISP("saturation [%d], ir saturation [%d] error\n", camParam->saturation, camParam->irSaturation);
	}
	else if (abs(camParam->sharpness) > COLOR_ADJUSTMENT_MAX_THRESHOLD || abs(camParam->irSharpness) > COLOR_ADJUSTMENT_MAX_THRESHOLD) {
		APP_DBG_ISP("sharpness [%d], ir sharpness [%d] error\n", camParam->sharpness, camParam->irSharpness);
	}
	else if (camParam->dnParam.day2NightThreshold == 0 || camParam->dnParam.night2DayThreshold == 0 ||
			 camParam->dnParam.night2DayThreshold >= camParam->dnParam.day2NightThreshold) {
		APP_DBG_ISP("day night threshold error, day2night: %d, night2day: %d\n", camParam->dnParam.day2NightThreshold, camParam->dnParam.night2DayThreshold);
	}
	else {
		APP_DBG_ISP("isp configuration ok\n");
		return APP_CONFIG_SUCCESS;
	}

	return APP_CONFIG_ERROR_DATA_INVALID;
}

int IspCtrl::controlCamWithNewParam(const string &type, fca_cameraParam_t *camParam) {
	int ret = APP_CONFIG_SUCCESS;
	if (type == "NightVision") {
		/* Set mode: FLOOD LIGHT (ON, OFF, AUTO) */
		if (camParam->dnParam.lightingEnable != false) {
			controlSmartLight(camParam->dnParam.lightingMode);
		}
		else {
			controlLighting(false);
		}
		mCamParam.dnParam.lightingEnable = camParam->dnParam.lightingEnable;
		mCamParam.dnParam.lightingMode	 = camParam->dnParam.lightingMode;

		/* Set mode: NIGHT VISION (ON, OFF, AUTO) */
		mCamParam.dnParam.nightVisionMode		 = camParam->dnParam.nightVisionMode;
		mCamParam.dnParam.day2NightThreshold	 = camParam->dnParam.day2NightThreshold;
		mCamParam.dnParam.night2DayThreshold	 = camParam->dnParam.night2DayThreshold;
		mCamParam.dnParam.awbNightDisMax		 = camParam->dnParam.awbNightDisMax;
		mCamParam.dnParam.dayCheckFrameNum		 = camParam->dnParam.dayCheckFrameNum;
		mCamParam.dnParam.nightCheckFrameNum	 = camParam->dnParam.nightCheckFrameNum;
		mCamParam.dnParam.expStableCheckFrameNum = camParam->dnParam.expStableCheckFrameNum;
		mCamParam.dnParam.stableWaitTimeoutMs	 = camParam->dnParam.stableWaitTimeoutMs;
		mCamParam.dnParam.gainR					 = camParam->dnParam.gainR;
		mCamParam.dnParam.offsetR				 = camParam->dnParam.offsetR;
		mCamParam.dnParam.gainB					 = camParam->dnParam.gainB;
		mCamParam.dnParam.offsetB				 = camParam->dnParam.offsetB;
		mCamParam.dnParam.gainRb				 = camParam->dnParam.gainRb;
		mCamParam.dnParam.offsetRb				 = camParam->dnParam.offsetRb;
		mCamParam.dnParam.blkCheckFlag			 = camParam->dnParam.blkCheckFlag;
		mCamParam.dnParam.lowLumiTh				 = camParam->dnParam.lowLumiTh;
		mCamParam.dnParam.highLumiTh			 = camParam->dnParam.highLumiTh;
		mCamParam.dnParam.nonInfraredRatioTh	 = camParam->dnParam.nonInfraredRatioTh;
		mCamParam.dnParam.highlightLumiTh		 = camParam->dnParam.highlightLumiTh;
		mCamParam.dnParam.highlightBlkCntMax	 = camParam->dnParam.highlightBlkCntMax;
		mCamParam.dnParam.lockCntTh				 = camParam->dnParam.lockCntTh;
		mCamParam.dnParam.lockTime				 = camParam->dnParam.lockTime;
		ret										 = setDayNightConfig(mCamParam.dnParam);
	}
	else if (type == "FlipMirror") {
		ret						= controlFlipMirror(camParam->pictureFlip, camParam->pictureMirror);
		mCamParam.pictureFlip	= camParam->pictureFlip;
		mCamParam.pictureMirror = camParam->pictureMirror;
	}
	else if (type == "ImageSettings") {
		mCamParam.brightness   = camParam->brightness;
		mCamParam.contrast	   = camParam->contrast;
		mCamParam.saturation   = camParam->saturation;
		mCamParam.sharpness	   = camParam->sharpness;
		mCamParam.irBrightness = camParam->irBrightness;
		mCamParam.irContrast   = camParam->irContrast;
		mCamParam.irSaturation = camParam->irSaturation;
		mCamParam.irSharpness  = camParam->irSharpness;
		task_post_pure_msg(GW_TASK_AV_ID, GW_AV_ADJUST_IMAGE_WITH_DAYNIGHT_STATE_REQ);
	}
	else if (type == "AntiFlicker") {
		ret						  = controlAntiFlickerMode(camParam->antiFlickerMode);
		mCamParam.antiFlickerMode = camParam->antiFlickerMode;
	}
	else if (type == "Lighting") {
		controlLighting(camParam->dnParam.lightingEnable);
		mCamParam.dnParam.lightingEnable = camParam->dnParam.lightingEnable;
	}
	else if (type == "Smartlight") {
		controlSmartLight(camParam->dnParam.lightingMode);
		mCamParam.dnParam.lightingMode = camParam->dnParam.lightingMode;
	}
	else if (type.empty()) {
		return controlCamWithNewParam("NightVision", camParam) + controlCamWithNewParam("FlipMirror", camParam) + controlCamWithNewParam("ImageSettings", camParam) +
			   controlCamWithNewParam("AntiFlicker", camParam) + controlCamWithNewParam("Lighting", camParam) + controlCamWithNewParam("Smartlight", camParam);
	}
	else {
		APP_DBG_ISP("unknown type isp control\n");
		ret = APP_CONFIG_ERROR_DATA_INVALID;
	}

	return ret;
}

int IspCtrl::getCamParamJs(json &json_in) {
	if (!fca_jsonSetParam(json_in, &mCamParam)) {
		APP_DBG_ISP("[ERR] data camera param config invalid\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
	return APP_CONFIG_SUCCESS;
}

int IspCtrl::controlLighting(bool b_Light) {
	IR_Lighting::STATUS state = (b_Light) ? (IR_Lighting::STATUS::ON) : (IR_Lighting::STATUS::OFF);
	gpiosHelpers.irLighting.setLighting(state);
	return 0;
}

int IspCtrl::controlSmartLight(int modeSmartLight) {
#if SUPPORT_FLOOD_LIGHT
	timer_remove_attr(GW_TASK_CONFIG_ID, GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ);
	timer_remove_attr(GW_TASK_CONFIG_ID, GW_CONFIG_RESTART_AUTO_LIGHTING_CONTROL_REQ);
	if (modeSmartLight == FCA_LIGHT_FULL_COLOR) {
		APP_DBG_ISP("smart light full color mode\n");
		controlLighting(true);
	}
	else if (modeSmartLight == FCA_LIGHT_BLACK_WHITE) {
		APP_DBG_ISP("smart light black white mode \n");
		controlLighting(false);
	}
	else {
		APP_DBG_ISP("smart light auto mode\n");
		ispCtrl.lightingChangeCount = 0;
		int ctrlState				= ispCtrl.getDayNightState();
		if (ctrlState == DN_DAY_STATE) {
			ispCtrl.controlLighting(false);
			APP_DBG_ISP("\tlighting OFF\n");
		}
		else {
			ispCtrl.controlLighting(true);
			APP_DBG_ISP("\tlighting ON\n");
		}
		timer_set(GW_TASK_CONFIG_ID, GW_CONFIG_AUTO_LIGHTING_CONTROL_REQ, GW_CONFIG_AUTO_LIGHTING_CONTROL_INTERVAL, TIMER_PERIODIC);
	}
#endif

	return 0;
}

int IspCtrl::controlWDR() {
	FCA_ISP_WDR_MODE_E modeSet = FCA_ISP_WDR_AUTO, modeGet;
	if (fca_isp_set_wdr_mode(FCA_ISP_WDR_AUTO) != 0) {
		APP_DBG_ISP("set wdr mode error\n");
		return -1;
	}

	fca_isp_get_wdr_mode(&modeGet);
	if (modeGet != modeSet) {
		APP_DBG_ISP("diff set-get wdr mode, set error\n");
		return -1;
	}

	APP_DBG_ISP("control wdr mode: %d success\n", modeSet);
	return 0;
}

int IspCtrl::controlDNR() {
	FCA_ISP_DNR_MODE_E modeSet = FCA_ISP_DNR_ENABLE, modeGet;
	if (fca_isp_set_2dnr(modeSet) != 0) {
		APP_DBG_ISP("set 2dnr mode error\n");
		return -1;
	}

	fca_isp_get_2dnr(&modeGet);
	if (modeGet != modeSet) {
		APP_DBG_ISP("diff set-get 2dnr mode, set error\n");
		return -1;
	}

	APP_DBG_ISP("control 2dnr mode success\n");
	return 0;
}

int IspCtrl::controlAWB() {
	FCA_ISP_AWB_MODE_E modeSet = FCA_ISP_AWB_AUTO, modeGet;
	if (fca_isp_set_wb_mode(modeSet) != 0) {
		APP_DBG_ISP("set awb mode error\n");
		return -1;
	}

	fca_isp_get_wb_mode(&modeGet);
	if (modeGet != modeSet) {
		APP_DBG_ISP("diff set-get awb mode, set error\n");
		return -1;
	}

	APP_DBG_ISP("control awb mode success\n");
	return 0;
}

void IspCtrl::getDayNightData(json &data) {
	// int lumi = 0, dis = 0;
	// unsigned int ev = 0, pnon_infrared_ratio = 0;

	// data = json::object();

	// ak_vpss_get_ev(DN_VIDEO_DEV, &ev);
	// ak_vpss_get_cur_lumi(DN_VIDEO_DEV, &lumi);
	// struct ak_non_infrared_ratio_attr nir_attr = {
	// 	mCamParam.dnParam.awbNightDisMax, mCamParam.dnParam.gainR,	  mCamParam.dnParam.offsetR,   mCamParam.dnParam.gainB,		 mCamParam.dnParam.offsetB,
	// 	mCamParam.dnParam.gainRb,		  mCamParam.dnParam.offsetRb, mCamParam.dnParam.lowLumiTh, mCamParam.dnParam.highLumiTh, mCamParam.dnParam.nonInfraredRatioTh};
	// ak_vpss_get_non_infrared_ratio(DN_VIDEO_DEV, &nir_attr, &pnon_infrared_ratio);
	// ak_vpss_get_rgb_dis(DN_VIDEO_DEV, mCamParam.dnParam.gainR, mCamParam.dnParam.offsetR, mCamParam.dnParam.gainB, mCamParam.dnParam.offsetB, &dis);

	// data = {
	// 	{"CurEV",				  ev					},
	// 	{"CurLumi",				lumi				},
	// 	{"CurNonInfraredRatio", pnon_infrared_ratio},
	// 	{"CurDis",			   dis				  },
	// };
	// APP_DBG_ISP("\nget data dn: %s\n", data.dump().data());
}

int IspCtrl::getDayNightState() {
	int status = DN_STATE_UNKNOWN;
	// FCA_DAY_NIGHT_STATE_E stTmp;
	// int ret = fca_get_dn_status_by_current_env(&stTmp);
	// if (ret == 0) {
	// 	APP_DBG_ISP("fca_get_dn_status: %d success\n", stTmp);
	// 	status = stTmp == FCA_DAY_STATE ? DN_DAY_STATE : DN_NIGHT_STATE;
	// }
	// else {
	// 	APP_DBG_ISP("fca_get_dn_status failed\n");
	// }
	return status;
}
