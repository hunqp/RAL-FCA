#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "mp4v2_mp4.h"

/* AAC Object types */
enum {
	GF_M4A_AAC_MAIN			   = 1,
	GF_M4A_AAC_LC			   = 2,
	GF_M4A_AAC_SSR			   = 3,
	GF_M4A_AAC_LTP			   = 4,
	GF_M4A_AAC_SBR			   = 5,
	GF_M4A_AAC_SCALABLE		   = 6,
	GF_M4A_TWINVQ			   = 7,
	GF_M4A_CELP				   = 8,
	GF_M4A_HVXC				   = 9,
	GF_M4A_TTSI				   = 12,
	GF_M4A_MAIN_SYNTHETIC	   = 13,
	GF_M4A_WAVETABLE_SYNTHESIS = 14,
	GF_M4A_GENERAL_MIDI		   = 15,
	GF_M4A_ALGO_SYNTH_AUDIO_FX = 16,
	GF_M4A_ER_AAC_LC		   = 17,
	GF_M4A_ER_AAC_LTP		   = 19,
	GF_M4A_ER_AAC_SCALABLE	   = 20,
	GF_M4A_ER_TWINVQ		   = 21,
	GF_M4A_ER_BSAC			   = 22,
	GF_M4A_ER_AAC_LD		   = 23,
	GF_M4A_ER_CELP			   = 24,
	GF_M4A_ER_HVXC			   = 25,
	GF_M4A_ER_HILN			   = 26,
	GF_M4A_ER_PARAMETRIC	   = 27,
	GF_M4A_SSC				   = 28,
	GF_M4A_AAC_PS			   = 29,
	GF_M4A_LAYER1			   = 32,
	GF_M4A_LAYER2			   = 33,
	GF_M4A_LAYER3			   = 34,
	GF_M4A_DST				   = 35,
	GF_M4A_ALS				   = 36
};

static uint64_t elapsedInMicroseconds() {
	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (ts.tv_nsec / 1000ULL);
}

int MP4v2ReadOneNaluFromBuffers(const unsigned char *buffer, unsigned int nBufferSize, unsigned int offSet, MP4V2_NaluUnit &nalu) {
	unsigned int i = offSet;

	while (i < nBufferSize) {
		if (buffer[i++] == 0x00 && buffer[i++] == 0x00 && buffer[i++] == 0x00 && buffer[i++] == 0x01) {
			unsigned int pos = i;
			while (pos < nBufferSize) {
				if (buffer[pos++] == 0x00 && buffer[pos++] == 0x00 && buffer[pos++] == 0x00 && buffer[pos++] == 0x01) {
					break;
				}
			}
			if (pos == nBufferSize) {
				nalu.size = pos - i;
			}
			else {
				nalu.size = (pos - 4) - i;
			}

			nalu.type = buffer[i] & 0x1f;
			nalu.data = (unsigned char *)&buffer[i];
			return (nalu.size + i - offSet);
		}
	}
	return 0;
}

/* Returns the sample rate index */
static int GetSRIndex(unsigned int sampleRate) {
	if (92017 <= sampleRate)
		return 0;
	if (75132 <= sampleRate)
		return 1;
	if (55426 <= sampleRate)
		return 2;
	if (46009 <= sampleRate)
		return 3;
	if (37566 <= sampleRate)
		return 4;
	if (27713 <= sampleRate)
		return 5;
	if (23004 <= sampleRate)
		return 6;
	if (18783 <= sampleRate)
		return 7;
	if (13856 <= sampleRate)
		return 8;
	if (11502 <= sampleRate)
		return 9;
	if (9391 <= sampleRate)
		return 10;
	return 11;
}

static void GetAudioSpecificConfig(
	uint8_t AudioType, 
	uint8_t SampleRateID, 
	uint8_t Channel, 
	uint8_t *pHigh, 
	uint8_t *pLow) 
{
	uint16_t Config;

	Config = 0xffff & (AudioType & 0x1f);
	Config <<= 4;
	Config |= SampleRateID & 0x0f;
	Config <<= 4;
	Config |= Channel & 0x0f;
	Config <<= 3;

	*pLow = Config & 0xff;
	Config >>= 8;
	*pHigh = Config & 0xff;
}

