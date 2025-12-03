#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include <string>
#include <stdint.h>
#include <pthread.h>
#include <vector>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

enum {
    NETWORK_ERROR_NONE = 0,
    NETWORK_ERROR_REBOOT,
    NETWORK_ERROR_DISCONNECT,
	CHANGE_SERVER,
};

typedef struct t_offlineEvent {
	int type;
	uint32_t ts;
	int priority;
} offlineEvent_t;

class OfflineEventManager {
public:
	OfflineEventManager();
	~OfflineEventManager();

	int getErrorReason(json &arrErrJs);
	void updateTimeForEvent(const int &type, const uint32_t &ts);
	void addEvent(const offlineEvent_t &event);
	void removeEvent(const int &type);
	void clearEvents();
    string genEventType(const int &type);
	void setWaitTimeUpdate(bool wait);

private:
	std::vector<offlineEvent_t> _offlineEvents;
	pthread_mutex_t _mutex;
	bool _waitTimeUpdate;
	offlineEvent_t _lastEvent;
};

#endif	  // __NETWORK_MANAGER_H__
