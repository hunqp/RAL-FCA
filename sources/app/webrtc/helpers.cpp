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
static rtc::binary prefeatchOneNalu(std::string pathStr, uint32_t *cursor);

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
	
	setPlbSdControl(PB_CTL_STOP, NULL);
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
void Client::setPlbSdSource(PlbSdSource pbSdSrc) {
	pthread_mutex_lock(&mPOSIXMutex);

	mPbSdSource = pbSdSrc;
	this->mPbSdSource.startTime = currentTimeInMicroSeconds();

	pthread_mutex_unlock(&mPOSIXMutex);
}

void Client::setPlbSdControl(PLAYBACK_CONTROL ctl, uint32_t *argv) {
	pthread_mutex_lock(&mPOSIXMutex);

	switch (ctl) {
	case PB_CTL_PLAY: {
		mPbState = PB_STATE_PLAYING;
		if (!mPblCntlId) {
			mLoop = true;
			pthread_create(&mPblCntlId, NULL, Client::hdlSendSdSourceToPeer, this);
			APP_DBG_RTC("Playback started for client: %s\n", mId.c_str());
		}
	}
	break;

	case PB_CTL_PAUSE: {
		mPbState = PB_STATE_PAUSING;
	}
	break;

	case PB_CTL_STOP: {
		mPbState = PB_STATE_STOPPED;
		mPbSdSource.reset();

		mLoop = false;
		if (mPblCntlId) {
			pthread_mutex_unlock(&mPOSIXMutex);
			APP_DBG_RTC("Waiting for playback thread to join...");
			pthread_join(mPblCntlId, NULL);
			APP_DBG_RTC("Playback thread joined.");
			pthread_mutex_lock(&mPOSIXMutex);
			mPblCntlId = (pthread_t)NULL;
		}
	}
	break;

	case PB_CTL_RESUME: {
		mPbState = PB_STATE_PLAYING;
	}
	break;

	case PB_CTL_SEEK: {
		if (*argv < mPbSdSource.durationInSecs) {
			mPbSdSource.setCursorPos(*argv);
		}
	}
	break;

	case PB_CTL_SPEED: {
		mPbSdSource.cntlSpeed(*argv);
	}
	break;
	
	default:
	break;
	}

	pthread_mutex_unlock(&mPOSIXMutex);
}

PLAYBACK_STATUS Client::getPlbSdStatus() {
	pthread_mutex_lock(&mPOSIXMutex);

	auto ret = mPbState;
	if (mPbSdSource.audio.fileSize > 0 && mPbSdSource.video.fileSize > 0) {
		if (mPbSdSource.audio.cursor >= mPbSdSource.audio.fileSize) {
			ret = PB_STATE_DONE;
		}
	}

	pthread_mutex_unlock(&mPOSIXMutex);

	return ret;
}

uint32_t Client::getPlbTimeSpentInSecs() {
	pthread_mutex_lock(&mPOSIXMutex);

	auto ret = (uint32_t)(((double)mPbSdSource.video.cursor / mPbSdSource.video.fileSize) * mPbSdSource.durationInSecs);
	
	pthread_mutex_unlock(&mPOSIXMutex);

	return ret;
}

rtc::binary Client::getPlbSdSample(bool bVideo) {
	pthread_mutex_lock(&mPOSIXMutex);

	auto sample = (bVideo) ? mPbSdSource.prefetchSrcVideoSamples() : mPbSdSource.prefetchSrcAudioSamples();

	pthread_mutex_unlock(&mPOSIXMutex);

	return sample;
}

// FUNCTION LOAD SAMPLE FORM SD CARD
static rtc::binary prefetchFrom(std::string pathStr, uint32_t size, uint32_t cursor) {
	int fd		  = -1;
	uint8_t *data = NULL;
	rtc::binary sample;

	fd = open(pathStr.c_str(), O_RDONLY);
	if (fd < 0) {
		return {};
	}

	data = (uint8_t *)malloc(size * sizeof(uint8_t));
	if (data == NULL) {
		close(fd);
		return {};
	}

	int read = pread(fd, data, size, cursor);
	if (read) {
		sample.insert(sample.end(), reinterpret_cast<std::byte *>(data), reinterpret_cast<std::byte *>(data) + read);
	}

	close(fd);
	free(data);

	return sample;
}

rtc::binary prefeatchOneNalu(std::string pathStr, uint32_t *cursor) {
    #define STAGE_NALU_INITIAL      0
    #define STAGE_NALU_STORAGE      1
    #define STAGE_NALU_COMPLETED    2

    int stage = STAGE_NALU_INITIAL;
    bool naluCompleted = false;

    rtc::binary nalu;

    while (!naluCompleted) {
        auto buffer = prefetchFrom(pathStr, 4096, *cursor);
        if (buffer.empty()) {
            break;
        }

        if (buffer.size() <= 4) {
            nalu.insert(nalu.end(), 
                        reinterpret_cast<std::byte*>(buffer.data()), 
                        reinterpret_cast<std::byte*>(buffer.data()) + buffer.size());
            cursor += buffer.size();
            break;
        }

        for (size_t id = 0; id < buffer.size(); ++id) {
            uint8_t *p = (uint8_t*)buffer.data() + id;
            if (IS_NALU4_START(p)) {
                ++(stage);
            }
            
            if (stage == STAGE_NALU_INITIAL) {
                ++(*cursor);
            }
            else if (stage == STAGE_NALU_STORAGE) {
                nalu.emplace_back(static_cast<std::byte>(buffer[id]));
                ++(*cursor);
            }
            else if (stage == STAGE_NALU_COMPLETED) {
                naluCompleted = true;
                break;
            }
        }
    }
    
    return nalu;
}