int Mp4v2WriteALawToMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	uint64_t micros) 
{
	if (!pHandle) {
		return -1;
	}

	/* Audio MUST-BE written after video catches first I-Frame */
	if (pHandle->isFirstFrame) {
		return 0;
	}

	int rc = MP4WriteSample(
				pHandle->hMp4File,
				pHandle->audioTrackId,
				frame,
				frameSize,
				(MP4Duration)frameSize,
				0,
				false);

	return (rc == true) ? 0 : -2;
}

int Mp4v2WriteVideoToMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	unsigned int framePerSecons,
	uint64_t micros)
{
	if (!pHandle) {
		return -1;
	}

	switch (pHandle->audioType) {
	case MP4V2_MP4_VIDEO_H264: {
		return Mp4v2WriteH264toMP4(pHandle, frame, frameSize, framePerSecons, micros);
	}
	break;

	case MP4V2_MP4_VIDEO_H265: {
		return Mp4v2WriteH265toMP4(pHandle, frame, frameSize, framePerSecons, micros);
	}
	break;

	default:
	break;
	}

	return -3;
}

int Mp4v2WriteAudioToMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	uint64_t micros) 
{
	if (!pHandle) {
		return -1;
	}

	switch (pHandle->audioType) {
	case MP4V2_MP4_AUDIO_ALAW: {
		return Mp4v2WriteALawToMP4(pHandle, frame, frameSize, micros);
	}
	break;

	case MP4V2_MP4_AUDIO_AAC: printf("Unsupport MP4V2_MP4_AUDIO_AAC\r\n");
	break;

	case MP4V2_MP4_AUDIO_ULAW: printf("Unsupport MP4V2_MP4_AUDIO_ULAW\r\n");
	break;

	case MP4V2_MP4_AUDIO_OPUS: printf("Unsupport MP4V2_MP4_AUDIO_OPUS\r\n");
	break;

	default:
	break;
	}
	return -3;
}

int Mp4v2WriteH264toMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize,
	unsigned int framePerSeconds,
	uint64_t micros) 
{
	if (!pHandle) {
		return -1;
	}

	int rc = -1;
	unsigned char *naluData = (unsigned char *)&frame[MP4V2_MP4_H264_START_CODE_LENGTH];
	unsigned int naluDataSize = frameSize - MP4V2_MP4_H264_START_CODE_LENGTH;
	int naluType = MP4v2NaluType(frame[MP4V2_MP4_H264_START_CODE_LENGTH], MP4V2_MP4_VIDEO_H264);
	if (micros == 0) {
		micros = elapsedInMicroseconds();
	}

    /* AVC parameter set types:
       SPS = 33, PPS = 34
    */
	if (naluType == MP4V2_MP4_AVC_NAL_SPS) {
		if (pHandle->videoTrackId == MP4_INVALID_TRACK_ID) {
			pHandle->videoTrackId = MP4AddH264VideoTrack(
										pHandle->hMp4File, 
										pHandle->timeScale, 
										pHandle->timeScale / framePerSeconds, 
										pHandle->width, 
										pHandle->height,
										naluData[1], 
										naluData[2], 
										naluData[3], 
										3);
			MP4SetVideoProfileLevel(pHandle->hMp4File, 0x7F);
		}
		MP4AddH264SequenceParameterSet(pHandle->hMp4File, pHandle->videoTrackId, naluData, naluDataSize);
		pHandle->isFirstSPS = false;
		return 0;
	}
	else if (naluType == MP4V2_MP4_AVC_NAL_PPS) {
		MP4AddH264PictureParameterSet(pHandle->hMp4File, pHandle->videoTrackId, naluData, naluDataSize);
		pHandle->isFirstPPS = false;
		return 0;
	}
	else if (naluType == MP4V2_MP4_AVC_NAL_SEI) {
		/* Ignore this frame */
		return 0;
	}
	
	/* Only write samples after PPS and SPS have been added (keeps your original flag logic) */
	if (pHandle->isFirstSPS || pHandle->isFirstPPS) {
		return 0;
	}

	bool isSyncSample = (naluType == MP4V2_MP4_AVC_NAL_CODED_SLICE_IDR) ? true : false;
    uint32_t naluNLen = htonl((uint32_t)naluDataSize);
    memcpy(frame, &naluNLen, 4); /* Change Annex B to AVCC */
	
	MP4Duration duration = 0;
	if (pHandle->videoTime == 0) {
        duration = pHandle->timeScale / framePerSeconds;
    }
    else {
        uint64_t delta = micros - pHandle->videoTime;
        duration = (MP4Duration)((delta * pHandle->timeScale) / 1000000ULL);
    }
	if (duration == 0) {
		duration = pHandle->timeScale / framePerSeconds;
	}
	
    rc = MP4WriteSample(
			pHandle->hMp4File,
			pHandle->videoTrackId,
			frame,
			frameSize,
			duration,
			0,
			isSyncSample);
	
	pHandle->videoTime = micros;
	pHandle->isFirstFrame = false;

    return (rc ? 0 : -2);
}

