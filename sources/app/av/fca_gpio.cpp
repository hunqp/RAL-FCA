#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#if SUPPORT_ETH
#include "eth.h"
#endif

#include "app.h"
#include "app_dbg.h"
#include "app_config.h"
#include "fca_common.hpp"
#include "utils.h"
#include "task_list.h"
#include "fca_gpio.hpp"

Indicator ledStatus;
MOTORS_H_AND_V motors;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int afterGettingPosition(int *horizone, int *vertical) {
	if (motors.implDoAfterGettingPosition) {
		motors.implDoAfterGettingPosition(*horizone, *vertical);
	}

	return 0;
}

static int afterSettingPosition(int horizone, int vertical) {
	if (motors.implDoAfterSettingPosition) {
		motors.implDoAfterSettingPosition(horizone, vertical);
	}

	return 0;
}

void MOTORS_H_AND_V::initialise(
	void (*doAfterGettingPosition)(int,int), 
	void (*doAfterSettingPosition)(int,int)
)
{
	implDoAfterGettingPosition = doAfterGettingPosition;
	implDoAfterSettingPosition = doAfterSettingPosition;

	#if SUPPORT_PTZ
	FCA_PTZ_CB_PARAM cb;
    cb.fca_ptz_get_saved_pos_cb = afterGettingPosition;
    cb.fca_ptz_set_saved_pos_cb = afterSettingPosition;
	fca_ptz_init(&cb);

	/* Set high speed for initialization when camera boot up */
	FCA_API_ASSERT(fca_ptz_set_speed_h(TURN_SPEED_HIGH) == 0);
	FCA_API_ASSERT(fca_ptz_set_speed_v(TURN_SPEED_HIGH) == 0);
	FCA_API_ASSERT(fca_ptz_selfcheck(FCA_PTZ_SELFCHECK_DEF_POS) == 0);
	FCA_API_ASSERT(fca_ptz_set_patrol_onoff(0) == 0);
	
	/* Set medium speed for user's experience */
	FCA_API_ASSERT(fca_ptz_set_speed_h(TURN_SPEED_MID) == 0);
	FCA_API_ASSERT(fca_ptz_set_speed_v(TURN_SPEED_MID) == 0);
	#endif
}

void MOTORS_H_AND_V::run(FCA_PTZ_DIRECTION direction) {
	#if SUPPORT_PTZ
	FCA_API_ASSERT(fca_ptz_go_direction(direction) == 0);
	#endif
}

void MOTORS_H_AND_V::getPosition(int *horizone, int *vertical) {
	#if SUPPORT_PTZ
	FCA_API_ASSERT(fca_ptz_get_position(horizone, vertical) == 0);
	#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LED_FLOODLIGHT::getValues(int *value) {
	FCA_API_ASSERT(fca_get_led_lighting(FCA_LIGHTING_LED_WHITE, value) == 0);
}

