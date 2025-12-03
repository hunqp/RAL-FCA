#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <vector>

#include <algorithm>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <chrono>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <atomic>

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "datachannel_hdl.h"
#include "app_capture.h"

#include "task_list.h"

#include "dispatchqueue.hpp"
#include "helpers.hpp"
#include "rtc/rtc.hpp"
#include "json.hpp"
#include "stream.hpp"

#include "videosource.hpp"
#include "audiosource.hpp"
#include "fca_video.hpp"
#include "fca_audio.hpp"

#include "parser_json.h"
#include "utils.h"
#include "STUNExternalIP.h"
#include "g711.h"

using namespace rtc;
using namespace std;
using namespace chrono_literals;
using namespace chrono;
using ALAWRtpPacketizer = AudioRtpPacketizer<8000>;
using json = nlohmann::json;

q_msg_t gw_task_webrtc_mailbox;
string camIpPublic = "";

#ifdef TEST_USE_WEB_SOCKET
shared_ptr<Client> createPeerConnection(const Configuration &rtcConfig, weak_ptr<WebSocket> wws, string id);
#endif
shared_ptr<Client> createPeerConnection(const Configuration &rtcConfig, string id);

static shared_ptr<Stream> createStream(void);
static void addToStream(shared_ptr<Client> client, bool isAddingVideo);
static void startStream();
static shared_ptr<ClientTrackData> addVideo(const shared_ptr<PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const string cname, const string msid,
											const function<void(void)> onOpen);
static shared_ptr<ClientTrackData> addAudio(const shared_ptr<PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const string cname, const string msid,
											const function<void(void)> onOpen, string id);

static string wsUrl;
static vector<char> aLawPeerBuffers;
static Configuration rtcConfig;
static void printAllClients();
static int8_t loadIceServersConfigFile(Configuration &rtcConfig);
static int8_t cloudSetRtcServers(const string &serverType, const json &data);
static int8_t cloudGetRtcServers(const string &serverType, json &data);
static pair<string, int> extractAddressAndPort(const string &serverUrl);
static int getPublicIpAddr(string &ipPublic);

