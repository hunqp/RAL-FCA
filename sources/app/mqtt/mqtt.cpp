#include "app_config.h"
#include "app_data.h"
#include "task_list.h"
#include "mqtt.h"

volatile bool mqtt::lastMqttStatus = false;

/*clean section = true: mean every reconnect or start need to subcribe all topics*/
mqtt::mqtt(mqttTopicCfg_t *mqtt_parameter, FCA_MQTT_CONN_S *mqtt_config) : mosquittopp(mqtt_config->clientID, true) {
	mosqpp::lib_init();
	m_mutex = PTHREAD_MUTEX_INITIALIZER;
	/* init private data */
	setConnected(false);
	m_mid				= 0;	// messages ID
	lenLastMsg			= 0;
	lastMsgPtr			= NULL;
	m_enOnDisconnectCb	= true;
	checkRunLoopCounter = 0;

	/* save some data */
	memset(&m_cfg, 0, sizeof(m_cfg));
	memset(&m_topics, 0, sizeof(m_topics));
	memcpy(&m_cfg, mqtt_config, sizeof(m_cfg));
	memcpy(&m_topics, mqtt_parameter, sizeof(m_topics));

	/* setup conection */
	if (m_cfg.username && m_cfg.password) {
		username_pw_set(m_cfg.username, m_cfg.password);
	}
}

mqtt::~mqtt() {
	tryDisconnect();
	loop_stop(true);
	if (lastMsgPtr != NULL) {
		free(lastMsgPtr);
	}
	mosqpp::lib_cleanup();
	pthread_mutex_destroy(&m_mutex);

	lastMqttStatus = false;
	APP_DBG("~mqtt() called\n");
}

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/
bool mqtt::publishResponse(json &js, int qos, bool retain) {
	if (isConnected()) {
		APP_DBG_MQTT("[mqtt][publish] %s\n", js.dump().data());
		return (publish(&m_mid, m_topics.topicResponse, js.dump().length(), js.dump().data(), qos, (retain == true ? true : false)) == MOSQ_ERR_SUCCESS);
	}
	return false;
}

bool mqtt::publishSignalingResponse(json &js, int qos, bool retain) {
	if (isConnected()) {
		APP_DBG_MQTT("[mqtt][publish] %s\n", js.dump().data());
		return (publish(&m_mid, m_topics.topicSignalingResponse, js.dump().length(), js.dump().data(), qos, (retain == true ? true : false)) == MOSQ_ERR_SUCCESS);
	}
	return false;
}

bool mqtt::publishStatus(json &js, int qos, bool retain) {
	if (isConnected()) {
		APP_DBG_MQTT("[mqtt][publish] %s\n", js.dump().data());
		return (publish(&m_mid, m_topics.topicStatus, js.dump().length(), js.dump().data(), qos, (retain == true ? true : false)) == MOSQ_ERR_SUCCESS);
	}
	return false;
}

bool mqtt::publishAlarm(json &js, int qos, bool retain) {
	if (isConnected()) {
		APP_DBG_MQTT("[mqtt][publish][qos: %d] %s\n", qos, js.dump().data());
		return (publish(&m_mid, m_topics.topicAlarm, js.dump().length(), js.dump().data(), qos, (retain == true ? true : false)) == MOSQ_ERR_SUCCESS);
	}
	return false;
}

bool mqtt::subcribePerform(string &topic, int qos) {
	if (isConnected()) {
		int ret = subscribe(&m_mid, topic.data(), qos);
		addSubTopicMap(topic, m_mid, qos);
		timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_CHECK_SUB_TOPIC_REQ, GW_CLOUD_MQTT_CHECK_SUB_TOPIC_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
		return (ret == MOSQ_ERR_SUCCESS);
	}
	return false;
}

bool mqtt::unsubcribePerform(string &topic) {
	if (isConnected()) {
		removeSubTopicMap(topic);
		return (unsubscribe(&m_mid, topic.data()) == MOSQ_ERR_SUCCESS);
	}
	return false;
}

