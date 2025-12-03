/**
 * @file fca_ota.h
 * @brief FPT Camera Agent Over-The-Air (OTA) Update API
 * 
 * This header file defines the interface for performing firmware updates
 * over-the-air on FPT camera devices. It provides functions to initiate,
 * monitor, and complete OTA firmware updates.
 */

#ifndef FCA_OTA_H
#define FCA_OTA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perform firmware Over-The-Air (OTA) update for the system
 * @param filepath Path to the OTA firmware file
 * @return int Returns 0 on success, negative error code on failure
 */
int fca_sys_fw_ota(char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* FCA_OTA_H */
