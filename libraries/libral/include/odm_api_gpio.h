/**
 * @file fca_gpio.h
 * @brief GPIO interface for FPT Camera Agent
 * 
 * This header defines the GPIO control interface including LED status control,
 * lighting control, and button status reading functionalities.
 */

#ifndef FCA_GPIO_H
#define FCA_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED color enumeration
 * Currently, the product only has white lights and green lights;
 * modify the definition of the yellow light to green light.
 */
typedef enum {
    FCA_LED_COLOR_UNKNOWN = 0,  ///< Unknown LED color
    FCA_LED_COLOR_GREEN = 1,    ///< GREEN LED color
    // FCA_LED_COLOR_YELLOW = 1,   ///< Yellow LED color
    FCA_LED_COLOR_WHITE = 2,    ///< White LED color
    FCA_LED_COLOR_MAX           ///< Maximum LED color value
} FCA_LED_COLOR_E;

/**
 * @brief LED mode enumeration
 */
typedef enum {
    FCA_LED_MODE_OFF = 0,      ///< LED turned off
    FCA_LED_MODE_ON,           ///< LED turned on
    FCA_LED_MODE_BLINKING,     ///< LED blinking mode
} FCA_LED_MODE_E;

/**
 * @brief LED status structure
 */
typedef struct {
    FCA_LED_COLOR_E color;     ///< LED color
    FCA_LED_MODE_E mode;       ///< LED mode (off/on/blinking)
    int interval;              ///< Blinking interval in milliseconds
    int luminance;             ///< LED brightness level,not support
} fca_led_status_t;

/**
 * @brief Lighting LED type enumeration
 */
typedef enum {
    FCA_LIGHTING_LED_WHITE = 0,  ///< White lighting LED,not support
    FCA_LIGHTING_LED_IR          ///< IR lighting LED
} FCA_LIGHTING_LED_TYPE;

/**
 * @brief Button type enumeration
 */
typedef enum {
    FCA_BTN_RESET,              ///< Reset button
    FCA_BTN_USER                ///< User button
} FCA_BTN_TYPE;

/**
 * @brief Button status enumeration
 */
typedef enum {
    FCA_BTN_STATUS_UNKNOWN = 0, ///< Unknown button status
    FCA_BTN_STATUS_PRESS,       ///< Button pressed
    FCA_BTN_STATUS_RELEASE,     ///< Button released
} FCA_BTN_STATUS;

/**
 * @brief Set the status of guiding LED
 * 
 * @param status Pointer to LED status structure
 * @return int 0 on success, negative error code on failure
 */
int fca_gpio_led_status_set(fca_led_status_t status);

/**
 * @brief Get the current status of guiding LED
 * 
 * @param status Pointer to store LED status
 * @return int 0 on success, negative error code on failure
 */
int fca_gpio_led_status_get(fca_led_status_t *status);

/**
 * @brief Set lighting LED status
 * 
 * @param type Lighting LED type (white/IR)
 * @param value Lighting value (0-100)
 * @return int 0 on success, negative error code on failure
 * The call example of this function has been placed in the IRCUT module.
 */
int fca_set_led_lighting(FCA_LIGHTING_LED_TYPE type, int value);

/**
 * @brief Get lighting LED status
 * 
 * @param type Lighting LED type (white/IR)
 * @param value Pointer to store lighting value
 * @return int 0 on success, negative error code on failure
 * The call example of this function has been placed in the IRCUT module.
 */
int fca_get_led_lighting(FCA_LIGHTING_LED_TYPE type, int *value);

/**
 * @brief Initialize LED and button GPIO
 * 
 * @return int 0 on success, negative error code on failure
 */
int fca_gpio_key_init();
/**
 * @brief Deinitialize LED and button GPIO
 * 
 * @return int 0 on success, negative error code on failure
 */
int fca_gpio_key_deinit();
/**
 * @brief Get button status
 * 
 * @param type Button type (reset/user)
 * @param status Pointer to store button status
 * @return int 0 on success, negative error code on failure
 */
int fca_get_button_status(FCA_BTN_TYPE type, FCA_BTN_STATUS *status);

#ifdef __cplusplus
}
#endif

#endif /* FCA_GPIO_H */
