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
#include "app_config.h"
#include "task_list.h"
#include "utils.h"

#include "parser_json.h"
#include "fca_gpio.hpp"
#include "fca_audio.hpp"
#include "fca_video.hpp"
#include "mqtt.h"

#define SPEAKER_VOLUME_DEFAULT (110)
#define CA_FILE_DOWNLOAD_PATH  "/tmp/" FCA_NETWORK_CA_FILE
#define DOMAINS_LIST_FILE_PATH "/tmp/domains_list.txt"
#define WATCH_DOG_TASK_LIST_LEN_MAX (2)	   // system task: AK_TASK_TIMER_ID, GW_TASK_SYS_ID

q_msg_t gw_task_sys_mailbox;
systemCtrls_t sysCtrls;

int systemSetup(const string &type, systemCtrls_t *sysCtrls);

static bool cloudWaitResponse = false;
static bool bDisableWdt	= true;
static devAuth_t devAuth;
static uint32_t pingPongId = 0;

static RTSPD rtspd555;

static void *pollingButtonRstCalls(void *args);
static int setUbootPass(string pass);
static int setRootPass(string pass);
static int setRtspAccount(string user, string pass);
static void initDeviceAuth(devAuth_t *param);

void *gw_task_sys_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;

	wait_all_tasks_started();

	APP_DBG("[STARTED] gw_task_sys_entry\n");

	pthread_t pid = 0;
	pthread_create(&pid, NULL, pollingButtonRstCalls, NULL);

// #if defined(RELEASE) && (RELEASE == 1) 
// 	APP_DBG_SYS("[watchdog] enable for release");
// 	if (AK_TASK_LIST_LEN > WATCH_DOG_TASK_LIST_LEN_MAX) {
// 		if (fca_sys_watchdog_init(GW_SYS_WATCH_DOG_TIMEOUT_INTERVAL) == 0) {
// #if (CHECK_TIME_WATCHDOG == 1)
// 			auto start = std::chrono::high_resolution_clock::now();
// 			std::cout << "[EXEC-TIME] task_sys::watchdog start time: " << std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count() << " s\n";
// #endif
// 			timer_set(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ, GW_SYS_WATCH_DOG_PING_TASK_REQ_INTERVAL, TIMER_ONE_SHOT);
// 		}
// 	}
// #else
// 	APP_DBG_SYS("[watchdog] is disable for deverloper and testing ---------------------------------------------------------------------------------------------\n");
// 	if (AK_TASK_LIST_LEN > WATCH_DOG_TASK_LIST_LEN_MAX) {
// 		timer_set(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ, GW_SYS_WATCH_DOG_PING_TASK_REQ_INTERVAL, TIMER_ONE_SHOT);
// 	}
// #endif

// 	/* init all password */
// 	initDeviceAuth(&devAuth);

