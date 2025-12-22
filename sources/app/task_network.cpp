#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "task_list.h"
#include "task_network.h"

#include "wifi.h"
#include "mqtt.h"
#include "parser_json.h"
#include "utils.h"
#if SUPPORT_ETH
#include "eth.h"
#endif
#include "fca_bluetooth.h"
#include "network_manager.h"

#include "nettime.hh"
#include "streamsocket.hh"
#include "network_data_encrypt.h"

#define TAG							   "[network] "
#define NETWORK_PING_INTERNET_INTERVAL (5)
#define NETWORK_DOMAIN_SERVER		   "alive.fcam.vn"

q_msg_t gw_task_network_mailbox;
std::atomic<bool> networkStatus(false);
OfflineEventManager offlineEventManager;

#if SUPPORT_ETH
int lanPortLastState		= FCA_NET_ETH_STATE_UNKNOWN;
static fca_netEth_t ethSettings = {0};
static int netVerifyConfigFromMQTT(fca_netEth_t *eth);
#endif

static FCA_NET_WIFI_S wiFiSettings = {0};
int netSetupState = NET_SETUP_STATE_NONE;
bool isFistTimeStartModeNetwork;

static bool bSelectedBLE;
static SocketStream hostapd;
static volatile bool isInternetConnected = false;

static void *pingInternetCalls(void *args);
static bool netGetConnectToRouter(void);
static int netVerifyConfigFromMQTT(FCA_NET_WIFI_S *wifi);
static void fca_netStaticStart(const char *ifname, const fca_netConf_t *info);
static void fca_netStaticStop(const char *ifname);
static void setNetVoice(int &st);
static void removeCheckErrorNetworkTimers();

