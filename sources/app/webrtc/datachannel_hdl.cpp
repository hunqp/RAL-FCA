#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <mntent.h>
#include <unordered_map>

#include <fcntl.h>
#include <chrono>
#include <fstream>

#include "app.h"
#include "app_data.h"
#include "app_dbg.h"
#include "app_config.h"
#include "firmware.h"
#include "task_list.h"

#include "rtc/rtc.hpp"
#include "datachannel_hdl.h"
#include "json.hpp"
#include "helpers.hpp"
#include "videosource.hpp"
#include "h26xparsec.h"
#include "utils.h"

#include "fca_video.hpp"

using namespace std;
using namespace rtc;
using json = nlohmann::json;
using namespace chrono;

#define PACKET_TRANSFER_SIZE (51200)	// 5kb - 2096, 4096, 5120, 6144, 8192, 16384, 51200

static int rtcControlPanTilt(json &content, bool &respFlag);
static int rtcControlStream(json &content, bool &respFlag);
static int rtcForceDisconnect(json &content, bool &respFlag);
static int rtcAsigneePushToTalk(json &content, bool &respFlag);
static int rtcControlSoundAlarm(json &content, bool &respFlag);

static int rtcQueryPlaylistOfExtDisks(json &content, bool &respFlag);
static int rtcCntlPlaybackOfExtDisks(json &content, bool &respFlag);
static int downloadHdl(json &content, bool &respFlag);
static int rtcForceFormatExtDisks(json &content, bool &respFlag);
static int recordCalendarHdl(json &content, bool &respFlag);
static int countRecordCalendarHdl(json &content, bool &respFlag);

static unordered_map<string, function<int(json &, bool &)>> dcCmdTblMaps = {
	{DC_CMD_PANTILT,				rtcControlPanTilt			},
	{DC_CMD_STREAM,					rtcControlStream			},
	{DC_CMD_DISCONNECT_REPORT,	   	rtcForceDisconnect			},
	{DC_CMD_PUSH_TO_TALK,		  	rtcAsigneePushToTalk		},
	{DC_CMD_EMERGENCY_ALARM,		rtcControlSoundAlarm		},
	{DC_CMD_PLAYLIST,			  	rtcQueryPlaylistOfExtDisks	},
	{DC_CMD_PLAYBACK,			  	rtcCntlPlaybackOfExtDisks	},
	{DC_CMD_DOWNLOAD,			  	downloadHdl			 		},
	{DC_CMD_FORMAT_STORAGE,			rtcForceFormatExtDisks	  	},
	{DC_CMD_RECORD_CALENDAR,		recordCalendarHdl	  		},
	{DC_CMD_COUNT_RECORD_CALENDAR, 	countRecordCalendarHdl		},
};

static string qrId; /* Current client Id is queried */
static int32_t qrSequenceId;
static int32_t qrSessionId;

void onDataChannelHdl(const string clId, const string &req, string &resp) {
	json JSON;
	bool boolean = true;
	JSON		 = json::parse(req);

	/* If the ID is not correct -> do notthing */
	if (JSON["Id"].get<string>().compare(fca_getSerialInfo()) != 0) {
		APP_DBG_RTC("The ID %s is not correct\n", JSON["Id"].get<string>().c_str());
		return;
	}

	boolean	   = (JSON["Type"].get<string>().compare("Request") == 0) ? true : false;
	string cmd = JSON["Command"].get<string>();
	if (cmd.compare("Hello") == 0 && boolean) {
		JSON["Type"] = "Respond";
		resp.assign(JSON.dump());
		return;
	}

	auto fit	= clients.find(clId);
	auto client = fit->second;
	if (JSON.contains("SequenceId")) {
		qrSequenceId = JSON["SequenceId"].get<int32_t>();
		client->setSequenceId(qrSequenceId);
	}
	if (JSON.contains("SessionId")) {
		qrSessionId = JSON["SessionId"].get<int32_t>();
		client->setSessionId(qrSessionId);
	}

	/* Traverse to find valid command */
	auto it = dcCmdTblMaps.find(cmd);
	if (it != dcCmdTblMaps.end() && boolean) {
		qrId	= clId;
		int ret = it->second(JSON.at("Content"), boolean);
		if (boolean) {
			JSON["Type"]			  = "Respond";
			JSON["Result"]["Ret"]	  = ret;
			JSON["Result"]["Message"] = (ret == APP_CONFIG_SUCCESS ? "Success" : "Fail");
			resp.assign(JSON.dump());
		}
	}
	else {
		APP_DBG_RTC("Invalid command %s\n", cmd.c_str());
	}
}

