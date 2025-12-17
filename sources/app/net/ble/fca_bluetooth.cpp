#include "fca_bluetooth.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "json.hpp"
#include "app.h"
#include "app_data.h"
#include "app_config.h"
#include "timer.h"
#include "task_list.h"
#include "fca_parameter.hpp"
#include <openssl/bio.h>
#include <openssl/sha.h>
#include "wifi.h"

#include "fca_assert.hpp"
#include "odm_api_ble.h"
#include "odm_api_network.h"
#include "odm_api_system.h"
#include "odm_api_factory.h"
// #include "queue_client.h"
#include "network_data_encrypt.h"
#include "utils.h"

#define TAG					 "[Ble] "
#define BLE_SET_WIFI		 (0x2B11)
#define BLE_SET_FACTORY_MODE (0x2B12)

typedef struct {
	fca_ble_data_buf_t adv;
    fca_ble_data_buf_t scan;
	fca_ble_data_buf_t noti;
	fca_ble_data_buf_t read;
	fca_ble_data_buf_t device_qr;
	fca_ble_data_buf_t noti_success;
    fca_ble_data_buf_t noti_failure;
} BLUETOOTH_MESSAGE_S;

// Messae with private UUID Sevice : 7e136f20-00a1-44dd-c9c3-77772b8b3377
static pthread_t pid = (pthread_t)NULL;
static bool boolean = false;
static std::string ble_ssid;
static std::string ble_pssk;
static BLUETOOTH_MESSAGE_S ble_group_msgs;

static unsigned char ble_device_qr[128] = {0x77, 0x33, 0x8b, 0x2b, 0x77, 0x77, 0xc3, 0xc9, 0xdd, 0x44, 0xa1, 0x00, 0x22, 0x2b, 0x13, 0x7e};

// static unsigned char ble_adv_data[] = {0x02, 0x01, 0x06,0x07, 0x09, 'F','C','-','P','0','4',0x11, 0x07, 0x00, 0x00, 0xFE, 0xE7, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
// // static unsigned char ble_adv_data[] = {0x02, 0x01, 0x06,0x07, 0x09, 'F','C','-','R','A','L',0x11, 0x07, 0x00, 0x00, 0xFE, 0xE7, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
// static unsigned char ble_scan_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// static unsigned char ble_read_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc9, 0xfe, 0x00, 0x00, 'H', 'e', 'l', 'l', 'o'};
// static unsigned char ble_noti_success_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc9, 0xfe, 0x00, 0x00, 'C', 'O', 'N', 'F', 'I', 'G', '_', 'O', 'K'};
// static unsigned char ble_noti_failure_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc9, 0xfe, 0x00, 0x00, 'C', 'O', 'N', 'F', 'I', 'G', '-', 'F', 'A', 'I', 'L'};