void LED_FLOODLIGHT::setValues(int value) {
	FCA_API_ASSERT(fca_set_led_lighting(FCA_LIGHTING_LED_WHITE, value) == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Indicator::init() {
	eventCur = {TASK_PRI_LEVEL_8, EVENT::UNKNOWN, STATUS::UNKNOWN, STATUS::UNKNOWN};
	setNetStatus(STATUS::UNKNOWN);
	getUserControlFromFile(&userledSt);
}

void Indicator::setState(COLOR color, STATUS state, uint32_t ms) {
	led_status_t *ledRealtime;
	fca_led_status_t status;
	if (state == STATUS::ON) {
		status.mode = FCA_LED_MODE_ON;
	}
	else if (state == STATUS::OFF) {
		status.mode = FCA_LED_MODE_OFF;
	}
	else {
		status.mode		= FCA_LED_MODE_BLINKING;
		status.interval = ms / 2;
	}

	if (color == COLOR::RED) {
		status.color = FCA_LED_COLOR_GREEN;
		ledRealtime	 = &ledIndi.red;
	}
	else {
		status.color = FCA_LED_COLOR_WHITE;
		ledRealtime	 = &ledIndi.white;
	}

	ledRealtime->ms	= ms;
	ledRealtime->color = color;
	ledRealtime->state = state;
	FCA_API_ASSERT(fca_gpio_led_status_set(status) == 0);
}

Indicator::STATUS Indicator::getNetStatus() const {
	return netStatus;
}

void Indicator::setNetStatus(STATUS newNetStatus) {
	netStatus = newNetStatus;
}

int Indicator::getUserControlFromFile(int *userCtrl) {
	json cfgJs;
	string filePath = FCA_VENDORS_FILE_LOCATE(FCA_IO_DRIVER_CONTROL_FILE);

	if (!read_json_file(cfgJs, filePath)) {
		APP_DRIVER("read io user control file failed\n");
		return APP_CONFIG_ERROR_FILE_OPEN;
	}

	if (!convertJsonToStruct(cfgJs, userCtrl)) {
		APP_DRIVER("get user control failed\n");
		return APP_CONFIG_ERROR_DATA_INVALID;
	}
	APP_DRIVER("get io user control success\n");
	return APP_CONFIG_SUCCESS;
}

int Indicator::setUserControlToFile(int *userCtrl) {
	json cfgJs;
	string filePath = FCA_VENDORS_FILE_LOCATE(FCA_IO_DRIVER_CONTROL_FILE);

	if (!read_json_file(cfgJs, filePath)) {
		APP_DRIVER("read io user control file failed\n");
	}

	cfgJs["LedIndicator"] = {
		{"Status", *userCtrl}
	 };

	if (!write_json_file(cfgJs, filePath)) {
		APP_DRIVER("Can not write: %s\n", filePath.data());
		return APP_CONFIG_ERROR_FILE_OPEN;
	}
	APP_DRIVER("update file io user control success\n");
	return APP_CONFIG_SUCCESS;
}

void Indicator::controlLedEvent(EVENT event, STATUS status, STATUS control) {
	pthread_mutex_lock(&_mutex);
	int pri = getPriority(event);
	if (pri <= eventCur.priority || event == EVENT::UNKNOWN) {
		parserEvent(pri, event, status, control);
	}
	pthread_mutex_unlock(&_mutex);
}

void Indicator::parserEvent(int &prioriy, EVENT &event, STATUS status, STATUS control) {
	APP_DRIVER("event type: %s\n", event == EVENT::POWER_ON				 ? "POWER ON"
								   : event == EVENT::MOTION				 ? "MOTION"
								   : event == EVENT::REBOOT				 ? "REBOOT"
								   : event == EVENT::NET_SETUP			 ? "NET_SETUP"
								   : event == EVENT::INTERNET_CONNECTION ? "INTERNET_CONNECTION"
								   : event == EVENT::UPDATE_FIRMWARE	 ? "UP FIRMWARE"
																		 : "UNKNOWN");
	switch (event) {
	case EVENT::UPDATE_FIRMWARE: {
		APP_DRIVER("EVENT::UPDATE_FIRMWARE\n");
		setState(Indicator::COLOR::WHITE, Indicator::STATUS::BLINK, 500);
		setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);
	} break;

	case EVENT::POWER_ON:
	case EVENT::REBOOT: {
		APP_DRIVER("EVENT::REBOOT or POWER_ON\n");
		setState(Indicator::COLOR::WHITE, Indicator::STATUS::OFF);
		setState(Indicator::COLOR::RED, Indicator::STATUS::ON);

		if (event == EVENT::POWER_ON) {
			setNetStatus(STATUS::UNKNOWN);
		}
	} break;

	case EVENT::NET_SETUP: {
		APP_DRIVER("EVENT::NET_SETUP\n");
		#if SUPPORT_ETH
		if (fca_check_cable(FCA_NET_WIRED_IF_NAME) == FCA_NET_ETH_STATE_PLUG && (status == STATUS::AP_MODE || status == STATUS::BLE_MODE || status == STATUS::SWITCH_MODE)) {
			APP_DRIVER("ignore ap mode\n");
			return;
		}
		#endif
		removeLedTimer();
		if (status == STATUS::AP_MODE) {
			APP_DRIVER("NET_SETUP AP_MODE\n");
			setState(Indicator::COLOR::WHITE, Indicator::STATUS::BLINK, 200);
			setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);
		}
		else if (status == STATUS::BLE_MODE) {
			if (control == STATUS::ON) {
				APP_DRIVER("LED BLE_MODE ON\n");
				setState(Indicator::COLOR::WHITE, Indicator::STATUS::BLINK, 200);
				setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);
				_idLedTimer.store(systemTimer.add(std::chrono::milliseconds(800), [](CppTime::timer_id id) {
					if (ledStatus._idLedTimer.load() == (int)id) {
						ledStatus._idLedTimer.store(-1);
						ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, STATUS::BLE_MODE, STATUS::OFF);
					}
				}));
			}
			else if (control == STATUS::OFF) {
				APP_DRIVER("LED BLE_MODE OFF\n");
				setState(Indicator::COLOR::WHITE, STATUS::OFF);
				setState(Indicator::COLOR::RED, STATUS::OFF);
				_idLedTimer.store(systemTimer.add(std::chrono::milliseconds(400), [](CppTime::timer_id id) {
					if (ledStatus._idLedTimer.load() == (int)id) {
						ledStatus._idLedTimer.store(-1);
						ledStatus.controlLedEvent(Indicator::EVENT::NET_SETUP, STATUS::BLE_MODE, STATUS::ON);
					}
				}));
			}
		}
		else if (status == STATUS::OK) {
			APP_DRIVER("NET_SETUP OK\n");
			setState(Indicator::COLOR::WHITE, Indicator::STATUS::BLINK, 1000);
			setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);
			setNetStatus(STATUS::UNKNOWN);
			prioriy = TASK_PRI_LEVEL_4;	   // prevent motion and timer control
		}
		else if (status == STATUS::SWITCH_MODE) {
			APP_DRIVER("SWITCH_MODE AP_MODE - BLE_MODE\n");
			setState(Indicator::COLOR::WHITE, Indicator::STATUS::OFF);
			setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);
		}
	} break;

	case EVENT::INTERNET_CONNECTION: {
		APP_DRIVER("EVENT::INTERNET_CONNECTION\n");
		if (status == getNetStatus() && control != STATUS::ON && control != STATUS::OFF) {
			APP_DRIVER("INTERNET_CONNECTION nothing\n");
			return;
		}

		removeLedTimer();	 // type TIMER UNKNOWN
		if (status == STATUS::SERVER_CONNECTED) {
			APP_DRIVER("SERVER_CONNECTED\n");
			setState(Indicator::COLOR::WHITE, Indicator::STATUS::ON);
			setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);

			_idLedTimer.store(systemTimer.add(std::chrono::milliseconds(3000), [](CppTime::timer_id id) {
				if (ledStatus._idLedTimer.load() == (int)id) {
					ledStatus._idLedTimer.store(-1);
					ledStatus.controlLedEvent(Indicator::EVENT::USER_CTRL);
				}
			}));
			prioriy		= TASK_PRI_LEVEL_8;
		}
		else if (status == STATUS::CONNECTED) {
			if (control == STATUS::ON || control == STATUS::START) {
				APP_DRIVER("LED CONNECTED ON\n");
				setState(Indicator::COLOR::WHITE, Indicator::STATUS::BLINK, 200);
				setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);

				_idLedTimer.store(systemTimer.add(std::chrono::milliseconds(400), [](CppTime::timer_id id) {
					if (ledStatus._idLedTimer.load() == (int)id) {
						ledStatus._idLedTimer.store(-1);
						ledStatus.controlLedEvent(Indicator::EVENT::INTERNET_CONNECTION, STATUS::CONNECTED, STATUS::OFF);
					}
				}));
			}
			else if (control == STATUS::OFF) {
				APP_DRIVER("LED CONNECTED OFF\n");
				setState(Indicator::COLOR::WHITE, STATUS::OFF);
				setState(Indicator::COLOR::RED, STATUS::OFF);
				_idLedTimer.store(systemTimer.add(std::chrono::milliseconds(600), [](CppTime::timer_id id) {
					if (ledStatus._idLedTimer.load() == (int)id) {
						ledStatus._idLedTimer.store(-1);
						ledStatus.controlLedEvent(Indicator::EVENT::INTERNET_CONNECTION, STATUS::CONNECTED, STATUS::ON);
					}
				}));
			}

			/* try update NTP time */
			if (control == STATUS::START) {
				// task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_TRY_UPDATE_NTP_REQ);
			}
		}
		else if (status == STATUS::DISCONNECT) {
			APP_DRIVER("INTERNET_CONNECTION disconnect\n");
			offlineEventManager.addEvent({NETWORK_ERROR_DISCONNECT, (uint32_t)time(nullptr), TASK_PRI_LEVEL_1});
			setState(Indicator::COLOR::WHITE, Indicator::STATUS::OFF);
			setState(Indicator::COLOR::RED, Indicator::STATUS::BLINK, 200);
			timer_set(GW_TASK_NETWORK_ID, GW_NET_RESTART_UDHCPC_NETWORK_INTERFACES_REQ, GW_NET_RESTART_NETWORK_SERVICES_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);
			timer_set(GW_TASK_NETWORK_ID, GW_NET_RELOAD_WIFI_DRIVER_REQ, GW_NET_RELOAD_WIFI_DRIVER_TIMEROUT_INTERVAL,
					  TIMER_ONE_SHOT);	  // NOTE: GIEC always reloads wifi driver every time it connects
			timer_set(GW_TASK_NETWORK_ID, GW_NET_REBOOT_ERROR_NETWORK_REQ, GW_NET_REBOOT_ERROR_NETWORK_TIMEROUT_INTERVAL, TIMER_ONE_SHOT);
		}
		setNetStatus(status);
	} break;

	case EVENT::MOTION: {
		removeLedTimer();
		if (status == STATUS::START) {
			APP_DRIVER("EVENT::MOTION start\n");
			eventWait = eventCur;
			ledIndiBk = ledIndi;

			setState(Indicator::COLOR::WHITE, Indicator::STATUS::BLINK, 166);	 // 1s/6
			setState(Indicator::COLOR::RED, Indicator::STATUS::OFF);

			_idLedTimer.store(systemTimer.add(std::chrono::milliseconds(1000), [](CppTime::timer_id id) {
				if (ledStatus._idLedTimer.load() == (int)id) {
					ledStatus._idLedTimer.store(-1);
					ledStatus.controlLedEvent(Indicator::EVENT::MOTION, STATUS::FINISH);
				}
			}));
		}
		else if (status == STATUS::FINISH) {
			APP_DRIVER("EVENT::MOTION finish\n");
			setState(ledIndiBk.red.color, ledIndiBk.red.state, ledIndiBk.red.ms);
			setState(ledIndiBk.white.color, ledIndiBk.white.state, ledIndiBk.white.ms);
			if (eventWait.event != EVENT::MOTION) {	   // prevent recurse
				if (eventWait.event == EVENT::INTERNET_CONNECTION || (eventWait.event == EVENT::USER_CTRL && userledSt != (int)STATUS::OFF)) {
					setNetStatus(STATUS::UNKNOWN);
				}
				parserEvent(eventWait.priority, eventWait.event, eventWait.status, eventWait.control);
				prioriy = eventWait.priority;
				event	= eventWait.event;
				status	= eventWait.status;
				control = eventWait.control;
				APP_DRIVER("backup pri: %d, event: %d, status: %d, control: %d\n", eventWait.priority, static_cast<int>(event), static_cast<int>(status),
						   static_cast<int>(control));
			}
			else {
				setNetStatus(STATUS::UNKNOWN);
			}
		}
	} break;

	case EVENT::USER_CTRL: {
		APP_DRIVER("EVENT::USER_CTRL\n");
		if (userledSt == (int)STATUS::OFF) {
			setState(Indicator::COLOR::WHITE, STATUS::OFF);
			setState(Indicator::COLOR::RED, STATUS::OFF);
		}
	} break;

	default: {
		APP_DRIVER("EVENT::UNKNOWN\n");
		setNetStatus(STATUS::UNKNOWN);
	} break;
	}
	eventCur = {prioriy, event, status, control};
}