void *gw_task_webrtc_entry(void *) {
	ak_msg_t *msg = AK_MSG_NULL;

	wait_all_tasks_started();

// #if defined(RELEASE) && (RELEASE == 1)
// 	InitLogger(LogLevel::None);
// #else
// 	InitLogger(LogLevel::Error);
// #endif
// 	if (fca_configGetP2P(&Client::maxClientSetting) != APP_CONFIG_SUCCESS) {
// 		APP_DBG_RTC("load p2p config error, set p2p default\n");
// 		Client::maxClientSetting = CLIENT_MAX;
// 	}
// 	loadIceServersConfigFile(rtcConfig);
// 	rtcConfig.disableAutoNegotiation = false;	 // NOTE call setLocalDescription auto
	
// 	timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_GET_STUN_EXTERNAL_IP_REQ, 3000, TIMER_ONE_SHOT);
	auto ws = make_shared<WebSocket>();
// #ifdef TEST_USE_WEB_SOCKET
// 	/* init websocket */
// 	auto ws = make_shared<WebSocket>();	   // init poll serivce and threadpool = 4
// 	ws->onOpen([]() {
// 		APP_DBG_RTC("WebSocket connected, signaling ready\n");
// 		timer_remove_attr(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_CONNECT_SOCKET_REQ);
// #ifdef RELEASE
// 		timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_CLOSE_SIGNALING_SOCKET_REQ, GW_WEBRTC_CLOSE_SIGNALING_SOCKET_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
// #endif
// 	});

// 	ws->onClosed([]() {
// 		APP_DBG_RTC("WebSocket closed\n");
// 		timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_CONNECT_SOCKET_REQ, GW_WEBRTC_TRY_CONNECT_SOCKET_INTERVAL, TIMER_ONE_SHOT);
// 	});

// 	ws->onError([](const string &error) { APP_DBG_RTC("WebSocket failed: %s\n", error.c_str()); });

// 	ws->onMessage([&](variant<binary, string> data) {
// 		if (!holds_alternative<string>(data)) {
// 			return;
// 		}
// 		string msg = get<string>(data);
// 		APP_DBG_RTC("%s\n", msg.data());
// 		task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_SIGNALING_SOCKET_REQ, (uint8_t *)msg.data(), msg.length() + 1);
// 	});

// #ifndef RELEASE
// 	/* For Debugging */
// 	wsUrl = "ws://42.116.138.42:8089/" + fca_getSerialInfo();
// 	APP_PRINT("wsURL: %s\n", wsUrl.data());
// #endif

// 	if (!wsUrl.empty()) {
// 		timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_CONNECT_SOCKET_REQ, GW_WEBRTC_TRY_CONNECT_SOCKET_INTERVAL, TIMER_ONE_SHOT);
// 	}
// #else
// 	rtc::Preload();	   // init RTC global, auto call when call create Peer
// #endif

// 	APP_DBG_RTC("[STARTED] gw_task_webrtc_entry\n");

// 	sleep(1);
// 	avStream = createStream();
// 	startListSocketListeners();

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_WEBRTC_ID);

		switch (msg->header->sig) {
#ifdef TEST_USE_WEB_SOCKET
		case GW_WEBRTC_TRY_CONNECT_SOCKET_REQ: {
			APP_DBG_SIG("GW_WEBRTC_TRY_CONNECT_SOCKET_REQ\n");
			timer_remove_attr(GW_TASK_WEBRTC_ID, GW_WEBRTC_CLOSE_SIGNALING_SOCKET_REQ);
			ws->close();
			if (ws->isClosed() && !wsUrl.empty()) {
				APP_DBG_RTC("try connect to websocket: %s\n", wsUrl.data());
				ws->open(wsUrl);
				timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_CONNECT_SOCKET_REQ, GW_WEBRTC_TRY_CONNECT_SOCKET_INTERVAL, TIMER_ONE_SHOT);
			}
		} break;

		case GW_WEBRTC_SIGNALING_SOCKET_REQ: {
			APP_DBG_SIG("GW_WEBRTC_SIGNALING_SOCKET_REQ\n");
			try {
				json message = json::parse(string((char *)msg->header->payload, msg->header->len));
				APP_DBG_RTC("websocket rev msg: %s\n", message.dump().c_str());

				auto it = message.find("ClientId");
				if (it == message.end())
					break;
				string id = it->get<string>();

				it = message.find("Type");
				if (it == message.end())
					break;
				string type = it->get<string>();

				if (auto jt = clients.find(id); jt != clients.end()) {
					auto pc = jt->second->peerConnection;
					if (jt->second->dataChannel.value()->isOpen()) {
						APP_DBG_RTC("datachannel is openned\n");
					}
					else if (type == "answer") {
						APP_DBG_RTC("rev answer\n");
						auto sdp	= message["Sdp"].get<string>();
						auto desRev = Description(sdp, type);
						if (pc->remoteDescription().has_value()) {
							if (desRev.iceUfrag().has_value() && desRev.icePwd().has_value()) {
								if (desRev.iceUfrag().value() == pc->remoteDescription().value().iceUfrag().value() &&
									desRev.icePwd().value() == pc->remoteDescription().value().icePwd().value()) {
									auto remoteCandidates = desRev.extractCandidates();
									for (const auto &candidate : remoteCandidates) {
										APP_DBG_RTC("candidate: %s\n", candidate.candidate().c_str());
										pc->addRemoteCandidate(candidate);
									}
								}
								else {
									APP_DBG_RTC("rev id: %s\n", id.c_str());
									APP_DBG_RTC("rev frag: %s - local frag: %s\n", desRev.iceUfrag().value().c_str(), pc->remoteDescription().value().iceUfrag().value().c_str());
									throw runtime_error("ice-ufag diff ufrag");
								}
							}
							else {
								APP_DBG_RTC("rev id: %s\n", id.c_str());
								throw runtime_error("answer no ice-ufag");
							}
						}
						else {
							APP_DBG_RTC("add new sdp\n");
							try {
								pc->setRemoteDescription(desRev);
								APP_DBG_RTC("new session des: %s\n", pc->remoteDescription().value().iceUfrag().value().c_str());
							}
							catch (const exception &error) {
								APP_DBG_RTC("add remote des error: %s\n", error.what());
								throw runtime_error("add remote des error");
							}
						}
					}
					else if (type == "candidate") {
						APP_DBG_RTC("add remote candidate\n");
						auto sdp = message["Candidate"].get<string>();
						auto mid = message["Mid"].get<string>();
						pc->addRemoteCandidate(Candidate(sdp, mid));
					}
				}
				else if (clients.size() <= CLIENT_SIGNALING_MAX && type == "request") {
					pthread_mutex_lock(&Client::mtxClientsProtect);
					if (Client::totalClientsConnectSuccess >= Client::maxClientSetting) {
						APP_DBG_RTC("[WARN] total client max: %d\n", Client::totalClientsConnectSuccess);
						pthread_mutex_unlock(&Client::mtxClientsProtect);
						break;
					}
					pthread_mutex_unlock(&Client::mtxClientsProtect);

					timer_remove_attr(GW_TASK_WEBRTC_ID, GW_WEBRTC_CLOSE_SIGNALING_SOCKET_REQ);
					shared_ptr<Client> newCl = createPeerConnection(rtcConfig, make_weak_ptr(ws), id);
					Client::isSignalingRunning.store(true);	   // camera create peer and collect candidate long time <= 3s
					lockMutexListClients();
					clients.emplace(id, newCl);
					printAllClients();
					auto cl = clients.at(id);
					cl->startTimeoutConnect((int)Client::eCheckConnectType::Signaling, GW_WEBRTC_ERASE_CLIENT_NO_ANSWER_TIMEOUT_INTERVAL);
					APP_DBG_RTC("start timeout connect\n");
					unlockMutexListClients();
					Client::isSignalingRunning.store(false);
#ifdef RELEASE
					timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_CLOSE_SIGNALING_SOCKET_REQ, GW_WEBRTC_CLOSE_SIGNALING_SOCKET_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
#endif
				}
				else {
					APP_DBG_RTC("socket nothing\n");
					APP_DBG_RTC("total signaling: %d\n", clients.size());
				}
			}
			catch (const exception &error) {
				APP_DBG_RTC("socket signaling error: %s\n", error.what());
			}
		} break;
