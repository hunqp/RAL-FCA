#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "app.h"
#include "fca_video.hpp"

struct RESOLUTION_MAP_S {
	int width;
	int height;
};

static uint32_t chnls0_FpsSelected;
static uint32_t chnls1_FpsSelected;
static VV_RB_MEDIA_ENCODE_TYPE chnls0_EncodeSelected;
static VV_RB_MEDIA_ENCODE_TYPE chnls1_EncodeSelected;

VV_RB_MEDIA_HANDLE_T VideoHelpers::VIDEO0_PRODUCER = NULL;
VV_RB_MEDIA_HANDLE_T VideoHelpers::VIDEO1_PRODUCER = NULL;

struct RESOLUTION_MAP_S dimension[] = {
	[FCA_VIDEO_DIMENSION_VGA	] = { 640 , 360  },
	[FCA_VIDEO_DIMENSION_720P	] = { 1280, 720  },
	[FCA_VIDEO_DIMENSION_1080P	] = { 1920, 1080 },
	[FCA_VIDEO_DIMENSION_2K		] = { 2304, 1296 }
};

VideoHelpers::VideoHelpers() {
	VideoHelpers::VIDEO0_PRODUCER = ringBufferCreateProducer(2 * 1024 * 1024);
	VideoHelpers::VIDEO1_PRODUCER = ringBufferCreateProducer(1 * 1024 * 1024);
}

VideoHelpers::~VideoHelpers() {
	ringBufferDelete(VideoHelpers::VIDEO0_PRODUCER);
	ringBufferDelete(VideoHelpers::VIDEO1_PRODUCER);
}

uint64_t nowInUs() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)(tv.tv_sec) * 1000000ULL + (uint64_t)(tv.tv_usec);
}

static void captureStream0Channels(const fca_video_in_frame_t *Frame) {
	VV_RB_MEDIA_RESOURCE_S resource = {
		.id = Frame->sequence,
		.pData = Frame->data,
		.dataLen = Frame->size,
		.timestamp = nowInUs(),
		.framePerSeconds = chnls0_FpsSelected,
		.type = (Frame->type == FCA_VIDEO_IN_FRAME_TYPE_I) ? VV_RB_MEDIA_FRAME_HDR_TYPE_I : VV_RB_MEDIA_FRAME_HDR_TYPE_PB,
		.encoder = chnls0_EncodeSelected
	};
	ringBufferSendTo(VideoHelpers::VIDEO0_PRODUCER, &resource);
}

static void captureStream1Channels(const fca_video_in_frame_t *Frame) {
	VV_RB_MEDIA_RESOURCE_S resource = {
		.id = Frame->sequence,
		.pData = Frame->data,
		.dataLen = Frame->size,
		.timestamp = nowInUs(),
		.framePerSeconds = chnls1_FpsSelected,
		.type = (Frame->type == FCA_VIDEO_IN_FRAME_TYPE_I) ? VV_RB_MEDIA_FRAME_HDR_TYPE_I : VV_RB_MEDIA_FRAME_HDR_TYPE_PB,
		.encoder = chnls1_EncodeSelected
	};
	ringBufferSendTo(VideoHelpers::VIDEO1_PRODUCER, &resource);
}

void VideoHelpers::initialise() {
	FCA_API_ASSERT(fca_video_in_init() == 0);
}

void VideoHelpers::deinitialise(void) {
	FCA_API_ASSERT(fca_video_in_uninit() == 0);
}

void VideoHelpers::startStreamChannels(void) {
    FCA_API_ASSERT(fca_video_in_start_callback(0, captureStream0Channels) == 0);
    FCA_API_ASSERT(fca_video_in_start_callback(1, captureStream1Channels) == 0);
}

void VideoHelpers::closeStreamChannels(void) {
	FCA_API_ASSERT(fca_video_in_stop_callback(0) == 0);
    FCA_API_ASSERT(fca_video_in_stop_callback(1) == 0);
}