int mqtt::tryConnect(const char *capath) {
	int rc = MOSQ_ERR_SUCCESS;

	rc = tls_set(capath);
	if (rc != MOSQ_ERR_SUCCESS) {
		APP_DBG_MQTT("tls_set error %d\n", rc);
		return rc;
	}

	tls_insecure_set(false);
	rc = tls_opts_set(FCA_SSL_VERIFY_PEER, m_cfg.TLSVersion);
	if (rc != MOSQ_ERR_SUCCESS) {
		APP_DBG_MQTT("tls_opts_set error %d\n", rc);
		return rc;
	}

	/* last will msg prepair */
	time_t rawtime = time(nullptr);
	json stt_msg;
	fca_configGetDeviceInfoStr(stt_msg);
	fca_configGetUpgradeStatus(stt_msg);
	stt_msg["Data"]["Status"] = "Disconnected";
	stt_msg["Timestamps"]	  = rawtime;
	APP_DBG_MQTT("will set info: %s\n", stt_msg.dump(4).data());
	rc = will_set(m_topics.topicStatus, stt_msg.dump().length(), stt_msg.dump().data(), m_cfg.QOS, m_cfg.retain);
	if (rc != MOSQ_ERR_SUCCESS) {
		APP_DBG_MQTT("will_set error %d\n", rc);
		return rc;
	}

	/* connect */
	APP_DBG_MQTT("try connect to host: %s, port: %d, keep: %d\n", m_cfg.host, m_cfg.port, m_cfg.keepAlive);
	rc = connect_async(m_cfg.host, m_cfg.port, m_cfg.keepAlive);
	if (rc != MOSQ_ERR_SUCCESS) {
		APP_DBG_MQTT("connect_async failed\n");
		return rc;
	}

	/* start loop */
	rc = loop_start();
	if (rc != MOSQ_ERR_SUCCESS) {
		APP_DBG_MQTT("loop start failed\n");
		return rc;
	}

	return rc;
}

void mqtt::clearRetainMsg(const char *topic, bool is_retain) {
	if (is_retain) {
		publish(&m_mid, topic, 0, NULL, 1, true);
	}
}

int mqtt::addSubTopicMap(const string &topic, const int &mid, const int &qos) {
	int size = 0;
	APP_DBG_MQTT("add sub topic: %s\n", topic.c_str());
	pthread_mutex_lock(&m_mutex);
	m_subTopicMap[topic]["mid"] = mid;
	m_subTopicMap[topic]["qos"] = qos;
	size						= m_subTopicMap.size();
	showSubTopicMap();
	pthread_mutex_unlock(&m_mutex);
	return size;
}

int mqtt::removeSubTopicMap(const string &topic) {
	int size = 0;
	APP_DBG_MQTT("remove sub topic: %s\n", topic.c_str());
	pthread_mutex_lock(&m_mutex);
	m_subTopicMap.erase(topic);
	size = m_subTopicMap.size();
	showSubTopicMap();
	pthread_mutex_unlock(&m_mutex);
	return size;
}

int mqtt::removeSubTopicMap(const int &mid) {
	APP_DBG_MQTT("remove sub topic mid: %d\n", mid);
	int size = 0;
	pthread_mutex_lock(&m_mutex);
	string topicRm = "";
	for (const auto &pair : m_subTopicMap) {
		if (pair.second.at("mid") == mid) {
			topicRm = pair.first;
			break;
		}
	}
	m_subTopicMap.erase(topicRm);
	size = m_subTopicMap.size();
	APP_DBG_MQTT("[SUB] erase sub topic: %s\n", topicRm.c_str());
	showSubTopicMap();
	pthread_mutex_unlock(&m_mutex);
	return size;
}

void mqtt::showSubTopicMap() {
	APP_DBG_MQTT("list sub topic ready\n");
	for (const auto &outerPair : m_subTopicMap) {
		APP_DBG_MQTT("[SUB] topic: %s, mid: %d, qos: %d\n", outerPair.first.c_str(), outerPair.second.at("mid"), outerPair.second.at("qos"));
	}
}

void mqtt::retrySubTopics() {
	if (isConnected()) {
		pthread_mutex_lock(&m_mutex);
		for (auto &pair : m_subTopicMap) {
			unsubscribe(&m_mid, pair.first.data());
			usleep(100 * 1000);
			subscribe(&m_mid, pair.first.data(), pair.second["qos"]);
			pair.second["mid"] = m_mid;
			APP_DBG_MQTT("[SUB] retry sub topic: %s, mid: %d, qos: %d\n", pair.first.c_str(), pair.second["mid"], pair.second["qos"]);
			timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_CHECK_SUB_TOPIC_REQ, GW_CLOUD_MQTT_CHECK_SUB_TOPIC_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
		}
		pthread_mutex_unlock(&m_mutex);
	}
}

void mqtt::tryDisconnect() {
	m_enOnDisconnectCb = false;
	disconnect();
	usleep(500 * 1000);
}

void mqtt::setConnected(bool state) {
	m_connected.store(state);
	lastMqttStatus = m_connected.load();
}

bool mqtt::isConnected(void) {
	return m_connected.load();
}

/******************************************************************************
 * CALLBACK FUNCTIONS
 ******************************************************************************/
