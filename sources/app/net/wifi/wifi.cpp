#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "wifi.h"
#include "task_list.h"
#include "utils.h"

#define FCA_WIFI_STA_CMD_SCAN_START	 "wpa_cli -i %s scan"
#define FCA_WIFI_STA_CMD_SCAN_RESULT "wpa_cli -i %s scan_result"

#define TAG "[wifi] "

typedef struct t_apSocketHdl {
	threadHdl_t thr;
	int client;
	int server;
} apSocketHdl_t;

extern int netSetupState;

void fca_netAPThreadStart();
void fca_netAPThreadStop();
void fca_netAPRelease();

static bool netGetDataFromSocket(char *buf, int len, fca_netWifiSocket_t *inf);
static int32_t netWifiGetPIDUdhcpc(const char *ifname, int32_t &pid);
static int32_t netWifiStaScanHostAP(uint8_t *cmd);
static int32_t netWifiStaScanHostAPResult(uint8_t *cmd, fca_netWifiStaScan_t *info);

static apSocketHdl_t apSocketControl = {
	.thr = {.id = 0, .is_running = false, .mtx = PTHREAD_MUTEX_INITIALIZER, .cond = PTHREAD_COND_INITIALIZER},
	.client = -1, .server = -1
};

void *fca_netAPServerHandle(void *args) {
	(void)args;
	int fServer = -1;
	int fClient = -1;
	int opt;
	char buffer[256];
	int isConfigOk = APP_CONFIG_ERROR_ANOTHER;

	apSocketHdl_t *apControl = (apSocketHdl_t *)args;
	struct sockaddr_in address;
	FCA_NET_WIFI_S staConf		   = {0};
	fca_netWifiSocket_t socketConf = {0}, socket_data;

	socketConf.port = NETWORK_AP_HOST_PORT;
	safe_strcpy(socketConf.hostIP, NETWORK_AP_HOST_IP, sizeof(socketConf.hostIP));

	APP_DBG("Hostapd listens -> \'%s:%d\'\n", socketConf.hostIP, socketConf.port);

	do {
		fServer = socket(AF_INET, SOCK_STREAM, 0);
		if (fServer < 0) {
			perror("socket()");
			break;
		}
		pthread_mutex_lock(&apControl->thr.mtx);
		apControl->server = fServer;
		pthread_mutex_unlock(&apControl->thr.mtx);

		opt = 1;
		if (setsockopt(fServer, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			perror("setsockopt()");
			break;
		}
		memset(&address, 0, sizeof(address));
		address.sin_family		= AF_INET;
		address.sin_addr.s_addr = inet_addr(NETWORK_AP_HOST_IP);
		address.sin_port		= htons(NETWORK_AP_HOST_PORT);

		if (bind(fServer, (struct sockaddr *)&address, sizeof(address)) < 0) {
			perror("bind()");
			break;
		}

		if (listen(fServer, 1) < 0) {
			perror("listen()");
			break;
		}

		task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_UNLOCK_SWITCH_AP_MODE_REQ);
		/* control led AP_MODE */
		ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, Indicator::STATUS::AP_MODE);
		if (isFistTimeStartModeNetwork == true) {
			isFistTimeStartModeNetwork = false;
		}
		else {
			audioHelpers.notifyChangeWiFiMode();
		}
		
		int addr_len = sizeof(address);
		while (1) {
			pthread_mutex_lock(&apControl->thr.mtx);
			if (!apControl->thr.is_running) {
				pthread_mutex_unlock(&apControl->thr.mtx);
				break;
			}
			pthread_mutex_unlock(&apControl->thr.mtx);

			fClient = accept(fServer, (struct sockaddr *)&address, (socklen_t *)&addr_len);
			if (fClient < 0) {
				perror("accept()");
				continue;
			}
			pthread_mutex_lock(&apControl->thr.mtx);
			apControl->client = fClient;
			pthread_mutex_unlock(&apControl->thr.mtx);

			int len = recv(fClient, buffer, sizeof(buffer) - 1, 0);
			if (len <= 0) {
				perror("recv()");
				break;
			}

			buffer[len] = 0;
			memset(&socket_data, 0, sizeof(socket_data));
			if (netGetDataFromSocket(buffer, len, &socket_data)) {
				staConf.enable = true;
				staConf.dhcp   = true;
				safe_strcpy(staConf.ssid, socket_data.ssid, sizeof(staConf.ssid));
				safe_strcpy(staConf.keys, socket_data.pass, sizeof(staConf.keys));
				isConfigOk = fca_configSetWifi(&staConf);
				if (isConfigOk == APP_CONFIG_SUCCESS) {
					APP_DBG("WiFi configure {%s, %s}\n", socket_data.ssid, socket_data.pass);
					send(fClient, SOCKET_STATUS_OK, sizeof(SOCKET_STATUS_OK), MSG_CONFIRM);
					sleep(1);
					break;
				}
			}
			send(fClient, SOCKET_STATUS_ERR, sizeof(SOCKET_STATUS_ERR), MSG_CONFIRM);
			sleep(1);
			close(fClient);
			pthread_mutex_lock(&apControl->thr.mtx);
			apControl->client = -1;
			pthread_mutex_unlock(&apControl->thr.mtx);
		}
	} while (0);

	pthread_mutex_lock(&apControl->thr.mtx);
	if (apControl->thr.is_running) {
		if (isConfigOk == APP_CONFIG_SUCCESS) {
			netSetupState = NET_SETUP_STATE_CONFIG;
			task_post_dynamic_msg(GW_TASK_NETWORK_ID, GW_NET_WIFI_DO_CONNECT, (uint8_t *)&staConf, sizeof(staConf));

			// set timestamp from phone UTC+0
			if (socket_data.ts > 0) {
				int ntpSt = getNtpStatus();
				if (ntpSt != 1) {
					APP_DBG("Set timestamp from phone: %u\n", socket_data.ts);
					systemCmd("date -u -s @%u", socket_data.ts);
				}
			}
		}
		else {
			APP_DBG("Hostapd has errors, try restarting ...\n");
			timer_set(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTAPD, GW_NET_START_AP_MODE_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
		}
	}
	pthread_mutex_unlock(&apControl->thr.mtx);

	if (fServer >= 0) {
		shutdown(fServer, SHUT_RDWR);
		close(fServer);
	}

	if (fClient >= 0) {
		shutdown(fClient, SHUT_RDWR);
		close(fClient);
	}

	return (void *)0;
}

