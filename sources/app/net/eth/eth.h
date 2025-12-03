#ifndef __NET_ETH_H__
#define __NET_ETH_H__

#include "fca_parameter.hpp"
typedef enum {
	FCA_NET_ETH_FAIL = -1,
	FCA_NET_ETH_OK	 = 0,
} FCA_NET_ETH_RET_E;

int32_t fca_netEthernetGetStatePlugPort();
int32_t fca_netEthernetDhcpStart(const char *ifname);
int32_t fca_netEthernetDhcpStop(const char *ifname);

#endif /* __NET_ETH_H__ */
