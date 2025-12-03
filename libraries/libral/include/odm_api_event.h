/**
 * @file fca_event.h
 * @brief Header file for FCA Event Module - Motion Detection and Smart Video Processing
 * 
 * This module provides APIs for motion detection and smart video processing capabilities
 * including human detection, face detection, and alarm zone configuration.
 */

#ifndef FCA_EVENT_H
#define FCA_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "odm_api_type_def.h"

/**
 * @brief Motion detection result structure
 */
typedef struct {
    fca_point_t motion_point;  ///< Coordinates of motion point (top-left is (0,0))
} fca_md_result_t;

/**
 * @brief SVP rectangle coordinates structure
 */
typedef struct {
    unsigned long x;      ///< X-coordinate of rectangle
    unsigned long y;      ///< Y-coordinate of rectangle
    unsigned long width;  ///< Width of rectangle
    unsigned long height; ///< Height of rectangle
} fca_svp_rect_t;

/**
 * @brief SVP detection types
 */
typedef enum {
    FCA_SVP_TYPE_FACE = 0,  ///< Face detection
    FCA_SVP_TYPE_PERSON,    ///< Person detection
} FCA_SVP_TYPE_E;

/**
 * @brief Motion detection callback function type
 * 
 * @param result Contains motion detection results including coordinates of detected motion
 */
typedef void (*FCA_MD_CALLBACK)(fca_md_result_t result);

/**
 * @brief SVP callback function type
 * 
 * @param rect Detected object rectangle coordinates
 * @param type Type of detected object (face or person)
 * @param score Detection confidence score
 */
typedef void (*FCA_SVP_CALLBACK)(fca_svp_rect_t rect, FCA_SVP_TYPE_E type, unsigned long score);

/* Motion Detection APIs */

/**
 * @brief Initialize motion detection module
 * 
 * @param cb Callback function for motion detection events
 * @return int 0 on success, negative error code on failure
 */
int fca_md_init(FCA_MD_CALLBACK cb);

/**
 * @brief Set ROI grid for motion detection
 * 
 * @param grid Pointer to ROI grid configuration
 * @return int 0 on success, negative error code on failure
 */
int fca_md_set_roi_grid(const fca_roi_t *grid);

/**
 * @brief Get current ROI grid configuration
 * 
 * @param grid Pointer to store current ROI grid
 * @return int 0 on success, negative error code on failure
 */
int fca_md_get_roi_grid(fca_roi_t *grid);

/**
 * @brief Uninitialize motion detection module
 * 
 * @return int 0 on success, negative error code on failure
 */
int fca_md_uninit_block(void);

/**
 * @brief Set motion detection sensitivity
 * 
 * @param sen_level Sensitivity level (implementation specific range)
 * @return int 0 on success, negative error code on failure
 */
int fca_md_set_sensitivity(int sen_level);

/**
 * @brief Enable/disable motion detection
 * 
 * @param onoff 1 to enable, 0 to disable
 * @return int 0 on success, negative error code on failure
 */
int fca_md_set_onoff(int onoff);

/* Smart Video Processing (SVP) APIs */

/**
 * @brief Initialize SVP module
 * 
 * @param cb Callback function for SVP detection events
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_init(FCA_SVP_CALLBACK cb);

/**
 * @brief Uninitialize SVP module
 * 
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_uninit_block(void);

/**
 * @brief Enable/disable human filtering
 * 
 * @param onoff 1 to enable, 0 to disable
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_humfil_set_onoff(int onoff);

/**
 * @brief Enable/disable human focus
 * 
 * @param onoff 1 to enable, 0 to disable
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_humfocus_set_onoff(int onoff);

/**
 * @brief Enable/disable alarm zone detection
 * 
 * @param onoff 1 to enable, 0 to disable
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_alarmzone_set_onoff(int onoff);

/**
 * @brief Set alarm zone detection area
 * 
 * @param index Zone index (0-4)
 * @param x X-coordinate of zone
 * @param y Y-coordinate of zone
 * @param w Width of zone
 * @param h Height of zone
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_alarmzone_set_area(int index, int x, int y, int w, int h);

/**
 * @brief Delete alarm zone area
 * 
 * @param index Zone index to delete
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_alarmzone_delet_area(int index);

/**
 * @brief Enable/disable frame drawing
 * 
 * @param index Frame index
 * @param enable 1 to enable, 0 to disable
 * @return int 0 on success, negative error code on failure
 */
int fca_svp_draw_enable(int index, int enable);

#ifdef __cplusplus
}
#endif

#endif /* FCA_EVENT_H */