bool Indicator::convertJsonToStruct(const json &cfgJs, int *userCtrl) {
	try {
		*userCtrl	= (int)STATUS::ON;
		json dataJs = cfgJs.at("LedIndicator");
		*userCtrl	= dataJs["Status"].get<int>();
		return true;
	}
	catch (const exception &e) {
		APP_DRIVER("json error: %s\n", e.what());
	}
	return false;
}

void Indicator::removeLedTimer() {
	if (_idLedTimer.load() >= 0) {
		systemTimer.remove(_idLedTimer.load());
		_idLedTimer.store(-1);
	}
}

int Indicator::getPriority(EVENT event) {
	for (int i = 0; i < NUM_EVENTS; ++i) {
		if (priorityTable[i].event == event) {
			return priorityTable[i].priority;
		}
	}
	// If event not found, return a default priority or handle as needed
	return 113;	   // Or any other default value
}

int Indicator::userSetState(const int &status) {
	pthread_mutex_lock(&_mutex);
	if ((eventCur.event == EVENT::INTERNET_CONNECTION && eventCur.status == STATUS::SERVER_CONNECTED) || eventCur.event == EVENT::USER_CTRL) {
		userledSt = status;
		if (userledSt == (int)STATUS::OFF) {
			setState(Indicator::COLOR::WHITE, STATUS::OFF);
			setState(Indicator::COLOR::RED, STATUS::OFF);
		}
		else {
			setNetStatus(STATUS::UNKNOWN);
		}
		setUserControlToFile(&userledSt);
		pthread_mutex_unlock(&_mutex);
		return APP_CONFIG_SUCCESS;
	}
	pthread_mutex_unlock(&_mutex);
	return APP_CONFIG_ERROR_ANOTHER;
}