#endif

		case GW_WEBRTC_SIGNALING_MQTT_REQ: {
			APP_DBG_SIG("GW_WEBRTC_SIGNALING_MQTT_REQ\n");
			string id;
			try {
				json message = json::parse(string((char *)msg->header->payload, msg->header->len));
				// APP_DBG_RTC("on msg mqtt: %s\n", message.dump().data());

				id			= message["ClientId"].get<string>();
				string type = message["Type"].get<string>();

				if (auto jt = clients.find(id); jt != clients.end()) {
					auto pc = jt->second->peerConnection;
					if (jt->second->dataChannel.value()->isOpen()) {
						throw runtime_error("no rev answer");
					}
					else if (type == "answer") {
						APP_DBG_RTC("rev answer\n");
						auto sdp	= message["Sdp"].get<string>();
						auto desRev = Description(sdp, type);
						if (pc->remoteDescription().has_value()) {
							if (desRev.iceUfrag().has_value() && desRev.icePwd().has_value()) {
								if (desRev.iceUfrag().value() == pc->remoteDescription().value().iceUfrag().value() &&
									desRev.icePwd().value() == pc->remoteDescription().value().icePwd().value()) {
									auto remoteCandidates = desRev.extractCandidates();
									for (const auto &candidate : remoteCandidates) {
										APP_DBG_RTC("candidate: %s\n", candidate.candidate().c_str());
										pc->addRemoteCandidate(candidate);
									}
								}
								else {
									APP_DBG_RTC("rev id: %s\n", id.c_str());
									APP_DBG_RTC("rev frag: %s - local frag: %s\n", desRev.iceUfrag().value().c_str(), pc->remoteDescription().value().iceUfrag().value().c_str());
									throw runtime_error("ice-ufag diff ufrag");
								}
							}
							else {
								APP_DBG_RTC("rev id: %s\n", id.c_str());
								throw runtime_error("answer no ice-ufag");
							}
						}
						else {
							APP_DBG_RTC("add new sdp\n");
							try {
								pc->setRemoteDescription(desRev);
								APP_DBG_RTC("new session des: %s\n", pc->remoteDescription().value().iceUfrag().value().c_str());
							}
							catch (const exception &error) {
								APP_DBG_RTC("add remote des error: %s\n", error.what());
								throw runtime_error("add remote des error");
							}
						}
					}
					else if (type == "candidate") {
						auto sdp = message["Candidate"].get<string>();
						auto mid = message["Mid"].get<string>();
						pc->addRemoteCandidate(Candidate(sdp, mid));
					}
					else {
						throw runtime_error("unknown type");
					}
					json repJs = {
						{"Qos",	0														  },
						{"Data",	 {{"ClientId", id}}										   },
						  {"Result", {{"Ret", FCA_MQTT_RESPONE_SUCCESS}, {"Message", "Success"}}}
					 };
					task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_SIGNALING_MQTT_RES, (uint8_t *)repJs.dump().data(), repJs.dump().length() + 1);
				}
				else if (clients.size() <= CLIENT_SIGNALING_MAX && type == "request") {
					pthread_mutex_lock(&Client::mtxClientsProtect);
					if (Client::totalClientsConnectSuccess >= Client::maxClientSetting) {
						APP_DBG_RTC("[WARN] total client max: %d\n", Client::totalClientsConnectSuccess);
						pthread_mutex_unlock(&Client::mtxClientsProtect);
						throw runtime_error("ERROR_01");
					}
					pthread_mutex_unlock(&Client::mtxClientsProtect);

					shared_ptr<Client> newCl = createPeerConnection(rtcConfig, id);
					Client::isSignalingRunning.store(true);	   // camera create peer and collect candidate long time <= 3s
					lockMutexListClients();
					APP_DBG_RTC("release lockMutexListClients()\n");
					clients.emplace(id, newCl);
					printAllClients();
					auto cl = clients.at(id);
					cl->startTimeoutConnect((int)Client::eCheckConnectType::Signaling, GW_WEBRTC_ERASE_CLIENT_NO_ANSWER_TIMEOUT_INTERVAL);
					APP_DBG_RTC("start timeout connect\n");
					unlockMutexListClients();
					Client::isSignalingRunning.store(false);
				}
				else {
					APP_DBG_RTC("total signaling: %d\n", clients.size());
					throw runtime_error("signaling nothing");
				}
			}
			catch (const exception &error) {
				APP_DBG_RTC("mqtt signaling error: %s\n", error.what());
				json data	  = json::object();
				string errStr = error.what();
				uint8_t qos = 0, rc = FCA_MQTT_RESPONE_FAILED;
				if (errStr.compare("ERROR_01") == 0) {
					data = {
						{"Type",				 "deny"							   },
						  {"CurrentClientsTotal", Client::totalClientsConnectSuccess},
						   {"ClientMax",			 Client::maxClientSetting			 }
					  };
					qos = 1;
					rc	= FCA_MQTT_RESPONE_DENY;
				}
				json repJs = {
					{"Qos",	qos							   },
					  {"Result", {{"Ret", rc}, {"Message", "Fail"}}}
				};
				data["ClientId"] = id;
				repJs["Data"]	 = data;
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_SIGNALING_MQTT_RES, (uint8_t *)repJs.dump().data(), repJs.dump().length() + 1);
			}
		} break;

		case GW_WEBRTC_SET_SIGNLING_SERVER_REQ: {
			APP_DBG_SIG("GW_WEBRTC_SET_SIGNLING_SERVER_REQ\n");
			int8_t ret	  = APP_CONFIG_ERROR_DATA_INVALID;
			string svType = "";
			try {
				json data = json::parse(string((char *)msg->header->payload, msg->header->len));
				svType	  = data["Type"].get<string>();
				if (svType == MESSAGE_TYPE_WEB_SOCKET_SERVER) {
					wsUrl.clear();
					wsUrl = data["Data"]["SocketUrl"].get<string>() + "/" + fca_getSerialInfo();
					APP_DBG_RTC("new socket signaling url: %s\n", wsUrl.data());
					task_post_pure_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_CONNECT_SOCKET_REQ);
					ret = APP_CONFIG_SUCCESS;
				}
				else {	  // set stun, turn server url
					ret = cloudSetRtcServers(svType, data["Data"]);
					if (ret == APP_CONFIG_SUCCESS) {
						loadIceServersConfigFile(rtcConfig);
						task_post_pure_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_GET_STUN_EXTERNAL_IP_REQ);

						for (const auto &server : rtcConfig.iceServers) {
							APP_DBG_RTC("set signaling server: %s\n", server.hostname.data());
							// task_post_dynamic_msg(GW_TASK_SYS_ID, GW_SYS_UPDATE_DOMAIN_LIST_REQ, (uint8_t *)server.hostname.data(), server.hostname.length() + 1);
						}
					}
				}
			}
			catch (const exception &error) {
				APP_DBG_RTC("%s\n", error.what());
			}
			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, svType.c_str(), ret)) {
				if (!svType.empty()) {
					resJs["Data"]["Type"] = svType;
				}
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_WEBRTC_GET_SIGNLING_SERVER_REQ: {
			APP_DBG_SIG("GW_WEBRTC_GET_SIGNLING_SERVER_REQ\n");
			string svType = string((char *)msg->header->payload);
			APP_DBG_RTC("svType: %s, len: %d\n", svType.c_str(), svType.length());
			json data  = json::object();
			int8_t ret = cloudGetRtcServers(svType, data);
			/* response to mqtt server */
			json resJs;
			if (fca_getConfigJsonRes(resJs, data, svType.c_str(), ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_WEBRTC_CHECK_CLIENT_CONNECTED_REQ: {
			APP_DBG_SIG("GW_WEBRTC_CHECK_CLIENT_CONNECTED_REQ\n");
			string id((char *)msg->header->payload);
			APP_DBG_RTC("check connect client id: %s\n", id.c_str());
			if (auto jt = clients.find(id); jt != clients.end()) {
				auto dc = jt->second->dataChannel.value();
				if (!dc->isOpen()) {
					task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ERASE_CLIENT_REQ, (uint8_t *)id.c_str(), id.length() + 1);
				}
			}
			else {
				APP_DBG_RTC("clientId not found\n");
			}
		} break;

		case GW_WEBRTC_ERASE_CLIENT_REQ: {
			APP_DBG_SIG("GW_WEBRTC_ERASE_CLIENT_REQ\n");
			string id((char *)msg->header->payload);
			APP_DBG_RTC("clear client id: %s\n", id.c_str());
			Client::isSignalingRunning.store(true);
			lockMutexListClients();
			clients.erase(id);
			unlockMutexListClients();
			Client::isSignalingRunning.store(false);

			printAllClients();
		} break;

		case GW_WEBRTC_DBG_IPC_SEND_MESSAGE_REQ: {
			APP_DBG_SIG("GW_WEBRTC_DBG_IPC_SEND_MESSAGE_REQ\n");
			DataChannelSend_t *sendMsg = (DataChannelSend_t *)msg->header->payload;
			string id(sendMsg->clientId);
			string msg(sendMsg->msg);
			sendMsgControlDataChannel(id, msg);
		} break;

		case GW_WEBRTC_ON_MESSAGE_CONTROL_DATACHANNEL_REQ: {
			APP_DBG_SIG("GW_WEBRTC_ON_MESSAGE_CONTROL_DATACHANNEL_REQ\n");
			try {
				json message = json::parse(string((char *)msg->header->payload, msg->header->len));
				string id	 = message["ClientId"].get<string>();
				string data	 = message["Data"].get<string>();
				string resp	 = "";
				APP_DBG_RTC("client id: %s, msg: %s\n", id.c_str(), data.c_str());
				onDataChannelHdl(id, data, resp);
				sendMsgControlDataChannel(id, resp);
			}
			catch (const exception &error) {
				APP_DBG_RTC("%s\n", error.what());
			}
			APP_DBG_RTC("end datachannel handle\n");
		} break;

		case GW_WEBRTC_WATCHDOG_PING_REQ: {
			// APP_DBG_SIG("GW_WEBRTC_WATCHDOG_PING_REQ\n");
#ifndef RELEASE
			printAllClients();
#endif
			task_post_pure_msg(GW_TASK_WEBRTC_ID, GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_PING_NEXT_TASK_RES);

		} break;

		case GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK: {
			APP_DBG_SIG("GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK\n");
			pushToTalkThread.removePending();
			aLawPeerBuffers.clear();
			aLawPeerBuffers.shrink_to_fit();
			
			audioHelpers.silent();
			Client::currentClientPushToTalk.clear();
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_LOAD_AUDIO_CONFIG_REQ);
			timer_set(GW_TASK_DETECT_ID, GW_DETECT_CONTINUE_TRIGGER_SOUND_ALARM_REQ, 1500, TIMER_ONE_SHOT);
		} break;

		case GW_WEBRTC_TRY_GET_STUN_EXTERNAL_IP_REQ: {
			APP_DBG_SIG("GW_WEBRTC_TRY_GET_STUN_EXTERNAL_IP_REQ\n");
			sysThread.dispatch([]() {
				string ipPublic;
				if (getPublicIpAddr(ipPublic) == 0 && !ipPublic.empty()) {
					camIpPublic = ipPublic;
					task_post_pure_msg(GW_TASK_CLOUD_ID, GW_CLOUD_GEN_MQTT_STATUS_REQ);
				}
				else {
					timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_TRY_GET_STUN_EXTERNAL_IP_REQ, GW_WEBRTC_TRY_GET_EXTERNAL_IP_INTERVAL, TIMER_ONE_SHOT);
				}
			});
		} break;

		case GW_WEBRTC_CLOSE_SIGNALING_SOCKET_REQ: {
			APP_DBG_SIG("GW_WEBRTC_CLOSE_SIGNALING_SOCKET_REQ\n");
			if (ws->isOpen()) {
				APP_DBG_RTC("close signaling socket\n");
				ws->close();
				json data = {
					{"SocketUrl", ""}
				 };
				cloudSetRtcServers(MESSAGE_TYPE_WEB_SOCKET_SERVER, data);
				wsUrl.clear();
			}
		} break;

		case GW_WEBRTC_SET_CLIENT_MAX_REQ: {
			APP_DBG_SIG("GW_WEBRTC_SET_CLIENT_MAX_REQ\n");
			int8_t ret = APP_CONFIG_ERROR_DATA_INVALID;
			try {
				json data	  = json::parse(string((char *)msg->header->payload, msg->header->len));
				int maxClient = data["ClientMax"].get<int>();
				if (maxClient >= 0) {
					pthread_mutex_lock(&Client::mtxClientsProtect);
					Client::maxClientSetting = maxClient;
					pthread_mutex_unlock(&Client::mtxClientsProtect);
					fca_configSetP2P(&Client::maxClientSetting);
					ret = APP_CONFIG_SUCCESS;
				}
			}
			catch (const exception &error) {
				APP_DBG_RTC("%s\n", error.what());
			}
			/* response to mqtt server */
			json resJs;
			if (fca_setConfigJsonRes(resJs, MESSAGE_TYPE_P2P, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_WEBRTC_GET_CLIENT_MAX_REQ: {
			APP_DBG_SIG("GW_WEBRTC_GET_CLIENT_MAX_REQ\n");
			json data  = json::object();
			int8_t ret = fca_jsonSetP2P(data, &Client::maxClientSetting) ? APP_CONFIG_SUCCESS : APP_CONFIG_ERROR_ANOTHER;
			/* response to mqtt server */
			json resJs;
			if (fca_getConfigJsonRes(resJs, data, MESSAGE_TYPE_P2P, ret)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_WEBRTC_GET_CURRENT_CLIENTS_TOTAL_REQ: {
			APP_DBG_SIG("GW_WEBRTC_GET_CURRENT_CLIENTS_TOTAL_REQ\n");
			json data = {
				{"CurrentClientsTotal", Client::totalClientsConnectSuccess}
			   };
			/* response to mqtt server */
			json resJs;
			if (fca_getConfigJsonRes(resJs, data, MESSAGE_TYPE_P2P_INFO, APP_CONFIG_SUCCESS)) {
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_CAMERA_CONFIG_RES, (uint8_t *)resJs.dump().data(), resJs.dump().length() + 1);
			}
		} break;

		case GW_WEBRTC_DATACHANNEL_DOWNLOAD_RELEASE_REQ: {
			APP_DBG_SIG("GW_WEBRTC_DATACHANNEL_DOWNLOAD_RELEASE_REQ\n");
			string id((char *)msg->header->payload);
			if (auto jt = clients.find(id); jt != clients.end()) {
				auto cl = jt->second;
				cl->setDownloadFlag(false);
			}
			else {
				APP_DBG_RTC("clientId not found\n");
			}

			/* Remove temporary file (file raw thumbnail) */
			systemCmd("rm -f %s/*%s", FCA_MEDIA_JPEG_PATH, RECORD_IMG_SUFFIX);
			SYSLOG_LOG(LOG_INFO, "logf=%s [DOWNLOAD] client %s release download", APP_CONFIG_SYSLOG_CPU_RAM_FILE_NAME, id.data());
		} break;

		default:
			break;
		}

		/* free message */
		ak_msg_free(msg);
	}

	rtc::Cleanup();	   // clean RTC lib
	return (void *)0;
}

// Create and setup a PeerConnection
shared_ptr<Client> createPeerConnection(const Configuration &rtcConfig, string id) {
	APP_DBG_RTC("call createPeerConnection()\n");
	auto pc		= make_shared<PeerConnection>(rtcConfig);
	auto client = make_shared<Client>(pc);
	client->setId(id);

	pc->onStateChange([id](PeerConnection::State state) {
		APP_DBG_RTC("State: %d\n", (int)state);
		if (state == PeerConnection::State::Closed) {
			// remove disconnected client
			APP_DBG_RTC("call erase client from lib\n");
			systemTimer.add(milliseconds(100),
							[id](CppTime::timer_id) { task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ERASE_CLIENT_REQ, (uint8_t *)id.c_str(), id.length() + 1); });
		}
	});

	pc->onGatheringStateChange([rtcConfig, wpc = make_weak_ptr(pc), id](PeerConnection::GatheringState state) {
		APP_DBG_RTC("Gathering State: %d\n", (int)state);
		if (state == PeerConnection::GatheringState::Complete) {
			if (auto pc = wpc.lock()) {
				/* get list ice servers */
				vector<string> iceList;
				for (const auto &ice : rtcConfig.iceServers) {
					iceList.push_back(ice.hostname);
				}
				auto description = pc->localDescription();
				json message	 = {
					{"Qos",	1																													 },
					{"Data",	 {{"Type", description->typeString()}, {"Sdp", string(description.value())}, {"ClientId", id}, {"IceServers", iceList}}},
					{"Result", {{"Ret", FCA_MQTT_RESPONE_SUCCESS}, {"Message", "Success"}}															  }
				   };
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_SIGNALING_MQTT_RES, (uint8_t *)message.dump().data(), message.dump().length() + 1);
			}
		}
	});

	client->video = addVideo(pc, 102, 1, "VideoStream", "Stream", [id, wc = make_weak_ptr(client)]() {
		if (auto c = wc.lock()) {
			addToStream(c, true);
		}
		APP_DBG_RTC("Video from %s opened\n", id.c_str());
	});

	client->audio = addAudio(
		pc, 8, 2, "AudioStream", "Stream",
		[id, wc = make_weak_ptr(client)]() {
			if (auto c = wc.lock()) {
				addToStream(c, false);
			}
			APP_DBG_RTC("Audio from %s opened\n", id.c_str());
		},
		id);

	auto dc = pc->createDataChannel("control");
	dc->onOpen([id, wcl = make_weak_ptr(client)]() {
		if (auto cl = wcl.lock()) {
			auto dc = cl->dataChannel.value();
			APP_DBG_RTC("open channel label: %s success\n", dc->label().c_str());
			dc->send("Hello from " + fca_getSerialInfo());
			APP_DBG_RTC("remove timeout connect\n");

			pthread_mutex_lock(&Client::mtxClientsProtect);
			if (Client::totalClientsConnectSuccess < Client::maxClientSetting) {
				cl->removeTimeoutConnect();
				cl->setIsSignalingOk(true);
				Client::totalClientsConnectSuccess++;
				APP_DBG_RTC("total client: %d\n", Client::totalClientsConnectSuccess);

				json message = {
					{"Qos",	0																			 },
					{"Data",	 {{"Type", "ccu"}, {"CurrentClientsTotal", Client::totalClientsConnectSuccess}}},
					{"Result", {{"Ret", FCA_MQTT_RESPONE_SUCCESS}, {"Message", "Success"}}					  }
				   };
				task_post_dynamic_msg(GW_TASK_CLOUD_ID, GW_CLOUD_SIGNALING_MQTT_RES, (uint8_t *)message.dump().data(), message.dump().length() + 1);
			}
			else {
				task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ERASE_CLIENT_REQ, (uint8_t *)id.c_str(), id.length() + 1);
				pthread_mutex_unlock(&Client::mtxClientsProtect);
				return;
			}
			pthread_mutex_unlock(&Client::mtxClientsProtect);

			auto pc = cl->peerConnection;
			Candidate iceCam, iceApp;
			if (pc->getSelectedCandidatePair(&iceCam, &iceApp)) {
				APP_DBG_RTC("local candidate selected: %s, transport type: %d, address: %s\n", iceCam.candidate().c_str(), (int)iceCam.transportType(),
							iceCam.address().value().c_str());
				APP_DBG_RTC("remote candidate selected: %s, transport type: %d, address: %s\n", iceApp.candidate().c_str(), (int)iceApp.transportType(),
							iceApp.address().value().c_str());
			}
			else {
				APP_DBG_RTC("get selected candidate pair failed\n");
			}
		}
	});

	dc->onClosed([id, wdc = make_weak_ptr(dc)]() {
		if (auto dc = wdc.lock()) {
			APP_DBG_RTC("DataChannel label: %s from: %s closed\n", dc->label().c_str(), id.c_str());
		}
	});

	dc->onMessage([id, wcl = make_weak_ptr(client)](auto data) {
		// data holds either string or rtc::binaryL
		if (holds_alternative<string>(data)) {
			string str	 = get<string>(data);
			json dataRev = {
				{"ClientId", id },
				  {"Data",	   str}
			   };
			task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ON_MESSAGE_CONTROL_DATACHANNEL_REQ, (uint8_t *)dataRev.dump().c_str(), dataRev.dump().length() + 1);
		}
		else {
			APP_DBG_RTC("Binary message from %s received, size= %d\n", id.c_str(), get<rtc::binary>(data).size());
		}
	});

	dc->onBufferedAmountLow([id]() { APP_DBG_RTC("clientId %s send done\n", id.c_str()); });
	client->dataChannel = dc;

	// pc->setLocalDescription();
	return client;
};

