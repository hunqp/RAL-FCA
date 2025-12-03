#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "fca_parameter.hpp"
#include "utils.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline int resolutionMapping(std::string res) {
	if (res == "2K") return FCA_VIDEO_DIMENSION_2K;
	if (res == "1080P") return FCA_VIDEO_DIMENSION_1080P;
	if (res == "720P") return FCA_VIDEO_DIMENSION_720P;
	if (res == "480P") return FCA_VIDEO_DIMENSION_VGA;
	return FCA_VIDEO_DIMENSION_UNKNOWN;
}

static inline std::string resolutionMapping(int res) {
	if (res == FCA_VIDEO_DIMENSION_2K) return "2K";
	if (res == FCA_VIDEO_DIMENSION_1080P) return "1080P";
	if (res == FCA_VIDEO_DIMENSION_720P) return "720P";
	if (res == FCA_VIDEO_DIMENSION_VGA) return "480P";
	return "UNKONWN";
}

static inline FCA_VIDEO_IN_CODEC_E codecMapping(std::string codec) {
	if (codec == "H.264") return FCA_CODEC_VIDEO_H264;
	if (codec == "H.265") return FCA_CODEC_VIDEO_H265;
	return FCA_CODEC_VIDEO_MJPEG;
}

static inline std::string codecMapping(FCA_VIDEO_IN_CODEC_E codec) {
	if (codec == FCA_CODEC_VIDEO_H264) return "H.264";
	if (codec == FCA_CODEC_VIDEO_H265) return "H.265";
	return "MJPEG";
}

static inline FCA_VIDEO_IN_ENCODING_MODE_E rcModeMapping(std::string rc) {
	if (rc == "CBR") return FCA_VIDEO_ENCODING_MODE_CBR;
	if (rc == "VBR") return FCA_VIDEO_ENCODING_MODE_VBR;
	return FCA_VIDEO_ENCODING_MODE_AVBR;
}

static inline std::string rcModeMapping(FCA_VIDEO_IN_ENCODING_MODE_E rc) {
	if (rc == FCA_VIDEO_ENCODING_MODE_CBR) return "CBR";
	if (rc == FCA_VIDEO_ENCODING_MODE_VBR) return "VBR";
	return "AVBR";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FCA_ParserUserFiles(nlohmann::json &js, std::string filename) {
	if (!read_json_file(js, FCA_VENDORS_FILE_LOCATE(filename))) {
		return read_json_file(js, FCA_DEFAULT_FILE_LOCATE(filename));
	}
	return true;
}

bool FCA_UpdateUserFiles(nlohmann::json &js, std::string filename) {
	return write_json_file(js, FCA_VENDORS_FILE_LOCATE(filename));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FCA_ParserParams(nlohmann::json &js, FCA_ENCODE_S *encoders) {
	if (!encoders) {
		return false;
	}

	try {
		/* Main stream */
		encoders->mainStream.fps = js["MainFormat"]["Video"]["FPS"].get<int>();
		encoders->mainStream.gop = js["MainFormat"]["Video"]["GOP"].get<int>();
		encoders->mainStream.bitrate = js["MainFormat"]["Video"]["BitRate"].get<int>();
		encoders->mainStream.quality = js["MainFormat"]["Video"]["Quality"].get<int>();
		encoders->mainStream.resolution = resolutionMapping(js["MainFormat"]["Video"]["Resolution"].get<std::string>());
		encoders->mainStream.codec = codecMapping(js["MainFormat"]["Video"]["Compression"].get<std::string>());
		encoders->mainStream.rcmode = rcModeMapping(js["MainFormat"]["Video"]["BitRateControl"].get<std::string>());

		/* Minor stream */
		encoders->minorStream.fps = js["ExtraFormat"]["Video"]["FPS"].get<int>();
		encoders->minorStream.gop = js["ExtraFormat"]["Video"]["GOP"].get<int>();
		encoders->minorStream.bitrate = js["ExtraFormat"]["Video"]["BitRate"].get<int>();
		encoders->minorStream.quality = js["ExtraFormat"]["Video"]["Quality"].get<int>();
		encoders->minorStream.resolution = resolutionMapping(js["ExtraFormat"]["Video"]["Resolution"].get<std::string>());
		encoders->minorStream.codec = codecMapping(js["ExtraFormat"]["Video"]["Compression"].get<std::string>());
		encoders->minorStream.rcmode = rcModeMapping(js["ExtraFormat"]["Video"]["BitRateControl"].get<std::string>());
		return true;
	}
	catch (...) {
		APP_WARN("Invalid parser");
	}
	return false;
}

bool FCA_UpdateParams(nlohmann::json &js, FCA_ENCODE_S *encoders) {
	if (!encoders) {
		return false;
	}

	try {
		/* Main stream */
		js["MainFormat"]["Video"]["FPS"] = encoders->mainStream.fps;
		js["MainFormat"]["Video"]["GOP"] = encoders->mainStream.gop;
		js["MainFormat"]["Video"]["BitRate"] = encoders->mainStream.bitrate;
		js["MainFormat"]["Video"]["Quality"] = encoders->mainStream.quality;
		js["MainFormat"]["Video"]["Resolution"] = resolutionMapping(encoders->mainStream.resolution);
		js["MainFormat"]["Video"]["Compression"] = codecMapping(encoders->mainStream.codec);
		js["MainFormat"]["Video"]["BitRateControl"] = rcModeMapping(encoders->mainStream.rcmode);

		/* Minor stream */
		js["ExtraFormat"]["Video"]["FPS"] = encoders->minorStream.fps;
		js["ExtraFormat"]["Video"]["GOP"] = encoders->minorStream.gop;
		js["ExtraFormat"]["Video"]["BitRate"] = encoders->minorStream.bitrate;
		js["ExtraFormat"]["Video"]["Quality"] = encoders->minorStream.quality;
		js["ExtraFormat"]["Video"]["Resolution"] = resolutionMapping(encoders->minorStream.resolution);
		js["ExtraFormat"]["Video"]["Compression"] = codecMapping(encoders->minorStream.codec);
		js["ExtraFormat"]["Video"]["BitRateControl"] = rcModeMapping(encoders->minorStream.rcmode);

		return FCA_UpdateUserFiles(js, FCA_ENCODE_FILE);
	}
	catch (...) {
		APP_WARN("Invalid update");
	}
	return false;
}

bool FCA_ParserParams(nlohmann::json &js, FCA_OSD_S *osds) {
	if (!osds) {
		return false;
	}

	try {
		osds->name = js["NameTitle"].get<std::string>();
		osds->timeAtt = js["TimeTitleAttribute"].get<bool>();
		osds->nameAtt = js["NameTitleAttribute"].get<bool>();
		return true;
	}
	catch (...) {
		APP_WARN("Invalid parser");
	}
	return false;
}

bool FCA_UpdateParams(nlohmann::json &js, FCA_OSD_S *osds) {
	if (!osds) {
		return false;
	}

	try {
		js["NameTitle"] = osds->name;
		js["TimeTitleAttribute"] = osds->timeAtt;
		js["NameTitleAttribute"] = osds->nameAtt;
		return FCA_UpdateUserFiles(js, FCA_WATERMARK_FILE);
	}
	catch (...) {
		APP_WARN("Invalid update");
	}
	return false;
}