static unsigned char ble_adv_data[] = {0x02, 0x01, 0x06, 0x18, 0x09, 'F', 'C', '-', 'R', 'A', 'L', 'I', '-', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
static unsigned char ble_scan_data[] = {0x11, 0x07, 0x00, 0x00, 0xFE, 0xE7, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
// static unsigned char ble_noti_success_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc7, 0xfe, 0x00, 0x00, 'C', 'O', 'N', 'F', 'I', 'G', '_', 'O', 'K'};
// static unsigned char ble_noti_failure_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc7, 0xfe, 0x00, 0x00, 'C', 'O', 'N', 'F', 'I', 'G', '-', 'F', 'A', 'I', 'L'};
// static unsigned char ble_read_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc9, 0xfe, 0x00, 0x00, 'H', 'e', 'l', 'l', 'o'};
// static unsigned char ble_read_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc8, 0xfe, 0x00, 0x00};

static unsigned char ble_noti_success_data[] = {0x00, 0x00, 0x00, 0xfe, 0xc9, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb, 'C', 'O', 'N', 'F', 'I', 'G', '_', 'O', 'K'};
static unsigned char ble_noti_failure_data[] = {0x00, 0x00, 0x00, 0xfe, 0xc9, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb, 'C', 'O', 'N', 'F', 'I', 'G', '-', 'F', 'A', 'I', 'L'};
static unsigned char ble_read_data[] = {0x00, 0x00, 0x00, 0xfe, 0xc8, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb, 'H', 'e', 'l', 'l', 'o'};

static int fca_ble_notify(int result) {
	if (result == 0) {
		APP_DBG("Notifies: CONFIG-OK\r\n");
		fca_ble_gatts_value_notify(&ble_group_msgs.noti_success);
		sleep(3);
	}
	else {
		APP_DBG("Notifies: CONFIG-FAIL\r\n");
		fca_ble_gatts_value_notify(&ble_group_msgs.noti_failure);
	}
	return 0;
}

static int fca_ble_setting_wifi(std::string msg) {
	int rc = -1;
	uint32_t ts = 0;
	FCA_NET_WIFI_S wiFi = {0};

	// int decoder64Len = 0;
	// unsigned char *decoder64 = OpenSSL_Base64Decoder(msg.c_str(), &decoder64Len);
	// if (!decoder64) {
	// 	SYSE("HOSTAPD - Invalid Decode Base64\r\n");
	// 	return -1;
	// }
	// int DecryptLen = 0;
	// char errBuffers[512] = {0};
	// unsigned char *Decrypt = rncryptorc_decrypt_data_with_password(
	// 	(const unsigned char*)decoder64, decoder64Len,
	// 	100,
	// 	"05hQGF7bpdfakooDoXM6Ad632YE3yDZv", 
	// 	strlen("05hQGF7bpdfakooDoXM6Ad632YE3yDZv"),
	// 	&DecryptLen,
	// 	errBuffers, sizeof(errBuffers));
	// if (!Decrypt) {
	// 	SYSE("HOSTAPD - Can't decrypt by RNCrypto\r\n");
	// 	return -1;
	// }
	// free(decoder64);
	// APP_DBG("Decrypt -> %s\r\n", Decrypt);
	
	try {
		nlohmann::json js = json::parse(msg);

		APP_DBG("%s\r\n", js.dump(4).c_str());

		if (!js.contains("Verified") || !js.contains("SSID") || !js.contains("Password")) {
			throw runtime_error("Missing required fields in JSON data");
		}
		std::string verified = js["Verified"].get<std::string>();
		std::string wiFiSsid = js["SSID"].get<std::string>();
		std::string wiFiPssk = js["Password"].get<std::string>();
		if (js.contains("Ts")) {
			ts = js["Ts"].get<uint32_t>();
		}
		APP_PRINT("Receive from BLE: ssid {%s}, pass {%s}, verification {%s}\n", wiFiSsid.c_str(), wiFiPssk.c_str(), verified.c_str());
		safe_strcpy(wiFi.ssid, wiFiSsid.c_str(), sizeof(wiFi.ssid));
		safe_strcpy(wiFi.keys, wiFiPssk.c_str(), sizeof(wiFi.keys));
		rc = 0;
	}
	catch (const std::exception &e) {
		APP_ERR("%s\r\n", e.what());
	}

	fca_ble_notify(rc);

	if (rc == 0) {
		wiFi.dhcp = true;
		wiFi.enable = true;
		int isConfigOk = fca_configSetWifi(&wiFi);
		if (isConfigOk == 0) {
			APP_DBG_NET(TAG "Wifi setting: \nssid: [%s] \npass: [%s]\n", wiFi.ssid, wiFi.keys);
			netSetupState = NET_SETUP_STATE_CONFIG;
			if (ts != 0) {
				systemCmd("date -u -s @%u", ts);
			}
			task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&wiFi, sizeof(wiFi));
		}
	}
	return rc;
}

int fca_ble_factory_mode(std::string msg) {
	int rc = -1;
	try {
		nlohmann::json js = json::parse(msg);
		if (!js.contains("FactoryMode") || !js.contains("Verified")) {
			throw runtime_error("Missing required fields in JSON data");
		}
		string verify = js["Verified"].get<string>();
		string factory = js["FactoryMode"].get<string>();
		APP_PRINT("Receives from BLE: factory {%s}, verification {%s}\n", verify.c_str(), factory.c_str());
		if (verify.compare(ble_pssk) == 0 && factory.compare("Enable") == 0) {
			rc = 0;
		}
	}
	catch (const exception &e) {
		APP_ERR("%s\r\n", e.what());
	}

	fca_ble_notify(rc);

	if (rc == 0) {
		fca_sys_set_factory();
		sleep(1);
		task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_REBOOT_REQ);
	}
	return rc;
}

