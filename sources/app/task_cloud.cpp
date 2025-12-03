#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <ctime>
#include <memory>
#include <string.h>
#include <iostream>

#include "ak.h"
#include "timer.h"

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "mqtt.h"
#include "utils.h"

#include "parser_json.h"

#include "task_list.h"
#include "network_manager.h"

#define MQTT_FAIL_COUNT_MAX (360)	 // retry 30min => set MQTT default config

using namespace std;

extern std::atomic<bool> networkStatus;
q_msg_t gw_task_cloud_mailbox;
static unique_ptr<mqtt> mospp;

// static bool mqttInitialized		 = false;
static int mqttFailCnt			 = 0;
static FCA_MQTT_CONN_S mqttService = {0};
static mqttTopicCfg_t mqttTopics;

string getMqttConnectStatus();

static void process_mqtt_msg(unsigned char *pload, int len);

void *gw_task_cloud_entry(void *) {
	wait_all_tasks_started();
	APP_DBG_MQTT("[STARTED] gw_task_cloud_entry\n");

	ak_msg_t *msg = AK_MSG_NULL;

	string serial = fca_getSerialInfo();
	sprintf(mqttTopics.topicStatus, "ipc/fss/%s/%s", serial.c_str(), "status");
	sprintf(mqttTopics.topicRequest, "ipc/fss/%s/%s", serial.c_str(), "reqcfg");
	sprintf(mqttTopics.topicResponse, "ipc/fss/%s/%s", serial.c_str(), "rescfg");
	sprintf(mqttTopics.topicSignalingRequest, "ipc/fss/%s/%s", serial.c_str(), "request/signaling");
	sprintf(mqttTopics.topicSignalingResponse, "ipc/fss/%s/%s", serial.c_str(), "response/signaling");
	sprintf(mqttTopics.topicAlarm, "ipc/fss/%s/%s", serial.c_str(), "alarm");

	if (fca_configGetMQTT(&mqttService) == APP_CONFIG_SUCCESS) {
		task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ);
	}
	// strcpy(mqttService.host, "beta-broker-mqtt.fcam.vn");
	// strcpy(mqttService.clientID, "ipc-ipc");
	// strcpy(mqttService.username, deviceSerialNumber.c_str());
	// strcpy(mqttService.password, deviceSerialNumber.c_str());
	// mqttService.port = 8883;
	// mqttService.keepAlive = 30;
	// mqttService.QOS = 1;
	// mqttService.retain = true;
	// mqttService.bTLSEnable = true;
	// strcpy(mqttService.TLSVersion, "tlsv1.2");
	// strcpy(mqttService.protocol, "mqtt\/tls");
	// mqttService.bEnable = true;
	// task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ);

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_CLOUD_ID);

		switch (msg->header->sig) {
		case GW_CLOUD_DATA_COMMUNICATION: {
			APP_DBG_SIG("GW_CLOUD_DATA_COMMUNICATION\n");
			uint8_t *data = (uint8_t *)msg->header->payload;
			int len	= (int)msg->header->len;
			process_mqtt_msg(data, len);
		}
		break;

		case GW_CLOUD_MQTT_TRY_CONNECT_REQ: {
			APP_DBG_SIG("GW_CLOUD_MQTT_TRY_CONNECT_REQ\n");

			timer_remove_attr(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_CHECK_MQTT_LOOP_REQ);
			timer_remove_attr(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ);
			timer_remove_attr(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_CHECK_CONNECT_STATUS_REQ);
			timer_remove_attr(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_CHECK_SUB_TOPIC_REQ);

			APP_PRINT("MQTT CONNECTION\r\n");
			APP_PRINT("\tHost: %s\r\n", mqttService.host);
			APP_PRINT("\tUsername: %s\r\n", mqttService.username);
			APP_PRINT("\tPassword: %s\r\n", mqttService.password);
			APP_PRINT("\tCERTIFICATE Path: %s\r\n", caSslPath.c_str());

			mospp.reset();
			usleep(500 * 1000);
			mospp = make_unique<mqtt>(&mqttTopics, &mqttService);
			int ecode = mospp->tryConnect(caSslPath.c_str());
			APP_DBG_MQTT("MQTT_ECODE: %d\n", ecode);
			if (ecode != MOSQ_ERR_SUCCESS) {
				timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ, GW_CLOUD_MQTT_TRY_CONNECT_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
			}
			else {
				timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_CHECK_CONNECT_STATUS_REQ, GW_CLOUD_MQTT_CHECK_CONNECT_STATUS_INTERVAL, TIMER_ONE_SHOT);
			}

			// /* check mqtt connect failed */
			// if (networkStatus.load()) {
			// 	if (++mqttFailCnt > MQTT_FAIL_COUNT_MAX) {
			// 		APP_DBG_MQTT("set mqtt default server\n");
			// 		mqttFailCnt = 0;

			// 		int ntpSt = getNtpStatus();
			// 		if (ntpSt == 1) {
			// 			APP_DBG("time sync ok\n");
			// 			if (caSslPath.find(FCA_USER_CONF_PATH) != string::npos) {
			// 				APP_DBG("set default CA file\n");
			// 				caSslPath = FCA_DFAUL_CONF_PATH "/" FCA_NETWORK_CA_FILE;
			// 				task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_INIT_REQ);
			// 				systemCmd("rm -f %s/%s", FCA_USER_CONF_PATH, FCA_NETWORK_CA_FILE);
			// 			}
			// 			else {
			// 				APP_DBG("try get CA file\n");
			// 				LOG_FILE_INFO("try get CA file\n");
			// 				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_GET_CA_FILE_REQ);
			// 			}

			// 			/* set default mqtt config */
			// 			systemCmd("rm -f %s/%s", FCA_USER_CONF_PATH, FCA_NETWORK_MQTT_FILE);
			// 			fca_configGetMQTT(&mqttService);
			// 		}
			// 	}
			// }
			// else {
			// 	APP_DBG_MQTT("network failed\n");
			// 	mqttFailCnt = 0;
			// }
			APP_DBG_MQTT("mqtt retry connect counter: %d\n", mqttFailCnt);
		}
		break;

		case GW_CLOUD_MQTT_CHECK_CONNECT_STATUS_REQ: {
			APP_DBG_SIG("GW_CLOUD_MQTT_CHECK_CONNECT_STATUS_REQ\n");
			if (mospp) {
				if (!mospp->isConnected()) {
					task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ);
				}
				else {
					APP_DBG_MQTT("mqtt connect ok, reset retry counter\n");
					mqttFailCnt = 0;
					timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_CHECK_MQTT_LOOP_REQ, GW_CLOUD_MQTT_CHECK_LOOP_TIMEOUT_INTERVAL, TIMER_PERIODIC);
				}
			}
		} break;

		case GW_CLOUD_SET_MQTT_CONFIG_REQ: {
			APP_DBG_SIG("GW_CLOUD_SET_MQTT_CONFIG_REQ\n");
			bool cfgDiff = false;
			int rc		 = APP_CONFIG_ERROR_DATA_INVALID;
			try {
				json mqttCfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				FCA_MQTT_CONN_S paramTmp;
				memset(&paramTmp, 0, sizeof(paramTmp));
				if (fca_jsonGetNetMQTT(mqttCfgJs, &paramTmp)) {
					if (memcmp(&mqttService, &paramTmp, sizeof(paramTmp)) != 0) {
						rc = fca_configSetMQTT(&paramTmp);
						if (rc == APP_CONFIG_SUCCESS) {
							APP_DBG_MQTT("set new mqtt config success\n");
							cfgDiff = true;
							memcpy(&mqttService, &paramTmp, sizeof(mqttService));
						}
					}
					else {
						APP_DBG_MQTT("[WAR] mqtt config same data\n");
						rc = APP_CONFIG_SUCCESS;
					}
				}
			}
			catch (const exception &error) {
				APP_DBG_MQTT("%s\n", error.what());
				rc = APP_CONFIG_ERROR_ANOTHER;
			}

			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_MQTT, rc)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}

			/* try connect to new server mqtt */
			if (rc == APP_CONFIG_SUCCESS && cfgDiff) {
				offlineEventManager.addEvent({CHANGE_SERVER, (uint32_t)time(nullptr), TASK_PRI_LEVEL_1});
				task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ);
			}
		} break;

		case GW_CLOUD_GET_MQTT_CONFIG_REQ: {
			APP_DBG_SIG("GW_CLOUD_GET_MQTT_CONFIG_REQ\n");
			json dataJs = json::object(), resJs;
			fca_jsonSetNetMQTT(dataJs, &mqttService);
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_MQTT, APP_CONFIG_SUCCESS)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		}
		break;

		case GW_CLOUD_CAMERA_STATUS_RES: {
			APP_DBG_SIG("GW_CLOUD_CAMERA_STATUS_RES\n");
			try {
				json resJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				/* response to mqtt server */
				if (mospp) {
					mospp->publishStatus(resJs, 1);	   // send with Qos: 1
				}
			}
			catch (const exception &error) {
				APP_DBG_MQTT("%s\n", error.what());
			}
		} break;

		case GW_CLOUD_CAMERA_CONFIG_RES: {
			APP_DBG_SIG("GW_CLOUD_CAMERA_CONFIG_RES\n");
			try {
				json resJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				/* response to mqtt server */
				if (mospp) {
					mospp->publishResponse(resJs, 1);	 // send with Qos: 1
				}
			}
			catch (const exception &error) {
				APP_DBG_MQTT("%s\n", error.what());
			}
		} break;

		case GW_CLOUD_CAMERA_ALARM_RES: {
			APP_DBG_SIG("GW_CLOUD_CAMERA_ALARM_RES\n");
			try {
				json resJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				/* response to mqtt server */
				if (mospp) {
					mospp->publishAlarm(resJs, 1);	  // send with Qos: 1, retain: false
				}
			}
			catch (const exception &error) {
				APP_DBG_MQTT("%s\n", error.what());
			}
		} break;

		case GW_CLOUD_SIGNALING_MQTT_RES: {
			APP_DBG_SIG("GW_CLOUD_SIGNALING_MQTT_RES\n");
			try {
				json revJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				json resJs = {
					{"Method",	   "ACT"				},
					 {"MessageType", "Signaling"		},
					   {"Serial",	  fca_getSerialInfo()},
					{"Data",		 revJs["Data"]	  },
					 {"Result",		revJs["Result"]	   },
					   {"Timestamp",	 time(nullptr)	  }
				   };

				APP_DBG_RTC("signaling report: %s\n", resJs.dump().data());
				/* response to mqtt server */
				int qos = revJs["Qos"].get<int>();
				if (mospp) {
					mospp->publishSignalingResponse(resJs, qos);
				}
			}
			catch (const exception &error) {
				APP_DBG_RTC("%s\n", error.what());
			}
		} break;

		case GW_CLOUD_MESSAGE_LENGTH_OUT_OF_RANGE_REP: {
			APP_DBG_SIG("GW_CLOUD_MESSAGE_LENGTH_OUT_OF_RANGE_REP\n");

		} break;

		case GW_CLOUD_CHECK_BROKER_STATUS_REQ: {
			APP_DBG_SIG("GW_CLOUD_CHECK_BROKER_STATUS_REQ\n");
			try {
				json revJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				if (mospp) {
					json localSt = json::object();
					if (fca_configGetUpgradeStatus(localSt) == APP_CONFIG_SUCCESS) {
						localSt["Data"]["Status"] = getMqttConnectStatus();
						if (localSt["Data"]["Status"] == revJs["Status"] && localSt["Data"]["UpgradeStatus"] == revJs["UpgradeStatus"]) {
							APP_DBG_MQTT("broker has updated the latest status\n");
							break;
						}
						APP_DBG_MQTT("retry update status to broker\n");
					
						task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_GEN_MQTT_STATUS_REQ);
					}
				}
			}
			catch (const exception &error) {
				APP_DBG_MQTT("%s\n", error.what());
			}
		} break;

		case GW_CLOUD_GEN_MQTT_STATUS_REQ: {
			APP_DBG_SIG("GW_CLOUD_GEN_MQTT_STATUS_REQ\n");
			if (mospp && mospp->isConnected()) {
				time_t rawtime = time(nullptr);
				json stt_msg;
				fca_configGetDeviceInfoStr(stt_msg);
				fca_configGetUpgradeStatus(stt_msg);
				stt_msg["Data"]["Status"] = getMqttConnectStatus();
				stt_msg["Timestamps"]	  = rawtime;

				/* get network status */
				json netJs		  = json::object();
				netJs["PublicIP"] = camIpPublic;

				/* get wifi status */
				FCA_NET_WIFI_S wifiConf;
				memset(&wifiConf, 0, sizeof(wifiConf));
				if (fca_configGetWifi(&wifiConf) == APP_CONFIG_SUCCESS) {
					netJs["Wifi"]["SSID"] = string(wifiConf.ssid);
				}
				else {
					netJs["Wifi"]["SSID"] = "Unknown";
				}

				if (systemCmd("wpa_cli -i %s status | grep -q 'wpa_state=COMPLETED'", FCA_NET_WIFI_STA_IF_NAME) == 0) {
					netJs["Wifi"]["Status"] = "Connected";
					try {
						string retStr = systemStrCmd("wpa_cli -i %s signal_poll", FCA_NET_WIFI_STA_IF_NAME);
						if (retStr.find("RSSI") == string::npos) {
							netJs["Wifi"]["Signal"] = "Unknown";
						}
						else {
							json signalInfo = json::object();
							std::istringstream stream(retStr);
							std::string line;
							while (std::getline(stream, line)) {
								size_t pos = line.find('=');
								if (pos != std::string::npos) {
									std::string key	  = line.substr(0, pos);
									std::string value = line.substr(pos + 1);
									signalInfo[key]	  = value;
								}
							}
							netJs["Wifi"]["Signal"] = signalInfo;
						}
						retStr.clear();
						retStr = systemStrCmd("route -n | awk '/^0.0.0.0/ {print $2}'");
						if (isIpValid(retStr)) {
							netJs["Gateway"]["IP"] = retStr;
							retStr.clear();
							retStr					= systemStrCmd("awk -v gw=%s '$1 == gw {print $4}' /proc/net/arp", netJs["Gateway"]["IP"].get<string>().c_str());
							netJs["Gateway"]["MAC"] = retStr;
						}
						else {
							netJs["Gateway"]["IP"]	= "Unknown";
							netJs["Gateway"]["MAC"] = "Unknown";
						}
					}
					catch (const std::exception &e) {
						APP_DBG_CMD("error: %s\n", e.what());
					}
				}
				else {
					netJs["Wifi"]["Status"] = "Disconnected";
				}
				APP_DBG_MQTT("net json: %s\n", netJs.dump().data());
				stt_msg["Data"]["Network"] = netJs;

				/* get offline detect */
				json reasonsOfflineJs = json::array();
				offlineEventManager.getErrorReason(reasonsOfflineJs);
				stt_msg["Data"]["ErrorReason"] = reasonsOfflineJs;
				APP_DBG_MQTT("offline detect json: %s\n", reasonsOfflineJs.dump().data());
				APP_DBG_MQTT("status json: %s\n", stt_msg.dump().data());

				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_STATUS_RES, (uint8_t *)stt_msg.dump().data(), stt_msg.dump().length() + 1);
			}
		} break;

		case GW_CLOUD_MQTT_SUB_TOPIC_REP: {
			APP_DBG_SIG("GW_CLOUD_MQTT_SUB_TOPIC_REP\n");
			int mid = *((int *)msg->header->payload);
			if (mospp) {
				mospp->removeSubTopicMap(mid);
			}
		} break;

		case GW_CLOUD_MQTT_CHECK_SUB_TOPIC_REQ: {
			APP_DBG_SIG("GW_CLOUD_MQTT_CHECK_SUB_TOPIC_REQ\n");
			if (mospp) {
				mospp->retrySubTopics();
			}
		} break;

		case GW_CLOUD_MQTT_CHECK_MQTT_LOOP_REQ: {
			APP_DBG_SIG("GW_CLOUD_MQTT_CHECK_MQTT_LOOP_REQ\n");
			if (mospp && mospp->isConnected()) {
				string cmd = "ps -T | grep \"mosquitto loop\" | grep -v grep > /dev/null 2>&1";
				if (systemCmd(cmd.c_str()) == 0) {	  // ok
					APP_DBG_MQTT("{mosquitto loop} is running\n");
					mospp->checkRunLoopCounter = 0;
				}
				else if (++mospp->checkRunLoopCounter <= 6) {	 // 30s reboot
					APP_DBG_MQTT("checkRunLoopCounter retry: %d\n", mospp->checkRunLoopCounter);
				}
				else {
					LOG_FILE_ERROR("{mosquitto loop} is not running, reboot\n");
					task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_REBOOT_REQ);
				}
			}
		} break;

		case GW_CLOUD_MQTT_RELEASE_RESOURCES_REQ: {
			APP_DBG_SIG("GW_CLOUD_MQTT_RELEASE_RESOURCES_REQ\n");
			if (mospp) {
				// unscribe topic status
				string topicSt = mqttTopics.topicStatus;
				mospp->unsubcribePerform(topicSt);

				// force send msg disconnect
				json stt_msg;
				time_t rawtime = time(nullptr);
				fca_configGetDeviceInfoStr(stt_msg);
				fca_configGetUpgradeStatus(stt_msg);
				stt_msg["Data"]["Status"] = "Disconnected";
				stt_msg["Timestamps"]	  = rawtime;

				/* get network status */
				json netJs		  = json::object();
				netJs["PublicIP"] = camIpPublic;

				/* get offline detect */
				json reasonsOfflineJs = json::array();
				offlineEventManager.getErrorReason(reasonsOfflineJs);
				stt_msg["Data"]["ErrorReason"] = reasonsOfflineJs;
				APP_DBG_MQTT("offline detect json: %s\n", reasonsOfflineJs.dump().data());
				APP_DBG_MQTT("status json: %s\n", stt_msg.dump().data());

				mospp->publishStatus(stt_msg, 1);	 // send with Qos: 1
				usleep(500 * 1000);
				mospp.reset();
			}
		} break;

		case GW_CLOUD_WATCHDOG_PING_REQ: {
			// APP_DBG_SIG("GW_CLOUD_WATCHDOG_PING_REQ\n");
			task_post_pure_msg(GW_TASK_CLOUD_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
		} break;

		default:
			break;
		}

		/* free message */
		ak_msg_free(msg);
	}

	return (void *)0;
}

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/
void process_mqtt_msg(unsigned char *pload, int len) {
	APP_DBG_MQTT("HELLO FROM BROKER MQTT\n");
	try {
		// parse json string
		json recv_js = json::parse(string((const char *)pload, len));
		// APP_DBG_MQTT("recv_js:%s\n", recv_js.dump(4).data());

		/* handler cmd msg
		{
			"MessageType": "Signaling",
			"Serial": "test-cam-fss",
			"Method": "ACT",
			"Timestamp": 1604311420,
			"Data": {<payload here>}
		}*/
		string method  = recv_js.contains("Method") ? recv_js["Method"] : "";
		string msgType = recv_js["MessageType"];
		json data	   = recv_js["Data"];

		if (msgType == MESSAGE_TYPE_DEVINFO) {
			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CHECK_BROKER_STATUS_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
			return;
		}

#ifdef RELEASE
		int msgTs	   = recv_js["Timestamp"];
		time_t rawtime = time(nullptr);
		int realtime   = stoi(to_string(rawtime));
		/* Parse local web command */
		if ((msgTs >= realtime && msgTs < realtime + 180) || (msgTs <= realtime && msgTs + 180 > realtime)) {
#endif
			if (method == "ACT") {
				if (msgType == MESSAGE_TYPE_SIGNALING) {
					task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_SIGNALING_MQTT_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_SNAPSHOT) {
					task_post_dynamic_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_SNAPSHOT_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else {
					APP_DBG_MQTT("MessageType not found\n");
				}
			}
			else if (method == "SET") {
				if (msgType == MESSAGE_TYPE_MQTT) {
					task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_SET_MQTT_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_RTMP) {
					// #ifdef FEATURE_RTMP
					// task_post_dynamic_msg(GW_TASK_RTMP_ID, GW_RTMP_SET_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
					// #endif
				}
				else if (msgType == MESSAGE_TYPE_MOTION) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_DETECT_ID, GW_DETECT_SET_MOTION_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_SOUND_EVENT) {
					// #ifdef FEATURE_BBCRY
					// task_post_dynamic_msg(GW_TASK_DETECT_ID, GW_DETECT_SET_SOUND_EVENT_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
					// #endif
				}
				else if (msgType == MESSAGE_TYPE_ENCODE) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_AV_ID, GW_AV_SET_ENCODE_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_WATERMARK) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_AV_ID, GW_AV_SET_WATERMARK_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_RESET) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_CONFIG_ID, GW_CONFIG_SET_RESET_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_CAMERA_PARAM) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_AV_ID, GW_AV_SET_CAMERAPARAM_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_WIFI) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_MQTT_SET_CONF_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_ETHERNET) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_ETHERNET_MQTT_SET_CONF_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_S3) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_SET_S3_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_BUCKET_SETTING) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_SET_BUCKET_SETTING_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_UPGRADE) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_FW_ID, GW_FW_GET_URL_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_REBOOT) {
					// Post to task
					task_post_pure_msg(GW_TASK_CLOUD_ID, GW_TASK_SYS_ID, GW_SYS_REBOOT_REQ);
				}
				else if (msgType == MESSAGE_TYPE_RECORD) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_RECORD_ID, GW_RECORD_SET_RECORD_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_UPGRADE_STATUS) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_CONFIG_ID, GW_CONFIG_SET_UPGRADE_STATUS_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_STUN_SERVER || msgType == MESSAGE_TYPE_TURN_SERVER || msgType == MESSAGE_TYPE_WEB_SOCKET_SERVER) {
					// Post to task
					json dataSend = {
						{"Type", msgType},
						   {"Data", data	}
					 };
					task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_SET_SIGNLING_SERVER_REQ, (uint8_t *)dataSend.dump().data(), dataSend.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_CONTROL_LED_INDICATOR) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_CONFIG_ID, GW_CONFIG_CLOUD_SET_LED_INDICATOR_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_OWNER_STATUS) {
					task_post_dynamic_msg(GW_TASK_CONFIG_ID, GW_CONFIG_SET_OWNER_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_ALARM_CONTROL) {
					task_post_dynamic_msg(GW_TASK_DETECT_ID, GW_DETECT_SET_ALARM_CONTROL_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_LOGIN_INFO) {
					task_post_dynamic_msg(GW_TASK_SYS_ID, GW_SYS_SET_LOGIN_INFO_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_SYSTEM_CONTROLS) {
					task_post_dynamic_msg(GW_TASK_SYS_ID, GW_SYS_SET_SYSTEM_CONTROLS_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_P2P) {
					task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_SET_CLIENT_MAX_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_TIMEZONE) {
					task_post_dynamic_msg(GW_TASK_CONFIG_ID, GW_CONFIG_SET_TIMEZONE_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_STORAGE_FORMAT) {
					// Post to task
					task_post_dynamic_msg(GW_TASK_RECORD_ID, GW_RECORD_FORMAT_CARD_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else {
					APP_DBG_MQTT("MessageType not found\n");
				}
			}
			else if (method == "GET") {
				APP_DBG_MQTT("MQTT get msg type [%s]\n", msgType.data());
				if (msgType == MESSAGE_TYPE_MQTT) {
					task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_GET_MQTT_CONFIG_REQ);
				}
				else if (msgType == MESSAGE_TYPE_RTMP) {
					// #ifdef FEATURE_RTMP
					// task_post_pure_msg(GW_TASK_RTMP_ID, GW_RTMP_GET_CONFIG_REQ);
					// #endif
				}
				else if (msgType == MESSAGE_TYPE_MOTION) {
					// Post to task
					task_post_pure_msg(GW_TASK_DETECT_ID, GW_DETECT_GET_MOTION_CONFIG_REQ);
				}
				else if (msgType == MESSAGE_TYPE_SOUND_EVENT) {
					// #ifdef FEATURE_BBCRY
					// task_post_dynamic_msg(GW_TASK_DETECT_ID, GW_DETECT_GET_SOUND_EVENT_CONFIG_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
					// #endif
				}
				else if (msgType == MESSAGE_TYPE_ENCODE) {
					// Post to task
					task_post_pure_msg(GW_TASK_AV_ID, GW_AV_GET_ENCODE_REQ);
				}
				else if (msgType == MESSAGE_TYPE_WATERMARK) {
					// Post to task
					task_post_pure_msg(GW_TASK_AV_ID, GW_AV_GET_WATERMARK_REQ);
				}
				else if (msgType == MESSAGE_TYPE_CAMERA_PARAM) {
					// Post to task
					task_post_pure_msg(GW_TASK_AV_ID, GW_AV_GET_CAMERAPARAM_REQ);
				}
				else if (msgType == MESSAGE_TYPE_WIFI) {
					task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_MQTT_GET_CONF_REQ);
				}
				else if (msgType == MESSAGE_TYPE_ETHERNET) {
					task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_ETHERNET_MQTT_GET_CONF_REQ);
				}
				else if (msgType == MESSAGE_TYPE_S3) {
					// Post to task
					task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_GET_S3_CONFIG_REQ);
				}
				else if (msgType == MESSAGE_TYPE_BUCKET_SETTING) {
					// Post to task
					task_post_pure_msg(GW_TASK_UPLOAD_ID, GW_UPLOAD_GET_BUCKET_SETTING_CONFIG_REQ);
				}
				else if (msgType == MESSAGE_TYPE_NETWORK_INFO) {
					// Post to task
					task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_MQTT_GET_NETWORK_INFO_REQ);
				}
				else if (msgType == MESSAGE_TYPE_SYSTEM_INFO) {
					// Post to task
					task_post_pure_msg(GW_TASK_CONFIG_ID, GW_CONFIG_GET_SYSINFO_CONFIG_REQ);
				}
				else if (msgType == MESSAGE_TYPE_RECORD) {
					// Post to task
					task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_GET_RECORD_REQ);
				}
				else if (msgType == MESSAGE_TYPE_STUN_SERVER || msgType == MESSAGE_TYPE_TURN_SERVER) {
					task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_GET_SIGNLING_SERVER_REQ, (uint8_t *)msgType.data(), msgType.length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_NETWORKAP) {
					// Post to task
					task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_MQTT_GET_LIST_AP_REQ);
				}
				else if (msgType == MESSAGE_TYPE_CONTROL_LED_INDICATOR) {
					// Post to task
					task_post_pure_msg(GW_TASK_CONFIG_ID, GW_CONFIG_CLOUD_GET_LED_INDICATOR_REQ);
				}
				else if (msgType == MESSAGE_TYPE_ALARM_CONTROL) {
					task_post_dynamic_msg(GW_TASK_DETECT_ID, GW_DETECT_GET_ALARM_CONTROL_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_LOGIN_INFO) {
					task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_GET_LOGIN_INFO_REQ);
				}
				else if (msgType == MESSAGE_TYPE_RUNNING_CMDS) {
					task_post_dynamic_msg(GW_TASK_SYS_ID, GW_SYS_RUNNING_CMDS_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else if (msgType == MESSAGE_TYPE_SYSTEM_CONTROLS) {
					task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_GET_SYSTEM_CONTROLS_REQ);
				}
				else if (msgType == MESSAGE_TYPE_P2P) {
					task_post_pure_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_GET_CLIENT_MAX_REQ);
				}
				else if (msgType == MESSAGE_TYPE_P2P_INFO) {
					task_post_pure_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_GET_CURRENT_CLIENTS_TOTAL_REQ);
				}
				else if (msgType == MESSAGE_TYPE_TIMEZONE) {
					task_post_pure_msg(GW_TASK_CONFIG_ID, GW_CONFIG_GET_TIMEZONE_REQ);
				}
				else if (msgType == MESSAGE_TYPE_STORAGE_INFO) {
					task_post_dynamic_msg(GW_TASK_RECORD_ID, GW_RECORD_GET_CAPACITY_REQ, (uint8_t *)data.dump().data(), data.dump().length() + 1);
				}
				else {
					APP_DBG_MQTT("MessageType not found\n");
				}
			}
			else {
				APP_DBG_MQTT("Method not found\n");
			}
#ifdef RELEASE
		}
#endif
	}
	catch (const exception &e) {
		(void)e;
		APP_DBG_MQTT("json::parse()\n");
		APP_DBG_MQTT("msg = %s\n", pload);
	}
	return;
}

string getMqttConnectStatus() {
	return mospp ? mospp->isConnected() ? "Connected" : "Disconnected" : "Unknown";
}
