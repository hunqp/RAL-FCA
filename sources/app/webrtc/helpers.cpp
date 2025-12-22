/**
 * libdatachannel streamer example
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "helpers.hpp"
#include "utils.h"

#include <ctime>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include "app.h"
#include "app_data.h"
#include "task_list.h"
#include "h26xparsec.h"

using namespace rtc;
using namespace std::chrono;

pthread_mutex_t Client::mtxClientsProtect	 = PTHREAD_MUTEX_INITIALIZER;
std::atomic<bool> Client::isSignalingRunning(false);
int Client::maxClientSetting				 = CLIENT_MAX;
int Client::totalClientsConnectSuccess		 = 0;
AtomicString Client::currentClientPushToTalk("");

Client::Client(std::shared_ptr<rtc::PeerConnection> pc) {
	_peerConnection = pc;
	mPOSIXMutex	= PTHREAD_MUTEX_INITIALIZER;
	mDownloadMutex = PTHREAD_MUTEX_INITIALIZER;
	mIsSignalingOk	= false;
	mConnTimerId.store(-1);
	mPacketTimerId.store(-1);
}

Client::~Client() {
	APP_DBG_RTC("~Client id: %s\n", mId.c_str());
	
	setPlbSdControl(PB_CTL_STOP, 0);
	removeTimeoutDownload();

	pthread_mutex_destroy(&mPOSIXMutex);
	pthread_mutex_destroy(&mDownloadMutex);
	if (isSignalingOk()) {
		pthread_mutex_lock(&mtxClientsProtect);
		if (--Client::totalClientsConnectSuccess < 0) {
			pthread_mutex_unlock(&mtxClientsProtect);
			APP_DBG_RTC("Client::totalClientsConnectSuccess < 0\n");
			FATAL("RTC", 0x02);
		}
		pthread_mutex_unlock(&mtxClientsProtect);
		APP_DBG("total client: %d\n", Client::totalClientsConnectSuccess);
	}
	removeTimeoutConnect();
}

void Client::setState(State mState) {
	this->mState = mState;
}

Client::State Client::getState() {
	return mState;
}

void Client::startTimeoutConnect(int typeCheck, int timeout) {
	mConnTimerId.store(systemTimer.add(milliseconds(timeout), [this, weak_this = weak_from_this(), typeCheck, timeout](CppTime::timer_id idT) {
		auto shared_this = weak_this.lock();
		if (!shared_this) {
			APP_DBG_RTC("client not found\n");
			return;
		}
		string id = getId();
		APP_DBG_RTC("check connect client: %s, type: %d, timeout: %d\n", id.c_str(), typeCheck, timeout);
		if (mConnTimerId.load() == (int)idT) {
			mConnTimerId.store(-1);
			if (typeCheck == (int)Client::eCheckConnectType::Signaling) {
				task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_CHECK_CLIENT_CONNECTED_REQ, (uint8_t *)id.c_str(), id.length() + 1);
			}
		}
	}));
}

void Client::removeTimeoutConnect() {
	if (mConnTimerId.load() >= 0) {
		systemTimer.remove(mConnTimerId.load());
		mConnTimerId.store(-1);
	}
}

void Client::setMediaStreamOptions(eOptions opt) {
	pthread_mutex_lock(&mPOSIXMutex);
	mOptions = opt;
	pthread_mutex_unlock(&mPOSIXMutex);
}

Client::eOptions Client::getMediaStreamOptions() {
	pthread_mutex_lock(&mPOSIXMutex);
	auto ret = mOptions;
	pthread_mutex_unlock(&mPOSIXMutex);

	return ret;
}

void Client::setLiveResolution(eResolution res) {
	pthread_mutex_lock(&mPOSIXMutex);
	mLiveResolution = res;
	pthread_mutex_unlock(&mPOSIXMutex);
}

Client::eResolution Client::getLiveResolution() {
	pthread_mutex_lock(&mPOSIXMutex);
	auto ret = mLiveResolution;
	pthread_mutex_unlock(&mPOSIXMutex);

	return ret;
}

void Client::setSequenceId(int32_t seq) {
	pthread_mutex_lock(&mPOSIXMutex);
	mSequenceId = seq;
	pthread_mutex_unlock(&mPOSIXMutex);
}

void Client::setSessionId(int32_t ses) {
	pthread_mutex_lock(&mPOSIXMutex);
	mSessionId = ses;
	pthread_mutex_unlock(&mPOSIXMutex);
}

int32_t Client::getSequenceId() {
	pthread_mutex_lock(&mPOSIXMutex);
	auto ret = mSequenceId;
	pthread_mutex_unlock(&mPOSIXMutex);
	return ret;
}

int32_t Client::getSessionId() {
	pthread_mutex_lock(&mPOSIXMutex);
	auto ret = mSessionId;
	pthread_mutex_unlock(&mPOSIXMutex);
	return ret;
}

bool Client::isSignalingOk() {
	return mIsSignalingOk;
}

void Client::setIsSignalingOk(bool value) {
	mIsSignalingOk = value;
}

string Client::getId() {
	return mId;
}

void Client::setId(const string &newId) {
	mId = newId;
}

/// DOWNLOAD FUNCITONS /////////////////////////////////////////////////////////////

bool Client::getDownloadFlag() {
	bool ret;
	pthread_mutex_lock(&mDownloadMutex);
	ret = mDownloadFlag;
	pthread_mutex_unlock(&mDownloadMutex);
	return ret;
}

void Client::setDownloadFlag(bool newDownloadFlag) {
	pthread_mutex_lock(&mDownloadMutex);
	mDownloadFlag = newDownloadFlag;
	pthread_mutex_unlock(&mDownloadMutex);
}

fileDownloadInfo_t Client::getCurrentFileTransfer() {
	return mCurrentFileTransfer;
}

void Client::setCurrentFileTransfer(const fileDownloadInfo_t &newCurrentFileTransfer) {
	mCurrentFileTransfer = newCurrentFileTransfer;
}

uint32_t Client::getCursorFile() {
	return mCursorFile;
}

void Client::setCursorFile(uint32_t newCursorFile) {
	mCursorFile = newCursorFile;
}

void Client::startTimeoutDownload() {
	mPacketTimerId.store(systemTimer.add(milliseconds(GW_WEBRTC_WAIT_DOWNLOAD_RESPONSE_TIMEOUT_INTERVAL), [this, weak_this = weak_from_this()](CppTime::timer_id idT) {
		auto shared_this = weak_this.lock();
		if (!shared_this) {
			APP_DBG_RTC("client not found\n");
			return;
		}
		if (mPacketTimerId.load() == (int)idT) {
			mPacketTimerId.store(-1);
			string id	   = getId();
			task_post_dynamic_msg(GW_TASK_WEBRTC_ID, GW_WEBRTC_DATACHANNEL_DOWNLOAD_RELEASE_REQ, (uint8_t *)id.c_str(), id.length() + 1);
		}
	}));
}

void Client::removeTimeoutDownload() {
	if (mPacketTimerId.load() >= 0) {
		systemTimer.remove(mPacketTimerId.load());
		mPacketTimerId.store(-1);
	}
}

/// PLAYBACK FUNCITONS ///////////////////////////////////////////////////////////// 
int Client::setPlbSdSource(const char *filename) {
	if (access(filename, F_OK) != 0) {
		return -1;
	}
	int rc = -1;
	pthread_mutex_lock(&mPOSIXMutex);
	rc = mMP4Auxs.begin(filename);
	pthread_mutex_unlock(&mPOSIXMutex);

	return rc;
}

int Client::setPlbSdControl(PLAYBACK_CONTROL cmd, int params) {
	int rc = 0;

	pthread_mutex_lock(&mPOSIXMutex);

	switch (cmd) {
	case PB_CTL_PLAY: {
		this->mSpeedFactor = 1.0;
		this->mPlbState = PB_STATE_PLAYING;
		if (params != 0) {
			this->mMP4Auxs.seeks(params);
		}
		if (!this->mPblCntlId) {
			mLoop = true;
			pthread_create(&this->mPblCntlId, NULL, Client::hdlSendSdSourceToPeer, this);
		}
	}
	break;

	case PB_CTL_RESUME: {
		mPlbState = PB_STATE_PLAYING;
	}
	break;

	case PB_CTL_PAUSE: {
		mPlbState = PB_STATE_PAUSING;
	}
	break;

	case PB_CTL_STOP: {
		mPlbState = PB_STATE_STOPPED;
	}
	break;

	case PB_CTL_SEEK: {
		if (this->mPlbState != PB_STATE_PAUSING) {
			this->mPlbState = PB_STATE_PLAYING;
		}
		rc = this->mMP4Auxs.seeks(params);
	}
	break;

	case PB_CTL_SPEED: {
		switch (params) {
		case PB_SPEED_SLOW05: this->mSpeedFactor = 0.5; 
		break;
		case PB_SPEED_NORMAL: this->mSpeedFactor = 1;
		break;
		case PB_SPEED_FASTx2: this->mSpeedFactor = 2;
		break;
		case PB_SPEED_FASTx4: this->mSpeedFactor = 4;
		break;
		case PB_SPEED_FASTx8: this->mSpeedFactor = 8;
		break;
		case PB_SPEED_FASTx16: this->mSpeedFactor = 16;
		break;
		default:
		break;
		}
	}
	break;
	
	default:
	break;
	}

	pthread_mutex_unlock(&mPOSIXMutex);

	if (cmd == PB_CTL_STOP && this->mPblCntlId) {
		mLoop = false;
		pthread_join(this->mPblCntlId, NULL);
		this->mPblCntlId = (pthread_t)NULL;
		this->mMP4Auxs.close();
	}

	return rc;
}

PLAYBACK_STATUS Client::getPlbSdStatus() {
	pthread_mutex_lock(&mPOSIXMutex);

	auto ret = mPlbState;

	pthread_mutex_unlock(&mPOSIXMutex);

	return ret;
}

std::pair<uint64_t, Stream::StreamSourceType> 
	Client::unsafePrepareForSample(uint64_t viSampleTime_us, uint64_t auSampleTime_us, uint64_t startTime) 
{
	uint64_t sampleTime_us = 0;
	Stream::StreamSourceType sst;

    if (auSampleTime_us < viSampleTime_us) {
		sst = Stream::StreamSourceType::Audio;
		sampleTime_us = auSampleTime_us;
	}
	else {
        sst = Stream::StreamSourceType::Video;
		sampleTime_us = viSampleTime_us;
    }

    auto elapsed = currentTimeInMicroSeconds() - startTime;
    if (sampleTime_us > elapsed) {
        auto waitTime = sampleTime_us - elapsed;
		pthread_mutex_unlock(&mPOSIXMutex);
        usleep(waitTime);
		pthread_mutex_lock(&mPOSIXMutex);
    }

	return {sampleTime_us, sst};
}

void *Client::hdlSendSdSourceToPeer(void *arg) {
	int elapsedSecs = 0;
	Client *me = static_cast<Client*>(arg);
	uint64_t startTime = currentTimeInMicroSeconds();

	struct {
		uint64_t sampleTime_us;
		uint64_t sampleDuration_us = VIDEO_PLAYBACK_DURATION_US;
	} videoTime;

	struct {
		uint64_t sampleTime_us;
		uint64_t sampleDuration_us = AUDIO_PLAYBACK_DURATION_US;
	} audioTime;

	videoTime.sampleTime_us = std::numeric_limits<uint64_t>::max() - videoTime.sampleDuration_us + 1;
	audioTime.sampleTime_us = std::numeric_limits<uint64_t>::max() - audioTime.sampleDuration_us + 1;
	videoTime.sampleTime_us += videoTime.sampleDuration_us;
	audioTime.sampleTime_us += audioTime.sampleDuration_us;

	while (me->mLoop) {
		pthread_mutex_lock(&me->mPOSIXMutex);

		auto [sampleTime, sst] = me->unsafePrepareForSample(videoTime.sampleTime_us, audioTime.sampleTime_us, startTime);
		auto trackData = (sst == Stream::StreamSourceType::Video) ? me->video.value() : me->audio.value();
		if (me->mPlbState != PB_STATE_PLAYING) {
			if (sst == Stream::StreamSourceType::Video) {
				videoTime.sampleTime_us += (videoTime.sampleDuration_us / me->mSpeedFactor);
			}
			else {
				audioTime.sampleTime_us += (audioTime.sampleDuration_us / me->mSpeedFactor);
			}
			pthread_mutex_unlock(&me->mPOSIXMutex);
			continue;
		}

		/* Load next samples and update samples time */
		rtc::binary sample = me->mMP4Auxs.readSamples(sst == Stream::StreamSourceType::Video);
		if (!sample.size()) {
			if (sst == Stream::StreamSourceType::Video) {
				videoTime.sampleTime_us += (videoTime.sampleDuration_us / me->mSpeedFactor);
			}
			else {
				audioTime.sampleTime_us += (audioTime.sampleDuration_us / me->mSpeedFactor);
			}
			pthread_mutex_unlock(&me->mPOSIXMutex);
			continue;
		}

		if (sst == Stream::StreamSourceType::Video) {
			videoTime.sampleDuration_us = 1000000 / me->mMP4Auxs.framePerSeconds();
			videoTime.sampleTime_us += (videoTime.sampleDuration_us / me->mSpeedFactor);
		}
		else {
			audioTime.sampleDuration_us = (1000000 * sample.size()) / AUDIO_SAMPLERATE;
			audioTime.sampleTime_us += (audioTime.sampleDuration_us / me->mSpeedFactor);
		}

		try {
			/* REPORT PLAYBACK: Timestamp elapsed in seconds */
			if (sst == Stream::StreamSourceType::Video) {
				if (elapsedSecs != me->mMP4Auxs.elapsedSeconds()) {
					elapsedSecs = me->mMP4Auxs.elapsedSeconds();
					nlohmann::json js = {
						{"Id"			,	fca_getSerialInfo()	},
						{"Type"			,	"Report"			},
						{"Command"		,	"Playback"			},
						{"SessionId"	,	me->getSessionId()	},
						{"SequenceId"	,	me->getSequenceId()	},
						{"Content"	,	
							{
								{"SeekPos"	, elapsedSecs	},
								{"Status"	, me->mPlbState	},
							}
						},
					};
					auto dc = me->dataChannel.value();
					dc->send(js.dump());
					printf("FPS: %d, elapsed seconds: %d\r\n", me->mMP4Auxs.framePerSeconds(), elapsedSecs);
				}
			}
			
			trackData->track->sendFrame(sample, std::chrono::duration<double, std::micro>(sampleTime));
		}
		catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	}
	return NULL;
}