#ifdef TEST_USE_WEB_SOCKET
shared_ptr<Client> createPeerConnection(const Configuration &rtcConfig, weak_ptr<WebSocket> wws, string id) {
	auto pc		= make_shared<PeerConnection>(rtcConfig);
	auto client = make_shared<Client>(pc);
	client->setId(id);

	pc->onStateChange([id](PeerConnection::State state) {
		APP_DBG_RTC("State: %d\n", (int)state);
		if (state == PeerConnection::State::Closed) {
			// remove disconnected client
			APP_DBG_RTC("call erase client from lib\n");
			systemTimer.add(milliseconds(100),
							[id](CppTime::timer_id) { task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ERASE_CLIENT_REQ, (uint8_t *)id.c_str(), id.length() + 1); });
		}
	});

	pc->onGatheringStateChange([wpc = make_weak_ptr(pc), id, wws](PeerConnection::GatheringState state) {
		APP_DBG_RTC("Gathering State: %d\n", (int)state);
		if (state == PeerConnection::GatheringState::Complete) {
			if (auto pc = wpc.lock()) {
				auto description = pc->localDescription();
				json message	 = {
					{"ClientId", id						   },
					  {"Type",	   description->typeString()	},
					   {"Sdp",	   string(description.value())}
				   };
				// Gathering complete, send answer
				if (auto ws = wws.lock()) {
					ws->send(message.dump());
				}
			}
		}
	});

	client->video = addVideo(pc, 102, 1, "VideoStream", "Stream", [id, wc = make_weak_ptr(client)]() {
		if (auto c = wc.lock()) {
			addToStream(c, true);
		}
		APP_DBG_RTC("Video from %s opened\n", id.c_str());
	});

	client->audio = addAudio(
		pc, 8, 2, "AudioStream", "Stream",
		[id, wc = make_weak_ptr(client)]() {
			if (auto c = wc.lock()) {
				addToStream(c, false);
			}
			APP_DBG_RTC("Audio from %s opened\n", id.c_str());
		},
		id);

	auto dc = pc->createDataChannel("control");
	dc->onOpen([id, wcl = make_weak_ptr(client)]() {
		if (auto cl = wcl.lock()) {
			auto dc = cl->dataChannel.value();
			APP_DBG_RTC("open channel label: %s success\n", dc->label().c_str());
			dc->send("Hello from " + fca_getSerialInfo());
			APP_DBG_RTC("remove timeout connect\n");
			
			pthread_mutex_lock(&Client::mtxClientsProtect);
			if (Client::totalClientsConnectSuccess < Client::maxClientSetting) {
				cl->removeTimeoutConnect();
				cl->setIsSignalingOk(true);
				Client::totalClientsConnectSuccess++;
				APP_DBG_RTC("total client: %d\n", Client::totalClientsConnectSuccess);
			}
			else {
				task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ERASE_CLIENT_REQ, (uint8_t *)id.c_str(), id.length() + 1);
				pthread_mutex_unlock(&Client::mtxClientsProtect);
				return;
			}
			pthread_mutex_unlock(&Client::mtxClientsProtect);

			// auto pc = cl->peerConnection;
			// Candidate iceCam, iceApp;
			// if (pc->getSelectedCandidatePair(&iceCam, &iceApp)) {
			// 	APP_DBG_RTC("local candidate selected: %s, transport type: %d, address: %s\n", iceCam.candidate().c_str(), (int)iceCam.transportType(),
			// 				iceCam.address().value().c_str());
			// 	APP_DBG_RTC("remote candidate selected: %s, transport type: %d, address: %s\n", iceApp.candidate().c_str(), (int)iceApp.transportType(),
			// 				iceApp.address().value().c_str());
			// }
			// else {
			// 	APP_DBG_RTC("get selected candidate pair failed\n");
			// }
		}
	});

	dc->onClosed([id, wdc = make_weak_ptr(dc)]() {
		if (auto dc = wdc.lock()) {
			APP_DBG_RTC("DataChannel label: %s from: %s closed\n", dc->label().c_str(), id.c_str());
		}
	});

	dc->onMessage([id, wcl = make_weak_ptr(client)](auto data) {
		// data holds either string or rtc::binary
		if (holds_alternative<string>(data)) {
			string str	 = get<string>(data);
			json dataRev = {
				{"ClientId", id },
				  {"Data",	   str}
			   };
			task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_ON_MESSAGE_CONTROL_DATACHANNEL_REQ, (uint8_t *)dataRev.dump().c_str(), dataRev.dump().length() + 1);
		}
		else {
			APP_DBG_RTC("Binary message from %s received, size= %d\n", id.c_str(), get<rtc::binary>(data).size());
		}
	});
	client->dataChannel = dc;
	return client;
};
#endif

