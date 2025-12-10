#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "task_list.h"
#include "task_av.h"

#include "fca_isp.hpp"
#include "fca_osd.hpp"
#include "fca_video.hpp"
#include "fca_audio.hpp"
#include "fca_common.hpp"

q_msg_t gw_task_av_mailbox;

void *gw_task_av_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;

	wait_all_tasks_started();

	APP_DBG_AV("[STARTED] Task AV\n");
	
	task_post_pure_msg(GW_TASK_AV_ID, GW_AV_INIT_REQ);

	while (1) {
		msg = ak_msg_rev(GW_TASK_AV_ID);

		switch (msg->header->sig) {
		case GW_AV_INIT_REQ: {
			APP_DBG_SIG("GW_AV_INIT_REQ\n");

			/* Initialise video */
			FCA_ENCODE_S encoders;
			{
				nlohmann::json js;
				FCA_ParserUserFiles(js, FCA_ENCODE_FILE);
				if (!FCA_ParserParams(js, &encoders)) {
					APP_ERROR("Can't parser encoders");
					break;
				}
			}
			videoHelpers.initialise();
			videoHelpers.setEncoders(&encoders);
			videoHelpers.startStreamChannels();
			videoHelpers.dummy();

			/* Initialise audio */
			audioHelpers.initialise();
			audioHelpers.startStreamChannels();

			/* TODO: Read configuration and set volume for mic and speaker */

			/* Set OSD */
			FCA_OSD_S osds;
			{
				nlohmann::json js;
				FCA_ParserUserFiles(js, FCA_WATERMARK_FILE);
				if (!FCA_ParserParams(js, &osds)) {
					APP_ERROR("Can't parser OSD");
				}
			}
			videoHelpers.setOsd(&osds);

			/* Query device has owner or not */
			std::string ownerStatusFile = FCA_VENDORS_FILE_LOCATE(FCA_OWNER_STATUS);
			if (doesFileExist(ownerStatusFile.c_str())) {
				audioHelpers.notifySalutation();
			}
			
			// timer_set(GW_TASK_DETECT_ID, GW_DETECT_INIT_REQ, GW_DETECT_INIT_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_START_RTSPD_REQ);
			
		}
		break;

		case GW_AV_SET_ENCODE_REQ: {
			APP_DBG_SIG("GW_AV_SET_ENCODE_REQ\n");

			FCA_ENCODE_S encoders;
			int rc = APP_CONFIG_ERROR_DATA_INVALID;
			nlohmann::json js = json::parse(string((char *)msg->header->payload, msg->header->len));

			if (FCA_ParserParams(js, &encoders)) {
				if (videoHelpers.setEncoders(&encoders)) {
					FCA_UpdateUserFiles(js, FCA_ENCODE_FILE); /* Save configuration */
					rc = APP_CONFIG_SUCCESS;
				}
				else APP_ERROR("Set encoders return failure");
			}
			else APP_ERROR("Parser payload encoders return failure");

			std::string response = FCA_TEMPLATE_SET_CONFIG_RESPONSE(MESSAGE_TYPE_ENCODE, rc);
			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)response.data(), response.length() + 1);
		}
		break;

		case GW_AV_GET_ENCODE_REQ: {
			APP_DBG_SIG("GW_AV_GET_ENCODE_REQ\n");

			nlohmann::json dataJs;
			int rc = APP_CONFIG_ERROR_DATA_INVALID;
			
			if (FCA_ParserUserFiles(dataJs, FCA_ENCODE_FILE)) {
				rc = APP_CONFIG_SUCCESS;
			}
			std::string response = FCA_TEMPLATE_GET_CONFIG_RESPONSE(MESSAGE_TYPE_ENCODE, dataJs, rc);
			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t*)response.data(), response.length() + 1);
		}
		break;

		case GW_AV_SET_WATERMARK_REQ: {
			APP_DBG_SIG("GW_AV_SET_WATERMARK_REQ\n");
			
			FCA_OSD_S osds;
			int rc = APP_CONFIG_ERROR_DATA_INVALID;
			nlohmann::json js = json::parse(string((char *)msg->header->payload, msg->header->len));

			if (FCA_ParserParams(js, &osds)) {
				if (videoHelpers.setOsd(&osds)) {
					FCA_UpdateUserFiles(js, FCA_WATERMARK_FILE); /* Save configuration */
					rc = APP_CONFIG_SUCCESS;
				}
				else APP_ERROR("Set OSD return failure");
			}
			else APP_ERROR("Parser payload OSD return failure");

			std::string response = FCA_TEMPLATE_SET_CONFIG_RESPONSE(MESSAGE_TYPE_WATERMARK, rc);
			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)response.data(), response.length() + 1);
		}
		break;

		case GW_AV_GET_WATERMARK_REQ: {
			APP_DBG_SIG("GW_AV_GET_WATERMARK_REQ\n");

			nlohmann::json dataJs;
			int rc = APP_CONFIG_ERROR_DATA_INVALID;

			if (FCA_ParserUserFiles(dataJs, FCA_WATERMARK_FILE)) {
				rc = APP_CONFIG_SUCCESS;
			}
			std::string response = FCA_TEMPLATE_GET_CONFIG_RESPONSE(MESSAGE_TYPE_WATERMARK, dataJs, rc);
			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t*)response.data(), response.length() + 1);
		}
		break;

		case GW_AV_SET_CAMERAPARAM_REQ: {
			APP_DBG_SIG("GW_AV_SET_CAMERAPARAM_REQ\n");
			string typeCtrl = "";
			int ret			= APP_CONFIG_ERROR_DATA_INVALID;
			try {
				json paramJs			   = json::parse(string((char *)msg->header->payload, msg->header->len));
				fca_cameraParam_t paramTmp = {0};
				if (fca_configGetParam(&paramTmp) == APP_CONFIG_SUCCESS) {
					typeCtrl = paramJs["Type"].get<string>();
					if (fca_jsonGetParamWithType(paramJs["Params"], &paramTmp, typeCtrl)) {
						if (ispCtrl.verifyConfig(&paramTmp) == APP_CONFIG_SUCCESS) {
							ret = ispCtrl.controlCamWithNewParam(typeCtrl, &paramTmp);
							fca_configSetParam(&paramTmp);
							ispCtrl.printCameraparamConfig();
						}
					}
				}
			}
			catch (const exception &error) {
				APP_DBG_AV("%s\n", error.what());
				ret = APP_CONFIG_ERROR_ANOTHER;
			}

			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_CAMERA_PARAM, ret)) {
				resJs["Data"]["Type"] = typeCtrl;
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}

		} break;

		case GW_AV_GET_CAMERAPARAM_REQ: {
			APP_DBG_SIG("GW_AV_GET_CAMERAPARAM_REQ\n");
			json resJs, dataJs = json::object(), dnDatajs;
			int ret = ispCtrl.getCamParamJs(dataJs);
			ispCtrl.getDayNightData(dnDatajs);
			dataJs["DnData"] = dnDatajs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_CAMERA_PARAM, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_AV_ADJUST_IMAGE_WITH_DAYNIGHT_STATE_REQ: {
			APP_DBG_SIG("GW_AV_ADJUST_IMAGE_WITH_DAYNIGHT_STATE_REQ\n");
			// fca_cameraParam_t camParam = ispCtrl.camParam();
			// if (ispCtrl.dayNightState.load() == DN_DAY_STATE) {
			// 	APP_DBG_ISP("adjust image in day mode: %d, %d, %d, %d\n", camParam.brightness, camParam.contrast, camParam.saturation, camParam.sharpness);
			// 	ispCtrl.imageAdjustments(camParam.brightness, camParam.contrast, camParam.saturation, camParam.sharpness);
			// 	SYSLOG_LOG(LOG_INFO, "logf=%s <======== DAY STATE =========>", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME);
			// }
			// else if (ispCtrl.dayNightState.load() == DN_NIGHT_STATE) {
			// 	APP_DBG_ISP("adjust image in night mode: %d, %d, %d, %d\n", camParam.irBrightness, camParam.irContrast, camParam.irSaturation, camParam.irSharpness);
			// 	ispCtrl.imageAdjustments(camParam.irBrightness, camParam.irContrast, camParam.irSaturation, camParam.irSharpness);
			// 	SYSLOG_LOG(LOG_INFO, "logf=%s <======== NIGHT STATE =========>", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME);
			// }
		} 
		break;

		case GW_AV_RELOAD_PICTURE_REQ: {
			APP_DBG_SIG("GW_AV_RELOAD_PICTURE_REQ\n");

			// fca_cameraParam_t camParam = ispCtrl.camParam();
			// FCA_OSD_S watermark  = osdCtrl.watermarkConf();

			// ispCtrl.controlFlipMirror(camParam.pictureFlip, camParam.pictureMirror);
			// osdCtrl.updateWatermark(&watermark);
		} 
		break;

		case GW_AV_WATCHDOG_PING_REQ: {
			// APP_DBG_SIG("GW_AV_WATCHDOG_PING_REQ\n");
			task_post_pure_msg(GW_TASK_AV_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
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
