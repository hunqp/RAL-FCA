/**
 * @file fca_ptz.h
 * @brief FPT Camera Agent PTZ (Pan-Tilt-Zoom) API Interface
 * 
 * This header file defines all PTZ control functions for FCA system including:
 * - PTZ initialization and deinitialization
 * - Movement control (direction, position, speed)
 * - Preset positions and patrol modes
 * - Sleep mode and self-check functions
 * - Zoom control
 */

#ifndef FCA_PTZ_H
#define FCA_PTZ_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PTZ callback parameters structure
 * 
 * Contains callback functions for getting and setting saved positions
 */
typedef struct FCA_PTZ_CB_PARAM_S {
    int (*fca_ptz_get_saved_pos_cb)(int* h, int* v); /**< Callback to get saved position */
    int (*fca_ptz_set_saved_pos_cb)(int h, int v);   /**< Callback to set saved position */
} FCA_PTZ_CB_PARAM;

/**
 * @brief PTZ sleep mode enumeration
 */
typedef enum {
    FCA_PTZ_SLEEPMODE_OFF = 0,     /**< Exit sleep mode (privacy mode) */
    FCA_PTZ_SLEEPMODE_CURR_POS,    /**< Enter sleep mode, keep current position */
    FCA_PTZ_SLEEPMODE_DEF_POS      /**< Enter sleep mode, turn to sleep position */
} FCA_PTZ_SLEEPMODE;

/**
 * @brief PTZ self-check mode enumeration
 */
typedef enum {
    FCA_PTZ_SELFCHECK_OFF = 0,     /**< Exit self-check */
    FCA_PTZ_SELFCHECK_CURR_POS,    /**< Self-check ends, return to current position */
    FCA_PTZ_SELFCHECK_DEF_POS,     /**< Self-check ends, return to factory default position */
    FCA_PTZ_SELFCHECK_SAVED_POS    /**< Self-check ends, return to saved position */
} FCA_PTZ_SELFCHECK;

/**
 * @brief PTZ movement direction enumeration
 */
typedef enum {
    FCA_PTZ_DIRECTION_UP = 0,      /**< Move upwards */
    FCA_PTZ_DIRECTION_DOWN,        /**< Move downwards */
    FCA_PTZ_DIRECTION_LEFT,        /**< Move left */
    FCA_PTZ_DIRECTION_RIGHT,       /**< Move right */
    FCA_PTZ_DIRECTION_UPLEFT,      /**< Move up-left */
    FCA_PTZ_DIRECTION_UPRIGHT,     /**< Move up-right */
    FCA_PTZ_DIRECTION_DOWNLEFT,    /**< Move down-left */
    FCA_PTZ_DIRECTION_DOWNRIGHT,   /**< Move down-right */
    FCA_PTZ_DIRECTION_STOP         /**< Stop movement */
} FCA_PTZ_DIRECTION;

/**
 * @brief PTZ speed enumeration
 */
typedef enum {
    TURN_SPEED_LOW = 5000,   /**< Low speed (25Hz) */
    TURN_SPEED_MID = 3125,   /**< Medium speed (40Hz) */
    TURN_SPEED_HIGH = 1250   /**< High speed (100Hz) */
} FCA_PTZ_SPEED;

/**
 * @brief PTZ motor type enumeration
 */
typedef enum {
    FCA_MOTOR_PAN = 0,   /**< Pan motor */
    FCA_MOTOR_TILT,      /**< Tilt motor */
    FCA_MOTOR_ZOOM       /**< Zoom motor */
} FCA_MOTOR_TYPE;

/**
 * @brief PTZ preset position structure
 */
typedef struct FCA_PTZ_PRESET_S {
    int h; /**< Pan step */
    int v; /**< Tilt step */
    int z; /**< Zoom level */
} FCA_PTZ_PRESET;

/**
 * @brief Zoom direction enumeration
 */
typedef enum {
    ZOOM_IN,   /**< Zoom in */
    ZOOM_OUT   /**< Zoom out */
} ZOOM_DIRECTION;

/**
 * @brief Initialize PTZ function
 * @param ptz_cb_param Callback parameters structure
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_init(FCA_PTZ_CB_PARAM *ptz_cb_param);

/**
 * @brief Uninitialize PTZ function
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_uninit_block(void);

/**
 * @brief Set PTZ sleep mode
 * @param mode Sleep mode to set
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_sleepmode(FCA_PTZ_SLEEPMODE mode);

/**
 * @brief Perform PTZ self-check
 * @param mode Self-check mode
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_selfcheck(FCA_PTZ_SELFCHECK mode);

/**
 * @brief Move PTZ in specified direction
 * @param direction Direction to move
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_go_direction(FCA_PTZ_DIRECTION direction);

/**
 * @brief Move PTZ to specified position
 * @param h Horizontal position
 * @param v Vertical position
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_go_position(int h, int v);

/**
 * @brief Get current PTZ position
 * @param h Pointer to store horizontal position
 * @param v Pointer to store vertical position
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_get_position(int* h, int* v);

/**
 * @brief Enable/disable motion tracking
 * @param onoff 1 to enable, 0 to disable
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_set_track_onoff(int onoff);

/**
 * @brief Enable/disable patrol
 * @param onoff 1 to enable, 0 to disable
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_set_patrol_onoff(int onoff);

/**
 * @brief Set patrol preset points
 * @param cnt Number of preset points (1~5)
 * @param list Array of preset points
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_patrol_set_preset_point(int cnt, FCA_PTZ_PRESET* list);

/**
 * @brief Set patrol point stay time
 * @param stay_time Time in seconds to stay at each point
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_patrol_set_stay_time(int stay_time);

/**
 * @brief Set patrol time period
 * @param start_hour Start hour (0-23)
 * @param start_minute Start minute (0-59)
 * @param end_hour End hour (0-23)
 * @param end_minute End minute (0-59)
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_patrol_set_time_period(int start_hour, int start_minute, int end_hour, int end_minute);

/**
 * @brief Set patrol mode
 * @param pmode Patrol mode:
 *              0: panorama round patrol
 *              1: preset point round patrol
 *              2: preset point clockwise patrol
 *              3: preset point counterclockwise patrol
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_patrol_set_pmode(int pmode);

/**
 * @brief Set patrol time mode
 * @param tmode Time mode:
 *              0: all day patrol
 *              1: one-time patrol
 *              2: custom patrol based on time period
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_patrol_set_tmode(int tmode);

/**
 * @brief Set horizontal speed
 * @param speed Speed to set
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_set_speed_h(FCA_PTZ_SPEED speed);

/**
 * @brief Set vertical speed
 * @param speed Speed to set
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_set_speed_v(FCA_PTZ_SPEED speed);

/**
 * @brief Set vertical step
 * @param step Step value
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_set_step_v(int step);

/**
 * @brief Set horizontal step
 * @param step Step value
 * @return 0 on success, negative error code on failure
 */
int fca_ptz_set_step_h(int step);

/**
 * @brief Get maximum steps for motor type
 * @param type Motor type
 * @param max_steps Pointer to store maximum steps
 * @return 0 on success, negative error code on failure
 */
int fca_get_motor_max_steps(FCA_MOTOR_TYPE type, int *max_steps);

#ifdef __cplusplus
}
#endif

#endif /* FCA_PTZ_H */