void printAllClients() {
	APP_DBG_RTC("******* List Clients ********\n");
	for (const auto &pair : clients) {
		APP_DBG_RTC("\tId: %s\n", pair.second->getId().c_str());
	}
}

int8_t loadIceServersConfigFile(Configuration &rtcConfig) {
	// rtcServersConfig_t rtcServerCfg;
	// int8_t ret = fca_configGetRtcServers(&rtcServerCfg);
	// try {
	// 	if (ret == APP_CONFIG_SUCCESS) {
	// 		rtcConfig.iceServers.clear();

	// 		APP_DBG_RTC("List stun server:\n");
	// 		string url = "";
	// 		int size   = rtcServerCfg.arrStunServerUrl.size();
	// 		for (int idx = 0; idx < size; idx++) {
	// 			url = rtcServerCfg.arrStunServerUrl.at(idx);
	// 			APP_DBG_RTC("\t[%d] url: %s\n", idx + 1, url.c_str());
	// 			if (url != "") {
	// 				rtcConfig.iceServers.emplace_back(url);
	// 			}
	// 		}
	// 		APP_DBG_RTC("\n");
	// 		APP_DBG_RTC("List turn server:\n");
	// 		size = rtcServerCfg.arrTurnServerUrl.size();
	// 		for (int idx = 0; idx < size; idx++) {
	// 			url = rtcServerCfg.arrTurnServerUrl.at(idx);
	// 			APP_DBG_RTC("\t[%d] url: %s\n", idx + 1, url.c_str());
	// 			if (url != "") {
	// 				rtcConfig.iceServers.emplace_back(url);
	// 			}
	// 		}
	// 		APP_DBG_RTC("\n");
	// 	}
	// }
	// catch (const exception &error) {
	// 	APP_DBG_RTC("loadIceServersConfigFile %s\n", error.what());
	// 	ret = APP_CONFIG_ERROR_DATA_INVALID;
	// }

	rtcConfig.iceServers.emplace_back("turn:turnuser:camfptvnturn133099@stunp-connect.fcam.vn:3478");
	rtcConfig.iceServers.emplace_back("turn:turnuser:camfptvnturn133099@stun-connect.fcam.vn:3478");

	return APP_CONFIG_SUCCESS;
}

