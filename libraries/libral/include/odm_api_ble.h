
/**
 * @file fca_ble.h
 * @brief Bluetooth Low Energy (BLE) interface for FPT Camera Agent
 * 
 * Provides APIs for BLE stack initialization, advertising, connection management
 * and GATT service operations.
 */

#ifndef FCA_BLE_H
#define FCA_BLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BLE event types
 */
typedef enum {
    FCA_BLE_INIT_DONE,            /**< BLE stack initialization complete */
    FCA_BLE_ADV_ENABLE,           /**< Advertising enabled */
    FCA_BLE_ADV_DISABLE,          /**< Advertising disabled */
    FCA_BLE_CONNECTION_IND,       /**< Connection indication */
    FCA_BLE_DISCONNECTED,         /**< Disconnected from peer */
    FCA_BLE_SMARTCONFIG_DATA_RECV /**< Received smart config data */
} FCA_BLE_EVT_CB;

/**
 * @brief Data buffer structure for BLE operations
 */
typedef struct {
    unsigned int len;         /**< Length of data */
    unsigned char *data;      /**< Pointer to data */
} fca_ble_data_buf_t;

/**
 * @brief Set Bluetooth service configuration file
 * @param path Path to configuration file
 * @return 0 on success, negative error code on failure
 */
int fca_ble_service_set(const char *path);

/**
 * @brief Load Bluetooth kernel module
 * @note Stops WiFi mode when executed
 * @return 0 on success, negative error code on failure
 */
int fca_ble_insmod_driver();

/**
 * @brief Initialize BLE protocol stack
 * @param fca_ble_msg_callback Callback for BLE events
 * @return 0 on success, negative error code on failure
 */
int fca_ble_stack_init(void (*fca_ble_msg_callback)(FCA_BLE_EVT_CB, fca_ble_data_buf_t *));

/**
 * @brief Set BLE device name
 * @param buff Pointer to name buffer
 * @param len Length of name
 * @return 0 on success, negative error code on failure
 */
int fca_ble_gap_name_set_msg_send(uint8_t *buff, uint8_t len);

/**
 * @brief Set advertising data and scan response
 * @param adv Advertising data (max 29 bytes)
 * @param scan_resp Scan response data (max 29 bytes)
 * @return 0 on success, negative error code on failure
 */
int fca_ble_gap_adv_rsp_data_set(const fca_ble_data_buf_t *adv, const fca_ble_data_buf_t *scan_resp);

/**
 * @brief Start BLE advertising
 * @param adv Advertising data (max 29 bytes)
 * @return 0 on success, negative error code on failure
 */
int fca_ble_gap_adv_start(const fca_ble_data_buf_t *adv);

/**
 * @brief Stop BLE advertising
 * @return 0 on success, negative error code on failure
 */
int fca_ble_gap_adv_stop();

/**
 * @brief Send GATT notification
 * @param notify_data Notification data
 * @return 0 on success, negative error code on failure
 */
int fca_ble_gatts_value_notify(const fca_ble_data_buf_t *notify_data);

/**
 * @brief Update characteristic read value
 * @param read_data New read value data
 * @return 0 on success, negative error code on failure
 */
int fca_ble_gatts_value_read_update(const fca_ble_data_buf_t *read_data);

/**
 * @brief Deinitialize BLE protocol stack
 * 
 * @return int 0 on success, negative error code on failure
 */
int fca_ble_deinit();
#ifdef __cplusplus
}
#endif

#endif /* FCA_BLE_H */