void *gw_task_network_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;

	wait_all_tasks_started();

	APP_DBG_NET(TAG "[STARTED] gw_task_network_entry\n");

	ntpd.onDoUpdateSuccess([](struct tm *local) {
		char datetime[64] = {0}, cmds[128] = {0};
		static bool isFirstTime = true;
		if (isFirstTime) {
			isFirstTime = false;
			ntpd.setTimePeriodicUpdate(5000);
			task_post_pure_msg(GW_TASK_RECORD_ID, GW_RECORD_INIT);
			task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_INIT_REQ);
			SYSI("Network time has updated: %s\r\n", datetime);
		}
		strftime(datetime, 64, "%Y-%m-%d %H:%M:%S", local);
		sprintf(cmds, "busybox date -s \"%s\" > /dev/null", datetime);
		system(cmds);
		APP_PRINT("Network time has updated: %s\r\n", datetime);
	});

	ntpd.onDoUpdateFailure([](char *domain, char* err) {
		APP_WARN("Time server %s update time failed -> %s", domain, err);
	});

	task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_NETWORK_INIT_REQ);

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_NETWORK_ID);

		switch (msg->header->sig) {
		case GW_NET_NETWORK_INIT_REQ: {
			APP_DBG_SIG("GW_NET_NETWORK_INIT_REQ\n");
			/* get state port ethernet */
			#if SUPPORT_ETH
			ethSettings.enable = true;
			ethSettings.dhcp   = true;
			fca_configGetEthernet(&ethSettings);
			task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_ETHERNET_CONTROL_WITH_CONFIG_REQ, (uint8_t *)&ethSettings, sizeof(ethSettings));

			/* timer periodic detect plug/unplug port ethernet */
			timer_set(GW_TASK_NETWORK_ID, GW_NET_GET_STATE_LAN_REQ, GW_NET_CHECK_STATE_PLUG_LAN_INTERVAL, TIMER_ONE_SHOT);
			#endif

			/* Select mode wiFi STA or AP */
			if (fca_configGetWifi(&wiFiSettings) != APP_CONFIG_SUCCESS) {
				/* Select mode AP or BLE, priority BLE more */
				// #if FEATURE_BLE
				// bSelectedBLE = true;
				// task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTBLE);
				// #else /* Not support BLE */
				// bSelectedBLE = false;
				// task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTAPD);
				// #endif
				if (selectedBluetoothMode) {
					task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTBLE);
				}
				else {
					task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTAPD);
				}
			}
			else {
				task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&wiFiSettings, sizeof(wiFiSettings));
			}

			/* Run NTP services */
			ntpd.begin();

			/* Create thread periodic ping to internet */
			pthread_t tPingId = 0;
			pthread_create(&tPingId, NULL, pingInternetCalls, NULL);
		}
		break;

		case GW_NET_WIFI_MQTT_SET_CONF_REQ: {
			APP_DBG_SIG("GW_NET_WIFI_MQTT_SET_CONF_REQ\n");
			int ret				  = APP_CONFIG_ERROR_DATA_INVALID;
			FCA_NET_WIFI_S tmpConf = {0};
			try {
				json wifiCfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				APP_DBG_NET("%s\n", wifiCfgJs.dump().data());
				if (fca_jsonGetWifi(wifiCfgJs, &tmpConf) && netVerifyConfigFromMQTT(&tmpConf) == APP_CONFIG_SUCCESS && fca_configSetWifi(&tmpConf) == APP_CONFIG_SUCCESS) {
					APP_DBG_NET(TAG "wifi config diff\n");
					netSetupState = NET_SETUP_STATE_CONFIG;
					task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&tmpConf, sizeof(tmpConf));
					ret = APP_CONFIG_SUCCESS;
				}
			}
			catch (const exception &error) {
				APP_DBG_NET(TAG "%s\n", error.what());
				ret = APP_CONFIG_ERROR_ANOTHER;
			}

			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_WIFI, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_NET_WIFI_MQTT_GET_CONF_REQ: {
			APP_DBG_SIG("GW_NET_WIFI_MQTT_GET_CONF_REQ\n");
			int ret		= APP_CONFIG_SUCCESS;
			json dataJs = json::object();
			if (!fca_jsonSetWifi(dataJs, &wiFiSettings)) {
				APP_DBG_NET(TAG "data wifi config invalid\n");
				ret = APP_CONFIG_ERROR_DATA_INVALID;
			}
			json resJs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_WIFI, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		}
		break;

		case GW_NET_WIFI_DO_CONNECT: {
			APP_DBG_SIG("GW_NET_WIFI_DO_CONNECT\n");

			timer_remove_attr(GW_TASK_NETWORK_ID, GW_NET_WIFI_STA_CONNECT_TO_ROUTER);

			memset(&wiFiSettings, 0, sizeof(wiFiSettings));
			get_data_dynamic_msg(msg, (uint8_t*)&wiFiSettings, sizeof(wiFiSettings));

			hostapd.doClose();
			fca_bluetooth_close();

			int rc = fca_wifi_connect(FCA_NET_WIFI_STA_IF_NAME, wiFiSettings.ssid, wiFiSettings.keys);
			if (rc != 0) {
				SYSW("APIs WiFi station connects %s failure return %d\r\n", wiFiSettings.ssid, rc);
				netSetupState = NET_SETUP_STATE_NONE;	 // prevent voice
				break;
			}

			ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, Indicator::STATUS::OK);
			audioHelpers.notifyWiFiIsConnecting();
		}
		break;

		case GW_NET_WIFI_MQTT_GET_LIST_AP_REQ: {
			APP_DBG_SIG("GW_NET_WIFI_MQTT_GET_LIST_AP_REQ\n");
			uint8_t i;
			fca_netWifiStaScan_t list = {0};
			fca_netWifiScanHostAP(FCA_NET_WIFI_STA_IF_NAME, &list);

			for (i = 0; i < list.number; i++) {
				APP_DBG_NET(TAG "%s\t%d\t%d\t%s\t%s\n", list.item[i].mac, list.item[i].freq, list.item[i].signal, list.item[i].flag, list.item[i].ssid);
			}

			json dataJs = json::object();
			int ret		= APP_CONFIG_ERROR_DATA_INVALID;
			if (fca_jsonSetNetAP(dataJs, &list)) {
				ret = APP_CONFIG_SUCCESS;
			}

			json resJs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_NETWORKAP, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_NET_MQTT_GET_NETWORK_INFO_REQ: {
			APP_DBG_SIG("GW_NET_MQTT_GET_NETWORK_INFO_REQ\n");

			int ret		= APP_CONFIG_SUCCESS;
			json dataJs = json::object(), resJs;

			string ip, mac;
			fca_netIPAddress_t lan = {0}, wifi = {0};
			#if SUPPORT_ETH
			get_net_info(FCA_NET_WIRED_IF_NAME, ip, mac);
			APP_DBG_NET(TAG "lan [%s] [%s]\n", ip.data(), mac.data());
			safe_strcpy(lan.hostIP, ip.data(), sizeof(lan.hostIP));
			safe_strcpy(lan.mac, mac.data(), sizeof(lan.mac));
			#endif
			get_net_info(FCA_NET_WIFI_STA_IF_NAME, ip, mac);
			APP_DBG_NET(TAG "wifi [%s] [%s]\n", ip.data(), mac.data());
			safe_strcpy(wifi.hostIP, ip.data(), sizeof(wifi.hostIP));
			safe_strcpy(wifi.mac, mac.data(), sizeof(wifi.mac));

			if (!fca_jsonSetNetInfo(dataJs, &lan, &wifi)) {
				APP_DBG_NET(TAG "wifi info config invalid\n");
				ret = APP_CONFIG_ERROR_DATA_INVALID;
			}

			/* respone to MQTT */
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_NETWORK_INFO, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		// case GW_NET_RESET_BUTTON_REQ: {
		// 	APP_DBG_SIG("GW_NET_RESET_BUTTON_REQ\n");
		// 	fca_resetButtonCounter();
		// } break;

		case GW_NET_SWITCH_AP_MODE_START_REQ: {
			APP_DBG_SIG("GW_NET_SWITCH_AP_MODE_START_REQ\n");

			if (fca_configGetWifi(&wiFiSettings) == APP_CONFIG_SUCCESS) {
				APP_DBG_NET("Have config wifi don't start ap mode\n");
				break;
			}

			// if (isLockSwitchApMode) {
			// 	APP_DBG("Switch ap mode is locked\n");
			// 	break;
			// }

			// if (!bSelectedBLE) {
			// 	bSelectedBLE = true;
			// 	task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTBLE);
			// }
			// else {
			// 	bSelectedBLE = false;
			// 	task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTAPD);
			// }

			// // TODO: Sound switch mode start
			// // AudioCtrl::audioPlayFileG711(FCA_DEVICE_SOUND_PATH "/" FCA_SOUND_WAIT_SWITCH_MODE_FILE);

			// isLockSwitchApMode = true;
			// ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, Indicator::STATUS::SWITCH_MODE);
			// timer_set(GW_TASK_NETWORK_ID, GW_NET_UNLOCK_SWITCH_AP_MODE_REQ, GW_NET_UNLOCK_SWITCH_AP_MODE_INTERVAL, TIMER_ONE_SHOT);
		}
		break;

		case GW_NET_RUN_HOSTBLE: {
			APP_DBG_SIG("GW_NET_RUN_HOSTBLE\n");

			/* Close all Hostapd services */
			hostapd.doClose();

			/* Start all BLE services */
			char mac[17] = {0}, ssid[32] = {0}, pass[32] = {0};
			fca_netWifiAPGenInfo(mac, MAX_MAC_LEN, ssid, pass);
			#if 1 /* Hard code for testing */
			strcpy(ssid, vendorsHostapdSsid.c_str());
			strcpy(pass, vendorsHostapdPssk.c_str());
			#endif
			
			/* Setup SSID */
			fca_bluetooth_start(deviceSerialNumber.c_str(), ssid, pass);	
		}
		break;

		case GW_NET_RUN_HOSTAPD: {
			APP_DBG_SIG("GW_NET_RUN_HOSTAPD\n");

			char mac[17] = {0}, ssid[32] = {0}, pass[32] = {0};

			/* Close all BLE services */
			fca_bluetooth_close();

			/* Start all Hostapd services */
			fca_netWifiAPGenInfo(mac, MAX_MAC_LEN, ssid, pass);
			#if 1 /* Hard code for testing */
			strcpy(ssid, vendorsHostapdSsid.c_str());
			strcpy(pass, vendorsHostapdPssk.c_str());
			#endif

			APP_PRINT("- WiFi AP -\r\n");
			APP_PRINT("\t-Mac : %s\r\n", mac);
			APP_PRINT("\t-SSID: %s\r\n", ssid);
			APP_PRINT("\t-PSSK: %s\r\n", pass);

			hostapd.onOpened([ssid, pass]() {
				int rc = fca_wifi_start_ap_mode(FCA_NET_WIFI_AP_IF_NAME, ssid, pass, NETWORK_AP_HOST_IP);
				if (rc != 0) {
					RAM_SYSW("APIs start wi-Fi access point %s failure return %d\r\n", NETWORK_AP_HOST_IP, rc);
					return;
				}
				APP_PRINT("Hostapd socket has opened\n");
			});
			hostapd.onClosed([]() {
				int rc = fca_wifi_stop_ap_mode(FCA_NET_WIFI_AP_IF_NAME);
				if (rc != 0) {
					RAM_SYSW("APIs stop wi-Fi access point %s failure return %d\r\n", NETWORK_AP_HOST_IP, rc);
					return;
				}
				APP_PRINT("Hostapd socket has closed\n");
			});
			hostapd.onIncome([](SS_INGOING_S &in, SS_OUGOING_S &ou) {
				APP_DBG("Hostapd incoming: %s\n", in.str.c_str());

				std::string decrypt = decryptMobileIncomeMessage(in.str);

				try {
					uint32_t ts = 0;
					nlohmann::json js = nlohmann::json::parse(decrypt);
					std::string ssid = js["ssid"].get<std::string>();
					std::string pass = js["password"].get<std::string>();
					if (js.contains("ts")) {
						ts = js["ts"].get<uint32_t>();
					}
					APP_PRINT("Receive from hostapd: ssid {%s}, pass {%s}, ts {%u}\n", ssid.c_str(), pass.c_str(), ts);

					{
						FCA_NET_WIFI_S wiFiSta = {0};
						wiFiSta.enable = true;
						wiFiSta.dhcp = true;
						safe_strcpy(wiFiSta.ssid, ssid.c_str(), sizeof(wiFiSta.ssid));
						safe_strcpy(wiFiSta.keys, pass.c_str(), sizeof(wiFiSta.keys));
						fca_configSetWifi(&wiFiSta);
						if (ts != 0) {
							systemCmd("date -u -s @%u", ts);
						}
						task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&wiFiSta, sizeof(wiFiSta));
					}
					ou.str.assign(SOCKET_STATUS_OK);
					return;
				}
				catch (const std::exception &e) {
					SYSW("Hostapd exception: %s\n", e.what());
				}
				ou.str.assign(SOCKET_STATUS_ERR);
			});
			hostapd.doOpen(NETWORK_AP_HOST_PORT);
		}
		break;

		case GW_NET_WIFI_STA_CONNECT_TO_ROUTER: {
			APP_DBG_SIG("GW_NET_WIFI_STA_CONNECT_TO_ROUTER\n");
			bool status = netGetConnectToRouter();
			if (status) {
				task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_STA_CONNECTED_TO_ROUTER);
			}
			else {
				APP_DBG_NET(TAG "Connecting to router...\n");
				timer_set(GW_TASK_NETWORK_ID, GW_NET_WIFI_STA_CONNECT_TO_ROUTER, GW_NET_CHECK_CONNECT_ROUTER_INTERVAL, TIMER_ONE_SHOT);
			}

		} break;

		case GW_NET_WIFI_STA_CONNECTED_TO_ROUTER: {
			APP_DBG_SIG("GW_NET_WIFI_STA_CONNECTED_TO_ROUTER\n");

			string ip, mac;
			#if SUPPORT_ETH
			get_net_info(FCA_NET_WIRED_IF_NAME, ip, mac);
			APP_DBG_NET(TAG "lan [%s] [%s]\n", ip.data(), mac.data());
			#endif

			get_net_info(FCA_NET_WIFI_STA_IF_NAME, ip, mac);
			APP_DBG("wifi [%s] [%s]\n", ip.data(), mac.data());
			APP_DBG("wifi dhcp retry MQTT\n");
			task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ);

			systemCmd("killall -9 %s", FCA_ONVIF_WSD_SERVER);	 // onvif service
		} break;

		#if SUPPORT_ETH
		case GW_NET_ETHERNET_MQTT_SET_CONF_REQ: {
			APP_DBG_SIG("GW_NET_ETHERNET_MQTT_SET_CONF_REQ\n");
			int ret				 = APP_CONFIG_ERROR_DATA_INVALID;
			fca_netEth_t tmpConf = {0};
			try {
				json ethCfgJs = json::parse(string((char *)msg->header->payload, msg->header->len));
				APP_DBG_NET("%s\n", ethCfgJs.dump().data());
				if (fca_jsonGetEthernet(ethCfgJs, &tmpConf) && netVerifyConfigFromMQTT(&tmpConf) == 0 && fca_configSetEthernet(&tmpConf) == APP_CONFIG_SUCCESS) {
					netSetupState = NET_SETUP_STATE_CONFIG;
					task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_ETHERNET_CONTROL_WITH_CONFIG_REQ, (uint8_t *)&tmpConf, sizeof(tmpConf));
					ret = APP_CONFIG_SUCCESS;
				}
			}
			catch (const exception &error) {
				APP_DBG_NET(TAG "%s\n", error.what());
				ret = APP_CONFIG_ERROR_ANOTHER;
			}

			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_ETHERNET, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_NET_ETHERNET_MQTT_GET_CONF_REQ: {
			APP_DBG_SIG("GW_NET_ETHERNET_MQTT_GET_CONF_REQ\n");
			int ret		= APP_CONFIG_SUCCESS;
			json dataJs = json::object();
			if (!fca_jsonSetEthernet(dataJs, &ethSettings)) {
				APP_DBG_NET(TAG "data ethernet config invalid\n");
				ret = APP_CONFIG_ERROR_DATA_INVALID;
			}
			json resJs;
			if (fca_getConfigJsonRes(resJs, dataJs, MESSAGE_TYPE_ETHERNET, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_NET_ETHERNET_CONTROL_WITH_CONFIG_REQ: {
			APP_DBG_SIG("GW_NET_ETHERNET_CONTROL_WITH_CONFIG_REQ\n");
			ethSettings = {0};
			get_data_dynamic_msg(msg, (uint8_t *)&ethSettings, sizeof(ethSettings));

			lanPortLastState = fca_check_cable(FCA_NET_WIRED_IF_NAME);
			APP_DBG_NET(TAG "control ethernet, lan lanPortLastState: %d\n", lanPortLastState);
			fca_netEthernetDhcpStop(FCA_NET_WIRED_IF_NAME);
			if (lanPortLastState == FCA_NET_ETH_STATE_PLUG) {
				if (ethSettings.dhcp) {
					APP_DBG_NET(TAG "setup DHCP ethernet\n");
					usleep(500000);	   // 500ms
					fca_netEthernetDhcpStart(FCA_NET_WIRED_IF_NAME);
				}
				else {
					APP_DBG_NET("setup dev %s static\n", FCA_NET_WIRED_IF_NAME);
					fca_netStaticStart(FCA_NET_WIRED_IF_NAME, &ethSettings.detail);
				}

				ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, Indicator::STATUS::OK);

				setNetVoice(netSetupState);
				APP_DBG_NET("eth retry MQTT\n");
				timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ, GW_NET_RETRY_CONNECT_MQTT_INTERVAL, TIMER_ONE_SHOT);
				timer_set(GW_TASK_NETWORK_ID, GW_NET_TRY_GET_LAN_IP_PRIO_ONVIF_REQ, GW_NET_TRY_GET_LAN_IP_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);
			}
			else {
				lanPortLastState = FCA_NET_ETH_STATE_UNPLUG;
				netSetupState	 = NET_SETUP_STATE_NONE;	// prevent voice
			}
		} break;

		case GW_NET_GET_STATE_LAN_REQ: {
			// APP_DBG_NET("GW_NET_GET_STATE_LAN_REQ\n");
			int state = fca_netEthernetGetStatePlugPort();
			if (state == FCA_NET_ETH_STATE_PLUG && lanPortLastState == FCA_NET_ETH_STATE_UNPLUG) {
				APP_DBG_NET(TAG "=================== LAN Plug ===================\n");
				lanPortLastState = FCA_NET_ETH_STATE_PLUG;
				ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, Indicator::STATUS::OK);
				if (ethSettings.dhcp) {
					fca_netEthernetDhcpStart(FCA_NET_WIRED_IF_NAME);
				}
				else {
					fca_netStaticStart(FCA_NET_WIRED_IF_NAME, &ethSettings.detail);
				}
				timer_set(GW_TASK_NETWORK_ID, GW_NET_TRY_GET_LAN_IP_PRIO_ONVIF_REQ, GW_NET_TRY_GET_LAN_IP_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);
			}
			else if (state == FCA_NET_ETH_STATE_UNPLUG && lanPortLastState == FCA_NET_ETH_STATE_PLUG) {
				APP_DBG_NET(TAG "=================== LAN Unplug ===================\n");
				lanPortLastState = FCA_NET_ETH_STATE_UNPLUG;
				fca_netEthernetDhcpStop(FCA_NET_WIRED_IF_NAME);
				systemCmd("killall -9 %s", FCA_ONVIF_WSD_SERVER);	 // onvif service
				timer_remove_attr(GW_TASK_NETWORK_ID, GW_NET_TRY_GET_LAN_IP_PRIO_ONVIF_REQ);
			}
			timer_set(GW_TASK_NETWORK_ID, GW_NET_GET_STATE_LAN_REQ, GW_NET_CHECK_STATE_PLUG_LAN_INTERVAL, TIMER_ONE_SHOT);
		} break;

		case GW_NET_TRY_GET_LAN_IP_PRIO_ONVIF_REQ: {	// READY_TEST
			APP_DBG_SIG("GW_NET_TRY_GET_LAN_IP_PRIO_ONVIF_REQ\n");

			string ip, mac;
			get_net_info(FCA_NET_WIRED_IF_NAME, ip, mac);
			APP_DBG_NET(TAG "lan [%s] [%s]\n", ip.data(), mac.data());
			if (ip.empty()) {
				APP_DBG_NET(TAG "retry get IP\n");
				timer_set(GW_TASK_NETWORK_ID, GW_NET_TRY_GET_LAN_IP_PRIO_ONVIF_REQ, GW_NET_TRY_GET_LAN_IP_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);
			}
			else {
				systemCmd("killall -9 %s", FCA_ONVIF_WSD_SERVER);	 // onvif service
			}
		} break;
		#endif

		case GW_NET_INTERNET_UPDATED: {
			APP_DBG_SIG("GW_NET_INTERNET_UPDATED\n");

			if (isInternetConnected) {
				SYSI("Internet has connected\r\n");
				ledStatus.controlLedEvent(Indicator::EVENT::INTERNET_CONNECTION, Indicator::STATUS::CONNECTED, Indicator::STATUS::START);
			}
			else {
				SYSW("Internet has disconnected\r\n");
				ledStatus.controlLedEvent(Indicator::EVENT::INTERNET_CONNECTION, Indicator::STATUS::DISCONNECT);
			}
		}
		break;

		case GW_NET_RESET_WIFI_REQ: {
			APP_DBG_SIG("GW_NET_RESET_WIFI_REQ\n");

			string path = FCA_VENDORS_FILE_LOCATE(FCA_WIFI_FILE);
			remove(path.c_str());	 // remove wifi setting
			#if SUPPORT_ETH
			path = FCA_VENDORS_FILE_LOCATE(FCA_ETHERNET_FILE);
			remove(path.c_str());	 // remove eth setting
			#endif

			ledStatus.controlLedEvent(Indicator::EVENT::REBOOT);
			audioHelpers.notifyFactoryReset();
			sleep(2);

			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_REBOOT_REQ);
		} break;

		case GW_NET_RESTART_UDHCPC_NETWORK_INTERFACES_REQ: {
			APP_DBG_SIG("GW_NET_RESTART_UDHCPC_NETWORK_INTERFACES_REQ\n");
			#if SUPPORT_ETH
			/* restart eth0 */
			fca_netEth_t ethTmpConf = {0};
			ethTmpConf.enable		= true;
			ethTmpConf.dhcp			= true;	   // default ethernet DHCP
			fca_configGetEthernet(&ethTmpConf);
			task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_ETHERNET_CONTROL_WITH_CONFIG_REQ, (uint8_t *)&ethTmpConf, sizeof(ethTmpConf));
			#endif

			/* restart wlan0 */
			FCA_NET_WIFI_S wifiTmpConf = {0};
			if (fca_configGetWifi(&wifiTmpConf) == APP_CONFIG_SUCCESS) {
				APP_DBG_NET("restart wifi sta\n");
				task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&wifiTmpConf, sizeof(wifiTmpConf));
			}

			timer_set(GW_TASK_NETWORK_ID, GW_NET_RESTART_UDHCPC_NETWORK_INTERFACES_REQ, GW_NET_RESTART_NETWORK_SERVICES_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);
		}
		break;

		case GW_NET_RELOAD_WIFI_DRIVER_REQ: {
			APP_DBG_SIG("GW_NET_RELOAD_WIFI_DRIVER_REQ\n");
			/* NOTE: GIEC always reloads wifi driver every time it connects */
			/* restart wlan0 */
			// FCA_NET_WIFI_S wifiTmpConf = {0};
			// if (fca_configGetWifi(&wifiTmpConf) == APP_CONFIG_SUCCESS) {
			// 	APP_DBG_NET("reload wifi driver\n");
			// 	systemCmd("modprobe -r 8188fu");	// == ifconfig wlan0 down
			// 	usleep(200000);						// 200ms
			// 	systemCmd("modprobe 8188fu");
			// 	usleep(100000);	   // 100ms
			// 	task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&wifiTmpConf, sizeof(wifiTmpConf));
			// }
			// timer_set(GW_TASK_NETWORK_ID, GW_NET_RELOAD_WIFI_DRIVER_REQ, GW_NET_RELOAD_WIFI_DRIVER_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);
		} break;

		case GW_NET_REBOOT_ERROR_NETWORK_REQ: {
			APP_DBG_SIG("GW_NET_REBOOT_ERROR_NETWORK_REQ\n");

			FCA_NET_WIFI_S wifiTmpConf = {0};
			#if SUPPORT_ETH
			int state = fca_netEthernetGetStatePlugPort();
			if (fca_configGetWifi(&wifiTmpConf) == APP_CONFIG_SUCCESS || state == FCA_NET_ETH_STATE_PLUG) {
				APP_DBG_NET("config ok, ping net error => reboot\n");
				LOG_FILE_ERROR("Net error => reboot\n");
				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_REBOOT_REQ);
			}
			#else
			if (fca_configGetWifi(&wifiTmpConf) == APP_CONFIG_SUCCESS) {
				APP_DBG_NET("config ok, ping net error => reboot\n");
				LOG_FILE_ERROR("Net error => reboot\n");
				task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_REBOOT_REQ);
			}
			#endif

			timer_set(GW_TASK_NETWORK_ID, GW_NET_REBOOT_ERROR_NETWORK_REQ, GW_NET_REBOOT_ERROR_NETWORK_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);

		} break;

		case GW_NET_WATCHDOG_PING_REQ: {
			task_post_pure_msg(GW_TASK_NETWORK_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);
		} break;

		default:
			break;
		}

		/* free message */
		ak_msg_free(msg);
	}
	return (void *)0;
}

