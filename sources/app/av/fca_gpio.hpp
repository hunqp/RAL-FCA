#ifndef __FCA_GPIO_HPP__
#define __FCA_GPIO_HPP__

#include <functional>
#include "ak.h"
#include "fca_parameter.hpp"

#define NUM_EVENTS (8)
#define SUPPORT_PTZ 1

using json = nlohmann::json;

struct MOTORS_H_AND_V {
	void initialise(
		void (*doAfterGettingPosition)(int,int), 
		void (*doAfterSettingPosition)(int,int)
	);
	void run(FCA_PTZ_DIRECTION direction);
	void getPosition(int *horizone, int *vertical);

public:
	void (*implDoAfterGettingPosition)(int,int) = NULL;
	void (*implDoAfterSettingPosition)(int,int) = NULL;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Indicator {
#define LED_INDICATOR_IDLE_PRIORITY (TASK_PRI_LEVEL_6)

	enum struct COLOR {
		RED,
		WHITE,
	};

	enum struct STATUS {
		OFF,				 // 0
		ON,					 // 1
		BLINK,				 // 2
		START,				 // 3
		FINISH,				 // 4
		CONNECTED,			 // 5
		SERVER_CONNECTED,	 // 6
		DISCONNECT,			 // 7
		AP_MODE,			 // 8
		BLE_MODE,			 // 9
		OK,					 // 10
		SWITCH_MODE,		 // 11
		UNKNOWN				 // 12
	};

	enum struct EVENT {
		UPDATE_FIRMWARE,		// 0
		REBOOT,					// 1
		NET_SETUP,				// 2 AP mode
		INTERNET_CONNECTION,	// 3 GW_NET_WIFI_DO_CONNECT fca_netWifiSTAStart()
		MOTION,					// 4
		POWER_ON,				// 5
		USER_CTRL,				// 6
		UNKNOWN
	};

	struct led_status_t {
		COLOR color;
		STATUS state;
		uint32_t ms;
	};

	struct led_indicator_status_t {
		led_status_t red;
		led_status_t white;
	};

	struct led_event_ctrl_t {
		int priority;
		EVENT event;
		STATUS status;
		STATUS control;
	};

	struct tbl_pri_t {
		EVENT event;
		int priority;
	};

	tbl_pri_t priorityTable[NUM_EVENTS] = {
		{EVENT::UPDATE_FIRMWARE,	 TASK_PRI_LEVEL_1},
		{EVENT::REBOOT,				TASK_PRI_LEVEL_2},
		  {EVENT::NET_SETUP,			 TASK_PRI_LEVEL_3},
		{EVENT::INTERNET_CONNECTION, TASK_PRI_LEVEL_4},
		{EVENT::MOTION,				TASK_PRI_LEVEL_5},
		  {EVENT::POWER_ON,			TASK_PRI_LEVEL_6},
		{EVENT::USER_CTRL,		   TASK_PRI_LEVEL_8},
		  {EVENT::UNKNOWN,			   TASK_PRI_LEVEL_8},
	};

	// Constructor để khởi tạo bảng ánh xạ sự kiện và độ ưu tiên
	Indicator() {
		_idLedTimer.store(-1);
		_mutex			 = PTHREAD_MUTEX_INITIALIZER;
		userledSt		 = (int)STATUS::ON;
	}

	~Indicator() {
		pthread_mutex_destroy(&_mutex);
	}

	void init();
	int getUserControlFromFile(int *userCtrl);
	int setUserControlToFile(int *userCtrl);
	void controlLedEvent(EVENT event, STATUS status = STATUS::UNKNOWN, STATUS control = STATUS::UNKNOWN);
	void setState(COLOR color, STATUS state, uint32_t ms = 0);
	int userSetState(const int &status);

	STATUS getNetStatus() const;
	void setNetStatus(STATUS newNetStatus);

public:
	std::atomic<int> _idLedTimer;

protected:
	void parserEvent(int &prioriy, EVENT &event, STATUS status = STATUS::UNKNOWN, STATUS control = STATUS::UNKNOWN);
	bool convertJsonToStruct(const json &dataJs, int *userCtrl);
	void removeLedTimer();
	int getPriority(EVENT event);

private:
	STATUS netStatus;
	pthread_mutex_t _mutex;
	int userledSt;
	struct led_event_ctrl_t eventWait, eventCur;
	struct led_indicator_status_t ledIndi, ledIndiBk;
};

struct LED_FLOODLIGHT {
	void getValues(int *value);
	void setValues(int value);
};

struct IR_Lighting {
	enum struct STATUS {
		ON,
		OFF,
	};

	int setIR(STATUS status);
	int setLighting(STATUS status);
	STATUS getLightingState();
};

struct GPIOHelpers {
	// Indicator ledStatus;
	// MOTORS_H_AND_V motors;
	IR_Lighting irLighting;
};

extern Indicator ledStatus;
extern MOTORS_H_AND_V motors;

#endif /*__FCA_PTZ_H__*/