void fca_netAPThreadStart() {
	pthread_mutex_lock(&apSocketControl.thr.mtx);
	apSocketControl.thr.is_running = true;
	pthread_mutex_unlock(&apSocketControl.thr.mtx);
	pthread_create(&apSocketControl.thr.id, NULL, fca_netAPServerHandle, &apSocketControl);
}

void fca_netTriggerApThreadStop() {
	pthread_mutex_lock(&apSocketControl.thr.mtx);
	apSocketControl.thr.is_running = false;
	if (apSocketControl.client >= 0) {
		shutdown(apSocketControl.client, SHUT_RDWR);
		close(apSocketControl.client);
	}
	if (apSocketControl.server >= 0) {
		shutdown(apSocketControl.server, SHUT_RDWR);
		close(apSocketControl.server);
	}
	pthread_mutex_unlock(&apSocketControl.thr.mtx);
}

void fca_netAPThreadStop() {
	int timeout = 5;	// 5s

	fca_netTriggerApThreadStop();
	usleep(500 * 1000);	   // 500ms

	if (apSocketControl.thr.id != 0) {
		int ret;
		while (timeout > 0) {
			ret = pthread_tryjoin_np(apSocketControl.thr.id, NULL);
			if (ret == 0) {
				break;
			}
			else if (ret == EBUSY) {
				fca_netTriggerApThreadStop();
				timeout--;
				sleep(1);
			}
		}
		if (ret != 0) {
			pthread_join(apSocketControl.thr.id, NULL);
		}
		apSocketControl.thr.id = 0;
		apSocketControl.client = -1;
		apSocketControl.server = -1;
	}
}

void fca_netAPRelease() {
	fca_netAPThreadStop();
	pthread_mutex_destroy(&apSocketControl.thr.mtx);
	pthread_cond_destroy(&apSocketControl.thr.cond);
}

int32_t fca_netWifiSTASetConfig(FCA_NET_WIFI_S *config) {
	APP_DBG("WiFi STA connects {%s, %s}\r\n", config->ssid, config->keys);
	fca_wifi_connect(FCA_NET_WIFI_STA_IF_NAME, config->ssid, config->keys);

	return FCA_NET_WIFI_OK;
}