int Mp4v2WriteH265toMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	unsigned int framePerSeconds, 
	uint64_t micros) 
{
	if (!pHandle) {
		return -1;
	}
	int rc = -1;
    unsigned char *naluData = (unsigned char *)&frame[MP4V2_MP4_H265_START_CODE_LENGTH];
	unsigned int naluDataSize = frameSize - MP4V2_MP4_H265_START_CODE_LENGTH;
	int naluType = MP4v2NaluType(frame[MP4V2_MP4_H265_START_CODE_LENGTH], MP4V2_MP4_VIDEO_H265);

    /* HEVC parameter set types:
       VPS = 32, SPS = 33, PPS = 34
    */
    if (naluType == MP4V2_MP4_HEVC_NAL_VPS) {
		if (pHandle->videoTrackId == MP4_INVALID_TRACK_ID) {
			pHandle->videoTrackId = MP4AddH265VideoTrack(
											pHandle->hMp4File,
											pHandle->timeScale,
											pHandle->timeScale / pHandle->framerate,
											pHandle->width,
											pHandle->height,
											1, 0, 120, 3);
			MP4SetVideoProfileLevel(pHandle->hMp4File, 0x7F);
		}
		MP4AddH265VideoParameterSet(pHandle->hMp4File, pHandle->videoTrackId, naluData, naluDataSize);
		pHandle->isFirstVPS = false;
		return 0;
    }
    else if (naluType == MP4V2_MP4_HEVC_NAL_SPS) {
		MP4AddH265SequenceParameterSet(pHandle->hMp4File, pHandle->videoTrackId, naluData, naluDataSize);
        pHandle->isFirstSPS = false;
		return 0;
    }
    else if (naluType == MP4V2_MP4_HEVC_NAL_PPS) {
        MP4AddH265PictureParameterSet(pHandle->hMp4File, pHandle->videoTrackId, naluData, naluDataSize);
        pHandle->isFirstPPS = false;
		return 0;
    }
    else if (naluType == MP4V2_MP4_HEVC_NAL_SEI_PREFIX || naluType == MP4V2_MP4_HEVC_NAL_SEI_SUFFIX) {
        /* SEI / unspecified – ignore (or handle if required) */
		return 0;
        
    }
    /* Only write samples after VPS and PPS and SPS have been added (keeps your original flag logic) */
    if (pHandle->isFirstSPS || pHandle->isFirstPPS || pHandle->isFirstVPS) {
        return 0;
    }

	bool isSyncSample = (naluType == MP4V2_MP4_HEVC_NAL_IDR_W_RADL) ? true : false;
    uint32_t naluNLen = htonl((uint32_t)naluDataSize);
    memcpy(frame, &naluNLen, 4); /* Change Annex B to AVCC */

	MP4Duration duration = 0;
	if (pHandle->videoTime == 0) {
        duration = pHandle->timeScale / framePerSeconds;
    }
    else {
        uint64_t delta = micros - pHandle->videoTime;
        duration = (MP4Duration)((delta * pHandle->timeScale) / 1000000ULL);
    }
	if (duration == 0) {
		duration = pHandle->timeScale / framePerSeconds;
	}

    rc = MP4WriteSample(
				pHandle->hMp4File,
				pHandle->videoTrackId,
				frame,
				frameSize,
				duration,
				0,
				isSyncSample);

	
	pHandle->videoTime = micros;
	pHandle->isFirstFrame = false;

    return (rc ? 0 : -2);
}