int8_t cloudSetRtcServers(const string &serverType, const json &data) {
	int8_t ret = APP_CONFIG_ERROR_DATA_INVALID;
	string type, keyUrl;
	if (serverType == MESSAGE_TYPE_STUN_SERVER) {
		type   = "stun";
		keyUrl = "StunUrl";
	}
	else if (serverType == MESSAGE_TYPE_TURN_SERVER) {
		type   = "turn";
		keyUrl = "TurnUrl";
	}
	else {
		APP_DBG_RTC("[ERR] unknown rtc server type\n");
		return ret;
	}

	try {
		json rtcSvCfgJs = json::object();
		if (data.contains(keyUrl)) {
			bool update		= false;
			string fileName = FCA_RTC_SERVERS_FILE;
			fca_readConfigUsrDfaulFileToJs(rtcSvCfgJs, fileName);
			json &connections = rtcSvCfgJs["Connections"];
			if (!connections.is_array()) {
				connections = json::array();
			}
			json urlJs = data[keyUrl].get<vector<string>>();
			for (auto &obj : connections) {
				if (obj["Type"] == type) {
					obj[keyUrl] = urlJs;
					update		= true;
					break;
				}
			}

			if (!update) {
				connections.push_back({
					{"Type", type },
					{keyUrl, urlJs}
				   });
			}
			APP_DBG_RTC("new rtc server config: %s\n", rtcSvCfgJs.dump().c_str());
			ret = fca_configSetRtcServers(rtcSvCfgJs);
		}
	}
	catch (const exception &error) {
		APP_DBG_RTC("%s\n", error.what());
	}
	return ret;
}