void mqtt::on_connect(int rc) {
	setConnected((rc == 0) ? true : false);
	std::string subTopic = std::string(m_topics.topicRequest);
	subcribePerform(subTopic, 0);
	subTopic = std::string(m_topics.topicSignalingRequest);
	subcribePerform(subTopic, 1);
	subTopic = std::string(m_topics.topicStatus);
	subcribePerform(subTopic, 1);

	timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_ON_CONNECTED, 1000, TIMER_ONE_SHOT);

	// if (rc == 0) {

	// 	setConnected(true);
	// 	task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_GEN_MQTT_STATUS_REQ);

	// 	subcribePerform(std::string(m_topics.topicRequest), 0);
	// 	subcribePerform(std::string(m_topics.topicSignalingRequest), 1);
	// 	subcribePerform(std::string(m_topics.topicStatus), 1);

	// 	/* send signal to LED task */
	// 	ledStatus.controlLedEvent(Indicator::EVENT::INTERNET_CONNECTION, Indicator::STATUS::SERVER_CONNECTED);
	// 	task_post_pure_msg(GW_TASK_NETWORK_ID, GW_NET_PLAY_VOICE_NETWORK_CONNECTED_REQ);
	// }
	// else {
	// 	// APP_DBG_MQTT("[MQTT_CONTROL] on_connect ERROR : %d\n", rc);
	// 	setConnected(false);
	// }
}

void mqtt::on_disconnect(int rc) {
	(void)rc;
	setConnected(false);
	task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_ON_DISCONNECTED);

	// if (m_enOnDisconnectCb) {
	// 	APP_DBG_MQTT("[mqtt] on_disconnect %d, %s\n", rc, mosqpp::connack_string(rc));
	// 	timer_set(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_TRY_CONNECT_REQ, GW_CLOUD_MQTT_TRY_CONNECT_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
	// }
	// else {
	// 	APP_DBG_MQTT("disable on_disconnect\n");
	// }
}

void mqtt::on_publish(int mid) {
	(void)mid;
	// APP_DBG_MQTT("[mqtt][on_publish] mid: %d\n", mid);
}

void mqtt::on_subscribe(int mid, int qos_count, const int *granted_qos) {
	(void)granted_qos;
	APP_DBG_MQTT("[mqtt][on_subscribe] mid:%d\tqos_count:%d\n", mid, qos_count);
	task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MQTT_SUB_TOPIC_REP, (uint8_t *)&mid, sizeof(mid));
}

void mqtt::on_message(const struct mosquitto_message *message) {
	if (message->payloadlen > 0) {
		// if (message->payloadlen < MAX_MQTT_MSG_LENGTH) {	//TODO open
		APP_DBG_MQTT("[cloud_mqtt][on _message] topic:%s\tpayloadlen:%d\n", message->topic, message->payloadlen);
		if (checkMessageDuplicate(message) == true) {
			/* post message to mqtt task */
			task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_PROCESS_INCOME_DATA, (uint8_t *)message->payload, message->payloadlen);
		}
		else {
			APP_DBG_MQTT("\n Duplicate message! \n");
		}
		// }
		// else {
		// 	task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_MESSAGE_LENGTH_OUT_OF_RANGE_REP, (uint8_t *)&(message->payloadlen), sizeof(message->payloadlen));
		// }
		/* because subcribe only 1 topic, not filter receive topic */
		if (string(message->topic) != m_topics.topicStatus) {
			clearRetainMsg(message->topic, message->retain);
		}
	}
}

void mqtt::on_log(int level, const char *log_str) {
	(void)level;
	if (!isConnected()) {
		APP_DBG_MQTT("[MQTT][onLog] -> %s\n", log_str);
	}
	return;
}

bool mqtt::checkMessageDuplicate(const struct mosquitto_message *message) {
	if (lastMsgPtr == NULL) {
		lastMsgPtr = (uint8_t *)malloc(message->payloadlen);
		if (lastMsgPtr == NULL) {
			APP_DBG_MQTT("\n error malloc! \n");
			return false;
		}

		memcpy(lastMsgPtr, (uint8_t *)message->payload, message->payloadlen);
		lenLastMsg = message->payloadlen;
	}
	else {
		if (message->payloadlen == lenLastMsg) {
			uint8_t result_ms = strncmp((const char *)message->payload, (const char *)lastMsgPtr, message->payloadlen);

			if (result_ms == 0) {	 // repeated
				return false;
			}
			else {
				memcpy(lastMsgPtr, (uint8_t *)message->payload, message->payloadlen);
				lenLastMsg = message->payloadlen;
			}
		}
		else {
			uint8_t *tmp = (uint8_t *)realloc((uint8_t *)lastMsgPtr, message->payloadlen);
			if (tmp == NULL) {
				APP_DBG_MQTT("\n error realloc! \n");
				return false;
			}
			lastMsgPtr = tmp;
			memcpy(lastMsgPtr, message->payload, message->payloadlen);
			lenLastMsg = message->payloadlen;
		}
	}

	return true;
}