static bool pingToDns(void) {
	const char *listDns[] = {
		"8.8.8.8",
		"8.8.4.4",
		"1.1.1.1",
		"1.0.0.1",
		"208.67.222.222",
		"208.67.220.220",
		NETWORK_DOMAIN_SERVER,
	};
	const int listDnsSize = sizeof(listDns) / sizeof(const char*);

	for (int id = 0; id < listDnsSize; ++id) {
		char cmds[128] = {0};
		snprintf(cmds, sizeof(cmds), "ping -c 1 -W 2 %s > /dev/null 2>&1", listDns[id]);
		if (system(cmds) == 0) {
			return true;
		}
	}
	return false;
}

static void *pingInternetCalls(void *args) {
	(void)args;
	bool lastState = false;

	while (true) {
		bool currState = pingToDns();
		if (currState != lastState) {
			lastState = currState;
			timer_set(GW_TASK_NETWORK_ID, GW_NET_INTERNET_UPDATED, 1500, TIMER_ONE_SHOT);
		}
		isInternetConnected = currState;
		sleep(5);
	}
	pthread_detach(pthread_self());
	return (void *)0;
}

static bool netGetConnectToRouter(void) {
	FILE *fp;
	char buf[4];
	fp = fopen(NET_CONNECT_ROUTER_STATUS_FILE, "r");
	if (fp == NULL) {
		APP_DBG_NET(TAG "open file %s failed: %s", NET_CONNECT_ROUTER_STATUS_FILE, strerror(errno));
		return false;
	}
	if (fgets(buf, sizeof(buf), fp) == NULL) {
		fclose(fp);
		return false;
	}

	fclose(fp);
	strtok(buf, "\n");

	if (strcmp(buf, "1") == 0) {
		return true;
	}

	return false;
}

