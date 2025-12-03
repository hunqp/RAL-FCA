/**
 * @file fca_network.h
 * @brief Network API for FPT Camera Agent (FCA) system
 * 
 * This header file defines all network-related interfaces including Ethernet and WiFi
 * functionality for the FCA system. It provides APIs for network configuration,
 * connection management, and status monitoring.
 */

#ifndef FCA_NETWORK_H
#define FCA_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Ethernet port state enumeration
 */
typedef enum {
    FCA_NET_ETH_STATE_UNPLUG,    /**< Ethernet cable is unplugged */
    FCA_NET_ETH_STATE_PLUG,      /**< Ethernet cable is plugged in */
    FCA_NET_ETH_STATE_UNKNOWN    /**< Ethernet cable state is unknown */
} FCA_NET_ETH_STATE_PORT_E;

/**
 * @brief Structure for static IP configuration
 */
typedef struct {
    char ip_address[16];     /**< IP address in dotted decimal notation */
    char subnet_mask[16];    /**< Subnet mask in dotted decimal notation */
    char gateway[16];        /**< Default gateway IP address */
    char dns1[16];           /**< Primary DNS server IP */
    char dns2[16];           /**< Secondary DNS server IP (optional) */
} StaticIPConfig;

/**
 * @brief WiFi network information structure
 */
typedef struct {
    char ssid[32];              /**< WiFi SSID (max 31 chars + null terminator) */
    char encrypt_mode[32];      /**< Encryption mode (e.g., WPA2, WEP, etc.) */
    int signal_strength;        /**< Signal strength in dBm */
    unsigned char mac_address[18]; /**< MAC address as string (XX:XX:XX:XX:XX:XX) */
} wifi_network_info_t;

/**
 * Ethernet Functions
 */

/**
 * @brief Get the Ethernet MAC address of the FCA system
 * @param macaddr Buffer to store the MAC address (min 18 bytes)
 * @param len Length of the buffer
 * @return 0 on success, negative error code on failure
 */
int fca_ethernet_get_mac(char *macaddr, int len);

/**
 * @brief Initialize Ethernet interface
 * @param ifname Network interface name (e.g., "eth0")
 * @return 0 on success, negative error code on failure
 */
int fca_ethernet_init(const char *ifname);

/**
 * @brief Connect to Ethernet using DHCP
 * @param ifname Network interface name
 * @return 0 on success, negative error code on failure
 */
int fca_ethernet_connect_dhcp(const char *ifname);

/**
 * @brief Configure static IP for Ethernet interface
 * @param ifname Network interface name
 * @param config Static IP configuration structure
 * @return 0 on success, negative error code on failure
 */
int fca_ethernet_set_static_ip(const char *ifname, const StaticIPConfig *config);

/**
 * @brief Check Ethernet cable connection status
 * @param ifname Network interface name
 * @return Ethernet port state (FCA_NET_ETH_STATE_PORT_E)
 */
FCA_NET_ETH_STATE_PORT_E fca_check_cable(const char *ifname);

/**
 * @brief Disconnect Ethernet interface
 * @param ifname Network interface name
 * @return 0 on success, negative error code on failure
 */
int fca_ethernet_disconnect(const char *ifname);

/**
 * WiFi Functions
 */

/**
 * @brief Get the WiFi MAC address of the FCA system
 * @param macaddr Buffer to store the MAC address (min 18 bytes)
 * @param len Length of the buffer
 * @return 0 on success, negative error code on failure
 */
int fca_wifi_get_mac(char *macaddr, int len);

/**
 * @brief Start WiFi Access Point (AP) mode
 * @param ifname Network interface name
 * @param ssid AP SSID
 * @param passwd AP password
 * @param ip AP IP address
 * @return 0 on success, negative error code on failure
 */

/*
Password configuration is temporarily unsupported.
*/
int fca_wifi_start_ap_mode(const char *ifname, const char *ssid, const char *passwd, const char* ip);

/**
 * @brief Stop WiFi Access Point (AP) mode
 * @param ifname Network interface name
 * @return 0 on success, negative error code on failure
 */
int fca_wifi_stop_ap_mode(const char *ifname);

/**
 * @brief Connect to a WiFi network
 * @param ifname Network interface name
 * @param ssid WiFi SSID to connect to
 * @param passwd WiFi password
 * @return 0 on success, negative error code on failure
 */
int fca_wifi_connect(const char *ifname, const char *ssid, const char *passwd);

/**
 * @brief Get WiFi connection status
 * @param ifname Network interface name
 * @param isConnected Pointer to store connection status (true if connected)
 * @return 0 on success, negative error code on failure
 */
int fca_wifi_get_status(const char *ifname, bool *isConnected);

/**
 * @brief Connect to WiFi using DHCP
 * @param ifname Network interface name
 * @return 0 on success, negative error code on failure
 */
int fca_wifi_connect_dhcp(const char *ifname);

/**
 * @brief Disconnect from WiFi network
 * @param ifname Network interface name
 * @return 0 on success, negative error code on failure
 */
int fca_wifi_disconnect(const char *ifname);

/**
 * @brief Configure static IP for WiFi interface
 * @param ifname Network interface name
 * @param config Static IP configuration structure
 * @return 0 on success, negative error code on failure
 */
int fca_wifi_set_static_ip(const char *ifname, const StaticIPConfig *config);

/**
 * @brief Scan for available WiFi networks
 * @param networks Array to store network information
 * @param networks_len Length of the array (max number of networks to scan)
 * @return Number of networks found on success, negative error code on failure
 */
int fca_wifi_scan(wifi_network_info_t *networks, int networks_len);

#ifdef __cplusplus
}
#endif

#endif /* FCA_NETWORK_H */