int8_t fca_netWifiDhcpStart(const char *ifname) {
	/* start udhcpc */
	uint8_t pidFileName[64] = {0};
	sprintf((char *)pidFileName, FCA_NET_UDHCPC_PID_FILE, ifname);
	string serial = fca_getSerialInfo();

	int8_t ret = systemCmd("udhcpc -i%s -s%s -x hostname:%s -p%s > /dev/null 2>&1 &", ifname, FCA_UDHCPC_SCRIPT, serial.data(), pidFileName);
	if (ret != 0) {
		APP_DBG_NET(TAG "STAmode udhcpc start failed\n");
		return FCA_NET_WIFI_START_FAIL;
	}
	return FCA_NET_WIFI_OK;
}

void fca_netWifiDhcpStop(const char *ifname) {
	int32_t udhcpcPID = 0;
	if (netWifiGetPIDUdhcpc(ifname, udhcpcPID) != 0) {
		APP_DBG_NET(TAG "get pid wifi station failed\n");
	}
	else if (udhcpcPID > 0) {
		systemCmd("kill -9 %d", udhcpcPID);
	}
	systemCmd("ifconfig %s 0.0.0.0", ifname);	 // auto clear route, not auto clear DNS
	netClearDns(ifname, FCA_DNS_FILE);
}

int32_t fca_netWifiSTAStart(const char *ifname) {
	int32_t ret;

	systemCmd("echo 0 > %s", NET_CONNECT_ROUTER_STATUS_FILE);

	ret = systemCmd("ifconfig %s up", ifname);
	if (ret != 0) {
		APP_DBG_NET(TAG "STAmode up interface failed\n");
		return FCA_NET_WIFI_START_FAIL;
	}
	ret = systemCmd("wpa_supplicant -Dnl80211 -i%s -c%s -B", ifname, FCA_NET_WIFI_STA_WPA_FILE);
	if (ret != 0) {
		APP_DBG_NET(TAG "STAmode wpa_supplicant start failed\n");
		return FCA_NET_WIFI_START_FAIL;
	}
	return FCA_NET_WIFI_OK;
}

int32_t fca_netWifiSTAStop(const char *ifname) {
	systemCmd("killall -9 wpa_supplicant");
	systemCmd("ifconfig %s down", ifname);
	return FCA_NET_WIFI_OK;
}

int32_t fca_netWifiAPGenInfo(char *mac, int len, char *ssid, char *pass) {
	int32_t ret;

	#if SUPPORT_ETH
	ret = fca_ethernet_get_mac(mac, len);
	if (ret != 0) {
		APP_DBG_NET(TAG "APmode get ethernet mac failed\n");
	}
	#else
	ret = fca_wifi_get_mac(mac, len);
	if (ret != 0) {
		APP_DBG_NET(TAG "APmode get wifi mac failed\n");
	}
	#endif

	APP_DBG_NET("mac get: %s\n", mac);
	if ((ret == 0) && (strlen(mac) == 17)) {
		/*genarate SSID*/
		char mac_convert[16];
		int i, j = 0;
		for (i = 0; i < (int)strlen(mac); i++) {
			if (mac[i] != ':') {
				mac_convert[j++] = mac[i];
			}
		}
		mac_convert[j] = '\0';
		sprintf(ssid, "FC-%s", mac_convert);

		/* genarate PASS */
		unsigned char hash[SHA256_DIGEST_LENGTH];
		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		SHA256_Update(&sha256, mac_convert, strlen(mac_convert));
		SHA256_Final(hash, &sha256);

#if (NETWORK_AP_PASS_LEN * 2 < SHA256_DIGEST_LENGTH)
		for (i = 0; i < NETWORK_AP_PASS_LEN / 2; i++) {
			sprintf(pass + (i * 2), "%02X", hash[i]);
		}
#else
		errno "NETWORK_AP_PASS_LEN over hash 256";
#endif
	}
	else {
		string serial = fca_getSerialInfo();
		safe_strcpy(mac, FCA_MAC_DEFAULT, len);
		sprintf(ssid, "FC-%s", serial.c_str());
		sprintf(pass, "%s", serial.c_str());
	}

	return FCA_NET_WIFI_OK;
}

