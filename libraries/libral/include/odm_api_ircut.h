/**
 * @file fca_ircut.h
 * @brief IR Cut Filter Control Module for FPT Camera Agent
 * 
 * This module provides APIs to control and configure the IR cut filter 
 * for automatic day/night mode switching in camera devices.
 */

#ifndef FCA_IRCUT_H
#define FCA_IRCUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Day/Night mode enumeration
 */
typedef enum {
    FCA_DN_MODE_AUTO,  /**< Auto mode (automatically switches between day/night) */
    FCA_DN_MODE_DAY,   /**< Force day mode */
    FCA_DN_MODE_NIGHT  /**< Force night mode */
} FCA_DN_MODE_E;

/**
 * @brief Current environment state
 */
typedef enum {
    FCA_DAY_STATE = 0, /**< Current environment is day */
    FCA_NIGHT_STATE,   /**< Current environment is night */
} FCA_DAY_NIGHT_STATE_E;

/**
 * @brief Day/Night switch callback function type
 * @param state Current environment state after switch
 * @param user_data User provided context data
 */
typedef void (*FCA_DAYNIGHT_CALLBACK)(FCA_DAY_NIGHT_STATE_E state, const void *user_data);

/**
 * @brief IR Cut filter attributes structure
 */
/*
The current device features a soft photosensitive design, 
which adjusts the state between daytime and night vision based on the image. 
Therefore, it does not support setting the judgment threshold for switching between daytime and night modes.
*/
typedef struct t_fca_dn_attr {
    uint32_t lock_time;          /**< IR cut lock duration in seconds (0 = no lock) */
    uint8_t lock_cnt_th;         /**< Switch count threshold to trigger lock (0 = no lock) */
    uint8_t trigger_interval;    /**< Minimum time interval between mode switches in seconds (cannot be 0) */
    int day2night_threshold;     /**< Light threshold for day→night transition  not support*/
    int night2day_threshold;     /**< Light threshold for night→day transition  not support*/
    FCA_DN_MODE_E init_mode;     /**< Initial control mode */
    FCA_DAYNIGHT_CALLBACK cb;    /**< Callback after mode switch */
    const void *user_data;       /**< User data passed to callback */
} fca_dn_attr_t;

/**
 * @brief Initialize IR Cut module with configuration
 * @param dn_attr Pointer to configuration attributes
 * @return 0 on success, negative error code on failure
 */
int fca_dn_switch_set_config(fca_dn_attr_t *dn_attr);

/**
 * @brief Get current IR Cut configuration
 * @param dn_attr Pointer to store current configuration
 * @return 0 on success, negative error code on failure
 */
int fca_dn_switch_get_config(fca_dn_attr_t *dn_attr);

/**
 * @brief Set day/night mode control
 * @param mode Desired control mode
 */
void fca_dn_set_config_mode(FCA_DN_MODE_E mode);

/**
 * @brief Get current day/night mode control
 * @param mode Pointer to store current mode
 */
void fca_dn_get_config_mode(FCA_DN_MODE_E *mode);

/**
 * @brief Set day/night switching thresholds
 * @param day2night_threshold Threshold for day→night transition
 * @param night2day_threshold Threshold for night→day transition
 * @return 0 on success, negative error code on failure
 */
/*
The current device features a soft photosensitive design, 
which adjusts the state between daytime and night vision based on the image. 
Therefore, it does not support setting the judgment threshold for switching between daytime and night modes.
*/
int fca_dn_threshold_set(int day2night_threshold, int night2day_threshold);

/**
 * @brief Get current environment status
 * @param env_status Pointer to store current environment status
 * @return 0 on success, negative error code on failure
 */
int fca_dn_get_current_env_status(FCA_DAY_NIGHT_STATE_E *env_status);

/**
 * @brief Release all resources used by IR Cut module
 * @return 0 on success, negative error code on failure
 */
int fca_dn_mode_resource_release();

#ifdef __cplusplus
}
#endif

#endif /* FCA_IRCUT_H */
