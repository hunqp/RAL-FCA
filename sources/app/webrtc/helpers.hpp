/**
 * libdatachannel streamer example
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef helpers_hpp
#define helpers_hpp

#include <vector>
#include <shared_mutex>
#include <string>

#include "app_dbg.h"
#include "rtc/rtc.hpp"
#include "stream.hpp"
#include "utils.h"

#define CLIENT_SIGNALING_MAX 20
#define CLIENT_MAX			 5

using namespace std;

typedef struct t_fileDownloadInfo {
	string type;
	string path;
	uint32_t size;
} fileDownloadInfo_t;

struct ClientTrackData {
	std::shared_ptr<rtc::Track> track;
	std::shared_ptr<rtc::RtcpSrReporter> sender;

	ClientTrackData(std::shared_ptr<rtc::Track> track, std::shared_ptr<rtc::RtcpSrReporter> sender) {
		this->track	 = track;
		this->sender = sender;
	}
};

struct ClientTrack {
	std::string id;
	std::shared_ptr<ClientTrackData> trackData;

	ClientTrack(std::string id, std::shared_ptr<ClientTrackData> trackData) {
		this->id		= id;
		this->trackData = trackData;
	}
};

typedef struct {
    std::string pathStr;
    uint32_t cursor;
    uint32_t fileSize;
	
	uint64_t sampleTime_us;
	uint64_t sampleDuration_us;
} SdPlbAttributes;

struct PlbSdSource {
	uint64_t startTime = 0;
	uint32_t durationInSecs;
	SdPlbAttributes video;
	SdPlbAttributes audio;

	void reset();
	void cntlSpeed(int wantedSpeed);
	void setCursorPos(int wantedSecs);
	bool loadattributes(std::string viPathStr, std::string auPathStr, std::string descStr);
	rtc::binary prefetchSrcVideoSamples();
	rtc::binary prefetchSrcAudioSamples();
};

class Client : public std::enable_shared_from_this<Client> {
public:
	enum eResolution {
		HD720p,
		FullHD1080p,
		Minimum,
	};

	enum class State {
		Waiting,
		WaitingForVideo,
		WaitingForAudio,
		Ready
	};

	enum class eOptions {
		Idle,
		LiveStream,
		Playback,
		Pending,
	};

	enum class ePushToTalkREQStatus {
		Begin,
		Stop,
		KeepSession
	};

	enum class eCheckConnectType {
		Signaling
	};

	Client(std::shared_ptr<rtc::PeerConnection> pc);
	~Client();

	void setState(State state);
	State getState();

	void startTimeoutConnect(int typeCheck, int timeout);
	void removeTimeoutConnect();

	void setMediaStreamOptions(eOptions opt);
	eOptions getMediaStreamOptions();
	void setLiveResolution(eResolution res);
	eResolution getLiveResolution();

	/* Download from EMMC */
	bool getDownloadFlag();
	void setDownloadFlag(bool newIsDownloading);

	fileDownloadInfo_t getCurrentFileTransfer();
	void setCurrentFileTransfer(const fileDownloadInfo_t &newCurrentFileTransfer);
	uint32_t getCursorFile();
	void setCursorFile(uint32_t newCursorFile);
	void startTimeoutDownload();
	void removeTimeoutDownload();

	/* Media Stream Methods (Live & PLayBack) */
	void setPlbSdSource(PlbSdSource pbSdSrc);
	void setPlbSdControl(PLAYBACK_CONTROL ctl, uint32_t *argv);
	PLAYBACK_STATUS getPlbSdStatus();
	uint32_t getPlbTimeSpentInSecs();
	rtc::binary getPlbSdSample(bool bVideo);

	void setSequenceId(int32_t seq);
	void setSessionId(int32_t ses);
	int32_t getSequenceId();
	int32_t getSessionId();
	bool isSignalingOk();
	void setIsSignalingOk(bool value);

	std::string getId();
	void setId(const std::string &newId);
	static void cleanup() {
		pthread_mutex_destroy(&mtxClientsProtect);
	}

public:
	const std::shared_ptr<rtc::PeerConnection> &peerConnection = _peerConnection;
	std::optional<std::shared_ptr<ClientTrackData>> video;
	std::optional<std::shared_ptr<ClientTrackData>> audio;
	std::optional<std::shared_ptr<rtc::DataChannel>> dataChannel;
	std::vector<fileDownloadInfo_t> arrFilesDownload;

	static std::atomic<bool> isSignalingRunning;
	static int maxClientSetting;
	static int totalClientsConnectSuccess;
	static pthread_mutex_t mtxClientsProtect;
	static AtomicString currentClientPushToTalk;

private:
	pthread_mutex_t mPOSIXMutex, mDownloadMutex;
	State mState = State::Waiting;
	std::string mId;
	std::shared_ptr<rtc::PeerConnection> _peerConnection;

	eOptions mOptions			  = eOptions::Idle;
	eResolution mLiveResolution	  = eResolution::HD720p;

	int32_t mSequenceId;
	int32_t mSessionId;
	bool mIsSignalingOk;
	std::atomic<int> mConnTimerId;

	/* Playback attributes */
	bool mDownloadFlag = false;
	fileDownloadInfo_t mCurrentFileTransfer;
	uint32_t mCursorFile;
	std::atomic<int> mPacketTimerId;
	
	pthread_t mPblCntlId = (pthread_t)NULL;
	PlbSdSource mPbSdSource;
	PLAYBACK_STATUS mPbState = PB_STATE_STOPPED;
	volatile bool mLoop = true;

	static void *hdlSendSdSourceToPeer(void *);
};

typedef std::unordered_map<std::string, std::shared_ptr<Client>> ClientsGroup_t;

#endif /* helpers_hpp */
