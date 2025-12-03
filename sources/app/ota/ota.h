#ifndef __OTA_H__
#define __OTA_H__

#include <stdint.h>

typedef enum {
	FCA_OTA_OK = 100,
	FCA_OTA_FAIL,
	FCA_OTA_DL_TIMEOUT,
} FCA_OTA_RET_E;

typedef enum {
	FCA_OTA_STATE_IDLE,
	FCA_OTA_STATE_UPGRADE_SUCCESS,
	FCA_OTA_STATE_DL_SUCCESS,
	FCA_OTA_STATE_DL_FAIL,
} FCA_DL_STATE_E;

int fca_otaDownloadFile(const string &url, const string &file, uint32_t timeout);
extern const char *fca_otaStatusUpgrade(int state);
#endif /* __OTA_H__ */