#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "recorder.h"

static std::string errorsToStrings(const char *fmt, ...) {
	char chars[128] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(chars, sizeof(chars), fmt, args);
    va_end(args);
	return std::string(chars, strlen(chars));
}

MP4v2Writer::MP4v2Writer() {
	mImplOpened = [](const char*) {};
	mImplFailed = [](const char*) {};
	mImplClosed = [](RECORDER_INDEX_S*) {};
	pthread_mutex_init(&mGeneralMutex, NULL);
	mMP4V2 = NULL;
}

MP4v2Writer::~MP4v2Writer() {
	pthread_mutex_destroy(&mGeneralMutex);
}

int MP4v2Writer::initialise(int w, int h, int fps, int samplerate) {
	mWidth = w;
	mHeight = h;
	mFrameperSeconds = fps;
	mSamplerate = samplerate;

	return 0;
}

int MP4v2Writer::begin(
	const char *location, 
	uint32_t createTs)
{
	int rc = 0;
	std::string FullPath;

	pthread_mutex_lock(&mGeneralMutex);

	if (mMP4V2) {
		FullPath = std::string(mLocation) + std::string("/") + std::string(mName);
		mImplFailed(errorsToStrings("Can't create because %s begin written", FullPath.c_str()).c_str());
		rc = -1;
	}
	else {
		mCreateTs = createTs;
		memset(mName, 0, sizeof(mName));
		memset(mLocation, 0, sizeof(mLocation));
		strncpy(mLocation, location, strlen(location));
		snprintf(mName, sizeof(mName), RECORDER_NAME_TEMPORARY, mCreateTs);
		FullPath = std::string(mLocation) + std::string("/") + std::string(mName);
		memset(&mListMetadata, 0, sizeof(RECORDER_LIST_METADATA_S));

		mMP4V2 = Mp4v2CreateMP4File(FullPath.c_str(), mWidth, mHeight, 90000, mFrameperSeconds, mSamplerate);
		if (!mMP4V2) {
			mImplFailed(errorsToStrings("Create %s has errors", FullPath.c_str()).c_str());
			rc = -1;
		}
		mPromiseSafeClosure = false;
	}

	pthread_mutex_unlock(&mGeneralMutex);

	if (rc == 0) {
		mImplOpened((const char*)mName);
	}

	return rc;
}

int MP4v2Writer::close(void) {	
	int rc = 1;
	char newName[32] = {0};
	uint32_t elapsedSeconds = 0;
	
	pthread_mutex_lock(&mGeneralMutex);
	
	if (mMP4V2) {
		uint32_t timescale = MP4GetTrackTimeScale(mMP4V2->hMp4File, mMP4V2->videoTrackId);
		MP4Duration duration = MP4GetTrackDuration(mMP4V2->hMp4File, mMP4V2->videoTrackId);
		if (timescale != 0 && duration != 0) {
			elapsedSeconds = duration / timescale;
		}
		else {
			rc = -2;
			mImplFailed(errorsToStrings("\'%s\' invalid timescale %llu, duration %llu\r\n",  mName, timescale, duration).c_str());
		}
		
		Mp4v2CloseMP4File(mMP4V2);
		mMP4V2 = NULL;
		if (elapsedSeconds > 0) {
			time_t t = (time_t)mCreateTs;
			struct tm *lc = localtime(&t);
			strftime(newName, sizeof(newName), RECORDER_NAME_COMPLETED, lc);
			std::string temporary = std::string(mLocation) + std::string("/") + std::string(mName);
			std::string completed = std::string(mLocation) + std::string("/") + std::string(newName);
			rc = rename(temporary.c_str(), completed.c_str());
			if (rc != 0) {
				mImplFailed(errorsToStrings("Rename \"%s\" to \"%s\" return %d, error %d {%s}\r\n", 
					mName, newName, rc, errno, strerror(errno)).c_str());
			}
		}
		else {
			rc = -3;
		}
	}

	pthread_mutex_unlock(&mGeneralMutex);

	if (rc == 0) {
		RECORDER_INDEX_S desc = {0};
		desc.startTs = mCreateTs;
		desc.closeTs = mCreateTs + elapsedSeconds;
		strncpy(desc.name, newName, sizeof(desc.name));
		memcpy(&desc.metadata, &mListMetadata, sizeof(RECORDER_LIST_METADATA_S));
		mImplClosed(&desc);
	}
	else if (rc != 1) {
		mImplFailed(errorsToStrings("Closes MP4 return %d", rc).c_str());
	}

	return rc;
}