// 	/* check env log server */
// 	string logServer = systemStrCmd("fw_printenv log_serverip | cut -d'=' -f2");
// 	if (!logServer.empty()) {
// 		size_t colonPos = logServer.find(':');
// 		if (colonPos != string::npos) {
// 			string ip = logServer.substr(0, colonPos);
// 			if (isIpValid(ip)) {
// 				string port = logServer.substr(colonPos + 1);
// 				APP_DBG_SYS("Log server IP: %s, Port: %s\n", ip.c_str(), port.c_str());
// 				startSyslog(ip.data(), port.data());
// 				timer_set(GW_TASK_SYS_ID, GW_SYS_GET_SYSTEM_INFO_REQ, 7000, TIMER_PERIODIC);
// 			}
// 		}
// 	}

	#if RELEASE
	fca_sys_watchdog_init(GW_SYS_WATCH_DOG_TIMEOUT_INTERVAL);
	#endif
	task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ);

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_SYS_ID);

		switch (msg->header->sig) {
		case GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ: {
			if (bDisableWdt) {
				break;
			}

			do {
				pingPongId = (pingPongId + 1) % AK_TASK_LIST_LEN;
			} 
			while (pingPongId == AK_TASK_TIMER_ID || pingPongId == GW_TASK_SYS_ID);
			task_post_pure_msg(pingPongId, AK_USER_WATCHDOG_SIG);
			timer_set(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_TIMEOUT, 30000, TIMER_ONE_SHOT);
			APP_DBG("\'%s\' -> PING\r\n", task_list[pingPongId].info);
		}
		break;

		case GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES: {
			if (bDisableWdt) {
				break;
			}

			if (msg->header->src_task_id == pingPongId) {
				APP_DBG("\'%s\' -> PONG\r\n", task_list[pingPongId].info);
				#if defined(RELEASE)
				vvc_ipc_wdt_refresh();
				#endif
				timer_set(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ, GW_SYS_WATCH_DOG_PING_TASK_REQ_INTERVAL, TIMER_ONE_SHOT);
			}
			else {
				SYSE("WCHDOG\r\n");
				FATAL("WCHDOG", 0x01);
			}
		}
		break;

		case GW_SYS_WATCH_DOG_PING_NEXT_TASK_TIMEOUT: {
			APP_DBG_SIG("GW_SYS_WATCH_DOG_PING_NEXT_TASK_TIMEOUT\r\n");

			SYSW("%s doesn't response\r\n", task_list[pingPongId].info);
		}
		break;

		case GW_SYS_REBOOT_REQ: {
			APP_DBG_SIG("GW_SYS_REBOOT_REQ\n");
			if (msg->header->src_task_id == GW_TASK_CLOUD_ID) {
				cloudWaitResponse = true;
			}
			if (!isOtaRunning.load()) {
				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_CMD_REBOOT_REQ);
				LOG_FILE_INFO("reboot system\n");
			}
			else {
				APP_DBG("OTA is running, not reboot\n");
			}
		} break;

		case GW_SYS_CMD_REBOOT_REQ: {
			APP_DBG_SIG("GW_SYS_CMD_REBOOT_REQ\n");
			offlineEventManager.addEvent({NETWORK_ERROR_REBOOT, (uint32_t)time(nullptr), TASK_PRI_LEVEL_8});

			if (cloudWaitResponse) {
				cloudWaitResponse = false;
				/* response to mqtt server */
				json resJs;
				if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_REBOOT, APP_CONFIG_SUCCESS)) {
					task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
				}
			}
			task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_RELEASE_RESOURCES_REQ);
			sleep(3);
			ledStatus.controlLedEvent(Indicator::EVENT::REBOOT);
			systemCmd("kill -9 %d && reboot", getpid());
			ledStatus.controlLedEvent(Indicator::EVENT::UNKNOWN);
		} break;

		case GW_SYS_START_WATCH_DOG_REQ: {
			APP_DBG_SIG("GW_SYS_START_WATCH_DOG_REQ\n");
			if (bDisableWdt) {
				bDisableWdt = false;
#if defined(RELEASE)
				fca_sys_watchdog_init(GW_SYS_WATCH_DOG_TIMEOUT_INTERVAL);
				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ);
#endif
			}
		} break;

		case GW_SYS_STOP_WATCH_DOG_REQ: {
			APP_DBG_SIG("GW_SYS_STOP_WATCH_DOG_REQ\n");

			bDisableWdt = true;
			timer_remove_attr(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_REQ);
			#if RELEASE
			FCA_API_ASSERT(fca_sys_watchdog_refresh() == 0);
    		FCA_API_ASSERT(fca_sys_watchdog_stop_block() == 0);
			#endif
		}
		break;

		case GW_SYS_SET_LOGIN_INFO_REQ: {
			APP_DBG_SIG("GW_SYS_SET_LOGIN_INFO_REQ\n");
			string type = "";
			int8_t ret	= APP_CONFIG_ERROR_ANOTHER;
			try {
				json data = json::parse(string((char *)msg->header->payload, msg->header->len));
				string passTmp;
				type = data["Type"].get<string>();
				if (type == "uboot") {
					passTmp = data["Uboot"].get<string>();
					ret		= setUbootPass(passTmp);
					if (ret == APP_CONFIG_SUCCESS) {
						devAuth.ubootPass = passTmp;
					}
				}
				else if (type == "root") {
					passTmp = data["Root"].get<string>();
					ret		= setRootPass(passTmp);
					if (ret == APP_CONFIG_SUCCESS) {
						devAuth.rootPass = passTmp;
					}
				}
				else if (type == "rtsp") {
					string accTmp = data["Account"].get<string>();
					passTmp		  = data["Password"].get<string>();
					ret			  = setRtspAccount(accTmp, passTmp);
					if (ret == APP_CONFIG_SUCCESS) {
						devAuth.rtspInfo.account  = accTmp;
						devAuth.rtspInfo.password = passTmp;
					}
				}
				else {
					APP_DBG("unknown type set\n");
				}
			}
			catch (const exception &error) {
				APP_DBG("%s\n", error.what());
				ret = APP_CONFIG_ERROR_DATA_INVALID;
			}

			if (ret == APP_CONFIG_SUCCESS) {
				fca_configSetDeviceAuth(&devAuth);
			}
			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_LOGIN_INFO, ret)) {
				if (!type.empty()) {
					resJs["Data"]["Type"] = type;
				}
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_SYS_GET_LOGIN_INFO_REQ: {
			APP_DBG_SIG("GW_SYS_GET_LOGIN_INFO_REQ\n");
			json resJs, dataJs;
			fca_jsonSetDeviceAuth(dataJs, &devAuth);
			/* response to mqtt server */
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_LOGIN_INFO, APP_CONFIG_SUCCESS)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_SYS_RUNNING_CMDS_REQ: {
			APP_DBG_SIG("GW_SYS_RUNNING_CMDS_REQ\n");
			int ret = APP_CONFIG_SUCCESS;
			json resJs, dataRes;
			dataRes		= json::object();
			string info = "";
			try {
				json data		= json::parse(string((char *)msg->header->payload, msg->header->len));
				string cmd		= data["Cmd"].get<string>();
				info			= systemStrCmd(cmd.c_str());
				dataRes			= data;
				dataRes["Info"] = info;
			}
			catch (const std::exception &e) {
				APP_DBG("%s\n", e.what());
				ret = APP_CONFIG_ERROR_ANOTHER;
			}

			/* response to mqtt server */
			if (fca_getConfigJsonRes(resJs, dataRes, MESSAGE_TYPE_RUNNING_CMDS, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_SYS_LOAD_SYSTEM_CONTROLS_CONFIG_REQ: {
			APP_DBG_SIG("GW_SYS_LOAD_SYSTEM_CONTROLS_CONFIG_REQ\n");
			/* default not speaker gain */
			// sysCtrls.speakerGain.enable = false;
			// /* system setup */
			// if (fca_configGetSystemControls(&sysCtrls) == APP_CONFIG_SUCCESS) {
			// 	APP_DBG("get system controls success\n");
			// 	timer_set(GW_TASK_SYS_ID, GW_SYS_LOAD_AUDIO_CONFIG_REQ, 500, TIMER_ONE_SHOT);
			// }
		}
		break;

		case GW_SYS_LOAD_AUDIO_CONFIG_REQ: {
			APP_DBG_SIG("GW_SYS_LOAD_AUDIO_CONFIG_REQ\n");
			// systemSetup(SYSTEM_CONTROL_AMIC, &sysCtrls);
			// systemSetup(SYSTEM_CONTROL_MASTER_PLAYBACK, &sysCtrls);
			// systemSetup(SYSTEM_CONTROL_AUDIO_EQ, &sysCtrls);
		}
		break;

		case GW_SYS_LOAD_SPEAKER_MIC_CONFIG_REQ: {
			APP_DBG_SIG("GW_SYS_LOAD_SPEAKER_MIC_CONFIG_REQ\n");
			systemSetup(SYSTEM_CONTROL_AMIC, &sysCtrls);
			systemSetup(SYSTEM_CONTROL_MASTER_PLAYBACK, &sysCtrls);
		} break;

		case GW_SYS_SET_SYSTEM_CONTROLS_REQ: {
			APP_DBG_SIG("GW_SYS_SET_SYSTEM_CONTROLS_REQ\n");
			// string typeCtrl = "";
			// int ret			= APP_CONFIG_ERROR_DATA_INVALID;
			// try {
			// 	json paramJs		   = json::parse(string((char *)msg->header->payload, msg->header->len));
			// 	systemCtrls_t paramTmp = {0};
			// 	if (fca_configGetSystemControls(&paramTmp) == APP_CONFIG_SUCCESS) {
			// 		if (fca_jsonGetSystemControls(paramJs, &paramTmp, false) == APP_CONFIG_SUCCESS) {
			// 			typeCtrl = paramJs["Type"].get<string>();
			// 			ret		 = systemSetup(typeCtrl, &paramTmp);
			// 			APP_DBG("set system controls: %s -> ret [%d]\n", typeCtrl.c_str(), ret);
			// 			if (ret == 0) {
			// 				memcpy(&sysCtrls, &paramTmp, sizeof(sysCtrls));
			// 				fca_configSetSystemControls(&paramTmp);
			// 			}
			// 		}
			// 	}
			// }
			// catch (const exception &error) {
			// 	APP_DBG("%s\n", error.what());
			// 	ret = APP_CONFIG_ERROR_ANOTHER;
			// }

			// /* response to mqtt server */
			// json resJs;
			// if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_SYSTEM_CONTROLS, ret)) {
			// 	resJs["Data"]["Type"] = typeCtrl;
			// 	task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			// }
		} break;

		case GW_SYS_GET_SYSTEM_CONTROLS_REQ: {
			APP_DBG_SIG("GW_SYS_GET_SYSTEM_CONTROLS_REQ\n");
			json resJs, dataJs = json::object();
			fca_jsonSetSystemControls(dataJs, &sysCtrls);
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_SYSTEM_CONTROLS, APP_CONFIG_SUCCESS)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_SYS_START_RTSPD_REQ: {
			APP_DBG_SIG("GW_SYS_START_RTSPD_REQ\n");

			if (!rtspd555.begin(554)) {
				timer_set(GW_TASK_SYS_ID, GW_SYS_START_RTSPD_REQ, 5000, TIMER_ONE_SHOT);
				break;
			}
			rtspd555.onOpened([]() {
				APP_PRINT("RTSPD555 has started\r\n");
			});
			rtspd555.onClosed([]() {
				APP_PRINT("RTSPD555 has stopped\r\n");
			});

			FCA_ENCODE_S encoders = videoHelpers.getEncoders();
			rtspd555.addMediaSession0("stream0", "FCA RTSPD main stream", 
				(encoders.mainStream.codec == FCA_CODEC_VIDEO_H264) ? RTSPD555_VIDEO_ENCODER_H264_ID : RTSPD555_VIDEO_ENCODER_H265_ID,
				RTSPD555_AUDIO_ENCODER_ALAW_ID, 1, 16, 8000);
			rtspd555.addMediaSession1("stream1", "FCA RTSPD minor stream", 
				(encoders.minorStream.codec == FCA_CODEC_VIDEO_H264) ? RTSPD555_VIDEO_ENCODER_H264_ID : RTSPD555_VIDEO_ENCODER_H265_ID,
				RTSPD555_AUDIO_ENCODER_ALAW_ID, 1, 16, 8000); 
			
			rtspd555.run();
		}
		break;

		case GW_SYS_STOP_RTSPD_REQ: {
			APP_DBG_SIG("GW_SYS_STOP_RTSPD_REQ\n");

			// rtspServer.stop();
			// task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_START_RTSPD_REQ);
		}
		break;

		case GW_SYS_GET_SYSTEM_INFO_REQ: {
			APP_DBG_SIG("GW_SYS_GET_SYSTEM_INFO_REQ\n");
			IF_SYS_LOG_DBG {
				// get cpu ram system
				string info = systemStrCmd("top -b -n 1 | awk 'NR==1'");
				SYSLOG_LOG(LOG_INFO, "logf=%s %s\n", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, info.data());
				info = systemStrCmd("top -b -n 1 | awk 'NR==2'");
				SYSLOG_LOG(LOG_INFO, "logf=%s %s\n", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, info.data());

				// get pid
				int pid = getpid();
				if (pid >= 0) {
					info = systemStrCmd("top -b -n 1 | grep %d | awk 'NR==1'", pid);
					SYSLOG_LOG(LOG_INFO, "logf=%s App: %s\n", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, info.data());
				}

				// get ram rss app
				info = systemStrCmd("cat /proc/%d/status | grep VmRSS", getpid());
				SYSLOG_LOG(LOG_INFO, "logf=%s VmRss: %s\n", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, info.data());

				// get temp
				info = systemStrCmd("cat /sys/devices/platform/soc/800024c.tsensor/chip_temperature");
				SYSLOG_LOG(LOG_INFO, "logf=%s Temp: %s\n", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, info.data());
			}
		} break;

		case GW_SYS_GET_CA_FILE_REQ: {
			APP_DBG_SIG("GW_SYS_GET_CA_FILE_REQ\n");
			sysThread.dispatch([]() {
				json cfgJs;
				string linkPath = FCA_DEFAULT_FILE_LOCATE(FCA_LINKS_FILE);
				if (read_json_file(cfgJs, linkPath)) {
					try {
						string link = cfgJs["SslCaLink"].get<string>();
						if (download_file(link, CA_FILE_DOWNLOAD_PATH, 10)) {
							APP_DBG("download file CA success\n");
							systemCmd("cp -f %s %s", CA_FILE_DOWNLOAD_PATH, FCA_VENDORS_FILE_LOCATE(FCA_NETWORK_CA_FILE).c_str());
							caSslPath = FCA_VENDORS_FILE_LOCATE(FCA_NETWORK_CA_FILE);
							task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ);
							task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_INIT_REQ);
							remove(CA_FILE_DOWNLOAD_PATH);
						}
						else {
							APP_DBG_MQTT("retry download CA file failed\n");
							LOG_FILE_INFO("download CA file failed\n");
							timer_set(GW_TASK_SYS_ID, GW_SYS_GET_CA_FILE_REQ, 15000, TIMER_ONE_SHOT);
						}
					}
					catch (const exception &e) {
						APP_DBG("json error: %s\n", e.what());
					}
				}
			});
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

int setUbootPass(string pass) {
	/* install uboot pass */
	if (pass.empty()) {
		pass = ROOT_DEVICE_PASS_DEFAULT;
	}
	APP_DBG("<***> uboot pass: %s\n", pass.data());
	if (fca_sys_set_uboot_passwd((char *)pass.c_str(), pass.length()) == 0) {
		APP_DBG("set pass uboot success\n");
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("set pass uboot failed\n");
		return APP_CONFIG_ERROR_ANOTHER;
	}
}

int setRootPass(string pass) {
	/* install root pass */
	if (pass.empty()) {
		pass = ROOT_DEVICE_PASS_DEFAULT;
	}
	APP_DBG("<***> root pass: %s\n", pass.data());
	if (fca_sys_set_root_passwd((char *)pass.c_str(), pass.length()) == 0) {
		APP_DBG("set pass root success\n");
		return APP_CONFIG_SUCCESS;
	}
	else {
		APP_DBG("set pass root failed\n");
		return APP_CONFIG_ERROR_ANOTHER;
	}
}

int setRtspAccount(string user, string pass) {
	// rtspServer.setUsernameAndPassword(onvifSetting.username, onvifSetting.password);

	return APP_CONFIG_SUCCESS;
}

void initDeviceAuth(devAuth_t *devAuth) {
	devAuth->clear();
	fca_configGetDeviceAuth(devAuth);
	setUbootPass(devAuth->ubootPass);
	setRootPass(devAuth->rootPass);
	setRtspAccount(devAuth->rtspInfo.account, devAuth->rtspInfo.password);
}

int systemSetup(const string &type, systemCtrls_t *sysCtrls) {
	int ret = 0;
	// if (type == SYSTEM_CONTROL_AMIC) {
	// 	if (sysCtrls->amicCapture.enable && sysCtrls->amicCapture.volume > 0) {
	// 		audioCtrl.setInVolume(sysCtrls->amicCapture.volume);
	// 		APP_DBG("amic enable, volume: %d\n", sysCtrls->amicCapture.volume);
	// 	}
	// 	else {
	// 		audioCtrl.setInVolume(0);
	// 		APP_DBG("amic disable\n");
	// 	}
	// }
	// else if (type == SYSTEM_CONTROL_MASTER_PLAYBACK) {
	// 	if (sysCtrls->masterPlayback.volume > 0) {
	// 		audioCtrl.setOuVolume(sysCtrls->masterPlayback.volume);
	// 	}
	// 	else {
	// 		audioCtrl.setOuVolume(0);
	// 	}
	// }
	// else if (type == SYSTEM_CONTROL_AEC) {
	// 	audioCtrl.controlAEC(sysCtrls->aecNoise.enable, sysCtrls->aecNoise.aecEn, sysCtrls->aecNoise.noiseEn, sysCtrls->aecNoise.aecThr, sysCtrls->aecNoise.aecScale);
	// }
	// else if (type == SYSTEM_CONTROL_SPEAKER_GAIN) {
	// 	/* Nothing */
	// }
	// else if (type == SYSTEM_CONTROL_AUDIO_EQ) {
	// 	if (sysCtrls->audioEq.applyDefault) {
	// 		fca_audio_config_default();
	// 		APP_DBG("Applied GIEC default audio configuration\n");
	// 	}
	// 	else {
	// 		if (fca_ai_set_gain(sysCtrls->audioEq.aiGain) == 0) {
	// 			APP_DBG("Set AIGain to %d successfully\n", sysCtrls->audioEq.aiGain);
	// 		}
	// 		else {
	// 			APP_DBG("Failed to set AIGain\n");
	// 		}
	// 		audioCtrl.controlNR(sysCtrls->audioEq.noiseReduction.noiseSuppressDbNr, sysCtrls->audioEq.noiseReduction.enable);

	// 		audioCtrl.controlAGC(sysCtrls->audioEq.agcControl.agc_level, sysCtrls->audioEq.agcControl.agc_max_gain, sysCtrls->audioEq.agcControl.agc_min_gain,
	// 							 sysCtrls->audioEq.agcControl.agc_near_sensitivity, sysCtrls->audioEq.agcControl.enable);

	// 		audioCtrl.controlASLC(sysCtrls->audioEq.aslcControl.limit, sysCtrls->audioEq.aslcControl.aslc_db);

	// 		audioCtrl.controlEQ(sysCtrls->audioEq.eqControl.pre_gain, sysCtrls->audioEq.eqControl.bands, sysCtrls->audioEq.eqControl.bandfreqs,
	// 							sysCtrls->audioEq.eqControl.bandgains, sysCtrls->audioEq.eqControl.bandQ, sysCtrls->audioEq.eqControl.band_types,
	// 							sysCtrls->audioEq.eqControl.enable, sysCtrls->audioEq.eqControl.band_enable);
	// 	}
	// }
	// else if (type.empty()) {
	// 	ret = systemSetup(SYSTEM_CONTROL_AMIC, sysCtrls) + systemSetup(SYSTEM_CONTROL_MASTER_PLAYBACK, sysCtrls) + systemSetup(SYSTEM_CONTROL_AEC, sysCtrls) +
	// 		  systemSetup(SYSTEM_CONTROL_SPEAKER_GAIN, sysCtrls);
	// 	ret += systemSetup(SYSTEM_CONTROL_AUDIO_EQ, sysCtrls);
	// }
	// else {
	// 	APP_DBG("type failed\n");
	// 	ret = -1;
	// }
	return ret;
}

void *pollingButtonRstCalls(void *args) {
	(void)args;
	APP_DBG("POLLING BUTTON \n");
	while (1) {
		// appBtnPolling();
		usleep(10000);
	}

	return (void *)0;
}
