#include "app_dbg.h"
#include "network_manager.h"

#define REPORT_INTERVAL_SEC (300)	 // 5 minutes

static bool comparePriority(const offlineEvent_t &a, const offlineEvent_t &b);

OfflineEventManager::OfflineEventManager() {
	pthread_mutex_init(&_mutex, nullptr);
	_waitTimeUpdate = false;
	memset(&_lastEvent, 0, sizeof(_lastEvent));
}

OfflineEventManager::~OfflineEventManager() {
	pthread_mutex_destroy(&_mutex);
}

int OfflineEventManager::getErrorReason(json &arrErrJs) {
	pthread_mutex_lock(&_mutex);
	if (_offlineEvents.empty()) {
		APP_DBG("No offline events\n");
		pthread_mutex_unlock(&_mutex);
		return -1;
	}

	// Sort events by priority
	sort(_offlineEvents.begin(), _offlineEvents.end(), comparePriority);

	/* check interval 5min */
	// APP_DBG("alarm duration: %u seconds\n", (uint32_t)time(nullptr) - _lastEvent.ts);
	if (_lastEvent.type == _offlineEvents.at(0).type && ((uint32_t)time(nullptr) - _lastEvent.ts < REPORT_INTERVAL_SEC)) {
		APP_DBG("Same event type within 5 minutes, not reporting again\n");
		_offlineEvents.clear();
		pthread_mutex_unlock(&_mutex);
		return -1;
	} 

	for (const auto &event : _offlineEvents) {
		APP_DBG("Event type: %d, priority: %d, timestamp: %u\n", event.type, event.priority, event.ts);
		if (event.type == NETWORK_ERROR_REBOOT) {
			json errJs = {
				{"Details", {{"Description", "Offline detected – device rebooted."}, {"ErrorAt", event.ts}}},
				  {"Type",	   genEventType(event.type)													   }
			};
			arrErrJs.push_back(errJs);
		}
		else if (event.type == NETWORK_ERROR_DISCONNECT) {
			json errJs = {
				{"Type",	 genEventType(event.type)																			   },
				{"Details", {{"ErrorCode", "NETWORK-ERROR-01"}, {"Description", "Network connection lost."}, {"ErrorAt", event.ts}}}
			};
			arrErrJs.push_back(errJs);
		}
		else if (event.type == CHANGE_SERVER) {
			json errJs = {
				{"Type",	 genEventType(event.type)																	  },
				{"Details", {{"ErrorCode", "CHANGE-SERVER-01"}, {"Description", "Server changed."}, {"ErrorAt", event.ts}}}
			   };
			arrErrJs.push_back(errJs);
		}
	}
	_lastEvent = _offlineEvents.at(0);
	_offlineEvents.clear();
	pthread_mutex_unlock(&_mutex);
	return 0;
}

void OfflineEventManager::updateTimeForEvent(const int &type, const uint32_t &ts) {
	pthread_mutex_lock(&_mutex);
	auto it = std::find_if(_offlineEvents.begin(), _offlineEvents.end(), [type](const offlineEvent_t &event) { return event.type == type; });
	if (it != _offlineEvents.end()) {
		it->ts = ts;
	}
	else {
		APP_DBG("Event type %d not found\n", type);
	}
	pthread_mutex_unlock(&_mutex);
}

void OfflineEventManager::addEvent(const offlineEvent_t &event) {
	if (_waitTimeUpdate) {
		APP_DBG("Wait for time update, not adding event\n");
		return;
	}

	pthread_mutex_lock(&_mutex);
	// Check if the event already exists
	auto it = std::find_if(_offlineEvents.begin(), _offlineEvents.end(), [&event](const offlineEvent_t &e) { return e.type == event.type; });
	if (it != _offlineEvents.end()) {
		// Update the existing event's timestamp and priority
		it->ts		 = event.ts;
		it->priority = event.priority;
	}
	else {
		// Add a new event
		_offlineEvents.push_back(event);
	}
	pthread_mutex_unlock(&_mutex);
}

void OfflineEventManager::clearEvents() {
	pthread_mutex_lock(&_mutex);
	_offlineEvents.clear();
	pthread_mutex_unlock(&_mutex);
}

string OfflineEventManager::genEventType(const int &type) {
	switch (type) {
	case NETWORK_ERROR_REBOOT:
		return "RebootError";
	case NETWORK_ERROR_DISCONNECT:
		return "NetworkError";
	case CHANGE_SERVER:
		return "ChangeServerError";
	default:
		return "UnknownError";
	}
}

void OfflineEventManager::removeEvent(const int &type) {
	pthread_mutex_lock(&_mutex);
	auto it = std::remove_if(_offlineEvents.begin(), _offlineEvents.end(), [type](const offlineEvent_t &event) { return event.type == type; });
	_offlineEvents.erase(it, _offlineEvents.end());
	pthread_mutex_unlock(&_mutex);
}

bool comparePriority(const offlineEvent_t &a, const offlineEvent_t &b) {
	return a.priority > b.priority;
}

void OfflineEventManager::setWaitTimeUpdate(bool wait) {
	_waitTimeUpdate = wait;
}
