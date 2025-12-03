/**
 * @file fca_isp.h
 * @brief Image Signal Processing API for FPT Camera Agent
 * 
 * This header file defines the interface for controlling various image processing
 * functions including brightness, contrast, saturation, sharpness adjustments,
 * mirror/flip operations, anti-flicker settings, white balance, WDR, denoising,
 * OSD text/time display, and privacy mask configurations.
 */

#ifndef FCA_ISP_H
#define FCA_ISP_H

#include <stdint.h>
#include "odm_api_type_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ISP configuration structure containing basic image parameters
 */
typedef struct {
    int brightness;   /**< Brightness level */
    int contrast;     /**< Contrast level */
    int saturation;   /**< Saturation level */
    int sharpness;    /**< Sharpness level */
} fca_isp_config_t;

/**
 * @brief Enumeration for mirror and flip directions
 */
typedef enum {
    FCA_ISP_MIRROR_FLIP_NONE,        /**< No mirror or flip */
    FCA_ISP_MIRROR,                  /**< Mirror only */
    FCA_ISP_FLIP,                    /**< Flip only */
    FCA_ISP_MIRROR_AND_FLIP          /**< Mirror and flip */
} FCA_ISP_MIRROR_FLIP_E;

/**
 * @brief Enumeration for flicker reduction types
 */
typedef enum {
    FCA_ISP_FLICKER_TYPE_AUTO,       /**< Automatic flicker detection */
    FCA_ISP_FLICKER_TYPE_50HZ,       /**< 50 Hz flicker reduction */
    FCA_ISP_FLICKER_TYPE_60HZ,       /**< 60 Hz flicker reduction */
    FCA_ISP_FLICKER_TYPE_OFF         /**< Flicker reduction disabled */
} FCA_ISP_FLICKER_TYPE_E;

/**
 * @brief Enumeration for white balance modes
 */
typedef enum {
    FCA_ISP_AWB_MANUAL = 0,          /**< Manual white balance */
    FCA_ISP_AWB_AUTO,                /**< Automatic white balance */
} FCA_ISP_AWB_MODE_E;

/**
 * @brief Range structure for parameter ranges
 */
typedef struct {
    int minimum;                     /**< Minimum value */
    int maximum;                     /**< Maximum value */
} fca_range_t;

/**
 * @brief White balance temperature structure
 */
typedef struct {
    int value;                       /**< Current temperature value */
    fca_range_t range;               /**< Valid range for temperature */
} fca_wb_temp_t;

/**
 * @brief Enumeration for WDR modes
 */
typedef enum {
    FCA_ISP_WDR_DISABLE = 0,         /**< WDR disabled */
    FCA_ISP_WDR_MANUAL,              /**< Manual WDR control */
    FCA_ISP_WDR_AUTO,                /**< Automatic WDR control */
} FCA_ISP_WDR_MODE_E;

/**
 * @brief WDR level structure
 */
typedef struct {
    int value;                       /**< Current WDR level */
    fca_range_t range;               /**< Valid range for WDR level */
} fca_wdr_level_t;

/**
 * @brief Enumeration for denoise modes
 */
typedef enum {
    FCA_ISP_DNR_DISABLE = 0,         /**< Denoise disabled */
    FCA_ISP_DNR_ENABLE,              /**< Denoise enabled */
} FCA_ISP_DNR_MODE_E;

/**
 * @brief Enumeration for OSD positions
 */
typedef enum {
    FCA_OSD_TOP_LEFT = 0,            /**< Top left position */
    FCA_OSD_BOTTOM_LEFT,             /**< Bottom left position */
    FCA_OSD_TOP_RIGHT,               /**< Top right position */
    FCA_OSD_BOTTOM_RIGHT,            /**< Bottom right position */
    FCA_OSD_DISABLE,                 /**< OSD disabled */
} FCA_OSD_POSITION_E;

/**
 * @brief Window configuration structure for privacy masks
 */
typedef struct {
    int x;                           /**< X coordinate of window */
    int y;                           /**< Y coordinate of window */
    int w;                           /**< Width of window */
    int h;                           /**< Height of window */
} fca_window_config_t;

