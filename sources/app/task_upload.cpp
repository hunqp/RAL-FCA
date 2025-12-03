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
#include "parser_json.h"

q_msg_t gw_task_upload_mailbox;

static uint8_t retry = 0;
static S3Notification_t s3Notification;

static int calcDurationInSecsMP4(std::string fPath);

void *gw_task_upload_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;
	wait_all_tasks_started();
	APP_DBG("[STARTED] gw_task_upload_entry\n");

	task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_INIT_REQ);

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_UPLOAD_ID);

		switch (msg->header->sig) {
// 		case GW_UPLOAD_INIT_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_INIT_REQ\n");

// 			fca_s3_t s3Cfg;
// 			s3Cfg.clear();

// 			APP_DBG("S3 init with CA path: %s\n", caSslPath.c_str());
// 			s3Init(fca_getSerialInfo().c_str(), caSslPath.c_str());
// #ifdef RELEASE
// 			s3SetLogLevel(S3_LOG_DISABLE);
// #else
// 			s3SetLogLevel(S3_LOG_DEBUG);
// #endif
// 			bool bLoadS3KeySuccess = false;
// 			if (fca_configGetS3BucketSetting(&s3Cfg) != APP_CONFIG_SUCCESS) {
// 				if (fca_configGetS3(&s3Cfg) == APP_CONFIG_SUCCESS) {
// 					bLoadS3KeySuccess = true;
// 				}
// 				else {
// 					APP_DBG("[ERR] s3 config file not foud\n");
// 				}
// 			}
// 			else {
// 				bLoadS3KeySuccess = true;
// 			}

// 			if (bLoadS3KeySuccess) {
// 				if (s3SetConf(s3Cfg.accessKey.data(), s3Cfg.endPoint.data()) != 0) {
// 					APP_DBG("Failed to Set AccessKey S3\n");
// 				}
// 				else {
// 					APP_DBG("set AccessKey S3 ok\n");
// 				}
// 			}
// 		}
// 		break;

// 		case GW_UPLOAD_SET_S3_CONFIG_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_SET_S3_CONFIG_REQ\n");

// 			int ret = APP_CONFIG_ERROR_DATA_INVALID;
// 			try {
// 				json s3CfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));
// 				fca_s3_t paramTmp, s3Cfg;
// 				s3Cfg.clear();
// 				paramTmp.clear();
// 				/* get current s3 config */
// 				fca_configGetS3(&s3Cfg);
// 				if (fca_jsonGetS3(s3CfgJs, &paramTmp)) {
// 					if (s3SetConf(paramTmp.accessKey.data(), paramTmp.endPoint.data()) == 0) {
// 						ret = fca_configSetS3(&paramTmp);
// 						APP_DBG("[%s] Set S3 access key: %s\n", (ret == APP_CONFIG_SUCCESS) ? "SUCCESS" : "FAILED", s3Cfg.accessKey.data());
// 					}
// 				}
// 			}
// 			catch (const exception &error) {
// 				APP_DBG("%s\n", error.what());
// 				ret = APP_CONFIG_ERROR_ANOTHER;
// 			}

// 			/* response to mqtt server */
// 			json resJs;
// 			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_S3, ret)) {
// 				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
// 			}
// 		}
// 		break;

// 		case GW_UPLOAD_GET_S3_CONFIG_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_GET_S3_CONFIG_REQ\n");

// 			fca_s3_t s3Cfg;
// 			s3Cfg.clear();
// 			json dataJs	= json::object();
// 			int ret	= fca_configGetS3(&s3Cfg);
// 			if (ret == APP_CONFIG_SUCCESS) {
// 				if (!fca_jsonSetS3(dataJs, &s3Cfg)) {
// 					ret = APP_CONFIG_ERROR_DATA_INVALID;
// 				}
// 			}
// 			json resJs;
// 			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_S3, ret)) {
// 				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
// 			}
// 		}
// 		break;

// 		case GW_UPLOAD_SET_BUCKET_SETTING_CONFIG_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_SET_BUCKET_SETTING_CONFIG_REQ\n");

// 			int ret = APP_CONFIG_ERROR_DATA_INVALID;
// 			try {
// 				json s3CfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));
// 				fca_s3_t paramTmp, s3Cfg;
// 				s3Cfg.clear();
// 				paramTmp.clear();
// 				fca_configGetS3BucketSetting(&s3Cfg);
// 				if (fca_jsonGetS3(s3CfgJs, &paramTmp)) {
// 					if (s3SetConf(paramTmp.accessKey.data(), paramTmp.endPoint.data()) == 0) {
// 						ret = fca_configSetS3BucketSetting(&paramTmp);
// 						APP_DBG("[%s] Set S3 access key: %s\n", (ret == APP_CONFIG_SUCCESS) ? "SUCCESS" : "FAILED", s3Cfg.accessKey.data());
// 					}
// 				}
// 			}
// 			catch (const exception &error) {
// 				APP_DBG("%s\n", error.what());
// 				ret = APP_CONFIG_ERROR_ANOTHER;
// 			}

// 			/* response to mqtt server */
// 			json resJs;
// 			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_BUCKET_SETTING, ret)) {
// 				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
// 			}
// 		}
// 		break;

// 		case GW_UPLOAD_GET_BUCKET_SETTING_CONFIG_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_GET_BUCKET_SETTING_CONFIG_REQ\n");

// 			fca_s3_t s3Cfg;
// 			s3Cfg.clear();
// 			json dataJs	= json::object();
// 			int ret	= fca_configGetS3BucketSetting(&s3Cfg);
// 			if (ret == APP_CONFIG_SUCCESS) {
// 				if (!fca_jsonSetS3(dataJs, &s3Cfg)) {
// 					ret = APP_CONFIG_ERROR_DATA_INVALID;
// 				}
// 			}
// 			json resJs;
// 			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_BUCKET_SETTING, ret)) {
// 				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
// 			}
// 		}
// 		break;

// 		case GW_UPLOAD_SET_INFO_UPLOAD: {
// 			APP_DBG_SIG("GW_UPLOAD_SET_INFO_UPLOAD\n");

// 			memset(&s3Notification, 0, sizeof(S3Notification_t));
// 			memcpy(&s3Notification, msg->header->payload, sizeof(S3Notification_t));
// 			retry = 2;
// 			APP_DBG("Current snap information:\r\n");
// 			APP_DBG("Path     : %s\r\n", s3Notification.path);
// 			APP_DBG("bJPEG    : %d\r\n", s3Notification.bJPEG);
// 			APP_DBG("Type     : %d\r\n", s3Notification.type);
// 			APP_DBG("Timestamp: %d\r\n", s3Notification.timestamp);

// 			if (s3Notification.bJPEG) {
// 				task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_FILE_JPEG_REQ);
// 			}
// 			else {
// 				task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_FILE_MP4_REQ);
// 			}
// 		}
// 		break;

// 		case GW_UPLOAD_FILE_JPEG_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_FILE_JPEG_REQ\n");

// 			S3_EVENT_TYPE_E s3Type = S3_EVENT_NONE;
// 			switch (s3Notification.type) {
// 			case JPEG_SNAP_TYPE_MOTION: s3Type = S3_EVENT_MOTION;
// 				break;
// 			case JPEG_SNAP_TYPE_HUMAN: s3Type = S3_EVENT_HUMAN;
// 				break;
// 			default:
// 				break;
// 			}

// 			if (!doesFileExist(s3Notification.path) || s3Type == S3_EVENT_NONE) {
// 				break;
// 			}

// 			int ret = -1;
// 			s3_object_t s3_obj;
// 			memset(&s3_obj, 0, sizeof(s3_obj));

// 			try {
// 				ret = s3PutImage(s3Notification.path, s3Type, s3Notification.timestamp, NULL, 5, GW_UPLOAD_PUT_BITRATE_LIMIT, &s3_obj);
// 			}
// 			catch (std::exception &excp) {
// 				APP_DBG("push s3 image excp: %s\n", excp.what());
// 			}

// 			if (ret != 0) {
// 				LOG_FILE_SERVICE("S3Put jpeg %d error %d %s\n", s3Notification.timestamp, ret, s3GetStatusName(ret));
// 				if (retry-- > 0) {
// 					timer_set(GW_TASK_UPLOAD_ID, GW_UPLOAD_FILE_JPEG_REQ, GW_UPLOAD_FILE_JPEG_REQ_INTERVAL, TIMER_ONE_SHOT);
// 					break;
// 				}
// 			}
// 			s3_obj.ret = ret;
// 			task_post_dynamic_msg(GW_TASK_DETECT_ID, GW_DETECT_PUSH_MQTT_ALARM_NOTIFICATION, (uint8_t *)&s3_obj,sizeof(s3_object_t));
// 			remove(s3Notification.path);
// 		}
// 		break;

// 		case GW_UPLOAD_FILE_MP4_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_FILE_MP4_REQ\n");

// 			S3_EVENT_TYPE_E s3Type = S3_EVENT_NONE;
// 			switch (s3Notification.type) {
// 			case JPEG_SNAP_TYPE_MOTION: s3Type = S3_EVENT_MOTION;
// 				break;
// 			case JPEG_SNAP_TYPE_HUMAN: s3Type = S3_EVENT_HUMAN;
// 				break;
// 			default:
// 				break;
// 			}

// 			if (!doesFileExist(s3Notification.path) || s3Type == S3_EVENT_NONE) {
// 				break;
// 			}

// 			int ret = -1;
// 			s3_object_t s3_obj;
// 			memset(&s3_obj, 0, sizeof(s3_obj));

// 			try {
// 				APP_DBG("upload S3 mp4 size:\n%s\n", systemStrCmd("ls -la %s", s3Notification.path).data());
// 				ret = s3PutVideoMp4(s3Notification.path, s3Type, s3Notification.timestamp, NULL, 20, GW_UPLOAD_PUT_BITRATE_LIMIT, &s3_obj);
// 			}
// 			catch (std::exception &excp) {
// 				APP_DBG("push s3 image excp: %s\n", excp.what());
// 			}

// 			if (ret != 0) {
// 				LOG_FILE_SERVICE("S3Put mp4 %d error %d %s\n", s3Notification.timestamp, ret, s3GetStatusName(ret));
// 				if (retry-- > 0) {
// 					timer_set(GW_TASK_UPLOAD_ID, GW_UPLOAD_FILE_MP4_REQ, GW_UPLOAD_FILE_MP4_REQ_INTERVAL, TIMER_ONE_SHOT);
// 					break;
// 				}
// 			}

// 			/* Publish MQTT message when put MP4 to S3 succeed */
// 			s3_obj.ret = ret;
// 			uint32_t duration = calcDurationInSecsMP4(s3Notification.path);
// 			std::string rep = getPayloadMQTTSignaling(s3Notification.timestamp, s3Notification.type, duration, &s3_obj);
// 			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_ALARM_RES, (uint8_t *)rep.data(), rep.length());

// 			remove(s3Notification.path);
// 		}
// 		break;

// 		case GW_UPLOAD_SNAPSHOT_REQ: {
// 			APP_DBG_SIG("GW_UPLOAD_SNAPSHOT_REQ\n");

// 			int ret = APP_CONFIG_ERROR_ANOTHER;
// 			std::string filename;
// 			json reqJs, resJs;

// 			try {
// 				reqJs = json::parse(std::string((char *)msg->header->payload, msg->header->len));
// 				std::string url = reqJs["PutUrl"].get<std::string>();
// 				filename = reqJs["FileName"].get<std::string>();

// 				if (!url.empty() && !filename.empty()) {
// 					std::string filePath = FCA_MEDIA_JPEG_PATH + filename;
// 					HAL_VIDEO_Snapshot(filePath.c_str());
// 					string http_res;
// 					if (CURL_Upload(caSslPath, url, filePath, http_res) == 0) {
// 						ret = APP_CONFIG_SUCCESS;
// 					}
// 					unlink(filePath.c_str());
// 				}
// 			}
// 			catch(const exception &error) {
// 				APP_DBG_MD("%s\n", error.what());
// 				ret = APP_CONFIG_ERROR_DATA_INVALID;
// 			}

// 			/* Response MQTT */
// 			reqJs.clear();
// 			reqJs["FileName"] = filename;
// 			fca_getSnapshotJsonRes(resJs, reqJs, ret);
// 			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
// 		}
// 		break;

// 		case GW_UPLOAD_WATCHDOG_PING_REQ: {
// 			// APP_DBG_SIG("GW_UPLOAD_WATCHDOG_PING_REQ\n");
// 			task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
// 		}
// 		break;

		default:
        break;
		}

		/* free message */
		ak_msg_free(msg);
	}

	return (void *)0;
}

int calcDurationInSecsMP4(std::string fPath) {
    // MP4FileHandle mp4File = MP4Read(fPath.c_str());
    // if (mp4File == MP4_INVALID_FILE_HANDLE) {
    //     return 0;
    // }

    // uint32_t numTracks = MP4GetNumberOfTracks(mp4File);
    // if (numTracks == 0) {
    //     MP4Close(mp4File);
    //     return 0;
    // }

    int maxDuration = 0;

    // for (uint32_t i = 0; i < numTracks; i++) {
    //     MP4TrackId trackId = i + 1;

    //     MP4Duration trackDuration = MP4GetTrackDuration(mp4File, trackId);
    //     if (trackDuration == MP4_INVALID_DURATION) {
    //         continue;
    //     }

    //     uint32_t timescale = MP4GetTrackTimeScale(mp4File, trackId);
    //     if (timescale == 0) {
    //         continue;
    //     }

    //     int durationSeconds = static_cast<int>(trackDuration / timescale);

    //     if (durationSeconds > maxDuration) {
    //         maxDuration = durationSeconds;
    //     }
    // }

    // MP4Close(mp4File);

    // if (maxDuration == 0.0) {
    //     return 0;
    // }

    return maxDuration;
}
