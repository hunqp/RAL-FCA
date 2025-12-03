/**
 * @file fca_system.h
 * @brief System API for FPT Camera Agent
 * 
 * This header file contains declarations for system-level operations including:
 * - Password management (uboot and root)
 * - Watchdog control
 * - System information retrieval
 */

#ifndef FCA_SYSTEM_H
#define FCA_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set uboot password
 * 
 * This function allows to set uboot password. Default password is 'AskFPTForPasswordLater'.
 * Max length of password is 32 characters.
 * 
 * @param passwd The new password to set
 * @param len Length of the new password 
 * @return int 0 on success, non-zero on failure
 */
int fca_sys_set_uboot_passwd(char *passwd,int len);

/**
 * @brief Set root login password
 * 
 * This function allows to set root login password. Default password is none,
 * login with root account with 'enter' key. Max length of password is 32 characters.
 * 
 * @param passwd The new password to set
 * @param len Length of the new password 
 * @return int 0 on success, non-zero on failure
 */
int fca_sys_set_root_passwd(char *passwd,int len);

/**
 * @brief Initialize the watchdog
 * 
 * FCA will call the reset watchdog function in the app.
 * Input watchdog timeout in seconds (0 < timeout <= 300). If the value is incorrect,
 * it will be automatically set to 120.
 * 
 * @param timeout Watchdog timeout in seconds
 * @return int 0 on success, non-zero on failure
 */
int fca_sys_watchdog_init(int timeout);

/**
 * @brief Stop the watchdog (blocking)
 * 
 * @return int 0 on success, non-zero on failure
 */
int fca_sys_watchdog_stop_block(void);

/**
 * @brief Refresh (feed) the watchdog
 * 
 * @return int 0 on success, non-zero on failure
 */
int fca_sys_watchdog_refresh(void);

/**
 * @brief Get the serial number of the FCA system
 * 
 * The serial number can be obtained via printenv: serial_number=serial_O24020000093
 * 
 * @param sn Buffer to store the serial number
 * @param len Length of the buffer
 * @return int 0 on success, non-zero on failure
 */
int fca_sys_get_sn(char *sn, int len);

#ifdef __cplusplus
}
#endif

#endif /* FCA_SYSTEM_H */