int IR_Lighting::setIR(STATUS status) {
	int setVal, getVal;

	setVal = status == STATUS::ON ? 1 : 0;
	if (fca_set_led_lighting(FCA_LIGHTING_LED_IR, setVal) != 0) {
		APP_DRIVER("set ir led error\n");
		return -1;
	}

#if 1 /* VERIFY */
	fca_get_led_lighting(FCA_LIGHTING_LED_IR, &getVal);
	if (getVal != setVal) {
		APP_DRIVER("diff set and get param ir led\n");
		return -1;
	}
#endif
	return 0;
}

int IR_Lighting::setLighting(STATUS status) {
#if SUPPORT_FLOOD_LIGHT
	int getState = 0xFF;
	int setState = (status == STATUS::ON) ? 1 : 0;
	SYSLOG_LOG(LOG_INFO, "logf=%s [LIGHTING] state: %d", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, setState);

	fca_get_led_lighting(FCA_LIGHTING_LED_WHITE, &getState);
	if (getState != setState) {
		fca_set_led_lighting(FCA_LIGHTING_LED_WHITE, setState);
	}

#else
	APP_PRINT("[isp] Device not support led lighting\n");
#endif

	return 0;
}

IR_Lighting::STATUS IR_Lighting::getLightingState() {
	int val = 0;

#if SUPPORT_FLOOD_LIGHT
	fca_get_led_lighting(FCA_LIGHTING_LED_WHITE, &val);
#else
	APP_PRINT("[isp] Device not support led lighting\n");
#endif

	return (val == 1 ? (STATUS::ON) : (STATUS::OFF));
}