bool MP4v2Writer::isOpening(void) {
	pthread_mutex_lock(&mGeneralMutex);

	bool ret = (mMP4V2 != NULL) ? true : false;

	pthread_mutex_unlock(&mGeneralMutex);

	return ret;
}

void MP4v2Writer::setPromiseSafeClosure() {
	pthread_mutex_lock(&mGeneralMutex);

	mPromiseSafeClosure = true;

	pthread_mutex_unlock(&mGeneralMutex);
}
	
bool MP4v2Writer::getPromiseSafeClosure(void) {
	pthread_mutex_lock(&mGeneralMutex);

	bool ret = mPromiseSafeClosure;

	pthread_mutex_unlock(&mGeneralMutex);

	return ret;
}

void MP4v2Writer::addMetadata(
	uint8_t type, 
	uint32_t beginTS, 
	uint32_t endTS)
{
	pthread_mutex_lock(&mGeneralMutex);

	if (!mMP4V2 || beginTS < mCreateTs) {
		pthread_mutex_unlock(&mGeneralMutex);
		return;
	}

	if (mListMetadata.ind < sizeof(mListMetadata.events) / sizeof(mListMetadata.events[0])) {
		mListMetadata.events[mListMetadata.ind].offsetSeconds = beginTS - mCreateTs;
		mListMetadata.events[mListMetadata.ind].beginTS = beginTS;
		mListMetadata.events[mListMetadata.ind].endTS = endTS;
		mListMetadata.events[mListMetadata.ind].type = type;
		++(mListMetadata.ind);
	}

	pthread_mutex_unlock(&mGeneralMutex);
}

int MP4v2Writer::addVideoFrame(
	uint8_t *frame, 
	uint32_t frameSize, 
	int framePerSeconds, 
	uint64_t micros, 
	MP4V2_MP4_MediaType frameType)
{
	pthread_mutex_lock(&mGeneralMutex);

	if (!mMP4V2) {
		pthread_mutex_unlock(&mGeneralMutex);
		return -1;
	}

	int rc = -1;
	uint32_t offset = 0;
	while (1) {
		MP4V2_NaluUnit nalueRead;
		int readBytes = MP4v2ReadOneNaluFromBuffers(frame, frameSize, offset, nalueRead);
		if (readBytes > 0) {
			if (frameType == MP4V2_MP4_VIDEO_H264) {
				rc = Mp4v2WriteH264toMP4(mMP4V2, &frame[offset], readBytes, framePerSeconds, micros);
			}
			else { 
				rc = Mp4v2WriteH265toMP4(mMP4V2, &frame[offset], readBytes, framePerSeconds, micros);
			}
			offset += readBytes; 
		}
		else break;
	}

	pthread_mutex_unlock(&mGeneralMutex);
	
	if (rc != 0) {
		mImplFailed(errorsToStrings("Write video type %d samples size %d failure", frameType, frameSize).c_str());
	}

	return rc;
}

int MP4v2Writer::addAudioFrame(
	uint8_t *frame, 
	uint32_t frameSize,
	uint64_t micros, 
	MP4V2_MP4_MediaType frameType)
{
	pthread_mutex_lock(&mGeneralMutex);

	if (!mMP4V2) {
		pthread_mutex_unlock(&mGeneralMutex);
		return -1;
	}

	int rc = -1;
	switch (frameType) {
	case MP4V2_MP4_AUDIO_ALAW: {
		rc = Mp4v2WriteALawToMP4(mMP4V2, frame, frameSize, micros);
	}
	break;

	case MP4V2_MP4_AUDIO_AAC: {
		/* Comming soon */
	}
	break;
	
	default:
	break;
	}

	pthread_mutex_unlock(&mGeneralMutex);

	if (rc != 0) {
		mImplFailed(errorsToStrings("Write audio type %d samples %d failure", frameType, frameSize).c_str());
	}

	return rc;
}

void MP4v2Writer::onOpened(std::function<void(const char*)> callback) {
	mImplOpened = callback;
}

void MP4v2Writer::onFailed(std::function<void(const char*)> callback) {
	mImplFailed = callback;
}

