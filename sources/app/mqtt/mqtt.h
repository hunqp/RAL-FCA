#ifndef __MQTT__H__
#define __MQTT__H__

#include <stdint.h>
#include <string.h>

#include <mosquittopp.h>

#include "ak.h"

#include "app.h"
#include "app_dbg.h"

class mqtt : public mosqpp::mosquittopp {
public:
	mqtt(mqttTopicCfg_t *mqtt_parameter, FCA_MQTT_CONN_S *mqtt_config);
	~mqtt();

	// Publish functions
	bool publishResponse(json &js, int qos = 0, bool retain = false);
	bool publishSignalingResponse(json &js, int qos = 0, bool retain = false);
	bool publishStatus(json &js, int qos = 0, bool retain = true);
	bool publishAlarm(json &js, int qos = 0, bool retain = false);
	bool subcribePerform(string &topic, int qos = 0);
	bool unsubcribePerform(string &topic);
	int tryConnect(const char *capath);
	void clearRetainMsg(const char *topic, bool is_retain);
	int addSubTopicMap(const string &topic, const int &mid, const int &qos);
	int removeSubTopicMap(const string &topic);
	int removeSubTopicMap(const int &mid);
	void showSubTopicMap();
	void retrySubTopics();
	void tryDisconnect();

	// getter and setter
	void setConnected(bool state);
	bool isConnected(void);

	// Callback functions
	void on_connect(int rc);
	void on_disconnect(int rc);
	void on_publish(int mid);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);
	void on_message(const struct mosquitto_message *message);
	void on_log(int level, const char *log_str);

	int checkRunLoopCounter;
	static volatile bool lastMqttStatus;

protected:
	bool checkMessageDuplicate(const struct mosquitto_message *message);

private:
	// store topic: default it will sub 1 topic when MQTT is connected
	FCA_MQTT_CONN_S m_cfg;
	mqttTopicCfg_t m_topics;
	int m_mid;
	std::atomic<bool> m_connected;
	bool m_enOnDisconnectCb;
	int lenLastMsg;
	uint8_t *lastMsgPtr;
	std::map<std::string, std::map<std::string, int>> m_subTopicMap;
	pthread_mutex_t m_mutex;
};

#endif	  //__MQTT__H__
