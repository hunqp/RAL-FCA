#ifndef __WIFI_H__
#define __WIFI_H__

#include <pthread.h>
#include "fca_parameter.hpp"

#define NETWORK_AP_PASS_LEN	 14
#define NETWORK_AP_HOST_PORT 1234

#define NETWORK_AP_HOST_IP	 "10.112.20.1"
#define NETWORK_AP_START_IP	 "10.112.20.100"
#define NETWORK_AP_END_IP	 "10.112.20.254"
#define NETWORK_AP_SUBNET_IP "255.255.255.0"

#define NET_CONNECT_ROUTER_STATUS_FILE "/var/run/netConnectStatus"

#define SOCKET_STATUS_OK  "{\"ok\"}"
#define SOCKET_STATUS_ERR "{\"error\"}"

typedef enum {
	FCA_NET_WIFI_FAIL = -1,
	FCA_NET_WIFI_OK	  = 0,
	FCA_NET_WIFI_SET_CONF_FAIL,
	FCA_NET_WIFI_GET_CONF_FAIL,
	FCA_NET_WIFI_START_FAIL,
	FCA_NET_WIFI_STA_SCAN_FAIL,
	FCA_NET_WIFI_STA_SCAN_RESULT_FAIL,
	FCA_NET_WIFI_INVALID_PARAMETER,
	FCA_NET_WIFI_OTHER_FAIL,
} FCA_NET_WIFI_RET_E;

typedef enum {
	FCA_NET_WIFI_IP_STATIC = 1,
	FCA_NET_WIFI_IP_DYNAMIC,
	FCA_NET_WIFI_IP_MAX_TYPE
} FCA_NET_WIFI_IP_TYPE_E;

typedef enum {
	FCA_NET_WIFI_MODE_IDLE,
	FCA_NET_WIFI_MODE_STA,
	FCA_NET_WIFI_MODE_AP,
	FCA_NET_WIFI_MODE_MAX,
} FCA_NET_WIFI_MODE_E;

typedef struct {
	int fClient;
	int fServer;
	uint32_t port;
	char hostIP[16];
	char ssid[64];
	char pass[64];
	uint32_t ts;
} fca_netWifiSocket_t;

typedef struct {
	char ssid[32];
	char pass[32];
	char startIP[16];
	char endIP[16];

	fca_netIPAddress_t ip;
} fca_netWifiAPConfig_t;

typedef struct {
	pthread_mutex_t mutex;
	fca_netIPAddress_t ip;
	FCA_NET_WIFI_MODE_E mode;
} fca_netWifiHandle_t;

extern int32_t fca_netWifiSTASetConfig(FCA_NET_WIFI_S *config);
extern int32_t fca_netWifiSTAStart(const char *ifname);
extern int32_t fca_netWifiSTAStop(const char *ifname);

extern void fca_netAPThreadStart();
extern void fca_netAPThreadStop();
extern void fca_netAPRelease();
extern int32_t fca_netWifiAPGenInfo(char *mac, int len, char *ssid, char *pass);
extern int32_t fca_netWifiAPSetConfig(const char *ifname, fca_netWifiAPConfig_t *config);
extern int32_t fca_netWifiAPStart(const char *ifname, const char *ssid, const char *passwd, const char *ip);
extern int32_t fca_netWifiAPStop();

extern int32_t fca_netWifiScanHostAP(const char *ifname, fca_netWifiStaScan_t *list);
extern int8_t fca_netWifiDhcpStart(const char *ifname);
extern void fca_netWifiDhcpStop(const char *ifname);

#endif /* __WIFI_H__ */