int rtcControlPanTilt(json &content, bool &respFlag) {
	(void)(respFlag);
	int rc = APP_CONFIG_SUCCESS;
	APP_DBG_RTC("rtcControlPanTilt() -> %s\n", content.dump(4).c_str());

	if (content["Direction"].get<string>().compare("Up") == 0) {
		motors.run(FCA_PTZ_DIRECTION_UP);
	}
	else if (content["Direction"].get<string>().compare("Down") == 0) {
		motors.run(FCA_PTZ_DIRECTION_DOWN);
	}
	else if (content["Direction"].get<string>().compare("Left") == 0) {
		motors.run(FCA_PTZ_DIRECTION_LEFT);
	}
	else if (content["Direction"].get<string>().compare("Right") == 0) {
		motors.run(FCA_PTZ_DIRECTION_RIGHT);
	}
	else {
		rc = APP_CONFIG_ERROR_DATA_INVALID;
	}
	content.clear();
	timer_set(GW_TASK_CONFIG_ID, GW_CONFIG_FORCE_MOTOR_STOP_REQ, 500, TIMER_ONE_SHOT);

	return rc;
}

int rtcControlStream(json &content, bool &respFlag) {
	(void)(respFlag);
	int rc = APP_CONFIG_SUCCESS;
	APP_DBG_RTC("rtcControlStream() -> %s\n", content.dump(4).c_str());

	Client::eOptions opt	= content["Option"].get<Client::eOptions>();
	Client::eResolution res = content["Resolution"].get<Client::eResolution>();

	if ((opt != Client::eOptions::Idle && opt != Client::eOptions::LiveStream && opt == Client::eOptions::Playback) ||
	(res != Client::eResolution::Minimum && res != Client::eResolution::HD720p && res != Client::eResolution::FullHD1080p)) {
		return APP_CONFIG_ERROR_DATA_INVALID;
	}

	auto it = clients.find(qrId);
	if (it == clients.end()) {
		return APP_CONFIG_ERROR_ANOTHER;
	}
	auto qrClient = it->second;

	/* Send previous NALU key frame so users don't have to wait to see stream works */
	if (opt == Client::eOptions::LiveStream) {
		opt = Client::eOptions::Pending;
	}
	qrClient->setMediaStreamOptions(opt);
	qrClient->setLiveResolution(res);

	content.clear();
	return rc;
}

int rtcForceDisconnect(json &content, bool &respFlag) {
	(void)(respFlag);
	(void)(content);
	APP_DBG_RTC("[MANUAL] clear client id: %s\n", qrId.c_str());
	task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ERASE_CLIENT_REQ, (uint8_t *)qrId.c_str(), qrId.length() + 1);
	return APP_CONFIG_SUCCESS;
}

int rtcAsigneePushToTalk(json &content, bool &respFlag) {
	(void)respFlag;
	Client::ePushToTalkREQStatus req;

	try {
		req = content["Mode"].get<Client::ePushToTalkREQStatus>();
	}
	catch (...) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	content.clear();
	content["Mode"]	  = (int)req;
	content["Status"] = false;

	switch (req) {
	case Client::ePushToTalkREQStatus::Begin: {
		if (Client::currentClientPushToTalk.empty()) {
			Client::currentClientPushToTalk.set(qrId);
			content["Status"] = true;

			audioHelpers.silent();

			task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_PENDING_TRIGGER_SOUND_ALARM_REQ);
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_LOAD_SPEAKER_MIC_CONFIG_REQ);
			timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK_INTERVAL, TIMER_ONE_SHOT);
		}
	} break;

	case Client::ePushToTalkREQStatus::Stop: {
		if (qrId == Client::currentClientPushToTalk.get()) {
			content["Status"] = true;

			/* Wait until speaker and mic has been configured completely */
			timer_remove_attr(GW_TASK_WEBRTC_ID, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK);
			task_post_pure_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK);
		}
	} break;

	case Client::ePushToTalkREQStatus::KeepSession: {
		if (qrId == Client::currentClientPushToTalk.get()) {
			content["Status"] = true;
			timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK_INTERVAL, TIMER_ONE_SHOT);
		}
	} break;
	default:
		break;
	}

	return APP_CONFIG_SUCCESS;
}