bool VideoHelpers::setEncoders(FCA_ENCODE_S *encoders) {
	if (!encoders) {
		return false;
	}

	if (!validEncoders(encoders)) {
		return false;
	}
	memcpy(&mEncoders, encoders, sizeof(FCA_ENCODE_S));

	fca_video_in_config_t config;

	/* Main stream */    
    config.codec = mEncoders.mainStream.codec;
    config.encoding_mode = mEncoders.mainStream.rcmode;
    config.gop = mEncoders.mainStream.gop;
    config.fps = mEncoders.mainStream.fps;
    config.width = dimension[mEncoders.mainStream.resolution].width;
    config.height = dimension[mEncoders.mainStream.resolution].height;
    config.max_qp = 28;
    config.min_qp = 42;
	config.bitrate_target = mEncoders.mainStream.bitrate;
	if (config.encoding_mode == FCA_VIDEO_ENCODING_MODE_CBR) {
		config.bitrate_max = mEncoders.mainStream.bitrate * 4 / 3;
	}
	else {
		config.bitrate_max = mEncoders.mainStream.bitrate;
	}
    FCA_API_ASSERT(fca_video_in_set_config(0, &config) == 0);

	/* Minor stream */
    config.codec = mEncoders.minorStream.codec;
    config.encoding_mode = mEncoders.minorStream.rcmode;
    config.gop = mEncoders.minorStream.gop;
    config.fps = mEncoders.minorStream.fps;
    config.width = dimension[mEncoders.minorStream.resolution].width;
    config.height = dimension[mEncoders.minorStream.resolution].height;
    config.max_qp = 28;
    config.min_qp = 42;
	config.bitrate_target = mEncoders.minorStream.bitrate;
	if (config.encoding_mode == FCA_VIDEO_ENCODING_MODE_CBR) {
		config.bitrate_max = mEncoders.minorStream.bitrate * 4 / 3;
	}
	else {
		config.bitrate_max = mEncoders.minorStream.bitrate;
	}
    FCA_API_ASSERT(fca_video_in_set_config(1, &config) == 0);

	/* Assignee frames value */
	chnls0_FpsSelected = mEncoders.mainStream.fps;
	chnls1_FpsSelected = mEncoders.minorStream.fps;
	chnls0_EncodeSelected = (mEncoders.mainStream.codec  == FCA_CODEC_VIDEO_H264) ? VV_RB_MEDIA_ENCODER_FRAME_H264 : VV_RB_MEDIA_ENCODER_FRAME_H265;
	chnls1_EncodeSelected = (mEncoders.minorStream.codec == FCA_CODEC_VIDEO_H264) ? VV_RB_MEDIA_ENCODER_FRAME_H264 : VV_RB_MEDIA_ENCODER_FRAME_H265;

	return true;
}

FCA_ENCODE_S VideoHelpers::getEncoders() {
	return mEncoders;
}

void VideoHelpers::dummy(void) {
	/* Test APIs */
	printf("\r\n- VIDEO ENCODERS -\r\n");
	for (uint8_t chnls = 0; chnls < VIDEON_CHANNEL_MAX; ++chnls) {
		fca_video_in_config_t config;
		FCA_API_ASSERT(fca_video_in_get_config(chnls, &config) == 0);
		printf("\tChannel   : %d\r\n", chnls);
		printf("\tResolution: %dx%d\r\n", config.width, config.height);
		printf("\tCodec type: %d\r\n", config.codec);
		printf("\tRC mode   : %d\r\n", config.encoding_mode);
		printf("\tFps       : %d\r\n", config.fps);
		printf("\tGop       : %d\r\n", config.gop);
		printf("\tQuality   : %d\r\n", config.max_qp);
		printf("\tBitrate   : %d\r\n", config.bitrate_target);
		printf("\r\n");
	}
}

bool VideoHelpers::setOsd(FCA_OSD_S *osds) {
	if (!osds) {
		return false;
	}

	// TODO 
	// fca_isp_set_osd_time(FCA_OSD_TOP_LEFT);
	// fca_isp_set_osd_text(FCA_OSD_BOTTOM_LEFT, "AAAaaaa123345");

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VideoHelpers::validEncoders(FCA_ENCODE_S *encoders) {
	for (int chnls = 0; chnls < VIDEON_CHANNEL_MAX; chnls++) {
		FCA_VIDEO_FORMAT_S *sel = (chnls == VIDEO0_CHANNEL_ID) ? &encoders->mainStream : &encoders->minorStream;

		if (chnls == VIDEO0_CHANNEL_ID && (sel->resolution >= FCA_VIDEO_DIMENSION_UNKNOWN || sel->resolution < FCA_CONFIG_NONE)) {
			return false;
		}
		if (chnls == VIDEO1_CHANNEL_ID && (sel->resolution > FCA_VIDEO_DIMENSION_720P || sel->resolution < FCA_CONFIG_NONE)) {
			return false;
		}
		if (sel->rcmode > FCA_VIDEO_ENCODING_MODE_AVBR || sel->rcmode < FCA_VIDEO_ENCODING_MODE_CBR) {
			return false;
		}
		if (sel->codec >= FCA_CODEC_VIDEO_MJPEG || sel->codec < FCA_CODEC_VIDEO_H264) {
			return false;
		}
		if (sel->bitrate > FCA_MAX_STREAM_BITRATE || sel->bitrate <= FCA_CONFIG_NONE) {
			return false;
		}
		if (sel->fps > FCA_MAX_STREAM_FPS || sel->fps <= FCA_CONFIG_NONE) {
			return false;
		}
		if (sel->gop > FCA_MAX_STREAM_GOP || sel->gop <= FCA_CONFIG_NONE) {
			return false;
		}
		if (sel->quality > FCA_MAX_STREAM_QP || sel->quality <= FCA_CONFIG_NONE) {
			return false;
		}
	}
	return true;
}