void MP4v2Writer::onClosed(std::function<void(RECORDER_INDEX_S*)> callback) {
	mImplClosed = callback;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
MP4v2Reader::MP4v2Reader() {
	mReader = NULL;
	mVideoTrackId = mAudioTrackId = MP4_INVALID_TRACK_ID;
	mVideoSampleId = mAudioSampleId = MP4_INVALID_SAMPLE_ID;
}

MP4v2Reader::~MP4v2Reader() {
	close();
}

int MP4v2Reader::begin(const char *filename) {
	/* We MUST close current file prepare for new instance */
	if (mFilename.length()) {
		close();
	}

	/* Start new instance */
	mReader = MP4Read(filename);
	if (!mReader) {
		return -1;
	}

	mVideoTrackId = MP4FindTrackId(mReader, 0, MP4_VIDEO_TRACK_TYPE);
	mAudioTrackId = MP4FindTrackId(mReader, 0, MP4_AUDIO_TRACK_TYPE);
	mVideoTotalSamples = (mVideoTrackId != MP4_INVALID_TRACK_ID) ? MP4GetTrackNumberOfSamples(mReader, mVideoTrackId) : 0;
    mAudioTotalSamples = (mAudioTrackId != MP4_INVALID_TRACK_ID) ? MP4GetTrackNumberOfSamples(mReader, mAudioTrackId) : 0;
	mVideoSampleId = 1;
	mAudioSampleId = 1;

	printf("Video name: %s\r\n", MP4GetTrackMediaDataName(mReader, mVideoTrackId));
	printf("Audio name: %s\r\n", MP4GetTrackMediaDataName(mReader, mAudioTrackId));
	// const char *name = MP4GetTrackMediaDataName(mReader, mVideoTrackId);
	// if (name && strcmp(name, "hvc1") == 0) {
	// 	mVideoType = MP4V2_MP4_VIDEO_H265;
	// }
	// else {
	// 	mVideoType = MP4V2_MP4_VIDEO_H264;
	// }
	// if (name && strcmp(name, "mp4a") == 0) {
	// 	mAudioType = MP4V2_MP4_AUDIO_AAC;
	// }
	// else {
	// 	mAudioType = MP4V2_MP4_AUDIO_ALAW;
	// }

	if (mVideoTotalSamples > 0 && mAudioTotalSamples > 0) {
		mFilename.assign(std::string(filename));
		return 0;
	}
	return -2;
}

int MP4v2Reader::seeks(int seconds) {
    if (!mReader || mVideoTrackId == MP4_INVALID_TRACK_ID || mAudioTrackId == MP4_INVALID_TRACK_ID) {
        return -1;
    }

	uint32_t timescale;
	MP4Timestamp target;

    /* For video track */
    timescale = MP4GetTrackTimeScale(mReader, mVideoTrackId);
    target = static_cast<MP4Timestamp>(static_cast<u_int64_t>(seconds) * timescale);
    mVideoSampleId = MP4GetSampleIdFromTime(mReader, mVideoTrackId, target, true);

    if (mVideoSampleId == MP4_INVALID_SAMPLE_ID) {
        mVideoSampleId = MP4GetTrackNumberOfSamples(mReader, mVideoTrackId);
    }

    /* For audio track */
	timescale = MP4GetTrackTimeScale(mReader, mAudioTrackId);
	target = static_cast<MP4Timestamp>(static_cast<u_int64_t>(seconds) * timescale);
	mAudioSampleId = MP4GetSampleIdFromTime(mReader, mAudioTrackId, target, false);
	if (mAudioSampleId == MP4_INVALID_SAMPLE_ID) {
		mAudioSampleId = MP4GetTrackNumberOfSamples(mReader, mAudioTrackId);
	}
    
    return 0;
}

int MP4v2Reader::close(void) {
	if (mReader) {
		MP4Close(mReader);
	}
	resetParams();
	return 0;	
}

rtc::binary MP4v2Reader::readOneSample(bool isVideo, MP4Timestamp *samplesTime) {
	bool boolean = false;
	rtc::binary sample {};
	uint8_t *buffers = NULL;
	uint32_t bufferSize = 0;

	if (isVideo) {
		boolean = MP4ReadSample(mReader, mVideoTrackId, mVideoSampleId++, &buffers, &bufferSize, samplesTime, NULL, NULL, NULL);
	}
	else {
		boolean = MP4ReadSample(mReader, mAudioTrackId, mAudioSampleId++, &buffers, &bufferSize, samplesTime, NULL, NULL, NULL);
	}

	if (boolean) {
		sample.insert(sample.end(), reinterpret_cast<std::byte*>(buffers), reinterpret_cast<std::byte*>(buffers) + bufferSize);
		free(buffers);
		return sample;
	}
	return {};
}

rtc::binary MP4v2Reader::loadNextSamples_PpsIdr() {
	bool isPPS = false;
	rtc::binary sample {};
	rtc::binary pps {}, idr {};

	/* Load PPS */
	pps = readOneSample(true, NULL);
	isPPS = (MP4v2NaluType(*((uint8_t*)pps.data() + 4), MP4V2_MP4_VIDEO_H264) == MP4V2_MP4_AVC_NAL_PPS) ? true : false;
	if (pps.size() < 5 || !isPPS) {
		return {};
	}
	sample.insert(sample.end(), pps.begin(), pps.end());

	/* Load -> IDR */
	idr = readOneSample(true, NULL);
	sample.insert(sample.end(), idr.begin(), idr.end());

	return sample;
}

rtc::binary MP4v2Reader::loadNextSamples_SpsPpsIdr() {
	bool isSPS = false;
	bool isPPS = false;
	rtc::binary sample {};
	rtc::binary sps {}, pps {}, idr {};

	/* Load SPS */
	sps = readOneSample(true, NULL);
	isSPS = (MP4v2NaluType(*((uint8_t*)sps.data() + 4), MP4V2_MP4_VIDEO_H265) == MP4V2_MP4_HEVC_NAL_SPS) ? true : false;
	if (sps.size() < 5 || !isSPS) {
		return {};
	}
	sample.insert(sample.end(), sps.begin(), sps.end());

	/* Load -> PPS */
	pps = readOneSample(true, NULL);
	isPPS = (MP4v2NaluType(*((uint8_t*)pps.data() + 4), MP4V2_MP4_VIDEO_H265) == MP4V2_MP4_HEVC_NAL_PPS) ? true : false;
	if (pps.size() < 5 || !isPPS) {
		return {};
	}
	sample.insert(sample.end(), pps.begin(), pps.end());

	/* Load -> IDR */
	idr = readOneSample(true, NULL);
	sample.insert(sample.end(), idr.begin(), idr.end());

	return sample;
}

rtc::binary MP4v2Reader::readSamples(bool isVideo) {
	if (!mReader) {
		return {};
	}

	rtc::binary sample {};

	if (isVideo) {
		if (mVideoSampleId > mVideoTotalSamples) {
			return {};
		}

		sample = readOneSample(true, &mSamplesTime);
		if (sample.size() < 5) {
			return {};
		}

		bool isIDR = false;

		#if 1
		/* H264 [SPS PPS IDR ... P/B] */
		isIDR = (MP4v2NaluType(*((uint8_t*)sample.data() + 4), MP4V2_MP4_VIDEO_H264) == MP4V2_MP4_AVC_NAL_CODED_SLICE_IDR) ? true : false;
		if (isIDR) {
			uint8_t **ppSps; uint32_t *pSpsSize;
			uint8_t **ppPps; uint32_t *pPpsSize;
			
			if (!MP4GetTrackH264SeqPictHeaders(mReader, mVideoTrackId, &ppSps, &pSpsSize, &ppPps, &pPpsSize)) {
				return sample;
			}

			uint8_t* sps = ppSps[0];
			uint32_t spsSize = pSpsSize[0];
			uint8_t* pps = ppPps[0];
			uint32_t ppsSize = pPpsSize[0];
			rtc::binary spsPpsIdr = {};

			/* SPS (AVCC length + payload) */
			uint32_t spsLen = htonl(spsSize);
			spsPpsIdr.insert(spsPpsIdr.end(), (std::byte*)&spsLen, (std::byte*)&spsLen + sizeof(uint32_t));
			spsPpsIdr.insert(spsPpsIdr.end(), reinterpret_cast<std::byte*>(sps), reinterpret_cast<std::byte*>(sps) + spsSize);
			/* PPS (AVCC length + payload) */
			uint32_t ppsLen = htonl(ppsSize);
			spsPpsIdr.insert(spsPpsIdr.end(), (std::byte*)&ppsLen, (std::byte*)&ppsLen + sizeof(uint32_t));
			spsPpsIdr.insert(spsPpsIdr.end(), reinterpret_cast<std::byte*>(pps), reinterpret_cast<std::byte*>(pps) + ppsSize);
			/* Append the IDR NALU (already AVCC: [4-byte size][data]) */
			spsPpsIdr.insert(spsPpsIdr.end(), sample.begin(), sample.end());

			return spsPpsIdr;
		}
		#else	
		/* H264 [SPS PPS IDR ... P/B] */
		bool isSPS = (MP4v2NaluType(*((uint8_t*)sample.data() + 4), MP4V2_MP4_VIDEO_H264) == MP4V2_MP4_AVC_NAL_SPS) ? true : false;
		if (isSPS) {
			rtc::binary ppsIdr = loadNextSamples_PpsIdr();
			sample.insert(sample.end(), ppsIdr.begin(), ppsIdr.end());
			return sample;
		}
		#endif

		#if 1
		/* H264 [VPS SPS PPS IDR ... P/B] */
		isIDR = (MP4v2NaluType(*((uint8_t*)sample.data() + 4), MP4V2_MP4_VIDEO_H265) == MP4V2_MP4_HEVC_NAL_IDR_W_RADL) ? true : false;
		if (isIDR) {
			uint8_t **ppVps; uint32_t *pVpsSize;
			uint8_t **ppSps; uint32_t *pSpsSize;
			uint8_t **ppPps; uint32_t *pPpsSize;

			if (!MP4GetTrackH265SeqPictHeaders(mReader, mVideoTrackId, &ppVps, &pVpsSize, &ppSps, &pSpsSize, &ppPps, &pPpsSize)) {
				return sample;
			}

			uint8_t* vps = ppVps[0];
			uint32_t vpsSize = pVpsSize[0];
			uint8_t* sps = ppSps[0];
			uint32_t spsSize = pSpsSize[0];
			uint8_t* pps = ppPps[0];
			uint32_t ppsSize = pPpsSize[0];
			rtc::binary vpsSpsPpsIdr = {};

			/* VPS (AVCC length + payload) */
			uint32_t vpsLen = htonl(vpsSize);
			vpsSpsPpsIdr.insert(vpsSpsPpsIdr.end(), (std::byte*)&vpsLen, (std::byte*)&vpsLen + sizeof(uint32_t));
			vpsSpsPpsIdr.insert(vpsSpsPpsIdr.end(), reinterpret_cast<std::byte*>(vps), reinterpret_cast<std::byte*>(vps) + vpsSize);
			/* SPS (AVCC length + payload) */
			uint32_t spsLen = htonl(spsSize);
			vpsSpsPpsIdr.insert(vpsSpsPpsIdr.end(), (std::byte*)&spsLen, (std::byte*)&spsLen + sizeof(uint32_t));
			vpsSpsPpsIdr.insert(vpsSpsPpsIdr.end(), reinterpret_cast<std::byte*>(sps), reinterpret_cast<std::byte*>(sps) + spsSize);
			/* PPS (AVCC length + payload) */
			uint32_t ppsLen = htonl(ppsSize);
			vpsSpsPpsIdr.insert(vpsSpsPpsIdr.end(), (std::byte*)&ppsLen, (std::byte*)&ppsLen + sizeof(uint32_t));
			vpsSpsPpsIdr.insert(vpsSpsPpsIdr.end(), reinterpret_cast<std::byte*>(pps), reinterpret_cast<std::byte*>(pps) + ppsSize);
			/* Append the IDR NALU (already AVCC: [4-byte size][data]) */
			vpsSpsPpsIdr.insert(vpsSpsPpsIdr.end(), sample.begin(), sample.end());

			return vpsSpsPpsIdr;
		}
		#else
		/* H264 [VPS SPS PPS IDR ... P/B] */
		bool isVPS = (MP4v2NaluType(*((uint8_t*)sample.data() + 4), MP4V2_MP4_VIDEO_H265) == MP4V2_MP4_HEVC_NAL_VPS) ? true : false;
		if (isVPS) {
			rtc::binary spsPpsIdr = loadNextSamples_SpsPpsIdr();
			sample.insert(sample.end(), spsPpsIdr.begin(), spsPpsIdr.end());
			return sample;
		}
		#endif
	}
	else {
		if (mAudioSampleId > mAudioTotalSamples) {
			return {};
		}
		sample = readOneSample(false, &mSamplesTime);
	}

	return sample;
}

int MP4v2Reader::resetParams() {
	mReader = NULL;
	mVideoTrackId = mAudioTrackId = MP4_INVALID_TRACK_ID;
	mVideoSampleId = mAudioSampleId = MP4_INVALID_SAMPLE_ID;
	mVideoTotalSamples = mAudioTotalSamples = 0;
	mSamplesTime = 0;
	mFilename.clear();
	return 0;
}

int MP4v2Reader::elapsedSeconds() {
	if (!mReader || mVideoTrackId == MP4_INVALID_TRACK_ID) {
		return 0;
	}
	uint32_t ts = MP4GetTrackTimeScale(mReader, mVideoTrackId);
    if (ts == 0) {
		return 0;
	}
    uint64_t seconds = mSamplesTime / ts;
    return (int)seconds;
}

int MP4v2Reader::framePerSeconds() {
	uint32_t timScale = MP4GetTrackTimeScale(mReader, mVideoTrackId);
	uint64_t duration = MP4GetTrackDuration(mReader, mVideoTrackId);
	if (duration == 0 || timScale == 0) {
		return 1;
	}

	int durationSec = static_cast<int>(duration) / timScale;
	if (durationSec == 0) {
		return 1;
	}
	int fps = mVideoTotalSamples / durationSec;
	return (fps) == 0 ? 1 : fps;
}
