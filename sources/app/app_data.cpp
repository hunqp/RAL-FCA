#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <openssl/sha.h>

#include "ak.h"

#include "rtc/rtc.hpp"
#include "task_list.h"
#include "app_config.h"
#include "app_data.h"
#include "videosource.hpp"
#include "app_capture.h"

GPIOHelpers gpiosHelpers;
ClientsGroup_t clients;
DispatchQueue sysThread("rtcHandler", 2);
DispatchQueue voiceGuideThread("playVoiceHandler");
DispatchQueue pushToTalkThread("push2TalkHdl");
optional<shared_ptr<Stream>> avStream = nullopt;
// RTSPServer rtspServer;
CppTime::Timer systemTimer;

std::string caSslPath;

static pthread_mutex_t mtxListClients = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t logFileMtx	  = PTHREAD_MUTEX_INITIALIZER;

AudioHelpers audioHelpers;
VideoHelpers videoHelpers;

void lockMutexListClients() {
	pthread_mutex_lock(&mtxListClients);
}

void unlockMutexListClients() {
	pthread_mutex_unlock(&mtxListClients);
}

void sendMsgControlDataChannel(const string &id, const string &msg) {
	if (msg.empty())
		return;

	lockMutexListClients();
	if (auto jt = clients.find(id); jt != clients.end()) {
		auto dc = jt->second->dataChannel.value();
		APP_DBG("Send message to %s\n", id.c_str());
		try {
			if (dc->isOpen()) {
				dc->send(msg);
			}
		}
		catch (const exception &error) {
			APP_DBG("%s\n", error.what());
		}
	}
	else {
		APP_DBG("%s not found\n", id.c_str());
	}
	unlockMutexListClients();
}

std::string fca_getSerialInfo() {
	return deviceSerialNumber;
}

void sendMsgRespondProcess(const string &otaProcess) {
	if (otaProcess.empty()) {
		return;
	}

	lockMutexListClients();
	for (auto pair : clients) {
		auto dc = pair.second->dataChannel.value();
		APP_DBG("Send message to %s\n", pair.first.c_str());
		try {
			if (dc->isOpen()) {
				dc->send(otaProcess);
			}
		}
		catch (const exception &error) {
			APP_DBG("%s\n", error.what());
		}
	}
	unlockMutexListClients();
}

string genPassword(int len) {
	char possibleChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()";
	char pw[len + 1];

	srand(getpid());	// seed for random number generation
	for (int i = 0; i < len; i++) {
		int randomIndex = rand() % (sizeof(possibleChars) - 1);
		pw[i]			= possibleChars[randomIndex];
	}

	pw[len] = '\0';	   // null terminate the string
	APP_DBG("The randomly generated pw is: %s\n", pw);
	return string(pw);
}

void genDefaultPasswordRtsp(string &user, string &pass) {
	// string pw = genPassword(RTSP_PASSWORD_LENGTH_MAX - 1);
	// APP_DBG("rtsp pass: %s, len: %d\n", pw.data(), pw.length());
	// memcpy(rtspPass, pw.data(), sizeof(rtspPass));
	char rtspPass[RTSP_PASSWORD_LENGTH_MAX];
	string data = fca_getSerialInfo() + ":rtsp";
	SHA256_CTX sha256;
	SHA224_Init(&sha256);

	APP_DBG("input sha224: %s\n", data.data());
	SHA224_Update(&sha256, data.data(), data.length());

	unsigned char hash[SHA224_DIGEST_LENGTH];
	SHA224_Final(hash, &sha256);

	memset(rtspPass, 0, sizeof(rtspPass));
	uint8_t nByte = (RTSP_PASSWORD_LENGTH_MAX - 1) / 2;
	for (int i = 0; i < nByte; i++) {
		sprintf(rtspPass + (i * 2), "%02X", hash[i]);
	}

	// APP_DBG("hash: ");
	// for (int i = 0; i < nByte; i++) {
	// 	APP_DBG("%02x", hash[i]);
	// }

	user = RTSP_ACCOUNT_DEFAULT;
	pass = rtspPass;
	APP_DBG("\ngen rtsp user: %s, pass: %s\n", user.data(), pass.data());
}

void rotateLog(const char *base_path, int rotation_count) {
	char old_path[256], new_path[256];
	APP_DBG("rotate log file: %s\n", base_path);
	snprintf(old_path, sizeof(old_path), "%s.%d", base_path, rotation_count);
	remove(old_path);

	for (int i = rotation_count - 1; i >= 1; --i) {
		snprintf(old_path, sizeof(old_path), "%s.%d", base_path, i);
		snprintf(new_path, sizeof(new_path), "%s.%d", base_path, i + 1);
		rename(old_path, new_path);
		APP_DBG("rename %s to %s\n", old_path, new_path);
	}

	snprintf(new_path, sizeof(new_path), "%s.1", base_path);
	rename(base_path, new_path);
	APP_DBG("rotate log file done\n");
}

