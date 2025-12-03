#ifndef ODM_API_VERSION_H
#define ODM_API_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif
/*------------------------------------------------------------------------------
 * Version Information
 *----------------------------------------------------------------------------*/
/**
 * @def FCA_API_VERSION_STRING
 * @brief String representation of FCA API version
 */
#define FCA_API_VERSION_STRING "V1.01.06"

/* Version history:
 * V1.01.00 (2025-10-23):
 *  Version Description: This version has a residual issue where the sub-stream abnormally fails to output streams, resulting in the temporary unavailability of QR code scanning and screenshot functions.
    It does not support the configuration or usage of the third stream.
    The audio input configuration only supports the setting of index 0.
    For audio output, testing at 8k or 16k is currently recommended; other higher sampling rates may malfunction when using AAC or G711 formats.
    
 * V1.01.01 (2025-10-24):
 * 1.Fixed the issue of secondary stream initialization failure and video stream loading failure. The QR code and screenshot-related interfaces have been verified to be normal and can be used properly.
 * 2.Fixed the error reporting issue with audio capture timestamps.
 * 3.For this update, the device needs to re-upgrade ovflash. Please confirm that the driver update is successful before verifying the above functions.

 * V1.01.02 (2025-11-1): 
 * 1.Update the WIFI-related functions in the network module; please refer to the usage of the test_wifi_sta function and test_wifi_ap function in test_all.
 * 2.Wired network configuration-related functions are not supported; all calls to these functions will return the error code FCA_ERROR_NOT_SUPPORTED (-3).
 * 
 * V1.01.03 (2025-11-7):
 * 1.Add the len field to the fca_sys_set_uboot_passwd function and the fca_sys_set_root_passwd function.
 * 2.Add a new OTA upgrade function: pre-store the version-specific upgrade files in the device's SD card,
 *   modify the file name specified in fca_sys_fw_ota within test_all, 
 *   run the test_all file to check if the function returns 0 as expected, and verify if the version number in the /etc/sysversion file updates to the expected version after the device restarts.
 * 
 * V1.01.04 (2025-11-14):
 * This update primarily focuses on the GPIO, IRCUT, and SYSTEM library functions:
 * GPIO:
 * 1.This product only has white LEDs and green LEDs; the original definition of FCA_LED_COLOR_YELLOW has been updated to FCA_LED_COLOR_GREEN.
 * 2.It does not support the luminance field in the fca_led_status_t structure. 
 *   The interval field represents the common blinking duration for all LEDs, and configuring two different blinking durations for two LEDs is not supported.
 * 3.The fca_set_led_lighting function and fca_get_led_lighting function should belong to the IRCUT module. 
 *   Relevant call test sets are included in the Test_IrCut function and can be used after the initialization of IRCUT is completed.
 * 4.Added a new fca_gpio_key_deinit function, which supports deinitializing GPIO.
 * 
 * IRCUT:
 * 1.This product features a soft photosensitive design, which adaptively enables night vision mode based on the brightness of the device's image. 
 *   Therefore, the configuration of the day2night_threshold field and night2day_threshold field is not supported.
 * 2.The fca_dn_threshold_set function will return "not supported".
 * 
 * SYSTEM:
 * 1.Since uboot cannot be logged into, the fca_sys_set_uboot_passwd function is meaningless and will return "not supported".
 * 2.Due to security concerns, the fca_sys_set_root_passwd interface is temporarily not supported, but default password configuration can be provided.
 * 3.The fca_sys_get_sn function has not yet defined the SN writing location and will temporarily return "not supported".
 * 
 * V1.01.05 (2025-11-26):
 * Update BLE (Bluetooth) related functions:
 * 1.A new fca_ble_deinit function is added to support deinitialization of the BLE module.
 * 2.For the calling logic of BLE-related functions, please refer to the test_ble_workflow function.
 * 
 * V1.01.06 (2025-11-28):
 * Update PTZ-related functions:
 * 1.The humanoid tracking function is not currently supported.
 */

/*----------------------------------------------------------------------------*/
/* Common Definitions                                                         */
/*----------------------------------------------------------------------------*/

/**
 * @defgroup common_defs Common Definitions
 * @brief Shared data types and constants used across modules
 * @{
 */
#ifdef __cplusplus
}
#endif

#endif