void PlbSdSource::reset() {
	video.cursor = 0;
	video.fileSize = 0;
	video.pathStr.clear();
	audio.cursor = 0;
	audio.fileSize = 0;
	audio.pathStr.clear();
	startTime = 0;
}

rtc::binary PlbSdSource::prefetchSrcVideoSamples() {
	rtc::binary sample = {};

	if (video.pathStr.empty() || video.fileSize == 0) {
		return {};
	}

		rtc::binary nalu = prefeatchOneNalu(video.pathStr, &video.cursor);
		if (nalu.size() > 4) {
			rtc::NalUnitHeader *header = (rtc::NalUnitHeader *)(nalu.data() + 4);
			if (header->unitType() == AVCNALUnitType::AVC_NAL_SPS) {
				auto pps = prefeatchOneNalu(video.pathStr, &video.cursor);
				auto idr = prefeatchOneNalu(video.pathStr, &video.cursor);
				nalu.insert(nalu.end(), pps.begin(), pps.end());
				nalu.insert(nalu.end(), idr.begin(), idr.end());
			}
			sample.insert(sample.end(), nalu.begin(), nalu.end());
		}

	video.sampleTime_us += video.sampleDuration_us;

	return sample;
}

rtc::binary PlbSdSource::prefetchSrcAudioSamples() {
	rtc::binary sample = {};

	if (audio.pathStr.empty() || audio.fileSize == 0) {
		return {};
	}

	sample = prefetchFrom(audio.pathStr, AUDIO_PLAYBACK_SAMPLE_SIZE, audio.cursor);
	if (sample.size()) {
		audio.cursor += sample.size();
	}

	audio.sampleTime_us += audio.sampleDuration_us;

	return sample;
}

bool PlbSdSource::loadattributes(std::string viPathStr, std::string auPathStr, std::string descStr) {
    video.pathStr = viPathStr + std::string("/") + descStr + RECORD_VID_SUFFIX;
    audio.pathStr = auPathStr + std::string("/") + descStr + RECORD_AUD_SUFFIX;
	
    video.cursor = 0;
    audio.cursor = 0;

    /* Get duration */
    struct tm tim;
    uint32_t begTs = 0, endTs = 0;
    char sfx[5], typeStr[4];
    memset(sfx, 0, sizeof(sfx));
    memset(typeStr, 0, sizeof(typeStr));
    sscanf(descStr.c_str(), RECORD_DESC_PARSER_FORMATTER,
            &tim.tm_year, &tim.tm_mon, &tim.tm_mday, &tim.tm_hour, &tim.tm_min, &tim.tm_sec, 
            &begTs, &endTs, typeStr, sfx);
    
    durationInSecs = endTs - begTs;

	video.fileSize = sizeOfFile(video.pathStr.c_str());
	audio.fileSize = sizeOfFile(audio.pathStr.c_str());

	video.sampleDuration_us = VIDEO_PLAYBACK_DURATION_US;
	audio.sampleDuration_us = AUDIO_PLAYBACK_DURATION_US;
	video.sampleTime_us = std::numeric_limits<uint64_t>::max() - video.sampleDuration_us + 1;
	audio.sampleTime_us = std::numeric_limits<uint64_t>::max() - audio.sampleDuration_us + 1;
	video.sampleTime_us += video.sampleDuration_us;
	audio.sampleTime_us += audio.sampleDuration_us;

	return (video.fileSize != 0 && audio.fileSize != 0) ? true : false;
}

void PlbSdSource::cntlSpeed(int wantedSpeed) {
	switch (wantedSpeed) {
		case PB_SPEED_SLOW05: {
			video.sampleDuration_us = VIDEO_PLAYBACK_DURATION_US * 2;
			audio.sampleDuration_us = AUDIO_PLAYBACK_DURATION_US * 2;
		}
		break;
	
		case PB_SPEED_NORMAL: {
			video.sampleDuration_us = VIDEO_PLAYBACK_DURATION_US;
			audio.sampleDuration_us = AUDIO_PLAYBACK_DURATION_US;
		}
		break;
	
		case PB_SPEED_FASTx2: {
			video.sampleDuration_us = VIDEO_PLAYBACK_DURATION_US / 2;
			audio.sampleDuration_us = AUDIO_PLAYBACK_DURATION_US / 2;
		}
		break;
	
		case PB_SPEED_FASTx4: {
			video.sampleDuration_us = VIDEO_PLAYBACK_DURATION_US / 4;
			audio.sampleDuration_us = AUDIO_PLAYBACK_DURATION_US / 4;
		}
		break;
	
		case PB_SPEED_FASTx8: {
			video.sampleDuration_us = VIDEO_PLAYBACK_DURATION_US / 8;
			audio.sampleDuration_us = AUDIO_PLAYBACK_DURATION_US / 8;
		}
		break;
	
		case PB_SPEED_FASTx16: {
			video.sampleDuration_us = VIDEO_PLAYBACK_DURATION_US / 16;
			audio.sampleDuration_us = AUDIO_PLAYBACK_DURATION_US / 16;
		}
		break;
		
		default:
		break;
		}
}