void logFile(const char *full_path, const uint32_t max_size, const uint8_t rotation_count, const char *fmt, ...) {
	if (full_path == NULL || fmt == NULL) {
		APP_DBG("path NULL\n");
		return;
	}

	va_list args;
	va_start(args, fmt);

	int needed = std::vsnprintf(nullptr, 0, fmt, args);
	va_end(args);

	if (needed < 0) {
		APP_DBG("vsnprintf failed\n");
		return;
	}

	std::string message(needed, '\0');

	va_start(args, fmt);
	std::vsnprintf(&message[0], message.size() + 1, fmt, args);
	va_end(args);
	// APP_DBG("log message: %s\n", message.data());

	pthread_mutex_lock(&logFileMtx);

	size_t size = sizeOfFile(full_path);	// NOTE: update return 0 for sizeOfFile() is defined in utils.cpp
	FILE *fp	= NULL;
	// APP_DBG("log file size: %zu, max_file: %zu\n", size, max_size);
	if (size >= (size_t)max_size) {
		if (rotation_count > 0) {
			rotateLog(full_path, rotation_count);
		}
		fp = fopen(full_path, "w");
	}
	else {
		fp = fopen(full_path, "a");
	}

	if (fp != NULL) {
		time_t now					   = time(NULL);
		char *time_str				   = ctime(&now);
		time_str[strlen(time_str) - 1] = '\0';
		fprintf(fp, "[%s] %s", time_str, message.data());
		fsync(fileno(fp));
		fclose(fp);
	}
	else {
		APP_DBG("Can not open file log: %s\n", full_path);
	}

	pthread_mutex_unlock(&logFileMtx);
}

std::string formatTimezoneOffset(const std::string &tz) {
	if (tz.rfind("UTC", 0) != 0 || tz.size() < 4)
		return "+00:00";	// fallback

	char sign			 = tz[3];	 // '+' or '-'
	std::string timePart = tz.substr(4);

	int hour = 0, min = 0;
	try {
		size_t colon = timePart.find(':');
		if (colon != std::string::npos) {
			hour = std::stoi(timePart.substr(0, colon));
			min	 = std::stoi(timePart.substr(colon + 1));
		}
		else {
			hour = std::stoi(timePart);
			min	 = 0;
		}
	}
	catch (const std::exception &e) {
		APP_DBG("Formatted TZ: fallback +00:00 due to parse error: %s\n", e.what());
		return "+00:00";
	}

	char buf[8];
	snprintf(buf, sizeof(buf), "%c%02d:%02d", sign, hour, min);
	return std::string(buf);
}

// std::string getPayloadMQTTSignaling(uint32_t time, JPEG_SNAP_TYPE_E triggerType, uint32_t duration, std::string typeStr, s3_object_t *s3_obj) {
// 	char buffers[20];
// 	const char *type = NULL;
// 	struct tm *tm;
// 	time_t ts;

// 	ts = (time_t)time;
// 	tm = localtime(&ts);
// 	memset(buffers, 0, sizeof(buffers));
// 	strftime(buffers, sizeof(buffers), "%Y%m%d%H%M%S", tm);

// 	switch (triggerType) {
// 	case JPEG_SNAP_TYPE_MOTION:
// 		type = MESSAGE_TYPE_MOTION_DETECT;
// 		break;

// 	case JPEG_SNAP_TYPE_HUMAN:
// 		type = MESSAGE_TYPE_HUMAN_DETECT;
// 		break;
// 	case JPEG_SNAP_TYPE_BABYCRY:
// 		type = MESSAGE_TYPE_BABYCRYING_DETECT;
// 		break;
		
// 	default:
// 		break;
// 	}

// 	string tz;
// 	string formattedTz = "+00:00";
// 	if (fca_configGetTZ(tz) == APP_CONFIG_SUCCESS) {
// 		formattedTz = formatTimezoneOffset(tz);
// 		APP_DBG("Formatted TZ: %s\n", formattedTz.c_str());
// 	}

// 	json JSON = {
// 		{"Alarm", string(type, strlen(type))},
// 		{"DeviceID", string(fca_getSerialInfo())},
// 		{"StartTime", string(buffers, strlen(buffers))},
// 		{"Duration", duration},
// 		{"TimeZone", formattedTz},
// 		{"Data", 
// 			{
// 				{"Bucket", s3_obj->status == S3_OK ? s3_obj->bucket: ""},
// 				{"ObjName", s3_obj->status == S3_OK ? s3_obj->name: ""},
// 				{"ObjType", typeStr},
// 				{"ErrorReason", s3GetStatusName(s3_obj->ret)},
// 				{"ErrorCode", s3_obj->ret}, 
// 				{"Labels", json::array()},
// 			}
// 		}
		
		
// 	};

// 	return JSON.dump();
// }

int getNtpStatus() {
	FILE *file = fopen(NTP_UPDATE_STATUS_FILE, "r");
	if (file) {
		int ntpSt = 0;
		if (fscanf(file, "%d", &ntpSt) == 1) {
			fclose(file);
			return ntpSt;
		}
		fclose(file);
	}
	return -1; // NTP status not found
}

std::string templatesResponseToCloud(std::string method, std::string messageType, nlohmann::json js, int rc) {
	nlohmann::json templates;
	templates["Method"] = method;
	templates["MessageType"] = messageType;
	templates["Serial"] = deviceSerialNumber;
	templates["Data"] = (js.dump().empty()) ? json::object() : js;
	templates["Result"]["Ret"] = (rc == APP_CONFIG_SUCCESS) ? FCA_MQTT_RESPONE_SUCCESS : FCA_MQTT_RESPONE_FAILED;
	templates["Result"]["Message"] = (rc == APP_CONFIG_SUCCESS) ? "Success" : "Fail";
	templates["Timestamp"] = time(NULL);
	return templates.dump();
}