static bool MP4v2CreateAudioTrackId(MP4V2_MP4_T *pHandle) {
	switch (pHandle->audioType) {
	case MP4V2_MP4_AUDIO_AAC: {
		pHandle->audioTrackId = MP4AddAudioTrack(pHandle->hMp4File, pHandle->samplerate, MP4_INVALID_DURATION, MP4_MPEG4_AUDIO_TYPE);
		if (pHandle->audioTrackId != MP4_INVALID_TRACK_ID) {
			MP4SetAudioProfileLevel(pHandle->hMp4File, 0x2);
			uint8_t Type = GF_M4A_AAC_LC;
			uint8_t aacInfor[2];
			unsigned long aacInforSize = 2;
			uint8_t SampleRate = GetSRIndex(pHandle->samplerate);
			GetAudioSpecificConfig(Type, SampleRate, 1, &aacInfor[0], &aacInfor[1]);
			MP4SetTrackESConfiguration(pHandle->hMp4File, pHandle->audioTrackId, (uint8_t *)&aacInfor, aacInforSize);
			return true;
		}
	}
	break;

	case MP4V2_MP4_AUDIO_ALAW: {
		pHandle->audioTrackId = MP4AddALawAudioTrack(pHandle->hMp4File, pHandle->samplerate);
		if (pHandle->audioTrackId != MP4_INVALID_TRACK_ID) {
			MP4SetAudioProfileLevel(pHandle->hMp4File, 0xFE);
			return true;
		}
	}
	break;

	case MP4V2_MP4_AUDIO_ULAW: {
		pHandle->audioTrackId = MP4AddULawAudioTrack(pHandle->hMp4File, pHandle->samplerate);
		if (pHandle->audioTrackId != MP4_INVALID_TRACK_ID) {
			MP4SetAudioProfileLevel(pHandle->hMp4File, 0xFE);
			return true;
		}
	}
	break;

	case MP4V2_MP4_AUDIO_OPUS: printf("Unsupport MP4V2_MP4_AUDIO_OPUS\r\n");
	break;
	
	default:
	break;
	}
	return false;
}

MP4V2_MP4_T *Mp4v2CreateMP4File(
	const char *filename, 
	int width, 
	int height, 
	int timeScale, 
	int frameRate, 
	int samplerate,
	MP4V2_MP4_MediaType audioType,
	MP4V2_MP4_MediaType videoType) 
{
	MP4V2_MP4_T *pHandle = (MP4V2_MP4_T *)malloc(sizeof(MP4V2_MP4_T));
	if (pHandle == NULL) {
		return NULL;
	}

	pHandle->hMp4File = MP4Create(filename);
	if (pHandle->hMp4File == MP4_INVALID_FILE_HANDLE) {
		free(pHandle);
		return NULL;
	}

	pHandle->width = width;
	pHandle->height = height;
	pHandle->timeScale = timeScale;
	pHandle->framerate = frameRate;
	pHandle->samplerate = samplerate;
	pHandle->videoTime = 0;
	pHandle->audioTime = 0;
	pHandle->audioTrackId = MP4_INVALID_TRACK_ID;
	pHandle->videoTrackId = MP4_INVALID_TRACK_ID;
	pHandle->isFirstPPS	= true;
	pHandle->isFirstSPS	= true;
	pHandle->isFirstVPS	= true;
	pHandle->isFirstFrame = true;
	pHandle->audioType = audioType;
	pHandle->videoType = videoType;
	MP4SetTimeScale(pHandle->hMp4File, pHandle->timeScale);
	if (!MP4v2CreateAudioTrackId(pHandle)) {
		free(pHandle);
		pHandle = NULL;
	}
	return pHandle;
}

int Mp4v2CloseMP4File(MP4V2_MP4_T *pHandle) {
	int rc = 0;

	if (pHandle) {
		if (pHandle->videoTrackId == MP4_INVALID_TRACK_ID || pHandle->audioTrackId == MP4_INVALID_TRACK_ID) {
			printf("Invalid video or audio track before closing\r\n");
			rc = -1;
		}
		if (MP4_IS_VALID_FILE_HANDLE(pHandle->hMp4File)) {
			MP4Close(pHandle->hMp4File, 0);
			pHandle->hMp4File = NULL;
		}
		free(pHandle);
	}
	return rc;
}

int MP4v2NaluType(uint8_t naluIgnoreStartCode, MP4V2_MP4_MediaType videoType) {
	return (videoType == MP4V2_MP4_VIDEO_H265) ? ((naluIgnoreStartCode & 0x7E) >> 1) : (naluIgnoreStartCode & 0x1F);
}