void PlbSdSource::setCursorPos(int wantedSecs) {
	video.cursor = (wantedSecs / 2) * (video.fileSize / (durationInSecs / 2));

	do {
		rtc::binary nalu = prefeatchOneNalu(video.pathStr, &video.cursor);
		if (nalu.size() > 4) {
			rtc::NalUnitHeader *header = (rtc::NalUnitHeader *)(nalu.data() + 4);
			if (header->unitType() == AVCNALUnitType::AVC_NAL_SPS) {
				break;
			}
		}
		else break;
	}
	while (1);

	int percent = video.cursor * 100 / video.fileSize;
	audio.cursor = percent * audio.fileSize / 100;
}

static std::pair<uint64_t, Stream::StreamSourceType> unsafePrepareForSample(PlbSdSource *plbCntlPtr) {
	uint64_t nextTime;
	uint64_t sampleTime_us = 0;
	Stream::StreamSourceType sst;

    if (plbCntlPtr->audio.sampleTime_us < plbCntlPtr->video.sampleTime_us) {
		sst = Stream::StreamSourceType::Audio;
		nextTime = sampleTime_us = plbCntlPtr->audio.sampleTime_us;
	}
	else {
        sst = Stream::StreamSourceType::Video;
		nextTime = sampleTime_us = plbCntlPtr->video.sampleTime_us;
    }

    auto currentTime = currentTimeInMicroSeconds();

    auto elapsed = currentTime - plbCntlPtr->startTime;
    if (nextTime > elapsed) {
        auto waitTime = nextTime - elapsed;
        usleep(waitTime);
    }

	return {sampleTime_us, sst};
}

void *Client::hdlSendSdSourceToPeer(void *arg) {
	Client *clientPtr = static_cast<Client*>(arg);

	while (clientPtr->mLoop) {
		if (clientPtr->getMediaStreamOptions() != Client::eOptions::Playback) {
			sleep(1);
			continue;
		}

		auto ssSST = unsafePrepareForSample(&clientPtr->mPbSdSource);
		auto sst = ssSST.second;
		auto sampleTime = ssSST.first;

		bool oneSecsElapsed = false;
		auto trackData = (sst == Stream::StreamSourceType::Video) ? clientPtr->video.value() : clientPtr->audio.value();

		auto rtpConfig = trackData->sender->rtpConfig;
		auto elapsedSeconds	= double(sampleTime) / (1000.0 * 1000);
		uint32_t elapsedTimestamp = rtpConfig->secondsToTimestamp(elapsedSeconds);
		rtpConfig->timestamp = rtpConfig->startTimestamp + elapsedTimestamp;
		auto reportElapsedTimestamp = rtpConfig->timestamp - trackData->sender->lastReportedTimestamp();
		if (rtpConfig->timestampToSeconds(reportElapsedTimestamp) > 1) {
			trackData->sender->setNeedsToReport();
			oneSecsElapsed = true;
		}

		if (clientPtr->getPlbSdStatus() != PB_STATE_PLAYING) {
			/* Balance sample timestamp */
			if (sst == Stream::StreamSourceType::Video) clientPtr->mPbSdSource.video.sampleTime_us += clientPtr->mPbSdSource.video.sampleDuration_us;
			else clientPtr->mPbSdSource.audio.sampleTime_us += clientPtr->mPbSdSource.audio.sampleDuration_us;
		
			continue;
		}

		auto sample = clientPtr->getPlbSdSample(sst == Stream::StreamSourceType::Video);

		try {
			/* REPORT PLAYBACK: Timestamp elapsed in seconds */
			if (sst == Stream::StreamSourceType::Video && oneSecsElapsed) {
				nlohmann::json JS = {
					{"Id"			,	fca_getSerialInfo()			},
					{"Type"			,	"Report"					},
					{"Command"		,	"Playback"					},
					{"SessionId"	,	clientPtr->getSessionId()	},
					{"SequenceId"	,	clientPtr->getSequenceId()	},
					{"Content"	,	
						{
							{"SeekPos"	, clientPtr->getPlbTimeSpentInSecs()},
							{"Status"	, clientPtr->getPlbSdStatus()		},
						}
					},
				};
				auto dc = clientPtr->dataChannel.value();
				dc->send(JS.dump());
			}
			
			if (sample.size()) {
				trackData->track->send(sample);
			}
		}
		catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	}
	return NULL;
}
