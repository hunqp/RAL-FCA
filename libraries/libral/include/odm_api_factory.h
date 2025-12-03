/**
 * @file fca_factory.h
 * @brief Factory test mode interface for FCA Camera System
 * 
 * This header provides APIs to control and check factory test mode of the camera system.
 * Factory mode is used during production testing at manufacturing facilities.
 */

#ifndef FCA_FACTORY_H
#define FCA_FACTORY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Restore the system to production test mode
 * 
 * This function enables the ENV flag so that the camera runs the ODM test code after reboot.
 * Used for full feature testing of the camera at the factory.
 * 
 * @return int 0 on success, negative error code on failure
 */
int fca_sys_set_factory(void);

/**
 * @brief Check if system is in production test mode
 * 
 * Reads the ENV to determine if it's in factory test mode or running the normal FCA app.
 * 
 * @return int 1 if in factory mode, 0 if in normal mode, negative error code on failure
 */
int fca_sys_get_factory(void);

/**
 * @brief Start factory test procedures
 * 
 * This function initiates the factory test sequence when called.
 * Should only be called when system is in factory mode.
 */
void fca_sys_factory_start(void);

#ifdef __cplusplus
}
#endif

#endif /* FCA_FACTORY_H */
