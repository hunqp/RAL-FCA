#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>

#include "videosource.hpp"

VideoSource::VideoSource(VV_RB_MEDIA_HANDLE_T producer, uint64_t sampleDurationUs) : StreamSource(producer, 250 * 1024) {
	sampleDuration_us = sampleDurationUs;
}

VideoSource::~VideoSource() {
	stop();
}

void VideoSource::start() {
	sampleTime_us = std::numeric_limits<uint64_t>::max() - sampleDuration_us + 1;
	sampleTime_us += sampleDuration_us;
}

void VideoSource::stop() {
	sample = {};
	sampleTime_us = 0;
}

void VideoSource::loadNextSample() {
	int rc = -1;
	VV_RB_MEDIA_RESOURCE_S Frame;

	sample = {};
	rc = ringBufferReadFrom(consumer, &Frame);

	if (rc == 0) {
		sample.insert(sample.end(), reinterpret_cast<std::byte*>(Frame.pData), reinterpret_cast<std::byte*>(Frame.pData) + Frame.dataLen);
		sampleTime_us += (1000000 / Frame.framePerSeconds);

		if (Frame.type == VV_RB_MEDIA_FRAME_HDR_TYPE_I) {
			mInitialNalus = sample;
			mSpsPpsIdrOrVpsSpsPpsIdr = true;
		}
		else {
			mSpsPpsIdrOrVpsSpsPpsIdr = false;
		}
		mIsEncoderH264 = (Frame.encoder == VV_RB_MEDIA_ENCODER_FRAME_H264) ? true : false;
	}
}

bool VideoSource::isEncoderH264() {
	return mIsEncoderH264;
}

bool VideoSource::isSpsPpsIdrOrVpsSpsPpsIdr() {
	return mSpsPpsIdrOrVpsSpsPpsIdr;
}

rtc::binary VideoSource::getInitialNalus() {
	return mInitialNalus;
}