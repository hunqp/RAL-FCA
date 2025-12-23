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
#include "recorder.h"
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
	int setPlbSdSource(const char *filename);
	int setPlbSdControl(PLAYBACK_CONTROL cmd, int params);
	PLAYBACK_STATUS getPlbSdStatus();

	void setSequenceId(int32_t seq);
	void setSessionId(int32_t ses);
	int32_t getSequenceId();
	int32_t getSessionId();

	std::string getId();
	void setId(const std::string &newId);

public:
	const std::shared_ptr<rtc::PeerConnection> &peerConnection = mPeerConnection;
	std::optional<std::shared_ptr<ClientTrackData>> video;
	std::optional<std::shared_ptr<ClientTrackData>> audio;
	std::optional<std::shared_ptr<rtc::DataChannel>> dataChannel;
	std::vector<fileDownloadInfo_t> arrFilesDownload;

	static int maxClientSetting;
	static int totalClientsConnectSuccess;
	static pthread_mutex_t mtxClientsProtect;
	static AtomicString currentClientPushToTalk;

private:
	pthread_mutex_t mPOSIXMutex, mDownloadMutex;
	State mState = State::Waiting;
	std::string mId;
	std::shared_ptr<rtc::PeerConnection> mPeerConnection;

	eOptions mOptions = eOptions::Idle;
	eResolution mLiveResolution = eResolution::HD720p;

	int32_t mSequenceId;
	int32_t mSessionId;
	bool mIsSignalingOk;
	std::atomic<int> mConnTimerId;

	/* Playback attributes */
	bool mDownloadFlag = false;
	fileDownloadInfo_t mCurrentFileTransfer;
	uint32_t mCursorFile;
	std::atomic<int> mPacketTimerId;
	
	/* Playback */
	volatile bool mLoop;
	float mSpeedFactor;
	MP4v2Reader mMP4Auxs;
	PLAYBACK_STATUS mPlbState = PB_STATE_STOPPED;
	pthread_t mPblCntlId = (pthread_t)NULL;

	static void *hdlSendSdSourceToPeer(void *);
	std::pair<uint64_t, Stream::StreamSourceType> unsafePrepareForSample(uint64_t viSampleTime_us, uint64_t auSampleTime_us, uint64_t startTime);
};

typedef std::unordered_map<std::string, std::shared_ptr<Client>> ClientsGroup_t;

#endif /* helpers_hpp */