int32_t fca_netWifiAPSetConfig(const char *ifname, fca_netWifiAPConfig_t *config) {
	FILE *f = fopen(FCA_NET_WIFI_AP_HOSTADP_FILE, "w");
	if (f == NULL) {
		APP_DBG_NET(TAG "APmode open file %s failed: %s\n", FCA_NET_WIFI_AP_HOSTADP_FILE, strerror(errno));
		return FCA_NET_WIFI_SET_CONF_FAIL;
	}

	fprintf(f, "interface=%s\n", ifname);
	fprintf(f, "ctrl_interface=/var/run/hostapd\n");
	fprintf(f, "hw_mode=g\n");
	fprintf(f, "ieee80211n=1\n");
	fprintf(f, "ssid=%s\n", config->ssid);
	fprintf(f, "channel=1\n");
	fprintf(f, "wpa=2\n");
	fprintf(f, "wpa_passphrase=%s\n", config->pass);

	fclose(f);

	f = fopen(FCA_NET_WIFI_AP_UDHCPD_FILE, "w");
	if (f == NULL) {
		APP_DBG_NET(TAG "APmode open file %s failed: %s\n", FCA_NET_WIFI_AP_UDHCPD_FILE, strerror(errno));
		return FCA_NET_WIFI_SET_CONF_FAIL;
	}

	fprintf(f, "interface %s\n", ifname);
	fprintf(f, "start %s\n", config->startIP);
	fprintf(f, "end %s\n", config->endIP);
	fprintf(f, "opt subnet %s\n", config->ip.subnet);
	fprintf(f, "opt lease 28800\n");
	fprintf(f, "opt router %s\n", config->ip.gateway);
	fprintf(f, "opt dns %s\n", config->ip.dns);

	fclose(f);
	return FCA_NET_WIFI_OK;
}

int32_t fca_netWifiAPStart(const char *ifname, const char *ssid, const char *passwd, const char *ip) {
	int ret = fca_wifi_start_ap_mode(ifname, ssid, passwd, ip);
	if (ret != FCA_NET_WIFI_OK) {
		return FCA_NET_WIFI_START_FAIL;
	}
	fca_netAPThreadStart();
	return FCA_NET_WIFI_OK;
}

int32_t fca_netWifiAPStop() {
	timer_remove_attr(GW_TASK_NETWORK_ID, GW_NET_RUN_HOSTAPD);
	fca_netAPThreadStop();
	// int ret = fca_wifi_stop_ap_mode(ifname);
	// if (ret != FCA_NET_WIFI_OK) {
	// 	APP_DBG_NET("stop ap mode failed\n");
	// 	return FCA_NET_WIFI_START_FAIL;
	// }
	return FCA_NET_WIFI_OK;
}