int8_t cloudGetRtcServers(const string &serverType, json &data) {
	rtcServersConfig_t rtcServerCfg;
	int8_t ret = fca_configGetRtcServers(&rtcServerCfg);
	if (ret == APP_CONFIG_SUCCESS) {
		if (serverType == MESSAGE_TYPE_STUN_SERVER) {
			data["StunUrl"] = rtcServerCfg.arrStunServerUrl;
		}
		else if (serverType == MESSAGE_TYPE_TURN_SERVER) {
			data["TurnUrl"] = rtcServerCfg.arrTurnServerUrl;
		}
		else {
			APP_DBG_RTC("[ERR] unknown rtc server type\n");
			ret = APP_CONFIG_ERROR_DATA_INVALID;
		}
	}
	return ret;
}

pair<string, int> extractAddressAndPort(const string &serverUrl) {
	string address;
	int port = -1;

	size_t colonPos = serverUrl.find_last_of(':');
	if (colonPos != string::npos) {
		stringstream(serverUrl.substr(colonPos + 1)) >> port;
		string tempStr = serverUrl.substr(0, colonPos);
		size_t atPos   = tempStr.find_last_of('@');
		if (atPos == string::npos) {
			atPos = tempStr.find_last_of(':');
		}

		if (atPos != string::npos) {
			address = tempStr.substr(atPos + 1);
		}
		else {
			address = tempStr.substr(0);
		}
	}
	else {
		address = "";
	}

	return make_pair(address, port);
}

int getPublicIpAddr(string &ipPublic) {
	int ret;
	rtcServersConfig_t rtcServerCfg;
	ipPublic = "";
	ret		 = fca_configGetRtcServers(&rtcServerCfg);
	if (ret == APP_CONFIG_SUCCESS) {
		struct STUNServer server;
		vector<string> arrServers;
		arrServers.insert(arrServers.end(), rtcServerCfg.arrStunServerUrl.begin(), rtcServerCfg.arrStunServerUrl.end());
		arrServers.insert(arrServers.end(), rtcServerCfg.arrTurnServerUrl.begin(), rtcServerCfg.arrTurnServerUrl.end());
#ifndef RELEASE
		for (const string &url : arrServers) {
			cout << url << endl;
		}
#endif
		for (const auto &url : arrServers) {
			auto result = extractAddressAndPort(url);
			if (result.second != -1) {
				APP_DBG_NET("stun address: %s, port: %d\n", result.first.c_str(), result.second);
				server.address = result.first.c_str();
				server.port	   = result.second;

				char *address = (char *)malloc(sizeof(char) * 100);
				if (address) {
					bzero(address, 100);
					ret = getPublicIPAddress(server, address);
					if (ret != 0) {
						APP_DBG_NET("%s: Failed. Error: %d\n", server.address, ret);
					}
					else {
						string ip(address);
						if (!isLocalIP(ip)) {
							ipPublic = ip;
							APP_DBG_NET("%s: Public IP: %s\n", server.address, ipPublic.c_str());
							ret = 0;
							free(address);
							break;
						}
					}
					free(address);
				}
			}
		}
	}

	return ret;
}