/**
 * @brief Set/Get ISP configuration
 * @param config Pointer to ISP configuration structure
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_config(fca_isp_config_t *config);
int fca_isp_get_config(fca_isp_config_t *config);

/**
 * @brief Set/Get video image rotation and mirror
 * @param direction Mirror/flip direction
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_mirror_flip(FCA_ISP_MIRROR_FLIP_E direction);
int fca_isp_get_mirror_flip(FCA_ISP_MIRROR_FLIP_E *direction);

/**
 * @brief Set/Get anti-flicker configuration
 * @param antiflicker Flicker reduction type
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_antiflicker(FCA_ISP_FLICKER_TYPE_E antiflicker);
int fca_isp_get_antiflicker(FCA_ISP_FLICKER_TYPE_E *antiflicker);

/**
 * @brief Set/Get white balance mode
 * @param mode White balance mode
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_wb_mode(FCA_ISP_AWB_MODE_E mode);
int fca_isp_get_wb_mode(FCA_ISP_AWB_MODE_E *mode);

/**
 * @brief Set/Get white balance temperature
 * @param wb_temp White balance temperature structure
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_wb_temperature(fca_wb_temp_t *wb_temp);
int fca_isp_get_wb_temperature(fca_wb_temp_t *wb_temp);

/**
 * @brief Set/Get wide dynamic range mode
 * @param mode WDR mode
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_wdr_mode(FCA_ISP_WDR_MODE_E mode);
int fca_isp_get_wdr_mode(FCA_ISP_WDR_MODE_E *mode);

/**
 * @brief Set/Get WDR level
 * @param wdr_level WDR level structure
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_wdr_level(fca_wdr_level_t *wdr_level);
int fca_isp_get_wdr_level(fca_wdr_level_t *wdr_level);

/**
 * @brief Set/Get 2D denoise mode
 * @param mode Denoise mode
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_2dnr(FCA_ISP_DNR_MODE_E mode);
int fca_isp_get_2dnr(FCA_ISP_DNR_MODE_E *mode);

/**
 * @brief Set/Get 3D denoise mode
 * @param mode Denoise mode
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_3dnr(FCA_ISP_DNR_MODE_E mode);
int fca_isp_get_3dnr(FCA_ISP_DNR_MODE_E *mode);

/**
 * @brief Set OSD text at specified position
 * @param position OSD position
 * @param text Text to display
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_osd_text(FCA_OSD_POSITION_E position, char *text);

/**
 * @brief Get current OSD text and position
 * @param position Pointer to store OSD position
 * @param text Buffer to store text
 * @param text_len Length of text buffer
 * @return 0 on success, negative error code on failure
 */
int fca_isp_get_osd_text(FCA_OSD_POSITION_E *position, char *text, int text_len);

/**
 * @brief Set OSD time at specified position
 * @param position OSD position
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_osd_time(FCA_OSD_POSITION_E position);

/**
 * @brief Get current OSD time position
 * @param position Pointer to store OSD position
 * @return 0 on success, negative error code on failure
 */
int fca_isp_get_osd_time(FCA_OSD_POSITION_E *position);

/**
 * @brief Set privacy mask configuration for a specific window
 * @param index Window index (0-3)
 * @param window Window configuration (NULL or (0,0,0,0) to disable)
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_privacy_mask(int index, fca_window_config_t *window);
int fca_isp_get_privacy_mask(int index, fca_window_config_t *window);

/**
 * @brief Set/Get ROI grid for privacy mask
 * @param grid ROI grid configuration
 * @return 0 on success, negative error code on failure
 */
int fca_isp_set_roi_grid_privacy_mask(const fca_roi_t *grid);
int fca_isp_get_roi_grid_privacy_mask(fca_roi_t *grid);

/**
 * @brief Enable/disable privacy mask function
 * @param onoff 1 to enable, 0 to disable
 * @return 0 on success, negative error code on failure
 */
int fca_isp_privacy_mask_set_onoff(int onoff);

#ifdef __cplusplus
}
#endif

#endif /* FCA_ISP_H */
