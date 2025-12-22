#ifndef MP4V2_MP4_H
#define MP4V2_MP4_H

#include <stdbool.h>
#include <mp4v2/mp4v2.h>

#define MP4V2_MP4_H264_START_CODE_LENGTH (4)
#define MP4V2_MP4_H265_START_CODE_LENGTH (4)

enum MP4V2_MP4_MediaType {
	/* Video */
	MP4V2_MP4_VIDEO_H264,
	MP4V2_MP4_VIDEO_H265,
	/* Audio */
	MP4V2_MP4_AUDIO_AAC,
	MP4V2_MP4_AUDIO_ALAW,
	MP4V2_MP4_AUDIO_ULAW,
	MP4V2_MP4_AUDIO_OPUS,
};

enum MP4V2_MP4_AVCNALUnitType {
	MP4V2_MP4_AVC_NAL_EXTERNAL				 	= 0,
	MP4V2_MP4_AVC_NAL_CODED_SLICE				= 1,
	MP4V2_MP4_AVC_NAL_CODED_SLICE_DATAPART_A	= 2,
	MP4V2_MP4_AVC_NAL_CODED_SLICE_DATAPART_B	= 3,
	MP4V2_MP4_AVC_NAL_CODED_SLICE_DATAPART_C	= 4,
	MP4V2_MP4_AVC_NAL_CODED_SLICE_IDR			= 5,
	MP4V2_MP4_AVC_NAL_SEI						= 6,
	MP4V2_MP4_AVC_NAL_SPS						= 7,
	MP4V2_MP4_AVC_NAL_PPS						= 8,
	MP4V2_MP4_AVC_NAL_ACCESS_UNIT_DELIMITER	 	= 9,
	MP4V2_MP4_AVC_NAL_END_OF_SEQUENCE			= 10,
	MP4V2_MP4_AVC_NAL_END_OF_STREAM			 	= 11,
	MP4V2_MP4_AVC_NAL_FILLER_DATA				= 12,
	MP4V2_MP4_AVC_NAL_SUBSET_SPS				= 15,
	MP4V2_MP4_AVC_NAL_CODED_SLICE_PREFIX		= 14,
	MP4V2_MP4_AVC_NAL_CODED_SLICE_SCALABLE	 	= 20,
	MP4V2_MP4_AVC_NAL_CODED_SLICE_IDR_SCALABLE 	= 21
};

enum MP4V2_MP4_HEVCNALUnitType {
	MP4V2_MP4_HEVC_NAL_TRAIL_N		= 0,
	MP4V2_MP4_HEVC_NAL_TRAIL_R		= 1,
	MP4V2_MP4_HEVC_NAL_TSA_N		= 2,
	MP4V2_MP4_HEVC_NAL_TSA_R		= 3,
	MP4V2_MP4_HEVC_NAL_STSA_N		= 4,
	MP4V2_MP4_HEVC_NAL_STSA_R		= 5,
	MP4V2_MP4_HEVC_NAL_RADL_N		= 6,
	MP4V2_MP4_HEVC_NAL_RADL_R		= 7,
	MP4V2_MP4_HEVC_NAL_RASL_N		= 8,
	MP4V2_MP4_HEVC_NAL_RASL_R		= 9,
	MP4V2_MP4_HEVC_NAL_BLA_W_LP		= 16,
	MP4V2_MP4_HEVC_NAL_BLA_W_RADL 	= 17,
	MP4V2_MP4_HEVC_NAL_BLA_N_LP		= 18,
	MP4V2_MP4_HEVC_NAL_IDR_W_RADL 	= 19,
	MP4V2_MP4_HEVC_NAL_IDR_N_LP		= 20,
	MP4V2_MP4_HEVC_NAL_CRA_NUT		= 21,
	MP4V2_MP4_HEVC_NAL_VPS			= 32,
	MP4V2_MP4_HEVC_NAL_SPS			= 33,
	MP4V2_MP4_HEVC_NAL_PPS			= 34,
	MP4V2_MP4_HEVC_NAL_AUD			= 35,
	MP4V2_MP4_HEVC_NAL_EOS_NUT		= 36,
	MP4V2_MP4_HEVC_NAL_EOB_NUT		= 37,
	MP4V2_MP4_HEVC_NAL_FD_NUT		= 38,
	MP4V2_MP4_HEVC_NAL_SEI_PREFIX 	= 39,
	MP4V2_MP4_HEVC_NAL_SEI_SUFFIX 	= 40,
};

typedef struct {
	int type;
	int size;
	unsigned char *data;
} MP4V2_NaluUnit;

typedef struct {
	int width;
	int height;
	int framerate;
	int timeScale;
	bool isFirstSPS;
	bool isFirstPPS;
	bool isFirstVPS;
	bool isFirstFrame;
	int samplerate;
	uint64_t videoTime;
	uint64_t audioTime;
	MP4TrackId videoTrackId;
	MP4TrackId audioTrackId;
	MP4V2_MP4_MediaType audioType;
	MP4V2_MP4_MediaType videoType;
	MP4FileHandle hMp4File;
} MP4V2_MP4_T;

extern MP4V2_MP4_T *Mp4v2CreateMP4File(
	const char *filename, 
	int width, 
	int height, 
	int timeScale, 
	int frameRate, 
	int samplerate, 
	MP4V2_MP4_MediaType audioType = MP4V2_MP4_AUDIO_ALAW,
	MP4V2_MP4_MediaType videoType = MP4V2_MP4_VIDEO_H264);

extern int Mp4v2CloseMP4File(MP4V2_MP4_T *pHandle);

extern int Mp4v2WriteH264toMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	unsigned int framePerSecons,
	uint64_t micros);

extern int Mp4v2WriteH265toMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	unsigned int framePerSecons,
	uint64_t micros);

extern int Mp4v2WriteALawToMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	uint64_t micros);

extern int Mp4v2WriteVideoToMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	unsigned int framePerSecons,
	uint64_t micros);

extern int Mp4v2WriteAudioToMP4(
	MP4V2_MP4_T *pHandle, 
	unsigned char *frame, 
	unsigned int frameSize, 
	uint64_t micros);

extern int MP4v2ReadOneNaluFromBuffers(
	const unsigned char *buffer, 
	unsigned int nBufferSize, 
	unsigned int offSet, 
	MP4V2_NaluUnit &nalu);

extern int MP4v2NaluType(
	uint8_t naluIgnoreStartCode, 
	MP4V2_MP4_MediaType videoType = MP4V2_MP4_VIDEO_H264);

#endif /* _MP4V2_MP4_H_ */