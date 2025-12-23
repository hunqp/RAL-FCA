#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>

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
	VV_RB_MEDIA_FRAMED_S Frame;

	sample = {};
	rc = ringBufferReadFrom(consumer, &Frame);

	if (rc == 0) {
		splitNalus(Frame.pData, Frame.dataLen);
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

void VideoSource::appendSample(uint8_t *buffer, size_t length) {
	uint32_t nalu_nlen = htonl((uint32_t)length);
	sample.insert(sample.end(), reinterpret_cast<std::byte *>(&nalu_nlen), reinterpret_cast<std::byte *>(&nalu_nlen) + sizeof(nalu_nlen));
	sample.insert(sample.end(), reinterpret_cast<std::byte *>(buffer), reinterpret_cast<std::byte *>(buffer) + length);
}

void VideoSource::splitNalus(uint8_t *buffer, size_t length) {
	uint8_t *found = NULL;
	uint8_t *begin = buffer;
	size_t remain_length = length;
	const uint8_t zero_sequence_4[4] = {0, 0, 0, 1};

	found = (uint8_t *)memmem(begin, remain_length, zero_sequence_4, sizeof(zero_sequence_4));
	while (found) {
		int nalu_hlen = found - begin;
		if (nalu_hlen) {
			appendSample(begin, nalu_hlen);
		}
		begin = found + sizeof(zero_sequence_4);
		remain_length -= (nalu_hlen + sizeof(zero_sequence_4));
		found = (uint8_t *)memmem(begin, remain_length, zero_sequence_4, sizeof(zero_sequence_4));
	}
	if (remain_length) {
		appendSample(begin, remain_length);
	}
}