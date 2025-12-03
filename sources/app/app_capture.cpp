#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_capture.h"
#include "videosource.hpp"
#include "audiosource.hpp"

static bool volatile bLoop = false;

static void *video0ListenSampleCalls(void *);
static void *video1ListenSampleCalls(void *);
static void *audio0ListenSampleCalls(void *);
static void  mediaLiveviewSendToPeer(Client::eResolution res, bool bIdr, rtc::binary &sample, uint64_t sampleDurationUs, Stream::StreamSourceType sst);

/* MUST BE called after avStream created */
void startListSocketListeners(void) {
	bLoop = true;
	pthread_t video0, video1, audio0;
	pthread_create(&video0, NULL, video0ListenSampleCalls, NULL);
	pthread_create(&video1, NULL, video1ListenSampleCalls, NULL);
	pthread_create(&audio0, NULL, audio0ListenSampleCalls, NULL);
}

void closeAllSocketListeners(void) {
	bLoop = false;
}

void *video0ListenSampleCalls(void *arg) {
	auto stream = avStream.value();
	auto viSrcs = dynamic_cast<VideoSource*>(stream->video0.get());

	while (bLoop) {
		viSrcs->loadNextSample();
		rtc::binary sample = viSrcs->getSample();
		bool bIdr = viSrcs->isSpsPpsIdrOrVpsSpsPpsIdr();
		uint64_t sampleTime = viSrcs->getSampleTime_us();

		if (sample.size() <= 0) {
			usleep(10000);
			continue;
		}

		// printf("Capture video 0 samples size %d\r\n", sample.size());

		/* -------------------------// P2P STREAM // -------------------------*/
		mediaLiveviewSendToPeer(Client::eResolution::FullHD1080p, bIdr, sample, sampleTime, Stream::StreamSourceType::Video);
	}

	pthread_detach(pthread_self());
	return NULL;
}

void *video1ListenSampleCalls(void *arg) {
	auto stream = avStream.value();
	auto viSrcs = dynamic_cast<VideoSource*>(stream->video1.get());

	while (bLoop) {
		viSrcs->loadNextSample();
		auto sample = viSrcs->getSample();
		bool bIdr = viSrcs->isSpsPpsIdrOrVpsSpsPpsIdr();
		uint64_t sampleTime = viSrcs->getSampleTime_us();

		if (sample.size() <= 0) {
			usleep(10000);
			continue;
		}

		// printf("Capture video 1 samples size %d\r\n", sample.size());

		/* -------------------------// P2P STREAM // -------------------------*/
		mediaLiveviewSendToPeer(Client::eResolution::HD720p, bIdr, sample, sampleTime, Stream::StreamSourceType::Video);
	}

	pthread_detach(pthread_self());
	return NULL;
}

void *audio0ListenSampleCalls(void *arg) {
	auto stream = avStream.value();
	auto auSrcs = stream->audio0;

	while (bLoop) {
		auSrcs->loadNextSample();
		rtc::binary sample = auSrcs->getSample();
		uint64_t sampleTime = auSrcs->getSampleTime_us();

		if (sample.size() <= 0) {
			usleep(10000);
			continue;
		}

		// printf("Capture audio 0 samples size %d\r\n", sample.size());

		/* -------------------------// P2P STREAM // -------------------------*/
		mediaLiveviewSendToPeer(Client::eResolution::HD720p, false, sample, sampleTime, Stream::StreamSourceType::Audio);
	}

	pthread_detach(pthread_self());
	return NULL;
}

void mediaLiveviewSendToPeer(Client::eResolution res, bool bIdr, rtc::binary &sample, uint64_t sampleTime, Stream::StreamSourceType sst) {
	if (clients.empty()) {
		return;
	}

	pthread_mutex_lock(&Client::mtxClientsProtect);

	string streamType = (sst == Stream::StreamSourceType::Video) ? "video" : "audio";

	for (auto it : clients) {
		auto id = it.first;
		auto client = it.second;
		auto optTrackData = (sst == Stream::StreamSourceType::Video) ? client->video : client->audio;

		/* Prerequisite condition validation */
		if (client->getState() == Client::State::Ready && optTrackData.has_value()) {
			if (sst == Stream::StreamSourceType::Video) {
				if (client->getLiveResolution() != res) {
					continue;
				}
				if (bIdr && client->getMediaStreamOptions() == Client::eOptions::Pending) {
					client->setMediaStreamOptions(Client::eOptions::LiveStream);
				}
			}

			/* LIVEVIEW: SEND SAMPLES */
			if (client->getMediaStreamOptions() == Client::eOptions::LiveStream) {
				auto trackData = optTrackData.value();
				try {
                    trackData->track->sendFrame(sample, std::chrono::duration<double, std::micro>(sampleTime));
					if (sst == Stream::StreamSourceType::Video) {
						// printf("Channel %d send video sample size %d\r\n", (int)res, sample.size());
					}
                }
				catch (const std::exception &e) {
                    cerr << "Unable to send "<< streamType << " packet: " << e.what() << endl;
                }
			}
		}
	}

	pthread_mutex_unlock(&Client::mtxClientsProtect);
}
