#ifndef _RECORDER_H_
#define _RECORDER_H_

#include <string>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <functional>

#include "rtc/rtc.hpp"
#include "mp4v2_mp4.h"

typedef enum {
	FILE_RECORD_TYPE_247 = 1,
	FILE_RECORD_TYPE_MOTION_DETECTED = 2,
	FILE_RECORD_TYPE_HUMAN_DETECTED = 3,

	/* More type here ... (1 << n) */
	FILE_RECORD_TYPE_INIT = 0xFFFF
} RECORDER_FILE_TYPE;

typedef struct {
	uint32_t beginTS;
	uint32_t endTS;
	uint16_t offsetSeconds;
	uint8_t type;
} RECORDER_METADATA_S;

typedef struct {
	uint8_t ind;
	RECORDER_METADATA_S events[5];
} RECORDER_LIST_METADATA_S;

typedef struct {
	char name[32];
	uint32_t startTs;
	uint32_t closeTs;
	RECORDER_LIST_METADATA_S metadata;
} RECORDER_INDEX_S;

#define RECORDER_NAME_TEMPORARY	".%u.mp4"
#define RECORDER_NAME_COMPLETED	"%Y-%m-%dT%H.%M.%S.mp4"

class MP4v2Writer {
public:
	enum class Mode {
		RecordingContinuous = 0,
		RecordingOnlyEvents,
		DisableFeatureRecord,
	};

	 MP4v2Writer();
	~MP4v2Writer();

	int initialise(
		int w, 
		int h, 
		int fps, 
		int samplerate);

	int begin(
		const char *location, 
		uint32_t createTs);

	bool isOpening(void);

	void setPromiseSafeClosure(void);

	bool getPromiseSafeClosure(void);
	
	int close(void);

	void addMetadata(
		uint8_t type, 
		uint32_t beginTS, 
		uint32_t endTS);

	int addVideoFrame(
		uint8_t *frame, 
		uint32_t frameSize, 
		int framePerSeconds, 
		uint64_t micros, 
		MP4V2_MP4_MediaType frameType);

	int addAudioFrame(
		uint8_t *frame, 
		uint32_t frameSize,
		uint64_t micros, 
		MP4V2_MP4_MediaType frameType);

	void onOpened(std::function<void(const char*)> callback);
	void onFailed(std::function<void(const char*)> callback);
	void onClosed(std::function<void(RECORDER_INDEX_S*)> callback);

private:
	std::function<void(const char*)> mImplOpened;
	std::function<void(const char*)> mImplFailed;
	std::function<void(RECORDER_INDEX_S*)> mImplClosed;

	char mName[32];
	char mLocation[32];
	uint32_t mCreateTs;

	int mWidth;
	int mHeight;
	int mSamplerate;
	int mFrameperSeconds;
	MP4V2_MP4_T *mMP4V2 = NULL;
	pthread_mutex_t mGeneralMutex;
	bool mPromiseSafeClosure = false;
	RECORDER_LIST_METADATA_S mListMetadata;
};

class MP4v2Reader {
public:
	 MP4v2Reader();
	~MP4v2Reader();

	int begin(const char *filename);
	int seeks(int seconds);
	int close(void);
	int resetParams();
	rtc::binary readSamples(bool isVideo);

	int elapsedSeconds();
	int framePerSeconds();

private:
	rtc::binary readOneSample(bool isVideo, MP4Timestamp *samplesTime);
	rtc::binary loadNextSamples_PpsIdr(void);
	rtc::binary loadNextSamples_SpsPpsIdr(void);

private:
	int mFps;
	std::string mFilename;
	MP4FileHandle mReader;
	MP4TrackId mVideoTrackId;
	MP4TrackId mAudioTrackId;
	MP4SampleId mVideoSampleId; /* Video samples index */
	MP4SampleId mAudioSampleId; /* Audio samples index */
	MP4SampleId mVideoTotalSamples; /* mVideoSampleId MUST BE less than mVideoTotalSamples */
	MP4SampleId mAudioTotalSamples; /* mAudioSampleId MUST BE less than mAudioTotalSamples */
	MP4Timestamp mSamplesTime;

	MP4V2_MP4_MediaType mVideoType;
	MP4V2_MP4_MediaType mAudioType;
};

#endif /* _RECORDER_H_ */