shared_ptr<ClientTrackData> addVideo(const shared_ptr<PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const string cname, const string msid,
									 const function<void(void)> onOpen) {
	auto video = Description::Video(cname, Description::Direction::SendOnly);

	/* Select encoder is H264 or H265 */
	FCA_ENCODE_S encoders = videoHelpers.getEncoders();
	if (encoders.mainStream.codec == FCA_CODEC_VIDEO_H264) {
		video.addH264Codec(payloadType);
		video.addSSRC(ssrc, cname, msid, cname);
		auto track = pc->addTrack(video);
		// create RTP configuration
		auto rtpConfig = make_shared<RtpPacketizationConfig>(ssrc, cname, payloadType, H264RtpPacketizer::ClockRate);
		// create packetizer
		auto packetizer = make_shared<H264RtpPacketizer>(NalUnit::Separator::LongStartSequence, rtpConfig);
		// add RTCP SR handler
		auto srReporter = make_shared<RtcpSrReporter>(rtpConfig);
		packetizer->addToChain(srReporter);

		// add RTCP NACK handler
		auto nackResponder = make_shared<RtcpNackResponder>();
		packetizer->addToChain(nackResponder);
		// set handler
		track->setMediaHandler(packetizer);
		track->onOpen(onOpen);
		auto trackData = make_shared<ClientTrackData>(track, srReporter);
		return trackData;
	}
	else {
		video.addH265Codec(payloadType);
		video.addSSRC(ssrc, cname, msid, cname);
		auto track = pc->addTrack(video);
		// create RTP configuration
		auto rtpConfig = make_shared<RtpPacketizationConfig>(ssrc, cname, payloadType, H265RtpPacketizer::ClockRate);
		// create packetizer
		auto packetizer = make_shared<H265RtpPacketizer>(NalUnit::Separator::LongStartSequence, rtpConfig);
		// add RTCP SR handler
		auto srReporter = make_shared<RtcpSrReporter>(rtpConfig);
		packetizer->addToChain(srReporter);

		// add RTCP NACK handler
		auto nackResponder = make_shared<RtcpNackResponder>();
		packetizer->addToChain(nackResponder);
		// set handler
		track->setMediaHandler(packetizer);
		track->onOpen(onOpen);
		auto trackData = make_shared<ClientTrackData>(track, srReporter);
		return trackData;
	}

	return nullptr;
}

shared_ptr<ClientTrackData> addAudio(const shared_ptr<PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const string cname, const string msid,
									 const function<void(void)> onOpen, string id) {
	auto audio = Description::Audio(cname, Description::Direction::SendRecv);
	audio.addPCMACodec(payloadType);
	audio.addSSRC(ssrc, cname, msid, cname);
	auto track = pc->addTrack(audio);
	// create RTP configuration
	auto rtpConfig = make_shared<RtpPacketizationConfig>(ssrc, cname, payloadType, ALAWRtpPacketizer::DefaultClockRate);
	// create packetizer
	auto packetizer = make_shared<ALAWRtpPacketizer>(rtpConfig);
	// add RTCP SR handler
	auto srReporter = make_shared<RtcpSrReporter>(rtpConfig);
	packetizer->addToChain(srReporter);
	// add RTCP NACK handler
	auto nackResponder = make_shared<RtcpNackResponder>();
	packetizer->addToChain(nackResponder);
	// set handler
	track->setMediaHandler(packetizer);

	/* Handling push to talk */
	track->onMessage(
		[id](rtc::binary message) {
			if (Client::currentClientPushToTalk.empty() == true || id != Client::currentClientPushToTalk.get()) {
				return;
			}

			if (message.size() < sizeof(rtc::RtpHeader)) {
				return;
			}

			/* This is an RTP packet */
			auto rtpHdr		  = reinterpret_cast<rtc::RtpHeader *>(message.data());
			char *rtpBody	  = rtpHdr->getBody();
			size_t bodyLength = message.size() - rtpHdr->getSize();

			if (bodyLength < 160) { /* This is not audio samples */
				return;
			}
			// APP_DBG("audio track rev msg: %d\n", message.size());

			aLawPeerBuffers.insert(aLawPeerBuffers.end(), rtpBody, rtpBody + bodyLength);
			if (aLawPeerBuffers.size() > 320) {
				audioHelpers.playBuffers((char*)aLawPeerBuffers.data(), aLawPeerBuffers.size());
				aLawPeerBuffers.clear();
			}

			timer_set(GW_TASK_WEBRTC_ID, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK, GW_WEBRTC_RELEASE_CLIENT_PUSH_TO_TALK_INTERVAL, TIMER_ONE_SHOT);
		},
		nullptr);

	track->onOpen(onOpen);
	auto trackData = make_shared<ClientTrackData>(track, srReporter);
	return trackData;
}

shared_ptr<Stream> createStream() {
	// extern VideoCtrl videoCtrl;
	// auto viEncodeSett = videoCtrl.videoEncodeConfig();
	// auto video		  = make_shared<VideoSource>(viEncodeSett.mainStream.format.FPS);
	/* TODO */
	auto video0 = make_shared<VideoSource>(VideoHelpers::VIDEO0_PRODUCER, VIDEO_LIVESTREAM_SAMPLE_DURATION_US);
	auto video1 = make_shared<VideoSource>(VideoHelpers::VIDEO1_PRODUCER, VIDEO_LIVESTREAM_SAMPLE_DURATION_US);
	auto audio0 = make_shared<AudioSource>(AudioHelpers::AUDIO0_PRODUCER, AUDIO_LIVESTREAM_SAMPLE_DURATION_US);
	auto stream = make_shared<Stream>(video0, video1, audio0);
	stream->start();

	return stream;
}

void startStream() {
	if (avStream.has_value()) {
		return;
	}

	shared_ptr<Stream> stream = createStream();
	avStream				  = stream;
	stream->start();
}

void addToStream(shared_ptr<Client> client, bool isAddingVideo) {
	if (client->getState() == Client::State::Waiting) {
		client->setState(isAddingVideo ? Client::State::WaitingForAudio : Client::State::WaitingForVideo);
	}
	else if ((client->getState() == Client::State::WaitingForAudio && !isAddingVideo) || (client->getState() == Client::State::WaitingForVideo && isAddingVideo)) {
		// Audio and video tracks are collected now
		assert(client->video.has_value() && client->audio.has_value());
		auto video = client->video.value();

		if (avStream.has_value()) {
			// sendInitialNalus(avStream.value(), video);
		}

		client->setState(Client::State::Ready);
	}
	if (client->getState() == Client::State::Ready) {
		startStream();
	}
}
