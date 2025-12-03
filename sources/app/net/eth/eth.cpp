#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "app.h"
#include "app_dbg.h"
#include "utils.h"
#include "eth.h"
// #include "fca_common.h"

#include "fca_common.hpp"

#define TAG						   "[lan] "
#define FCA_NET_ETH_STATE_PORT_LAN "/sys/class/net/eth0/carrier"

static int32_t netEthernetGetPIDUdhcpc(const char *ifname, int &pid);

int32_t fca_netEthernetGetStatePlugPort() {
	int32_t state = NET_ETH_STATE_UNKNOWN;
	char buf[4];

	FILE *f = fopen(FCA_NET_ETH_STATE_PORT_LAN, "r");
	if (f == NULL) {
		APP_DBG_NET(TAG "fopen file %s failed %s\n", FCA_NET_ETH_STATE_PORT_LAN, strerror(errno));
		return FCA_NET_ETH_FAIL;
	}

	if (fgets(buf, 4, f) != NULL) {
		if (strcmp(buf, "1\n") == 0) {
			state = NET_ETH_STATE_PLUG;
		}
		else if (strcmp(buf, "0\n") == 0)
			state = NET_ETH_STATE_UNPLUG;
		else {
			APP_DBG_NET(TAG "fgets get state from file %s not found state %s\n", FCA_NET_ETH_STATE_PORT_LAN, strerror(errno));
			state = NET_ETH_STATE_UNKNOWN;
		}
	}
	else {
		APP_DBG_NET(TAG "fgets get state from file %s failed %s\n", FCA_NET_ETH_STATE_PORT_LAN, strerror(errno));
		fclose(f);
		return FCA_NET_ETH_FAIL;
	}

	fclose(f);
	return state;
}

int32_t fca_netEthernetDhcpStart(const char *ifname) {
	uint8_t pidFileName[64] = {0};
	sprintf((char *)pidFileName, FCA_NET_UDHCPC_PID_FILE, ifname);
	int32_t ret = systemCmd("udhcpc -i%s -s%s -p%s > /dev/null 2>&1 &", ifname, FCA_UDHCPC_SCRIPT, pidFileName);
	if (ret != 0) {
		APP_DBG_NET(TAG "udhcpc start failed\n");
	}
	return ret;
}


extern void netClearDns(const char *ifname, string fileDNS);

int32_t fca_netEthernetDhcpStop(const char *ifname) {
	int pID = 0;
	if (netEthernetGetPIDUdhcpc(ifname, pID) != 0) {
		APP_DBG_NET(TAG "pID udhcpc not found\n");
	}
	else if (pID > 0) {
		systemCmd("kill -9 %d", pID);
	}

	systemCmd("ifconfig %s 0.0.0.0", ifname);
	netClearDns(ifname, FCA_DNS_FILE);
	return FCA_NET_ETH_OK;
}

static int32_t netEthernetGetPIDUdhcpc(const char *ifname, int32_t &pid) {
	uint8_t pidFileName[64] = {0};
	sprintf((char *)pidFileName, FCA_NET_UDHCPC_PID_FILE, ifname);

	FILE *f = fopen((const char *)pidFileName, "r");
	if (f == NULL) {
		APP_DBG_NET(TAG "fopen get file %s not found: %s\n", pidFileName, strerror(errno));
		return FCA_NET_ETH_FAIL;
	}

	if (fscanf(f, "%d", &pid) != 1) {
		APP_DBG_NET(TAG "fscanf get pid udhcpc %s failed: %s\n", pidFileName, strerror(errno));
		fclose(f);
		return FCA_NET_ETH_FAIL;
	}

	fclose(f);
	return 0;
}