static int netVerifyConfigFromMQTT(FCA_NET_WIFI_S *wifi) {
	int ret = 0;
	if ((strlen(wifi->ssid)) > (sizeof(wifi->ssid) - 1) || (strlen(wifi->ssid)) <= 0) {
		APP_DBG_NET(TAG "[Wifi] set ssid error leng: %d [Max leng : %d]\n", sizeof(wifi->ssid), strlen(wifi->ssid));
		ret = -1;
	}
	else if ((strlen(wifi->keys)) > (sizeof(wifi->keys) - 1) || (strlen(wifi->keys)) <= 0) {
		APP_DBG_NET(TAG "[Wifi] set keys error leng: %d [Max leng : %d]\n", sizeof(wifi->keys), strlen(wifi->keys));
		ret = -1;
	}
	else if (!wifi->dhcp) {
		string wifiIp, mac;
		get_net_info(FCA_NET_WIFI_STA_IF_NAME, wifiIp, mac);
		#if SUPPORT_ETH
		string ethIp;
		get_net_info(FCA_NET_WIRED_IF_NAME, ethIp, mac);
		if (strcmp(wifi->detail.hostIP, ethIp.data()) == 0) {
			APP_DBG_NET("[WARN] Wifi hostIP same LAN ip\n");
			return -1;
		}
		#endif
		if (strcmp(wifi->detail.hostIP, wifiIp.data()) != 0 && systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", wifi->detail.hostIP) == 0) {
			APP_DBG_NET("[WARN] Wifi hostIP exist\n");
			ret = -1;
		}
		else if (systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", wifi->detail.gateWay) != 0 ||
				 (strlen(wifi->detail.preferredDns) > 0 && systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", wifi->detail.preferredDns) != 0) ||
				 (strlen(wifi->detail.alternateDns) > 0 && systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", wifi->detail.alternateDns) != 0)) {
			/* check static */
			APP_DBG_NET("[WARN] ping static IP failed\n");
			ret = -1;
		}
	}

	return ret;
}

#if SUPPORT_ETH
static int netVerifyConfigFromMQTT(fca_netEth_t *eth) {
	int ret = 0;
	/* check static */
	if (!eth->dhcp) {
		string ethIp, wifiIp, mac;
		get_net_info(FCA_NET_WIRED_IF_NAME, ethIp, mac);
		get_net_info(FCA_NET_WIFI_STA_IF_NAME, wifiIp, mac);
		if (strcmp(eth->detail.hostIP, wifiIp.data()) == 0) {
			APP_DBG_NET("[WARN] LAN hostIP same wifi ip\n");
			ret = -1;
		}
		else if (strcmp(eth->detail.hostIP, ethIp.data()) != 0 && systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", eth->detail.hostIP) == 0) {
			APP_DBG_NET("[WARN] Wifi hostIP exist\n");
			ret = -1;
		}
		else if (systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", eth->detail.gateWay) != 0 ||
				 (strlen(eth->detail.preferredDns) > 0 && systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", eth->detail.preferredDns) != 0) ||
				 (strlen(eth->detail.alternateDns) > 0 && systemCmd("ping -c 1 -W 1 %s > /dev/null 2>&1", eth->detail.alternateDns) != 0)) {
			/* check static */
			APP_DBG_NET("[WARN] ping static IP failed\n");
			ret = -1;
		}
	}

	return ret;
}
#endif

void netClearDns(const char *ifname, string fileDNS) {
	std::string ifnameStr = "# " + string(ifname);
	string content		  = getLinesNotContainingWord(FCA_DNS_FILE, ifnameStr);
	write_raw_file(content, fileDNS);
}

void fca_netStaticStart(const char *ifname, const fca_netConf_t *info) {
	/*
	ifconfig eth0 <địa chỉ IP> netmask <subnet mask>
	route add default gw <địa chỉ IP gateway> dev eth0 metric [29/69]
	DNS: echo "nameserver 8.8.8.8" > /etc/resolv.conf
	*/

	/* start network static */
	systemCmd("ifconfig %s %s netmask %s &", ifname, info->hostIP, info->submask);
	/* up port */
	systemCmd("ifconfig %s up", ifname);
	/* delete route */
	systemCmd("route del default dev %s", ifname);
	/* add default gateway */
	systemCmd("route add default gw %s dev %s metric %s &", info->gateWay, ifname,
			  (strcmp(ifname, FCA_NET_WIRED_IF_NAME) == 0)	  ? "29"
			  : strcmp(ifname, FCA_NET_WIFI_STA_IF_NAME) == 0 ? "69"
															  : "0");

	/* set DNS */
	std::string ifnameStr = "# " + string(ifname);
	string content		  = getLinesNotContainingWord(FCA_DNS_FILE, ifnameStr);
	APP_DBG_NET("content: %s\n", content.c_str());

	string newContent  = "";
	vector<string> dns = {info->preferredDns, info->alternateDns};
	for (const std::string &i : dns) {
		if (!i.empty()) {
			newContent += "nameserver " + i + " " + ifnameStr + "\n";
		}
	}

	APP_DBG_NET("newContent: %s\n", newContent.c_str());
	if (strcmp(ifname, FCA_NET_WIRED_IF_NAME) == 0) {	 // eth0
		newContent += content;
	}
	else {
		newContent = content + newContent;
	}
	write_raw_file(newContent, FCA_DNS_FILE);

	APP_DBG_NET("data resolv file: %s\n", systemStrCmd("cat %s", FCA_DNS_FILE).c_str());
}

void fca_netStaticStop(const char *ifname) {
	APP_DBG_NET("stop static IP\n");
	systemCmd("ifconfig %s 0.0.0.0", ifname);	 // route auto clear when set ip 0.0.0.0, file resolve.conf not clear
	netClearDns(ifname, FCA_DNS_FILE);
}

void setNetVoice(int &st) {
	switch (st) {
	case NET_SETUP_STATE_CONFIG: {
		st = NET_SETUP_STATE_CONNECTING;
		audioHelpers.notifyWiFiIsConnecting();
	} break;

	case NET_SETUP_STATE_CONNECTING: {
		st = NET_SETUP_STATE_CONNECTED;
		audioHelpers.notifyWiFiHasConnected();
	} break;

	default:
		break;
	}
}

void removeCheckErrorNetworkTimers() {
	timer_remove_attr(GW_TASK_NETWORK_ID, GW_NET_RESTART_UDHCPC_NETWORK_INTERFACES_REQ);
	// timer_remove_attr(GW_TASK_NETWORK_ID, GW_NET_RELOAD_WIFI_DRIVER_REQ);  NOTE: GIEC always reloads wifi driver every time it connects
	timer_remove_attr(GW_TASK_NETWORK_ID, GW_NET_REBOOT_ERROR_NETWORK_REQ);
}