int32_t fca_netWifiScanHostAP(const char *ifname, fca_netWifiStaScan_t *list) {
	int ret = FCA_NET_WIFI_OK;
	uint8_t cmd[128];

	memset(cmd, 0, sizeof(cmd));
	sprintf((char *)cmd, FCA_WIFI_STA_CMD_SCAN_START, ifname);
	ret = netWifiStaScanHostAP(cmd);
	if (ret != FCA_NET_WIFI_OK) {
		APP_DBG_NET(TAG "start scan failed with ret: %d", ret);
		return FCA_NET_WIFI_STA_SCAN_FAIL;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf((char *)cmd, FCA_WIFI_STA_CMD_SCAN_RESULT, ifname);
	ret = netWifiStaScanHostAPResult(cmd, list);
	if (ret != FCA_NET_WIFI_OK) {
		APP_DBG_NET(TAG "get result scan failed with ret: %d", ret);
		return FCA_NET_WIFI_STA_SCAN_RESULT_FAIL;
	}

	return ret;
}

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

static bool netGetDataFromSocket(char *buffers, int len, fca_netWifiSocket_t *inf) {
	try {
		// /* Step 1: Decode Base64 */
		// int decoder64Len = 0;
		// unsigned char *decoder64 = OpenSSL_Base64Decoder(buffers, &decoder64Len);
		// if (!decoder64) {
		// 	APP_DBG("Can't decode Base64\r\n");
		// 	return false;
		// }

		// /* Step 2: Decrypt RNCrypto */
		// int DecryptLen = 0;
		// char errBuffers[512] = {0};
		// unsigned char *Decrypt = rncryptorc_decrypt_data_with_password(
		// 							(const unsigned char*)decoder64, decoder64Len,
		// 							100,
		// 							"FCA@123", strlen("FCA@123"),
		// 							&DecryptLen,
		// 							errBuffers,
		// 							sizeof(errBuffers));
		// if (!Decrypt) {
		// 	APP_DBG("Can't decrypt by RNCrypto\r\n");
		// 	return false;
		// }
		// free(decoder64);
		// APP_DBG("Decrypt -> %s\r\n", Decrypt);

		/* Step 3: Parser Json */
		nlohmann::json js = json::parse(string((const char*)buffers, len));
		// nlohmann::json js = json::parse(string((const char*)Decrypt, DecryptLen));
		string str	 = js["ssid"].get<string>();
		safe_strcpy(inf->ssid, str.data(), sizeof(inf->ssid));
		str = js["password"].get<string>();
		safe_strcpy(inf->pass, str.data(), sizeof(inf->pass));

		if (js.contains("ts")) {
			inf->ts = js["ts"].get<uint32_t>();
		}
		else {
			inf->ts = 0;
		}

		return true;
	}
	catch (const exception &error) {
		APP_DBG_NET(TAG "APmode %s\n", error.what());
	}

	return false;
}

static int32_t netWifiGetPIDUdhcpc(const char *ifname, int32_t &pid) {
	uint8_t pidFileName[64] = {0};
	sprintf((char *)pidFileName, FCA_NET_UDHCPC_PID_FILE, ifname);

	FILE *f = fopen((char *)pidFileName, "r");
	if (f == NULL) {
		APP_DBG_NET(TAG "STAmode get file %s not found: %s\n", pidFileName, strerror(errno));
		return FCA_NET_WIFI_FAIL;
	}

	if (fscanf(f, "%d", &pid) != 1) {
		APP_DBG_NET(TAG "STAmode get pid udhcpc %s failed: %s\n", pidFileName, strerror(errno));
		fclose(f);
		return FCA_NET_WIFI_FAIL;
	}

	fclose(f);
	APP_DBG_NET(TAG "STAmode udhcpc pid: %d\n", pid);
	return 0;
}

static int32_t netWifiStaScanHostAP(uint8_t *cmd) {
	int32_t ret		= FCA_NET_WIFI_FAIL;
	uint8_t cnt		= 10;
	uint8_t buf[16] = {0};

	while (--cnt) {
		FILE *f = popen((const char *)cmd, "r");
		if (f == NULL) {
			APP_DBG_NET(TAG "scan failed: %s", strerror(errno));
			return FCA_NET_WIFI_FAIL;
		}

		while (fgets((char *)buf, sizeof(buf), f) != NULL) {
			uint8_t *str = (uint8_t *)strstr((const char *)buf, "OK");
			if (str != NULL) {
				ret = FCA_NET_WIFI_OK;
				break;
			}
		}

		pclose(f);
		if (ret == FCA_NET_WIFI_OK)
			break;

		sleep(1);
	}

	return ret;
}

static int32_t netWifiStaScanHostAPResult(uint8_t *cmd, fca_netWifiStaScan_t *info) {
	uint8_t buf[2048] = {0};
	uint8_t freq[4]	  = {0};
	uint8_t signal[4] = {0};

	FILE *f = popen((const char *)cmd, "r");
	if (f == NULL) {
		APP_DBG_NET(TAG "scan failed: %s", strerror(errno));
		return FCA_NET_WIFI_FAIL;
	}

	int fd	  = fileno(f);
	int flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
	sleep(1);

	int32_t i = 0;
	while (fgets((char *)buf, sizeof(buf), f) != NULL) {
		if (strstr((const char *)buf, "bssid") != NULL)
			continue;

		sscanf((const char *)buf, "%s\t%s\t%s\t%s\t%[^\n]", info->item[i].mac, freq, signal, info->item[i].flag, info->item[i].ssid);
		info->item[i].freq	 = atoi((const char *)freq);
		info->item[i].signal = atoi((const char *)signal);

		if (++i >= FCA_WIFI_SCAN_HOST_AP_NUM_MAX)
			break;
	}

	info->number = i;

	pclose(f);
	return 0;
}