static inline bool isBase64Characters(uint8_t let) {
    return (let >= 'A' && let <= 'Z') ||
           (let >= 'a' && let <= 'z') ||
           (let >= '0' && let <= '9') ||
           (let == '+' || let == '/' || let == '=');
}

void fca_bluetooth_on_message(FCA_BLE_EVT_CB ble_evt, fca_ble_data_buf_t *data) {
	APP_DBG("BLE EVT: %d\r\n", ble_evt);

	switch (ble_evt) {
	case FCA_BLE_INIT_DONE: {
		APP_DBG("BLE -> FCA_BLE_INIT_DONE\r\n");

		int rc = -1;
		// char ssidBle[32] = "FC-P04L-C05I24100000XXX";

		FCA_API_ASSERT((rc = fca_ble_gap_name_set_msg_send((uint8_t *)ble_ssid.c_str(), ble_ssid.length())) == 0);
		if (rc != 0) {
			return;
		}
		FCA_API_ASSERT((rc = fca_ble_gap_adv_rsp_data_set(&ble_group_msgs.adv, &ble_group_msgs.scan)) == 0);
		if (rc != 0) {
			return;
		}

		FCA_API_ASSERT((rc = fca_ble_gatts_value_read_update(&ble_group_msgs.read)) == 0);
		if (rc != 0) {
			return;
		}

		FCA_API_ASSERT((rc = fca_ble_gap_adv_start(&ble_group_msgs.adv)) == 0);
		if (rc != 0) {
			return;
		}
		audioHelpers.notifyChangeBluetoothMode();
		APP_PRINT("Bluetooth advertises success\r\n");
	}
	break;

    case FCA_BLE_ADV_ENABLE: {
		APP_DBG("BLE -> FCA_BLE_ADV_ENABLE\r\n");
		ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, Indicator::STATUS::BLE_MODE, Indicator::STATUS::ON);
	}
	break;

    case FCA_BLE_ADV_DISABLE: {
		APP_DBG("BLE -> FCA_BLE_ADV_DISABLE\r\n");
	}
	break;

    case FCA_BLE_CONNECTION_IND: {
		APP_DBG("BLE -> FCA_BLE_CONNECTION_IND\r\n");
		audioHelpers.notifyBluetoothHasConnected();
	}
	break;

    case FCA_BLE_DISCONNECTED: {
		APP_DBG("BLE -> FCA_BLE_DISCONNECTED\r\n");
		audioHelpers.notifyBluetoothHasDisconnected();
	}
	break;

    case FCA_BLE_SMARTCONFIG_DATA_RECV: {
		APP_DBG("BLE -> FCA_BLE_SMARTCONFIG_DATA_RECV\r\n");
		if (!data || !data->data || data->len <= 1) {
			APP_DBG("BLE -> Invalid data\r\n");
			return;
		}

		int start = 0;
		for (int id = 0; id < (int)data->len - 2; id++) {
			APP_DBG("%c", data->data[id]);
			if (isBase64Characters(data->data[id]) && isBase64Characters(data->data[id + 1]) && isBase64Characters(data->data[id + 2])) {
				start = id;
				break;
			}
		}
		string msg((char *)(data->data + start), data->len - start);
		APP_DBG("\r\nEncrypt: %s\r\n", msg.c_str());
		std::string decrypt = decryptMobileIncomeMessage(msg);
		APP_DBG("Decrypt: %s\r\n", decrypt.c_str());
		uint16_t services = (uint16_t)(data->data[13] << 8) + (uint16_t)(data->data[12]);
		APP_DBG("BLE Service UUID: %04X\r\n", services);
		fca_ble_setting_wifi(decrypt);
		// switch (services) {
		// case BLE_SET_WIFI: {
		// 	APP_DBG("-> -> BLE_SET_WIFI\n");
		// 	fca_ble_setting_wifi(msg);
		// }
		// break;

		// case BLE_SET_FACTORY_MODE: {
		// 	APP_DBG("-> -> BLE_SET_FACTORY_MODE\n");
		// 	fca_ble_factory_mode(msg);
		// }
		// break;

		// default:
		// break;
		// }
	}
	break;
	
	default:
	break;
	}
}

