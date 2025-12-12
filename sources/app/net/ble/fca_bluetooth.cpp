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

#include "rncryptor_c.h"
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
static std::string ble_ssid = "FC-P04L-C05I24100000002";
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
static unsigned char ble_noti_success_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc7, 0xfe, 0x00, 0x00, 'C', 'O', 'N', 'F', 'I', 'G', '_', 'O', 'K'};
static unsigned char ble_noti_failure_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc7, 0xfe, 0x00, 0x00, 'C', 'O', 'N', 'F', 'I', 'G', '-', 'F', 'A', 'I', 'L'};
static unsigned char ble_read_data[] = {0x00, 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xc9, 0xfe, 0x00, 0x00, 'H', 'e', 'l', 'l', 'o'};

static unsigned char* OpenSSL_Base64Decoder(const char *cipher, int *decodedLen) {
	BIO *bio, *b64;
    int cipherLen = strlen(cipher);
    unsigned char *buffers = (unsigned char *)malloc(cipherLen);
	if (!buffers) {
		return NULL;
	}
    memset(buffers, 0, cipherLen);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf((void *)cipher, -1);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); /* NO_WRAP (no newlines) */
    *decodedLen = BIO_read(bio, buffers, cipherLen);
    BIO_free_all(bio);
    return buffers;
}

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
		// nlohmann::json js = nlohmann::json::parse(std::string((char*)Decrypt, DecryptLen));

		printf("JS: %s\r\n", js.dump(4).c_str());

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
		// if (verified.compare(ble_pssk) == 0) {
		// 	safe_strcpy(wiFi.ssid, wiFiSsid.c_str(), sizeof(wiFi.ssid));
		// 	safe_strcpy(wiFi.keys, wiFiPssk.c_str(), sizeof(wiFi.keys));
		// 	rc = 0;
		// }
		rc = 0;
	}
	catch (const std::exception &e) {
		APP_ERR("%s\r\n", e.what());
	}

	fca_ble_notify(rc);

	if (rc == 0) {
		wiFi.enable = true;
		wiFi.dhcp   = true;
		int isConfigOk = fca_configSetWifi(&wiFi);
		if (isConfigOk == 0) {
			APP_DBG_NET(TAG "Wifi setting: \nssid: [%s] \npass: [%s]\n", wiFi.ssid, wiFi.keys);
			netSetupState = NET_SETUP_STATE_CONFIG;
			task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&wiFi, sizeof(wiFi));

			if (ts != 0) {
				systemCmd("date -u -s @%u", ts);
			}
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

void fca_bluetooth_on_message(FCA_BLE_EVT_CB ble_evt, fca_ble_data_buf_t *data) {
	APP_DBG("BLE EVT: %d\r\n", ble_evt);

	switch (ble_evt) {
	case FCA_BLE_INIT_DONE: {
		APP_DBG("BLE -> FCA_BLE_INIT_DONE\r\n");

		int rc = -1;
		char ssidBle[32] = "FC-P04L-C05I24100000002";

		FCA_API_ASSERT((rc = fca_ble_gap_name_set_msg_send((uint8_t *)ssidBle, strlen(ssidBle))) == 0);
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

		// for (uint8_t id = 0; id < data->len; ++id) {
		// 	printf("%d (%c)\r\n", data->data[id], data->data[id]);
		// }
		printf("$$$\r\n");
		printf("%s\r\n", (char*)data->data);
		printf("$$$\r\n");

		int start = 0;
		for (int id = 0; id < (int)data->len; id++) {
			APP_DBG("%c", data->data[id]);
			if (data->data[id] == '{') {
				start = id;
			}
		}
		string msg((char *)(data->data + start), data->len - start);
		uint16_t services = (uint16_t)(data->data[13] << 8) + (uint16_t)(data->data[12]);
		APP_DBG("BLE Service UUID: %04X\r\n", services);
		fca_ble_setting_wifi(msg);
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

static int bluetooth_setup_message(const char *sn, const char *ssid, const char *pssk) {
	if (ssid) ble_ssid.assign(ssid);
	if (pssk) ble_pssk.assign(pssk);

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

int fca_bluetooth_start(const char *sn, const char *ssid, const char *pssk) {
	int rc = -1;

	bluetooth_setup_message(sn, ssid, pssk);
	APP_DBG("BLUETOOTH has started -> {S/N: %s, SSID: %s, PSSK: %s}\n", sn, ble_ssid.c_str(), ble_pssk.c_str());

	// const char *ble_services_fp = (const char*)"/usr/bin/blh/ble_userconfig.json";
	const char *ble_services_fp = (const char*)"/tmp/envir/default/default/ble_userconfig.json";
	APP_PRINT("BLE services path: %s\r\n", ble_services_fp);
	FCA_API_ASSERT((rc = fca_ble_service_set(ble_services_fp)) == 0);
	if (rc != 0) {
		return -2;
	}

	FCA_API_ASSERT((rc = fca_ble_insmod_driver()) == 0);
	if (rc != 0) {
		return rc;
	}
	FCA_API_ASSERT((rc = fca_ble_stack_init(fca_bluetooth_on_message)) == 0);
	return rc;
}

int fca_bluetooth_close() {
	int rc = -1;
	FCA_API_ASSERT((rc = fca_ble_gap_adv_stop()) == 0);
	return rc;
}
