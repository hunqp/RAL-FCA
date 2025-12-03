#ifndef __FCA_BLUETOOTH_H__
#define __FCA_BLUETOOTH_H__

#include "fca_parameter.hpp"
#include "odm_api_ble.h"

extern int fca_bluetooth_start(const char *sn, const char *ssid, const char *pssk);
extern int fca_bluetooth_close(void);

#endif /*__FCA_BLUETOOTH_H__*/