static inline void bluetooth_setup_data(const char *sn, const char *ssid, const char *pssk) {
	if (ssid) {
		ble_ssid.assign(ssid);
	}
	if (pssk) {
		ble_pssk.assign(pssk);
	}

	/* Setup SSID */
	unsigned char *ptr = &ble_adv_data[5];
	short len = sizeof(ble_adv_data) - 5;
	memset(ptr, 0, len);
	if (ssid && strlen(ssid) <= len) {
		for (int id = 0; id < strlen(ssid); ++id) {
			ptr[id] = ssid[id];
		}
	}
}

static int bluetooth_setup_services(const char *sn, const char *ssid, const char *pssk) {
	bluetooth_setup_data(sn, ssid, pssk);

	/* Initialise advertising data */
	ble_group_msgs.adv.data = ble_adv_data;
	ble_group_msgs.adv.len	= sizeof(ble_adv_data);

    /* Initialise scan response data */
    ble_group_msgs.scan.data = ble_scan_data;
    ble_group_msgs.scan.len = sizeof(ble_scan_data);

	ble_group_msgs.read.data = ble_read_data;
    ble_group_msgs.read.len = sizeof(ble_read_data);

	/* Initialise notification response data */
    ble_group_msgs.noti_success.data = ble_noti_success_data;
    ble_group_msgs.noti_success.len = sizeof(ble_noti_success_data);
	ble_group_msgs.noti_failure.data = ble_noti_failure_data;
    ble_group_msgs.noti_failure.len = sizeof(ble_noti_failure_data);

	return 0;
}

static void *fca_bluetooth_run(void *args) {
	// const char *ble_services_fp = (const char*)"/usr/bin/blh/ble_userconfig.json";
	std::string ble_services_fp = FCA_DEFAULT_FILE_LOCATE("ble_userconfig.json");
	APP_PRINT("Bluetooth services path selected: %s\r\n", ble_services_fp.c_str());
	FCA_API_ASSERT(fca_ble_service_set(ble_services_fp.c_str()) == 0);
	FCA_API_ASSERT(fca_ble_insmod_driver() == 0);
	FCA_API_ASSERT(fca_ble_stack_init(fca_bluetooth_on_message) == 0);

	while (boolean) {
		usleep(100000);
	}

	APP_PRINT("BLE has closed\r\n");

	return NULL;
}

int fca_bluetooth_start(const char *sn, const char *ssid, const char *pssk) {
	if (pid) {
		return 0;
	}
	bluetooth_setup_services(sn, ssid, pssk);
	APP_PRINT("Bluetooth run {S/N: %s, SSID: %s, PSSK: %s}\r\n", sn, ble_ssid.c_str(), ble_pssk.c_str());

	boolean = true;
	pthread_create(&pid, NULL, fca_bluetooth_run, NULL);

	return 0;
}

int fca_bluetooth_close() {
	boolean = false;

	if (pid) {
		FCA_API_ASSERT(fca_ble_gap_adv_stop() == 0);
		FCA_API_ASSERT(fca_ble_deinit() == 0);
		pthread_join(pid, NULL);
		pid = (pthread_t)NULL;
	}
	return 0;
}
