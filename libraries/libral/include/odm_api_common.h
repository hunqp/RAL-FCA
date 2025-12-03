#ifndef ODM_API_COMMON_H
#define ODM_API_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the common part of the SDK.
 * This interface needs to be called before calling other APIs.
 * return 0 means success, other means failure.
 */
int fca_sdk_init();

#ifdef __cplusplus
}
#endif

#endif
