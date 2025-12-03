#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "app.h"
#include "app_data.h"
#include "app_config.h"
#include "app_dbg.h"
#include "parser_json.h"
#include "utils.h"
#include "wifi.h"
#include "SdCard.h"

#include "fca_parameter.hpp"
#include "fca_motion.hpp"

using namespace std;

bool fca_setConfigJsonRes(json &json_in, const char *mess_type, int ret) {
	try {
		int status = ret == APP_CONFIG_SUCCESS ? FCA_MQTT_RESPONE_SUCCESS : FCA_MQTT_RESPONE_FAILED;
		string des = status == FCA_MQTT_RESPONE_SUCCESS ? "Success" : "Fail";
		json_in	   = {
			   {"Method",	  "SET"							   },
			   {"MessageType", mess_type							},
			   {"Serial",	  string(fca_getSerialInfo())		 },
			   {"Data",		json::object()					  },
			   {"Result",	  {{"Ret", status}, {"Message", des}}},
			   {"Timestamp",	 time(nullptr)					  },
		   };
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_getConfigJsonRes(json &json_in, json &json_data, const char *mess_type, int ret) {
	try {
		int status = ret == APP_CONFIG_SUCCESS ? FCA_MQTT_RESPONE_SUCCESS : FCA_MQTT_RESPONE_FAILED;
		string des = status == FCA_MQTT_RESPONE_SUCCESS ? "Success" : "Fail";
		json_in	   = {
			   {"Method",	  "GET"							   },
			   {"MessageType", mess_type							},
			   {"Serial",	  string(fca_getSerialInfo())		 },
			   {"Data",		json_data							 },
			   {"Result",	  {{"Ret", status}, {"Message", des}}},
			   {"Timestamp",	 time(nullptr)					  },
		   };
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonAlarmUploadFile(json &json_in, json &json_data) {
	try {
		json_in = {
			{"Alarm",  MESSAGE_TYPE_UPLOAD_FILE	  },
			{"Data",	 json_data				  },
			{"Serial", string(fca_getSerialInfo())},
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonAlarmMotion(json &json_in, int channel, const char *startTime) {
	try {
		json_in = {
			{"Alarm",	  MESSAGE_TYPE_MOTION_DETECT },
			{"Channel",	channel					   },
			{"DeviceID",	 string(fca_getSerialInfo())},
			{"StartTime", string(startTime)		   },
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonAlarmHuman(json &json_in, int channel, const char *startTime) {
	try {
		json_in = {
			{"Alarm",	  MESSAGE_TYPE_HUMAN_DETECT  },
			{"Channel",	channel					   },
			{"DeviceID",	 string(fca_getSerialInfo())},
			{"StartTime", string(startTime)		   },
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetNetMQTT(json &json_in, FCA_MQTT_CONN_S *param) {
	try {
		json_in = {
			{"Password",	 string(param->password)	},
			{"Username",	 string(param->username)	},
			{"ClientID",	 string(param->clientID)	},
			{"Host",		 string(param->host)		},
			{"Port",		 param->port				},
			{"KeepAlive",  param->keepAlive		  },
			{"QOS",		param->QOS				  },
			{"Retain",	   (bool)param->retain	  },
			{"TLSEnable",  (bool)param->bTLSEnable	 },
			{"TLSVersion", string(param->TLSVersion)},
			{"Protocol",	 string(param->protocol)	},
			{"Enable",	   (bool)param->bEnable	   },
		};
		APP_DBG("json set: %s\n", json_in.dump().data());
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetNetMQTT(json &json_in, FCA_MQTT_CONN_S *param) {
	try {
		string serial = fca_getSerialInfo();
		char clientId[80];
		snprintf(clientId, sizeof(clientId), "%s-%s", "ipc", serial.c_str());
		string str = json_in["Password"].get<string>();
		str == "" ? safe_strcpy(param->password, serial.c_str(), sizeof(param->password)) : safe_strcpy(param->password, str.data(), sizeof(param->password));
		str = json_in["Username"].get<string>();
		str == "" ? safe_strcpy(param->username, serial.c_str(), sizeof(param->username)) : safe_strcpy(param->username, str.data(), sizeof(param->username));
		str = json_in["ClientID"].get<string>();
		str == "" ? safe_strcpy(param->clientID, clientId, sizeof(param->clientID)) : safe_strcpy(param->clientID, str.data(), sizeof(param->clientID));
		safe_strcpy(param->host, json_in["Host"].get<string>().data(), sizeof(param->host));
		param->port		  = json_in["Port"].get<int>();
		param->bTLSEnable = json_in["TLSEnable"].get<bool>();
		safe_strcpy(param->TLSVersion, json_in["TLSVersion"].get<string>().data(), sizeof(param->TLSVersion));
		safe_strcpy(param->protocol, json_in["Protocol"].get<string>().data(), sizeof(param->protocol));
		param->bEnable	 = json_in["Enable"].get<bool>();
		param->keepAlive = json_in["KeepAlive"].get<int>();
		param->QOS		 = json_in["QOS"].get<int>();
		param->retain	 = json_in["Retain"].get<int>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetRtcServers(json &json_in, rtcServersConfig_t *rtcSvCfg) {
	try {
		rtcSvCfg->clear();
		json arrJs = json_in["Connections"];
		for (auto &server : arrJs) {
			string type = server["Type"].get<string>();
			if (type == "stun") {
				rtcSvCfg->arrStunServerUrl = server["StunUrl"].get<vector<string>>();
			}
			else if (type == "turn") {
				rtcSvCfg->arrTurnServerUrl = server["TurnUrl"].get<vector<string>>();
			}
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetMotion(json &json_in, fca_motionSetting_t *param) {
	try {
		json_in = {
			{"Sensitive",			  param->sensitive	  },
			{"Interval",			 param->interval		},
			{"Duration",			 param->duration		},
			{"Enable",			   param->enable		},
			{"VideoAttribute",	   param->videoAtt	  },
			{"PictureAttribute",	 param->pictureAtt	  },
			{"HumanDetectAttribute", param->humanDetAtt   },
			{"HumanFocus",		   param->humanFocus	},
			{"HumanDraw",			  param->humanDraw	  },
			{"GuardZoneAttribute",   param->guardZoneAtt  },
			{"GuardZone",			  param->guardZone	  }
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

int fca_jsonGetMotion(json &json_in, fca_motionSetting_t *param) {
	try {
		param->sensitive   = json_in["Sensitive"].get<int>();
		param->interval	   = json_in["Interval"].get<int>();
		param->duration	   = json_in["Duration"].get<int>();
		param->enable	   = json_in["Enable"].get<bool>();
		param->videoAtt	   = json_in["VideoAttribute"].get<bool>();
		param->pictureAtt  = json_in["PictureAttribute"].get<bool>();
		param->humanDetAtt = json_in["HumanDetectAttribute"].get<bool>();

		param->humanFocus = param->humanDraw = false;
		if (json_in.contains("HumanFocus")) {
			param->humanFocus = json_in["HumanFocus"].get<bool>();
		}
		if (json_in.contains("HumanDraw")) {
			param->humanDraw = json_in["HumanDraw"].get<bool>();
		}

		param->guardZoneAtt = json_in["GuardZoneAttribute"].get<bool>();
		json guardZoneArray = json_in["GuardZone"].get<json::array_t>();
		param->lenGuardZone = guardZoneArray.size();
		for (size_t i = 0; i < guardZoneArray.size(); ++i) {
			param->guardZone[i] = guardZoneArray[i].get<int>();
		}
		// param->humanSensitive = MEDIUM_LEVEL;	 // default sensitivity
		// if (json_in.contains("HumanSensitive")) {
		// 	param->humanSensitive = json_in["HumanSensitive"].get<int>();
		// }
		
		// param->svpThrld	  = SVP_THRESHOLD_DEFAULT;		 // default threshold
		// param->svpMdThrld = SVP_MD_THRESHOLD_DEFAULT;	 // default threshold
		// if (json_in.contains("SVPThreshold")) {
		// 	param->svpThrld = json_in["SVPThreshold"].get<int>();
		// }
		// if (json_in.contains("SVPMotionThreshold")) {
		// 	param->svpMdThrld = json_in["SVPMotionThreshold"].get<int>();
		// }
		return APP_CONFIG_SUCCESS;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return APP_CONFIG_ERROR_DATA_INVALID;
}

bool fca_jsonSetEncode(json &json_in, FCA_ENCODE_S *param) {
	// try {
	// 	json_in = {
	// 		{"MainFormat",
	// 		 {{"AudioEnable", param->mainStream.bAudioEnable},
	// 		  {"Video",
	// 		   {{"Compression", param->mainStream.format.compression == FCA_CAPTURE_COMP_H264	 ? "H.264"
	// 							: param->mainStream.format.compression == FCA_CAPTURE_COMP_H265 ? "H.265"
	// 																						 : ""},
	// 			{"Resolution", param->mainStream.format.resolution == FCA_VIDEO_DIMENSION_2K		? "2K"
	// 						   : param->mainStream.format.resolution == FCA_VIDEO_DIMENSION_1080P ? "1080P"
	// 						   : param->mainStream.format.resolution == FCA_VIDEO_DIMENSION_720P	? "720P"
	// 						   : param->mainStream.format.resolution == FCA_VIDEO_DIMENSION_VGA	? "480P"
	// 																						: ""},
	// 			{"BitRate", param->mainStream.format.bitRate},
	// 			{"VirtualGOP", param->mainStream.format.virtualGOP},
	// 			{"BitRateControl", param->mainStream.format.bitRateControl == FCA_CAPTURE_BRC_CBR	 ? "CBR"
	// 							   : param->mainStream.format.bitRateControl == FCA_CAPTURE_BRC_VBR ? "VBR"
	// 							   : param->mainStream.format.bitRateControl == FCA_CAPTURE_BRC_MBR ? "MBR"
	// 																							 : ""},
	// 			{"FPS", param->mainStream.format.FPS},
	// 			{"Quality", param->mainStream.format.quality},
	// 			{"GOP", param->mainStream.format.GOP}}},
	// 		  {"VideoEnable", param->mainStream.bVideoEnable}} },
	// 		{"ExtraFormat",
	// 		 {{"AudioEnable", param->minorStream.bAudioEnable},
	// 		  {"Video",
	// 		   {{"Compression", param->minorStream.format.compression == FCA_CAPTURE_COMP_H264	  ? "H.264"
	// 							: param->minorStream.format.compression == FCA_CAPTURE_COMP_H265 ? "H.265"
	// 																						  : ""},
	// 			{"Resolution", param->minorStream.format.resolution == FCA_VIDEO_DIMENSION_2K		 ? "2K"
	// 						   : param->minorStream.format.resolution == FCA_VIDEO_DIMENSION_1080P ? "1080P"
	// 						   : param->minorStream.format.resolution == FCA_VIDEO_DIMENSION_720P	 ? "720P"
	// 						   : param->minorStream.format.resolution == FCA_VIDEO_DIMENSION_VGA	 ? "480P"
	// 																						 : ""},
	// 			{"BitRate", param->minorStream.format.bitRate},
	// 			{"VirtualGOP", param->minorStream.format.virtualGOP},
	// 			{"BitRateControl", param->minorStream.format.bitRateControl == FCA_CAPTURE_BRC_CBR	  ? "CBR"
	// 							   : param->minorStream.format.bitRateControl == FCA_CAPTURE_BRC_VBR ? "VBR"
	// 							   : param->minorStream.format.bitRateControl == FCA_CAPTURE_BRC_MBR ? "MBR"
	// 																							  : ""},
	// 			{"FPS", param->minorStream.format.FPS},
	// 			{"Quality", param->minorStream.format.quality},
	// 			{"GOP", param->minorStream.format.GOP}}},
	// 		  {"VideoEnable", param->minorStream.bVideoEnable}}}
	// 	 };
	// 	return true;
	// }
	// catch (const exception &e) {
	// 	APP_DBG("json error: %s\n", e.what());
	// }
	return false;
}

bool fca_jsonGetEncode(json &json_in, FCA_ENCODE_S *param) {
	// try {
	// 	// Main Format
	// 	json mainFormat				= json_in.at("MainFormat");
	// 	json mainVideo				= json_in.at("MainFormat").at("Video");
	// 	json extraFormat			= json_in.at("ExtraFormat");
	// 	json extraVideo				= json_in.at("ExtraFormat").at("Video");
	// 	param->mainStream.bAudioEnable = json_in["MainFormat"]["AudioEnable"].get<bool>();
	// 	param->mainStream.bVideoEnable = json_in["MainFormat"]["VideoEnable"].get<bool>();
	// 	string str					= json_in["MainFormat"]["Video"]["Compression"].get<string>();
	// 	 str == "H.264"	  ? param->mainStream.format.compression	= FCA_CAPTURE_COMP_H264
	// 					 : str == "H.265" ? param->mainStream.format.compression = FCA_CAPTURE_COMP_H265
	// 									  : param->mainStream.format.compression = FCA_CAPTURE_COMP_NONE;
	// 	str = json_in["MainFormat"]["Video"]["Resolution"].get<string>().data();
	// 	str == "2K"		 ? param->mainStream.format.resolution		 = FCA_VIDEO_DIMENSION_2K
	// 	: str == "1080P" ? param->mainStream.format.resolution = FCA_VIDEO_DIMENSION_1080P
	// 	: str == "720P"	 ? param->mainStream.format.resolution	 = FCA_VIDEO_DIMENSION_720P
	// 	: str == "480P"	 ? param->mainStream.format.resolution	 = FCA_VIDEO_DIMENSION_VGA
	// 					 : param->mainStream.format.resolution	 = FCA_VIDEO_DIMENSION_UNKNOWN;
	// 	param->mainStream.format.bitRate	 = json_in["MainFormat"]["Video"]["BitRate"].get<int>();
	// 	param->mainStream.format.virtualGOP = json_in["MainFormat"]["Video"]["VirtualGOP"].get<int>();
	// 	str								 = json_in["MainFormat"]["Video"]["BitRateControl"].get<string>().data();
	// 	 str == "CBR"	? param->mainStream.format.bitRateControl	 = FCA_CAPTURE_BRC_CBR
	// 								 : str == "VBR" ? param->mainStream.format.bitRateControl = FCA_CAPTURE_BRC_VBR
	// 								 : str == "MBR" ? param->mainStream.format.bitRateControl = FCA_CAPTURE_BRC_MBR
	// 												: param->mainStream.format.bitRateControl = FCA_CAPTURE_BRC_NR;
	// 	param->mainStream.format.FPS	  = json_in["MainFormat"]["Video"]["FPS"].get<int>();
	// 	param->mainStream.format.quality = json_in["MainFormat"]["Video"]["Quality"].get<int>();
	// 	param->mainStream.format.GOP	  = json_in["MainFormat"]["Video"]["GOP"].get<int>();
	// 	// Extra Format
	// 	param->minorStream.bAudioEnable = json_in["ExtraFormat"]["AudioEnable"].get<bool>();
	// 	param->minorStream.bVideoEnable = json_in["ExtraFormat"]["VideoEnable"].get<bool>();
	// 	str							 = json_in["ExtraFormat"]["Video"]["Compression"].get<string>();
	// 	 str == "H.264"	  ? param->minorStream.format.compression	 = FCA_CAPTURE_COMP_H264
	// 							 : str == "H.265" ? param->minorStream.format.compression = FCA_CAPTURE_COMP_H265
	// 											  : param->minorStream.format.compression = FCA_CAPTURE_COMP_NONE;
	// 	str = json_in["ExtraFormat"]["Video"]["Resolution"].get<string>().data();
	// 	str == "2K"		 ? param->minorStream.format.resolution	  = FCA_VIDEO_DIMENSION_2K
	// 	: str == "1080P" ? param->minorStream.format.resolution = FCA_VIDEO_DIMENSION_1080P
	// 	: str == "720P"	 ? param->minorStream.format.resolution  = FCA_VIDEO_DIMENSION_720P
	// 	: str == "480P"	 ? param->minorStream.format.resolution  = FCA_VIDEO_DIMENSION_VGA
	// 					 : param->minorStream.format.resolution  = FCA_VIDEO_DIMENSION_UNKNOWN;
	// 	param->minorStream.format.bitRate	  = json_in["ExtraFormat"]["Video"]["BitRate"].get<int>();
	// 	param->minorStream.format.virtualGOP = json_in["ExtraFormat"]["Video"]["VirtualGOP"].get<int>();
	// 	str								  = json_in["ExtraFormat"]["Video"]["BitRateControl"].get<string>().data();
	// 	  str == "CBR"	 ? param->minorStream.format.bitRateControl   = FCA_CAPTURE_BRC_CBR
	// 								  : str == "VBR" ? param->minorStream.format.bitRateControl = FCA_CAPTURE_BRC_VBR
	// 								  : str == "MBR" ? param->minorStream.format.bitRateControl = FCA_CAPTURE_BRC_MBR
	// 												 : param->minorStream.format.bitRateControl = FCA_CAPTURE_BRC_NR;
	// 	param->minorStream.format.FPS	   = json_in["ExtraFormat"]["Video"]["FPS"].get<int>();
	// 	param->minorStream.format.quality = json_in["ExtraFormat"]["Video"]["Quality"].get<int>();
	// 	param->minorStream.format.GOP	   = json_in["ExtraFormat"]["Video"]["GOP"].get<int>();
	// 	return true;
	// }
	// catch (const exception &e) {
	// 	APP_DBG("json error: %s\n", e.what());
	// }
	return false;
}

bool fca_jsonSetParam(json &json_in, fca_cameraParam_t *param) {
	try {
		json_in = {
			{"PictureFlip",			param->pictureFlip					  },
			{"PictureMirror",		  param->pictureMirror				  },
			{"AntiFlickerMode",		param->antiFlickerMode				  },
			{"Brightness",			   param->brightness					},
			{"Contrast",				 param->contrast						},
			{"Saturation",			   param->saturation					},
			{"Sharpness",			  param->sharpness					  },
			{"IrBrightness",			 param->irBrightness					},
			{"IrContrast",			   param->irContrast					},
			{"IrSaturation",			 param->irSaturation					},
			{"IrSharpness",			param->irSharpness					  },
			{"NightVisionMode",		param->dnParam.nightVisionMode		  },
			{"LightingMode",			 param->dnParam.lightingMode			},
			{"LightingEnable",		   param->dnParam.lightingEnable		},
			{"Day2NightThreshold",	   param->dnParam.day2NightThreshold	},
			{"Night2DayThreshold",	   param->dnParam.night2DayThreshold	},
			{"AwbNightDisMax",		   param->dnParam.awbNightDisMax		},
			{"DayCheckFrameNum",		 param->dnParam.dayCheckFrameNum		},
			{"NightCheckFrameNum",	   param->dnParam.nightCheckFrameNum	},
			{"ExpStableCheckFrameNum", param->dnParam.expStableCheckFrameNum},
			{"StableWaitTimeoutMs",	param->dnParam.stableWaitTimeoutMs	  },
			{"GainR",				  param->dnParam.gainR				  },
			{"OffsetR",				param->dnParam.offsetR				  },
			{"GainB",				  param->dnParam.gainB				  },
			{"OffsetB",				param->dnParam.offsetB				  },
			{"GainRb",				   param->dnParam.gainRb				},
			{"OffsetRb",				 param->dnParam.offsetRb				},
			{"BlkCheckFlag",			 param->dnParam.blkCheckFlag			},
			{"LowLumiTh",			  param->dnParam.lowLumiTh			  },
			{"HighLumiTh",			   param->dnParam.highLumiTh			},
			{"NonInfraredRatioTh",	   param->dnParam.nonInfraredRatioTh	},
			{"HighlightLumiTh",		param->dnParam.highlightLumiTh		  },
			{"HighlightBlkCntMax",	   param->dnParam.highlightBlkCntMax	},
			{"LockCntTh",			  param->dnParam.lockCntTh			  },
			{"LockTime",				 param->dnParam.lockTime				}
		 };
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

int fca_jsonGetParam(json &json_in, fca_cameraParam_t *param) {
	try {
		param->pictureFlip				  = json_in["PictureFlip"].get<bool>();
		param->pictureMirror			  = json_in["PictureMirror"].get<bool>();
		param->antiFlickerMode			  = json_in["AntiFlickerMode"].get<int>();
		param->brightness				  = json_in["Brightness"].get<int>();
		param->contrast					  = json_in["Contrast"].get<int>();
		param->saturation				  = json_in["Saturation"].get<int>();
		param->sharpness				  = json_in["Sharpness"].get<int>();
		param->irBrightness				  = json_in["IrBrightness"].get<int>();
		param->irContrast				  = json_in["IrContrast"].get<int>();
		param->irSaturation				  = json_in["IrSaturation"].get<int>();
		param->irSharpness				  = json_in["IrSharpness"].get<int>();
		param->dnParam.nightVisionMode	  = json_in["NightVisionMode"].get<int>();
		param->dnParam.lightingMode		  = json_in["LightingMode"].get<int>();
		param->dnParam.lightingEnable	  = json_in["LightingEnable"].get<bool>();
		param->dnParam.day2NightThreshold = json_in["Day2NightThreshold"].get<unsigned int>();
		param->dnParam.night2DayThreshold = json_in["Night2DayThreshold"].get<unsigned int>();

		if (!json_in.contains("LockTime")) {
			return APP_CONFIG_ERROR_DATA_MISSING;
		}
		param->dnParam.awbNightDisMax		  = json_in["AwbNightDisMax"].get<unsigned int>();
		param->dnParam.dayCheckFrameNum		  = json_in["DayCheckFrameNum"].get<unsigned int>();
		param->dnParam.nightCheckFrameNum	  = json_in["NightCheckFrameNum"].get<unsigned int>();
		param->dnParam.expStableCheckFrameNum = json_in["ExpStableCheckFrameNum"].get<unsigned int>();
		param->dnParam.stableWaitTimeoutMs	  = json_in["StableWaitTimeoutMs"].get<unsigned int>();
		param->dnParam.gainR				  = json_in["GainR"].get<unsigned int>();
		param->dnParam.offsetR				  = json_in["OffsetR"].get<int>();
		param->dnParam.gainB				  = json_in["GainB"].get<unsigned int>();
		param->dnParam.offsetB				  = json_in["OffsetB"].get<int>();
		param->dnParam.gainRb				  = json_in["GainRb"].get<unsigned int>();
		param->dnParam.offsetRb				  = json_in["OffsetRb"].get<int>();
		param->dnParam.blkCheckFlag			  = json_in["BlkCheckFlag"].get<unsigned int>();
		param->dnParam.lowLumiTh			  = json_in["LowLumiTh"].get<unsigned int>();
		param->dnParam.highLumiTh			  = json_in["HighLumiTh"].get<unsigned int>();
		param->dnParam.nonInfraredRatioTh	  = json_in["NonInfraredRatioTh"].get<unsigned int>();
		param->dnParam.highlightLumiTh		  = json_in["HighlightLumiTh"].get<unsigned int>();
		param->dnParam.highlightBlkCntMax	  = json_in["HighlightBlkCntMax"].get<unsigned int>();
		param->dnParam.lockCntTh			  = json_in["LockCntTh"].get<unsigned int>();
		param->dnParam.lockTime				  = json_in["LockTime"].get<unsigned int>();

		return APP_CONFIG_SUCCESS;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return APP_CONFIG_ERROR_DATA_INVALID;
}

bool fca_jsonGetParamWithType(json &json_in, fca_cameraParam_t *param, const string &type) {
	try {
		if (type == "NightVision") {
			param->dnParam.nightVisionMode		  = json_in["NightVisionMode"].get<int>();
			param->dnParam.lightingMode			  = json_in["LightingMode"].get<int>();
			param->dnParam.lightingEnable		  = json_in["LightingEnable"].get<bool>();
			param->dnParam.day2NightThreshold	  = json_in["Day2NightThreshold"].get<unsigned int>();
			param->dnParam.night2DayThreshold	  = json_in["Night2DayThreshold"].get<unsigned int>();
			param->dnParam.awbNightDisMax		  = json_in["AwbNightDisMax"].get<unsigned int>();
			param->dnParam.dayCheckFrameNum		  = json_in["DayCheckFrameNum"].get<unsigned int>();
			param->dnParam.nightCheckFrameNum	  = json_in["NightCheckFrameNum"].get<unsigned int>();
			param->dnParam.expStableCheckFrameNum = json_in["ExpStableCheckFrameNum"].get<unsigned int>();
			param->dnParam.stableWaitTimeoutMs	  = json_in["StableWaitTimeoutMs"].get<unsigned int>();
			param->dnParam.gainR				  = json_in["GainR"].get<unsigned int>();
			param->dnParam.offsetR				  = json_in["OffsetR"].get<int>();
			param->dnParam.gainB				  = json_in["GainB"].get<unsigned int>();
			param->dnParam.offsetB				  = json_in["OffsetB"].get<int>();
			param->dnParam.gainRb				  = json_in["GainRb"].get<unsigned int>();
			param->dnParam.offsetRb				  = json_in["OffsetRb"].get<int>();
			param->dnParam.blkCheckFlag			  = json_in["BlkCheckFlag"].get<unsigned int>();
			param->dnParam.lowLumiTh			  = json_in["LowLumiTh"].get<unsigned int>();
			param->dnParam.highLumiTh			  = json_in["HighLumiTh"].get<unsigned int>();
			param->dnParam.nonInfraredRatioTh	  = json_in["NonInfraredRatioTh"].get<unsigned int>();
			param->dnParam.highlightLumiTh		  = json_in["HighlightLumiTh"].get<unsigned int>();
			param->dnParam.highlightBlkCntMax	  = json_in["HighlightBlkCntMax"].get<unsigned int>();
			param->dnParam.lockCntTh			  = json_in["LockCntTh"].get<unsigned int>();
			param->dnParam.lockTime				  = json_in["LockTime"].get<unsigned int>();
		}
		else if (type == "FlipMirror") {
			param->pictureFlip	 = json_in["PictureFlip"].get<bool>();
			param->pictureMirror = json_in["PictureMirror"].get<bool>();
		}
		else if (type == "ImageSettings") {
			param->brightness	= json_in["Brightness"].get<int>();
			param->contrast		= json_in["Contrast"].get<int>();
			param->saturation	= json_in["Saturation"].get<int>();
			param->sharpness	= json_in["Sharpness"].get<int>();
			param->irBrightness = json_in["IrBrightness"].get<int>();
			param->irContrast	= json_in["IrContrast"].get<int>();
			param->irSaturation = json_in["IrSaturation"].get<int>();
			param->irSharpness	= json_in["IrSharpness"].get<int>();
		}
		else if (type == "AntiFlicker") {
			param->antiFlickerMode = json_in["AntiFlickerMode"].get<int>();
		}
		else if (type == "Lighting") {
			param->dnParam.lightingEnable = json_in["LightingEnable"].get<bool>();
		}
		else if (type == "Smartlight") {
			param->dnParam.lightingMode = json_in["LightingMode"].get<int>();
		}
		else if (type.empty()) {
			return fca_jsonGetParam(json_in, param) == APP_CONFIG_SUCCESS;
		}
		else {
			APP_DBG("unknow type get\n");
			return false;
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetEthernet(json &json_in, fca_netEth_t *param) {
	try {
		json_in = {
			{"Enable",	   param->enable						},
			{"GateWay",		string(param->detail.gateWay)	 },
			{"HostIP",	   string(param->detail.hostIP)	   },
			{"Submask",		string(param->detail.submask)	 },
			{"PreferredDNS", string(param->detail.preferredDns)},
			{"AlternateDNS", string(param->detail.alternateDns)},
			{"DHCP",		 param->dhcp						},
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetEthernet(json &json_in, fca_netEth_t *param) {
	try {
		param->enable = json_in["Enable"].get<bool>();
		safe_strcpy(param->detail.gateWay, json_in["GateWay"].get<string>().data(), sizeof(param->detail.gateWay));
		safe_strcpy(param->detail.hostIP, json_in["HostIP"].get<string>().data(), sizeof(param->detail.hostIP));
		safe_strcpy(param->detail.submask, json_in["Submask"].get<string>().data(), sizeof(param->detail.submask));
		safe_strcpy(param->detail.preferredDns, json_in["PreferredDNS"].get<string>().data(), sizeof(param->detail.preferredDns));
		safe_strcpy(param->detail.alternateDns, json_in["AlternateDNS"].get<string>().data(), sizeof(param->detail.alternateDns));
		param->dhcp = json_in["DHCP"].get<bool>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetWifi(json &json_in, FCA_NET_WIFI_S *param) {
	try {
		json_in = {
			{"Enable",	   param->enable						},
			{"SSID",		 string(param->ssid)				},
			{"Keys",		 string(param->keys)				},
			{"KeyType",		param->keyType					  },
			{"Auth",		 string(param->auth)				},
			{"Channel",		param->channel					  },
			{"EncrypType",   string(param->encrypType)			},
			{"GateWay",		string(param->detail.gateWay)	 },
			{"HostIP",	   string(param->detail.hostIP)	   },
			{"Submask",		string(param->detail.submask)	 },
			{"PreferredDNS", string(param->detail.preferredDns)},
			{"AlternateDNS", string(param->detail.alternateDns)},
			{"DHCP",		 param->dhcp						},
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetWifi(json &json_in, FCA_NET_WIFI_S *param) {
	try {
		param->enable = json_in["Enable"].get<bool>();
		safe_strcpy(param->ssid, json_in["SSID"].get<string>().data(), sizeof(param->ssid));
		safe_strcpy(param->keys, json_in["Keys"].get<string>().data(), sizeof(param->keys));
		param->keyType = json_in["KeyType"].get<int>();
		safe_strcpy(param->auth, json_in["Auth"].get<string>().data(), sizeof(param->auth));
		param->channel = json_in["Channel"].get<int>();
		safe_strcpy(param->encrypType, json_in["EncrypType"].get<string>().data(), sizeof(param->encrypType));
		safe_strcpy(param->detail.gateWay, json_in["GateWay"].get<string>().data(), sizeof(param->detail.gateWay));
		safe_strcpy(param->detail.hostIP, json_in["HostIP"].get<string>().data(), sizeof(param->detail.hostIP));
		safe_strcpy(param->detail.submask, json_in["Submask"].get<string>().data(), sizeof(param->detail.submask));
		if (json_in.contains("PreferredDNS")) {
			safe_strcpy(param->detail.preferredDns, json_in["PreferredDNS"].get<string>().data(), sizeof(param->detail.preferredDns));
		}
		if (json_in.contains("AlternateDNS")) {
			safe_strcpy(param->detail.alternateDns, json_in["AlternateDNS"].get<string>().data(), sizeof(param->detail.alternateDns));
		}
		param->dhcp = json_in["DHCP"].get<bool>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetRTMP(json &json_in, fca_rtmp_t *param) {
	try {
		json_in = {
			{"Host",		 string(param->host) },
			 {"Enable",		param->enable		 },
			{"Port",		 param->port			},
			{"Token",	  string(param->token)},
			 {"Node",		  string(param->node) },
			{"StreamType", param->streamType	},
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetRTMP(json &json_in, fca_rtmp_t *param) {
	try {
		safe_strcpy(param->host, json_in["Host"].get<string>().data(), sizeof(param->host));
		param->enable = json_in["Enable"].get<bool>();
		param->port	  = json_in["Port"].get<int>();
		safe_strcpy(param->token, json_in["Token"].get<string>().data(), sizeof(param->token));
		safe_strcpy(param->node, json_in["Node"].get<string>().data(), sizeof(param->node));
		param->streamType = json_in["StreamType"].get<int>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetWatermark(json &json_in, FCA_OSD_S *param) {
	try {
		json_in = {
			{"NameTitle",		  string(param->name)},
			{"TimeTitleAttribute", param->timeAtt	 },
			{"NameTitleAttribute", param->nameAtt	 },
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetWatermark(json &json_in, FCA_OSD_S *param) {
	// try {
	// 	safe_strcpy(param->name, json_in["NameTitle"].get<string>().data(), sizeof(param->name));
	// 	APP_DBG("param->name: %s\n", param->name);
	// 	param->timeAtt = json_in["TimeTitleAttribute"].get<bool>();
	// 	param->nameAtt = json_in["NameTitleAttribute"].get<bool>();
	// 	return true;
	// }
	// catch (const exception &e) {
	// 	APP_DBG("json error: %s\n", e.what());
	// }
	return false;
}

bool fca_jsonSetStorage(json &json_in, int capacity, int usage, int free) {
	(void)free;
	try {
		json_in = {
			{"Capacity", capacity},
			{"Usage",	  usage   },
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetStorage(json &json_in, fca_storage_t *param) {
	try {
		param->bIsFornat = json_in["IsFormat"].get<bool>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetS3(json &json_in, fca_s3_t *param) {
	try {
		json_in = {
			{"AccessKey", param->accessKey},
			{"EndPoint",	 param->endPoint },
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetS3(json &json_in, fca_s3_t *param) {
	try {
		param->accessKey = json_in["AccessKey"].get<string>();
		param->endPoint	 = json_in["EndPoint"].get<string>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetAccount(json &json_in, fca_account_t *param) {
	try {
		json_in = {
			{"Name",		 string(param->username)},
			   {"Password",	string(param->password)},
			  {"Reserved",  param->reserved		  },
			 {"Shareable", param->shareable	   },
			  {"Name",	   param->authoList	   },
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetAccount(json &json_in, fca_account_t *param) {
	try {
		safe_strcpy(param->username, json_in["Name"].get<string>().data(), sizeof(param->username));
		safe_strcpy(param->password, json_in["Password"].get<string>().data(), sizeof(param->password));
		param->reserved		= json_in["Reserved"].get<bool>();
		param->shareable	= json_in["Shareable"].get<bool>();
		json guardZoneArray = json_in["AuthorityList"].get<json::array_t>();
		for (size_t i = 0; i < guardZoneArray.size(); ++i) {
			safe_strcpy(param->authoList[i], guardZoneArray[i].get<string>().data(), sizeof(param->authoList[i]));
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetReset(json &json_in, fca_resetConfig_t *param) {
	try {
		param->bWatermark = json_in["WatermarkSetting"].get<bool>();
		param->bMotion	  = json_in["MotionSetting"].get<bool>();
		param->bMQTT	  = json_in["MQTTSetting"].get<bool>();
		param->bRTMP	  = json_in["RTMPSetting"].get<bool>();
		param->bWifi	  = json_in["WifiSetting"].get<bool>();
		param->bParam	  = json_in["ParamSetting"].get<bool>();
		param->bEncode	  = json_in["EncodeSetting"].get<bool>();
		param->bS3		  = json_in["S3Setting"].get<bool>();
		param->bLedIndi	  = json_in["LedIndicatorSetting"].get<bool>();
		param->bRainbow	  = json_in["RainbowSetting"].get<bool>();
		param->bMotor = param->bAlarm = param->bRtc = param->bLogin = param->bSystem = param->bEthernet = param->bP2p = param->bBucket = true;

		if (json_in.contains("BucketSetting")) {
			param->bBucket = json_in["BucketSetting"].get<bool>();
		}
		if (json_in.contains("MotorSetting")) {
			param->bMotor = json_in["MotorSetting"].get<bool>();
		}
		if (json_in.contains("AlarmSetting")) {
			param->bAlarm = json_in["AlarmSetting"].get<bool>();
		}
		if (json_in.contains("RtcSetting")) {
			param->bRtc = json_in["RtcSetting"].get<bool>();
		}
		if (json_in.contains("LoginSetting")) {
			param->bLogin = json_in["LoginSetting"].get<bool>();
		}
		if (json_in.contains("SystemSetting")) {
			param->bSystem = json_in["SystemSetting"].get<bool>();
		}
		if (json_in.contains("EthernetSetting")) {
			param->bEthernet = json_in["EthernetSetting"].get<bool>();
		}
		if (json_in.contains("P2PSetting")) {
			param->bP2p = json_in["P2PSetting"].get<bool>();
		}
		if (json_in.contains("OnvifSetting")) {
			param->bOnvif = json_in["OnvifSetting"].get<bool>();
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetNetInfo(json &json_in, fca_netIPAddress_t *lan, fca_netIPAddress_t *wifi) {
	try {
		json_in = {
			#if SUPPORT_ETH
			{"Eth",		{{"MAC", lan->mac}, {"HostIP", lan->hostIP}}	},
			#endif
			{"Wlan",	 {{"MAC", wifi->mac}, {"HostIP", wifi->hostIP}}},
			{"PublicIP", camIpPublic									}
		 };
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetSysInfo(json &json_in, fca_sysInfo_t *sysInfo) {
	try {
		json_in = {
			{"BuildTime",		  string(sysInfo->buildTime)	},
			   {"SoftwareVersion", string(sysInfo->softVersion)}
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}
bool fca_jsonGetSysInfo(json &json_in, fca_sysInfo_t *sysInfo) {
	try {
		safe_strcpy(sysInfo->buildTime, json_in["SoftBuildTime"].get<string>().data(), sizeof(sysInfo->buildTime));
		safe_strcpy(sysInfo->softVersion, json_in["SoftWareVersion"].get<string>().data(), sizeof(sysInfo->softVersion));
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetNetAP(json &json_in, fca_netWifiStaScan_t *param) {
	try {
		for (int i = 0; i < param->number; i++) {
			json_in["WifiList"][i] = {
				{"Auth",		 ""						   },
				  {"Channel",	  0						   },
				  {"EncrypType", ""						   },
				  {"NetType",	  ""							},
				   {"RSSI",		""						  },
				 {"SSID",		  string(param->item[i].ssid)},
				{"nRSSI",	  param->item[i].signal	   },
			};
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetAlarmControl(json &json_in, fca_alarmControl_t *param) {
	try {
		json_in = {
			{"Sound",	  param->setSound		 },
			{"Lighting", param->setLighting   },
			{"Duration", param->durationInSecs},
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetAlarmControl(json &json_in, fca_alarmControl_t *param) {
	try {
		param->setSound	   = json_in["Sound"].get<bool>();
		param->setLighting = json_in["Lighting"].get<bool>();

		if (json_in.contains("Duration")) {
			param->durationInSecs = json_in["Duration"].get<int>();
		}
		else {
			param->durationInSecs = 30; /* Default setting */
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetDeviceAuth(json &json_in, devAuth_t *param) {
	try {
		json_in = {
			{"uboot", param->ubootPass															  },
			 {"root",  param->rootPass																 },
			{"rtsp",	 {{"account", param->rtspInfo.account}, {"password", param->rtspInfo.password}}}
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetDeviceAuth(json &json_in, devAuth_t *param) {
	try {
		if (json_in.contains("uboot")) {
			param->ubootPass = json_in["uboot"].get<string>();
		}
		if (json_in.contains("root")) {
			param->rootPass = json_in["root"].get<string>();
		}
		if (json_in.contains("rtsp")) {
			param->rtspInfo.account	 = json_in["rtsp"]["account"].get<string>();
			param->rtspInfo.password = json_in["rtsp"]["password"].get<string>();
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetSystemControls(json &json_in, systemCtrls_t *param) {
	// try {
	// 	json eqControlJson = {
	// 		{"PreGain", param->audioEq.eqControl.pre_gain},
	// 		{"Bands",	  param->audioEq.eqControl.bands	},
	// 		   {"Enable",  param->audioEq.eqControl.enable	 }
	// 	  };

	// 	json bandFreqs	= json::array();
	// 	json bandGains	= json::array();
	// 	json bandQ		= json::array();
	// 	json bandTypes	= json::array();
	// 	json bandEnable = json::array();

	// 	for (unsigned long i = 0; i < FCA_EQ_MAX_BANDS; i++) {
	// 		if (i < param->audioEq.eqControl.bands) {
	// 			bandFreqs.push_back(param->audioEq.eqControl.bandfreqs[i]);
	// 			bandGains.push_back(param->audioEq.eqControl.bandgains[i]);
	// 			bandQ.push_back(round(param->audioEq.eqControl.bandQ[i] * 10.0) / 10.0);
	// 			bandTypes.push_back(param->audioEq.eqControl.band_types[i]);
	// 			bandEnable.push_back(param->audioEq.eqControl.band_enable[i]);
	// 		}
	// 		else {
	// 			bandFreqs.push_back(0);
	// 			bandGains.push_back(0);
	// 			bandQ.push_back(0.0);
	// 			bandTypes.push_back(FCA_NO);
	// 			bandEnable.push_back(0);
	// 		}
	// 	}

	// 	eqControlJson["BandFreqs"]	= bandFreqs;
	// 	eqControlJson["BandGains"]	= bandGains;
	// 	eqControlJson["BandQ"]		= bandQ;
	// 	eqControlJson["BandTypes"]	= bandTypes;
	// 	eqControlJson["BandEnable"] = bandEnable;

	// 	json audioEQJson = {
	// 		{"Default",		param->audioEq.applyDefault																							   },
	// 		{"AIGain",		   param->audioEq.aiGain																									 },
	// 		{"AgcControl",
	// 		 {{"Enable", param->audioEq.agcControl.enable},
	// 		  {"AgcLevel", round(param->audioEq.agcControl.agc_level * 10.0) / 10.0},
	// 		  {"AgcMaxGain", param->audioEq.agcControl.agc_max_gain},
	// 		  {"AgcMinGain", round(param->audioEq.agcControl.agc_min_gain * 10.0) / 10.0},
	// 		  {"AgcNearSensitivity", param->audioEq.agcControl.agc_near_sensitivity}}																	 },
	// 		{"AslcControl",	{{"AslcDb", param->audioEq.aslcControl.aslc_db}, {"Limit", round(param->audioEq.aslcControl.limit * 10.0) / 10.0}}		  },
	// 		{"EqControl",	  eqControlJson																											 },
	// 		{"NoiseReduction", {{"Enable", param->audioEq.noiseReduction.enable}, {"NoiseSuppressDbNr", param->audioEq.noiseReduction.noiseSuppressDbNr}}}
	// 	  };

	// 	json_in = {
	// 		{SYSTEM_CONTROL_AEC,
	// 		 {{"AecEn", param->aecNoise.aecEn},
	// 		  {"AecScale", param->aecNoise.aecScale},
	// 		  {"AecThr", param->aecNoise.aecThr},
	// 		  {"Enable", param->aecNoise.enable},
	// 		  {"NoiseEn", param->aecNoise.noiseEn}}																					  },
	// 		{SYSTEM_CONTROL_AMIC,			  {{"Enable", param->amicCapture.enable}, {"Volume", param->amicCapture.volume}}				},
	// 		{SYSTEM_CONTROL_BUTTON_CALL,	 {{"Enable", param->buttonCall.enable}}													   },
	// 		{SYSTEM_CONTROL_MASTER_PLAYBACK, {{"Volume", param->masterPlayback.volume}}												   },
	// 		{SYSTEM_CONTROL_SPEAKER_GAIN,	  {{"Enable", param->speakerGain.enable}, {"AmplifyFactor", param->speakerGain.amplifyFactor}}},
	// 		{SYSTEM_CONTROL_AUDIO_EQ,		  audioEQJson																				 }
	// 	  };

	// 	return true;
	// }
	// catch (const exception &e) {
	// 	APP_DBG("json error: %s\n", e.what());
	// }
	return false;
}

int fca_jsonGetSystemControls(json &json_in, systemCtrls_t *param, bool isCheckMissData) {
	// try {
	// 	if (json_in.contains(SYSTEM_CONTROL_AMIC)) {
	// 		param->amicCapture.enable = json_in[SYSTEM_CONTROL_AMIC]["Enable"].get<bool>();
	// 		param->amicCapture.volume = json_in[SYSTEM_CONTROL_AMIC]["Volume"].get<int>();
	// 	}

	// 	if (json_in.contains(SYSTEM_CONTROL_MASTER_PLAYBACK)) {
	// 		param->masterPlayback.volume = json_in[SYSTEM_CONTROL_MASTER_PLAYBACK]["Volume"].get<int>();
	// 	}

	// 	if (json_in.contains(SYSTEM_CONTROL_AEC)) {
	// 		param->aecNoise.enable	 = json_in[SYSTEM_CONTROL_AEC]["Enable"].get<int>();
	// 		param->aecNoise.aecEn	 = json_in[SYSTEM_CONTROL_AEC]["AecEn"].get<int>();
	// 		param->aecNoise.noiseEn	 = json_in[SYSTEM_CONTROL_AEC]["NoiseEn"].get<int>();
	// 		param->aecNoise.aecThr	 = json_in[SYSTEM_CONTROL_AEC]["AecThr"].get<int>();
	// 		param->aecNoise.aecScale = json_in[SYSTEM_CONTROL_AEC]["AecScale"].get<int>();
	// 	}

	// 	if (json_in.contains(SYSTEM_CONTROL_SPEAKER_GAIN)) {
	// 		param->speakerGain.enable		 = json_in[SYSTEM_CONTROL_SPEAKER_GAIN]["Enable"].get<bool>();
	// 		param->speakerGain.amplifyFactor = json_in[SYSTEM_CONTROL_SPEAKER_GAIN]["AmplifyFactor"].get<int>();
	// 	}

	// 	if (json_in.contains(SYSTEM_CONTROL_BUTTON_CALL)) {
	// 		param->buttonCall.enable = json_in[SYSTEM_CONTROL_BUTTON_CALL]["Enable"].get<bool>();
	// 	}
	// 	if (isCheckMissData == true) {
	// 		if (!json_in.contains(SYSTEM_CONTROL_AUDIO_EQ)) {
	// 			APP_DBG_AUDIO("Missing param [%s]\n", SYSTEM_CONTROL_AUDIO_EQ);
	// 			return APP_CONFIG_ERROR_DATA_MISSING;
	// 		}
	// 	}
	// 	if (json_in.contains(SYSTEM_CONTROL_AUDIO_EQ)) {
	// 		auto audioEQ									= json_in[SYSTEM_CONTROL_AUDIO_EQ];
	// 		param->audioEq.aiGain							= audioEQ[SYSTEM_CONTROL_AIGAIN].get<int>();
	// 		param->audioEq.applyDefault						= audioEQ["Default"].get<bool>();
	// 		param->audioEq.noiseReduction.noiseSuppressDbNr = audioEQ[SYSTEM_CONTROL_NOISEREDUCTION]["NoiseSuppressDbNr"].get<int>();
	// 		param->audioEq.noiseReduction.enable			= audioEQ[SYSTEM_CONTROL_NOISEREDUCTION]["Enable"].get<int>();

	// 		auto agc									   = audioEQ[SYSTEM_CONTROL_AGCCONTROL];
	// 		param->audioEq.agcControl.agc_level			   = agc["AgcLevel"].get<float>();
	// 		param->audioEq.agcControl.agc_max_gain		   = agc["AgcMaxGain"].get<signed short>();
	// 		param->audioEq.agcControl.agc_min_gain		   = agc["AgcMinGain"].get<float>();
	// 		param->audioEq.agcControl.agc_near_sensitivity = agc["AgcNearSensitivity"].get<signed short>();
	// 		param->audioEq.agcControl.enable			   = agc["Enable"].get<int>();

	// 		param->audioEq.aslcControl.limit   = audioEQ[SYSTEM_CONTROL_ASLCCONTROL]["Limit"].get<float>();
	// 		param->audioEq.aslcControl.aslc_db = audioEQ[SYSTEM_CONTROL_ASLCCONTROL]["AslcDb"].get<long>();

	// 		auto eqControl					  = audioEQ[SYSTEM_CONTROL_EQCONTROL];
	// 		param->audioEq.eqControl.pre_gain = eqControl["PreGain"].get<short>();
	// 		param->audioEq.eqControl.bands	  = eqControl["Bands"].get<unsigned long>();
	// 		param->audioEq.eqControl.enable	  = eqControl["Enable"].get<unsigned char>();

	// 		auto bandFreqs	= eqControl["BandFreqs"].get<std::vector<unsigned long>>();
	// 		auto bandGains	= eqControl["BandGains"].get<std::vector<short>>();
	// 		auto bandQ		= eqControl["BandQ"].get<std::vector<float>>();
	// 		auto bandTypes	= eqControl["BandTypes"].get<std::vector<unsigned short>>();
	// 		auto bandEnable = eqControl["BandEnable"].get<std::vector<unsigned char>>();

	// 		for (size_t i = 0; i < FCA_EQ_MAX_BANDS; i++) {
	// 			if (i < param->audioEq.eqControl.bands) {
	// 				param->audioEq.eqControl.bandfreqs[i]	= bandFreqs[i];
	// 				param->audioEq.eqControl.bandgains[i]	= bandGains[i];
	// 				param->audioEq.eqControl.bandQ[i]		= bandQ[i];
	// 				param->audioEq.eqControl.band_types[i]	= bandTypes[i];
	// 				param->audioEq.eqControl.band_enable[i] = bandEnable[i];
	// 			}
	// 			else {
	// 				param->audioEq.eqControl.bandfreqs[i]	= 0;
	// 				param->audioEq.eqControl.bandgains[i]	= 0;
	// 				param->audioEq.eqControl.bandQ[i]		= 0.0f;
	// 				param->audioEq.eqControl.band_types[i]	= FCA_NO;
	// 				param->audioEq.eqControl.band_enable[i] = 0;
	// 			}
	// 		}
	// 	}

	// 	return APP_CONFIG_SUCCESS;
	// }
	// catch (const std::exception &e) {
	// 	APP_DBG_AUDIO("json error: %s\n", e.what());
	// }
	return APP_CONFIG_ERROR_DATA_INVALID;
}

bool fca_jsonSetP2P(json &json_in, int *param) {
	try {
		json_in = {
			{"ClientMax", param[0]}
		   };
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetP2P(json &json_in, int *param) {
	try {
		*param = json_in["ClientMax"].get<int>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_getSnapshotJsonRes(json &json_in, json &json_data, int ret) {
	try {
		int status = ret == APP_CONFIG_SUCCESS ? FCA_MQTT_RESPONE_SUCCESS : FCA_MQTT_RESPONE_FAILED;
		string des = status == FCA_MQTT_RESPONE_SUCCESS ? "Success" : "Fail";
		json_in	   = {
			   {"Method",	  "ACT"							   },
			{"MessageType", MESSAGE_TYPE_SNAPSHOT				 },
			 {"Serial",		string(fca_getSerialInfo())		   },
			   {"Data",		json_data							 },
			{"Result",	   {{"Ret", status}, {"Message", des}}},
			 {"Timestamp",   time(nullptr)						},
		   };
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetOnvif(json &json_in, bool *en) {
	try {
		json_in = {
			{"Enable", en[0]}
		 };
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetOnvif(json &json_in, bool *en) {
	try {
		*en = json_in["Enable"].get<bool>();
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonSetRecord(json &json_in, StorageSdSetting_t *storageSd) {
	try {
		json_in = {
			{"Channel",	storageSd->channel		  },
			{"Mode",		 storageSd->opt			   },
			{"DayStorage", storageSd->maxDayStorage   },
			{"Duration",	 storageSd->regDurationInSec},
		};
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}

bool fca_jsonGetRecord(json &json_in, StorageSdSetting_t *storageSd) {
	try {
		storageSd->channel		 = json_in["Channel"].get<int>();
		storageSd->opt			 = json_in["Mode"].get<STORAGE_SD_OPTION>();
		storageSd->maxDayStorage = json_in["DayStorage"].get<int>();
		if (json_in.contains("Duration")) {
			storageSd->regDurationInSec = json_in["Duration"].get<int>();
		}
		else {
			storageSd->regDurationInSec = SDCARD_RECORD_DURATION_DEFAULT;	 // 900s
		}
		return true;
	}
	catch (const exception &e) {
		APP_DBG("json error: %s\n", e.what());
	}
	return false;
}