int rtcControlSoundAlarm(json &content, bool &respFlag) {
	bool enable;
	AlarmTrigger_t almTrigger;

	try {
		enable					 = content["Enable"].get<bool>();
		almTrigger.bSound		 = content["Sound"].get<bool>();
		almTrigger.bLight		 = content["Lighting"].get<bool>();
		almTrigger.elapsedSecond = content["Interval"].get<int>();

		if (almTrigger.elapsedSecond < 0) {
			return APP_CONFIG_ERROR_DATA_INVALID;
		}
	}
	catch (...) {
		return APP_CONFIG_ERROR_DATA_INVALID;
	}

	if (enable) {
		audioHelpers.playSiren(almTrigger.elapsedSecond);
		// task_post_common_msg(GW_TASK_DETECT_ID, GW_DETECT_START_TRIGGER_ALARM_REQ, (uint8_t *)&almTrigger, sizeof(AlarmTrigger_t));
	}
	else {
		audioHelpers.silent();
		// task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_STOP_TRIGGER_ALARM_REQ);
	}

	return APP_CONFIG_SUCCESS;
}

int rtcQueryPlaylistOfExtDisks(json &content, bool &respFlag) {
	(void)(respFlag);
	APP_DBG_RTC("rtcQueryPlaylistOfExtDisks() -> %s\n", content.dump(4).c_str());

	#define QRY_PLAYLIST_TYPE_ALL 			( 0)
	#define QRY_PLAYLIST_TYPE_REGULAR 		((int)FILE_RECORD_TYPE_247)
	#define QRY_PLAYLIST_TYPE_MOTION 		((int)FILE_RECORD_TYPE_MOTION_DETECTED)
	#define QRY_PLAYLIST_TYPE_HUMAN 		((int)FILE_RECORD_TYPE_HUMAN_DETECTED)

	/*
	{
		"Id": "camerainfake0134",
		"Type": "Request",
		"Command": "Playlist",
		"Content": {
			"Type": [2, 3],
			"BeginTime": 1766369361,
			"EndTime": 1766399361
		}
	}
	*/
	try {
		uint32_t beginTS = content["BeginTime"].get<uint32_t>();
		uint32_t endTS = content["EndTime"].get<uint32_t>();

		std::vector<RECORDER_INDEX_S> playlist {};

		playlist = SDCARD.queryPlaylists(beginTS, endTS);
		for (auto it : playlist) {
			bool hasEvents = (it.metadata.ind > 0) ? true : false;
			nlohmann::json js = {
				{"Name",		it.name		},
				{"StartTS",		it.startTs	},
				{"StopTS",		it.closeTs	},
				{"HasEvents",	hasEvents 	}
			};
			content["Playlist"].push_back(js);

			for (uint8_t id = 0; id < it.metadata.ind; ++id) {				
				for (auto jt : content["Type"]) {
					if (jt != QRY_PLAYLIST_TYPE_ALL || it.metadata.events[id].type != jt) {
						continue;
					}
					nlohmann::json sjs;
					sjs["Reference"] = it.name;
					sjs["Type"] = it.metadata.events[id].type;
					sjs["OffsetSeconds"] = it.metadata.events[id].offsetSeconds;
					sjs["Timestamp"] = {it.metadata.events[id].beginTS, it.metadata.events[id].endTS};
					content["MetadataEvents"].push_back(sjs);
					break;	
				}
			}
		}

		printf("Playlist:\r\n%s\r\rn", content.dump(4).c_str());
	}
	catch (...) {
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	// 	if (content["Type"].is_array()) {
	// 		for (auto it : content["Type"]) {
	// 			if (it == QRY_TYPE_EVT) {
	// 				qrMask = (RECORDER_TYPE)(RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 			}
	// 			else if (it == QRY_TYPE_ALL) {
	// 				qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG | RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 			}
	// 			else if (it == QRY_TYPE_REG) {
	// 				qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG);
	// 			}
	// 			else if (it == QRY_TYPE_MDT) {
	// 				qrMask = (RECORDER_TYPE)RECORD_TYPE_MDT;
	// 			}
	// 			else if (it == QRY_TYPE_HMD) {
	// 				qrMask = (RECORDER_TYPE)RECORD_TYPE_HMD;
	// 			}
	// 		}
	// 	}
	// 	else {
	// 		SD_QUERY_PLAYLIST qrPlaylits = content["Type"].get<SD_QUERY_PLAYLIST>();
	// 		if (qrPlaylits == QRY_TYPE_EVT) {
	// 			qrMask = (RECORDER_TYPE)(RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_ALL) {
	// 			qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG | RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_REG) {
	// 			qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG);
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_MDT) {
	// 			qrMask = (RECORDER_TYPE)RECORD_TYPE_MDT;
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_HMD) {
	// 			qrMask = (RECORDER_TYPE)RECORD_TYPE_HMD;
	// 		}
	// 	}
	// }
	// catch (...) {
	// 	return APP_CONFIG_ERROR_FILE_OPEN;
	// }

	// try {
	// 	begTsStr = content["BeginTime"].get<std::string>();
	// 	endTsStr = content["EndTime"].get<std::string>();

	// 	/* 	NOTE: content["Type"] is an integer for app old version <= 5.6.0,
	// 		extend query type array for app new version >= 5.6.0.
	// 	*/
	// 	if (content["Type"].is_array()) {
	// 		for (auto it : content["Type"]) {
	// 			if (it == QRY_TYPE_EVT) {
	// 				qrMask = (RECORDER_TYPE)(RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 			}
	// 			else if (it == QRY_TYPE_ALL) {
	// 				qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG | RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 			}
	// 			else if (it == QRY_TYPE_REG) {
	// 				qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG);
	// 			}
	// 			else if (it == QRY_TYPE_MDT) {
	// 				qrMask = (RECORDER_TYPE)RECORD_TYPE_MDT;
	// 			}
	// 			else if (it == QRY_TYPE_HMD) {
	// 				qrMask = (RECORDER_TYPE)RECORD_TYPE_HMD;
	// 			}
	// 		}
	// 	}
	// 	else {
	// 		SD_QUERY_PLAYLIST qrPlaylits = content["Type"].get<SD_QUERY_PLAYLIST>();
	// 		if (qrPlaylits == QRY_TYPE_EVT) {
	// 			qrMask = (RECORDER_TYPE)(RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_ALL) {
	// 			qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG | RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_REG) {
	// 			qrMask = (RECORDER_TYPE)(RECORD_TYPE_REG);
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_MDT) {
	// 			qrMask = (RECORDER_TYPE)RECORD_TYPE_MDT;
	// 		}
	// 		else if (qrPlaylits == QRY_TYPE_HMD) {
	// 			qrMask = (RECORDER_TYPE)RECORD_TYPE_HMD;
	// 		}
	// 	}
	// }
	// catch (...) {
	// 	return APP_CONFIG_ERROR_FILE_OPEN;
	// }

	// uint32_t qrBegTs = u32TsConvert(begTsStr, RECORD_DATETIME_FORMAT);
	// uint32_t qrEndTs = u32TsConvert(endTsStr, RECORD_DATETIME_FORMAT);
	// APP_DBG("qrMask : %d\r\n", qrMask);
	// APP_DBG("qrBegTs: %d\r\n", qrBegTs);
	// APP_DBG("qrEndTs: %d\r\n", qrEndTs);

	// content.clear();
	// content["Total"]	 = 0;
	// content["BeginTime"] = begTsStr;
	// content["EndTime"]	 = endTsStr;
	// content["Playlists"].clear();

	// vector<RecorderDescStructure> listRecords;
	// if (qrEndTs > qrBegTs) {
	// 	listRecords = sdCard.doOPER_GetPlaylists(qrBegTs, qrEndTs, qrMask);
	// }

	// content["Total"] = listRecords.size();
	// APP_DBG("Total records: %d\n", listRecords.size());
	// for (size_t id = 0; id < listRecords.size(); ++id) {
	// 	int type;
	// 	if (listRecords[id].type == RECORD_TYPE_REG) {
	// 		type = (int)QRY_TYPE_REG;
	// 	}
	// 	else if (listRecords[id].type == RECORD_TYPE_MDT) {
	// 		type = (int)QRY_TYPE_MDT;
	// 	}
	// 	else {
	// 		type = (int)QRY_TYPE_HMD;
	// 	}
	// 	json JS;
	// 	JS["OrderNumber"]	 = id;
	// 	JS["Type"]			 = type;
	// 	JS["FileName"]		 = listRecords[id].name;
	// 	JS["DurationInSecs"] = listRecords[id].durationInSecs;
	// 	JS["BeginTime"]		 = strTsConvert(listRecords[id].begTs);
	// 	JS["EndTime"]		 = strTsConvert(listRecords[id].endTs);
	// 	content["Playlists"].push_back(JS);
	// }

	return APP_CONFIG_SUCCESS;
}

int rtcCntlPlaybackOfExtDisks(json &content, bool &respFlag) {
	// std::string rcStr;
	// std::string dateStr;

	// APP_DBG("%s\r\n", content.dump(4).c_str());

	// try {
	// 	rcStr				   = content["FileName"].get<string>();
	// 	dateStr				   = content["BeginTime"].get<string>();
	// 	dateStr				   = dateStr.substr(0, dateStr.find(' '));
	// 	PLAYBACK_CONTROL pbCmd = content["Status"].get<PLAYBACK_CONTROL>();
	// 	PLAYBACK_STATUS pbStat = PB_STATE_PLAYING;

	// 	auto it = clients.find(qrId);
	// 	if (it == clients.end()) {
	// 		return APP_CONFIG_ERROR_ANOTHER;
	// 	}
	// 	auto qrClient = it->second;

	// 	if (pbCmd == PB_CTL_PLAY) {
	// 		PlbSdSource pbSdSrc;
	// 		const std::string viSolderStr = DEFAULT_VIDEO_FOLDER + std::string("/") + dateStr;
	// 		const std::string auSolderStr = DEFAULT_AUDIO_FOLDER + std::string("/") + dateStr;

	// 		bool b = pbSdSrc.loadattributes(viSolderStr, auSolderStr, rcStr);

	// 		if (!b) {
	// 			pbStat = PB_STATE_ERROR;
	// 		}
	// 		else {
	// 			qrClient->setPlbSdSource(pbSdSrc);
	// 		}

	// 		APP_DBG("[PLAYBACK] Play video path: %s {%d}\r\n", pbSdSrc.video.pathStr.c_str(), pbSdSrc.video.fileSize);
	// 		APP_DBG("[PLAYBACK] Play audio path: %s {%d}\r\n", pbSdSrc.audio.pathStr.c_str(), pbSdSrc.audio.fileSize);
	// 	}

	// 	if (pbStat != PB_STATE_ERROR) {
	// 		uint32_t argv = 0;
	// 		if (pbCmd == PB_CTL_SEEK) {
	// 			argv = content["SeekPos"].get<uint32_t>();
	// 		}
	// 		else if (pbCmd == PB_CTL_SPEED) {
	// 			argv = content["Speed"].get<uint32_t>();
	// 		}

	// 		qrClient->setMediaStreamOptions(Client::eOptions::Playback);
	// 		qrClient->setPlbSdControl(pbCmd, &argv);
	// 	}
	// }
	// catch (...) {
	// 	return APP_CONFIG_ERROR_DATA_INVALID;
	// }

	return APP_CONFIG_SUCCESS;
}

int downloadHdl(json &content, bool &respFlag) {
	// APP_DBG_RTC("downloadHdl() -> %s\n", content.dump(4).c_str());
	int rc = APP_CONFIG_SUCCESS;
	// if (auto jt = clients.find(qrId); jt != clients.end()) {
	// 	auto cl = jt->second;
	// 	try {
	// 		string signal = content["Signal"].get<string>();
	// 		/* state ready download */
	// 		APP_DBG_RTC("downloadHdl() -> 1\n");
	// 		if (!cl->getDownloadFlag()) {
	// 			APP_DBG_RTC("downloadHdl() -> 2\n");
	// 			if (signal == "Start") {
	// 				APP_DBG_RTC("[DOWNLOAD][SIG] Start\n");
	// 				string fileName = content["FileName"].get<string>();
	// 				string fileType = content["FileType"].get<string>();
	// 				string time		= content["Time"].get<string>();

	// 				SYSLOG_LOG(LOG_INFO, "logf=%s [DOWNLOAD] start transfer file: %s", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, fileName.data());

	// 				bool boolean = false;

	// 				if (fileType == "Media") {
	// 					string audioPath = std::string(DEFAULT_AUDIO_FOLDER) + "/" + time + "/" + fileName + RECORD_AUD_SUFFIX;
	// 					string videoPath = std::string(DEFAULT_VIDEO_FOLDER) + "/" + time + "/" + fileName + RECORD_VID_SUFFIX;

	// 					APP_DBG_RTC("audio file: %s\n", audioPath.c_str());
	// 					APP_DBG_RTC("video file: %s\n", videoPath.c_str());
	// 					if (doesFileExist(audioPath.c_str()) && doesFileExist(videoPath.c_str())) {
	// 						cl->arrFilesDownload.clear();
	// 						cl->arrFilesDownload.push_back({"Audio", audioPath});
	// 						cl->arrFilesDownload.push_back({"Video", videoPath});
	// 						APP_DBG_RTC("file: %s\n", cl->arrFilesDownload.at(0).path.c_str());
	// 						content["Total"] = cl->arrFilesDownload.size();

	// 						boolean = true;
	// 					}
	// 				}
	// 				else if (fileType == "Jpg") {
	// 					string videoPath = std::string(DEFAULT_VIDEO_FOLDER) + "/" + time + "/" + fileName + RECORD_VID_SUFFIX;

	// 					auto sample = H264_ExtractSPSPPSIDR(videoPath.c_str());

	// 					if (sample.size()) {
	// 						std::string thmbnailPath = std::string(FCA_MEDIA_JPEG_PATH) + "/" + fileName + RECORD_IMG_SUFFIX;

	// 						APP_DBG_RTC("Path thumbnail download: %s\n", thmbnailPath.c_str());

	// 						int fd = open(thmbnailPath.c_str(), O_RDWR | O_CREAT | O_APPEND, 0777);
	// 						if (fd != -1) {
	// 							write(fd, (uint8_t *)sample.data(), sample.size());
	// 							fsync(fd);
	// 							close(fd);

	// 							/* CREATE: File raw thumbnail samples */
	// 							cl->arrFilesDownload.clear();
	// 							cl->arrFilesDownload.push_back({"Thumbnail", thmbnailPath});
	// 							content["Total"] = cl->arrFilesDownload.size();

	// 							boolean = true;
	// 						}
	// 					}
	// 				}

	// 				if (boolean) {
	// 					cl->setDownloadFlag(true);
	// 					/* start timeout wait response */
	// 					cl->startTimeoutDownload();
	// 					rc = APP_CONFIG_SUCCESS;
	// 				}
	// 				else {
	// 					APP_DBG_RTC("file not exist\n");
	// 					rc = APP_CONFIG_ERROR_FILE_OPEN;
	// 				}
	// 			}
	// 		}
	// 		/* state busy download */
	// 		else {
	// 			APP_DBG_RTC("downloadHdl() -> 3\n");
	// 			if (signal == "Start") {
	// 				rc = APP_CONFIG_ERROR_BUSY;
	// 			}
	// 			else if (signal == "StartTransferFile") {
	// 				APP_DBG_RTC("[DOWNLOAD][SIG] StartTransferFile\n");
	// 				cl->removeTimeoutDownload();	// remove timeout packet
	// 				if (cl->arrFilesDownload.size() > 0) {
	// 					fileDownloadInfo_t &file = cl->arrFilesDownload.back();
	// 					fileInfo_t fileInfo{0, ""};
	// 					fileInfo.bin_len  = sizeOfFile(file.path.c_str());
	// 					fileInfo.checksum = get_md5_file(file.path.data());
	// 					if (fileInfo.bin_len > 0) {
	// 						content["FileName"] = getFileName(file.path);
	// 						content["FileType"] = file.type;
	// 						content["Size"]		= fileInfo.bin_len;
	// 						content["Checksum"] = fileInfo.checksum;
	// 						cl->setCurrentFileTransfer({file.type, file.path, fileInfo.bin_len});
	// 						APP_DBG_RTC("tran file: %s\n", file.path.c_str());
	// 						/* start timeout wait response */
	// 						cl->startTimeoutDownload();
	// 						rc = APP_CONFIG_SUCCESS;
	// 						cl->setCursorFile(0);
	// 					}
	// 					else {
	// 						APP_DBG_RTC("error file\n");
	// 						task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_DATACHANNEL_DOWNLOAD_RELEASE_REQ, (uint8_t *)qrId.c_str(), qrId.length() + 1);
	// 					}

	// 					cl->arrFilesDownload.pop_back();
	// 					APP_DBG_RTC("remain: %d\n", (int)cl->arrFilesDownload.size());
	// 				}
	// 				else {
	// 					APP_DBG_RTC("error file\n");
	// 					task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_DATACHANNEL_DOWNLOAD_RELEASE_REQ, (uint8_t *)qrId.c_str(), qrId.length() + 1);
	// 				}
	// 			}
	// 			else if (signal == "TransferPacket") {
	// 				APP_DBG_RTC("[DOWNLOAD][SIG] TransferPacket\n");
	// 				cl->removeTimeoutDownload();	// remove timeout packet
	// 				/* packet size: 16384(Byte) */
	// 				uint8_t data_temp[PACKET_TRANSFER_SIZE];
	// 				uint32_t packetLen, remain, binIdx;
	// 				fileDownloadInfo_t fileCur = cl->getCurrentFileTransfer();
	// 				binIdx					   = cl->getCursorFile();
	// 				remain					   = fileCur.size - binIdx;
	// 				if (remain <= PACKET_TRANSFER_SIZE) {
	// 					packetLen = remain;
	// 				}
	// 				else {
	// 					packetLen = PACKET_TRANSFER_SIZE;
	// 				}
	// 				firmware_read(data_temp, binIdx, packetLen, fileCur.path.c_str());
	// 				binIdx += packetLen;
	// 				cl->setCursorFile(binIdx);
	// 				if (binIdx <= fileCur.size && packetLen > 0) {
	// 					auto dc = cl->dataChannel.value();
	// 					auto *b = reinterpret_cast<const byte *>(data_temp);
	// 					if (dc->isOpen()) {
	// 						APP_DBG_RTC("send packect index: %d, size: %d, remain: %d, dc buffer available size: %d\n", binIdx + 1, packetLen, remain, dc->availableAmount());
	// 						dc->send(binary(b, b + packetLen));
	// 						// APP_DBG_RTC("send ret: %d, total buff added: %d\n", ret, dc->bufferedAmount());
	// 					}
	// 				}
	// 				else {
	// 					APP_DBG_RTC("finish transfer packet\n");
	// 				}
	// 				content["Percent"]	= binIdx * 100 / fileCur.size;
	// 				content["FileType"] = fileCur.type;
	// 				/* start timeout wait response */
	// 				cl->startTimeoutDownload();
	// 			}
	// 			else if (signal == "Stop") {
	// 				APP_DBG_RTC("[DOWNLOAD][SIG] Stop\n");
	// 				cl->removeTimeoutDownload();	// remove timeout packet
	// 				rc = APP_CONFIG_SUCCESS;
	// 				task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_DATACHANNEL_DOWNLOAD_RELEASE_REQ, (uint8_t *)qrId.c_str(), qrId.length() + 1);
	// 			}
	// 		}
	// 	}
	// 	catch (const exception &error) {
	// 		APP_DBG_RTC("%s\n", error.what());
	// 		rc = APP_CONFIG_ERROR_ANOTHER;
	// 	}
	// }
	// else {
	// 	APP_DBG_RTC("clientId: %s is erase\n", qrId.c_str());
	// 	respFlag = false;
	// 	rc		 = APP_CONFIG_ERROR_ANOTHER;
	// }

	APP_DBG_RTC("downloadHdl() -> 7\n");
	return rc;
}

int rtcForceFormatExtDisks(json &content, bool &respFlag) {
	bool enable;

	try {
		enable = content["Format"].get<bool>();
		if (enable) {
			timer_set(GW_TASK_RECORD_ID, GW_RECORD_FORMAT_CARD_REQ, 500, TIMER_ONE_SHOT);
		}
	}
	catch (...) {
		return APP_CONFIG_ERROR_DATA_INVALID;
	}

	content["Format"] = enable;

	return APP_CONFIG_SUCCESS;
}

int recordCalendarHdl(json &content, bool &respFlag) {
	// int typeMask = 0;
	// int y = 0, m = 0, d = 0;

	// try {
	// 	std::string dtStr = content["DateTime"].get<string>();
	// 	auto arrTypes	  = content["Type"];
	// 	for (auto it : arrTypes) {
	// 		if (it == QRY_TYPE_ALL) {
	// 			typeMask |= (int)(RECORD_TYPE_REG | RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 		}
	// 		else if (it == QRY_TYPE_REG) {
	// 			typeMask |= (int)(RECORD_TYPE_REG);
	// 		}
	// 		else if (it == QRY_TYPE_MDT) {
	// 			typeMask |= (int)(RECORD_TYPE_MDT);
	// 		}
	// 		else if (it == QRY_TYPE_HMD) {
	// 			typeMask |= (int)(RECORD_TYPE_HMD);
	// 		}
	// 	}

	// 	std::cout << dtStr << std::endl;
	// 	if (sscanf(dtStr.c_str(), "%d.%2d.%2d", &y, &m, &d) != 3) {
	// 		return APP_CONFIG_ERROR_DATA_INVALID;
	// 	};
	// }
	// catch (...) {
	// 	return APP_CONFIG_ERROR_DATA_INVALID;
	// }

	// content["DateTimeMask"] = sdCard.u32MaskCalenderRecorder((RECORDER_TYPE)typeMask, y, m);

	return APP_CONFIG_SUCCESS;
}

int countRecordCalendarHdl(json &content, bool &respFlag) {
	// int typeMask = 0;
	// int begY = 0, begM = 0, begD = 0;
	// int endY = 0, endM = 0, endD = 0;

	// try {
	// 	nlohmann::json dataJs;
	// 	std::string begTimeStr = content["StartTime"].get<string>();
	// 	std::string endTimeStr = content["EndTime"].get<string>();
	// 	auto arrTypes		   = content["Type"];
	// 	for (auto it : arrTypes) {
	// 		if (it == QRY_TYPE_ALL) {
	// 			typeMask |= (RECORD_TYPE_REG | RECORD_TYPE_MDT | RECORD_TYPE_HMD);
	// 		}
	// 		else if (it == QRY_TYPE_REG) {
	// 			typeMask |= RECORD_TYPE_REG;
	// 		}
	// 		else if (it == QRY_TYPE_MDT) {
	// 			typeMask |= RECORD_TYPE_MDT;
	// 		}
	// 		else if (it == QRY_TYPE_HMD) {
	// 			typeMask |= RECORD_TYPE_HMD;
	// 		}
	// 	}

	// 	if (sscanf(begTimeStr.c_str(), "%d.%2d.%2d", &begY, &begM, &begD) != 3) {
	// 		return APP_CONFIG_ERROR_DATA_INVALID;
	// 	};
	// 	if (sscanf(endTimeStr.c_str(), "%d.%2d.%2d", &endY, &endM, &endD) != 3) {
	// 		return APP_CONFIG_ERROR_DATA_INVALID;
	// 	};
	// 	if (begY != endY || begM != endM || begD > endD) {
	// 		return APP_CONFIG_ERROR_DATA_INVALID;
	// 	}

	// 	auto rcStatistic = sdCard.getStatistic();

	// 	for (auto it : rcStatistic) {
	// 		nlohmann::json subJs;
	// 		int y = 0, m = 0, d = 0;
	// 		std::string dtStr		  = it.first;
	// 		RecorderCounter rcCounter = it.second;
	// 		sscanf(dtStr.c_str(), "%d.%2d.%2d", &y, &m, &d);
	// 		if (y != begY || m != begM) {
	// 			continue;
	// 		}

	// 		if (d >= begD && d <= endD) {
	// 			nlohmann::json ojbJs;

	// 			subJs["DateTime"] = dtStr;

	// 			if (typeMask & RECORD_TYPE_REG) {
	// 				ojbJs["Type"]  = QRY_TYPE_REG;
	// 				ojbJs["Count"] = rcCounter.regCounts;
	// 				subJs["Types"].push_back(ojbJs);
	// 			}
	// 			if (typeMask & RECORD_TYPE_MDT) {
	// 				ojbJs["Type"]  = QRY_TYPE_MDT;
	// 				ojbJs["Count"] = rcCounter.mdtCounts;
	// 				subJs["Types"].push_back(ojbJs);
	// 			}
	// 			if (typeMask & RECORD_TYPE_HMD) {
	// 				ojbJs["Type"]  = QRY_TYPE_HMD;
	// 				ojbJs["Count"] = rcCounter.hmdCounts;
	// 				subJs["Types"].push_back(ojbJs);
	// 			}
	// 			content["Data"].push_back(subJs);
	// 		}
	// 	}
	// }
	// catch (...) {
	// 	return APP_CONFIG_ERROR_DATA_INVALID;
	// }

	return APP_CONFIG_SUCCESS;
}