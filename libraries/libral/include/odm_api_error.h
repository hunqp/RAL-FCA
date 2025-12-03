/**
 * @file fca_error.h
 * @brief Defines common error codes for FPT Camera Agent (FCA) API interfaces
 * 
 * This header contains standardized error codes that are used across all FCA modules
 * including Video, Audio, ISP, System, Network, Event, GPIO, PTZ, OTA, and Bluetooth APIs.
 */

#ifndef FCA_ERROR_H
#define FCA_ERROR_H
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#define FCA_LOG(msg, arg...)      printf("[XM_FCA] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define FCA_SUCCESS                       0   /**< Operation completed successfully */
#define FCA_ERROR                        -1   /**< General/unspecified error */
#define FCA_ERROR_INVALID_PARAM          -2   /**< Invalid parameter passed */
#define FCA_ERROR_NOT_SUPPORTED          -3   /**< Function not supported */
#define FCA_ERROR_NOT_INITIALIZED        -4   /**< Module not initialized */
#define FCA_ERROR_ALREADY_INIT           -5   /**< Module already initialized */
#define FCA_ERROR_TIMEOUT                -6   /**< Operation timed out */
#define FCA_ERROR_NO_MEMORY              -7   /**< Memory allocation failed */
#define FCA_ERROR_IO                     -8   /**< Input/output error */
#define FCA_ERROR_BUSY                   -9   /**< Resource busy */
#define FCA_ERROR_NO_RESOURCE           -10   /**< Resource not available */
#define FCA_ERROR_PERMISSION            -11   /**< Permission denied */
#define FCA_ERROR_NOT_FOUND             -12   /**< Resource not found */
#define FCA_ERROR_INVALID_STATE         -13   /**< Invalid state for operation */
#define FCA_ERROR_OVERFLOW              -14   /**< Buffer overflow */
#define FCA_ERROR_UNDERFLOW             -15   /**< Buffer underflow */
#define FCA_ERROR_HW_FAILURE            -16   /**< Hardware failure */
#define FCA_ERROR_SW_FAILURE            -17   /**< Software failure */
#define FCA_ERROR_COMM_FAILURE          -18   /**< Communication failure */
#define FCA_ERROR_AUTH_FAILURE          -19   /**< Authentication failure */
#define FCA_ERROR_CONFIG                -20   /**< Configuration error */
#define FCA_ERROR_DATA_CORRUPT          -21   /**< Data corruption detected */
#define FCA_ERROR_VERSION_MISMATCH      -22   /**< Version mismatch */
#define FCA_ERROR_OPERATION_ABORTED     -23  /**< Operation aborted */
#define FCA_ERROR_NOT_IMPLEMENTED       -24   /**< Function not implemented */
#define FCA_ERROR_EXTERNAL_INTERRUPT    -25   /**< Function stopped by external interrupt (e.g., forced termination from outside) */
#define FCA_ERROR_OTA_IN_PROGRESS       -26   /**< OTA update is already in progress, reject new request */
#define FCA_ERROR_ALREADY_INITIALIZED   -27   /**< Module already initialized */

#ifdef __cplusplus
}
#endif

#endif /* FCA_ERROR_H */
