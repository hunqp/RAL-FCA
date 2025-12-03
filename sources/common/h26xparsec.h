#ifndef __H26XPARSEC_H
#define __H26XPARSEC_H

#include <stdint.h>
#include <string>
#include <vector>
#include "rtc/rtc.hpp"

#define IS_NALU4_START(p) (!p[0] && !p[1] && !p[2] && (p[3] == 1))
#define IS_NALU3_START(p) (!p[0] && !p[1] && (p[2] == 1))

#define GET_H264_NALU_TYPE(p) (p & 0x1F)
#define GET_H265_NALU_TYPE(p) ((p >> 1) & 0x3F)

enum AVCNALUnitType {
	AVC_NAL_EXTERNAL				 = 0,
	AVC_NAL_CODED_SLICE				 = 1,
	AVC_NAL_CODED_SLICE_DATAPART_A	 = 2,
	AVC_NAL_CODED_SLICE_DATAPART_B	 = 3,
	AVC_NAL_CODED_SLICE_DATAPART_C	 = 4,
	AVC_NAL_CODED_SLICE_IDR			 = 5,
	AVC_NAL_SEI						 = 6,
	AVC_NAL_SPS						 = 7,
	AVC_NAL_PPS						 = 8,
	AVC_NAL_ACCESS_UNIT_DELIMITER	 = 9,
	AVC_NAL_END_OF_SEQUENCE			 = 10,
	AVC_NAL_END_OF_STREAM			 = 11,
	AVC_NAL_FILLER_DATA				 = 12,
	AVC_NAL_SUBSET_SPS				 = 15,
	AVC_NAL_CODED_SLICE_PREFIX		 = 14,
	AVC_NAL_CODED_SLICE_SCALABLE	 = 20,
	AVC_NAL_CODED_SLICE_IDR_SCALABLE = 21
};

enum HEVCNALUnitType {
	HEVC_NAL_TRAIL_N	= 0,
	HEVC_NAL_TRAIL_R	= 1,
	HEVC_NAL_TSA_N		= 2,
	HEVC_NAL_TSA_R		= 3,
	HEVC_NAL_STSA_N		= 4,
	HEVC_NAL_STSA_R		= 5,
	HEVC_NAL_RADL_N		= 6,
	HEVC_NAL_RADL_R		= 7,
	HEVC_NAL_RASL_N		= 8,
	HEVC_NAL_RASL_R		= 9,
	HEVC_NAL_BLA_W_LP	= 16,
	HEVC_NAL_BLA_W_RADL = 17,
	HEVC_NAL_BLA_N_LP	= 18,
	HEVC_NAL_IDR_W_RADL = 19,
	HEVC_NAL_IDR_N_LP	= 20,
	HEVC_NAL_CRA_NUT	= 21,
	HEVC_NAL_VPS		= 32,
	HEVC_NAL_SPS		= 33,
	HEVC_NAL_PPS		= 34,
	HEVC_NAL_AUD		= 35,
	HEVC_NAL_EOS_NUT	= 36,
	HEVC_NAL_EOB_NUT	= 37,
	HEVC_NAL_FD_NUT		= 38,
	HEVC_NAL_SEI_PREFIX = 39,
	HEVC_NAL_SEI_SUFFIX = 40,
};

extern std::vector<std::byte> H26X_ReadOneNaluFromURL(std::string url, uint32_t *cursor);
extern std::vector<std::byte> H26X_ReadOneNaluFromBuffer(uint8_t *data, uint32_t dataLen, uint32_t *cursor);

extern std::vector<std::byte> H264_ExtractSPSPPSIDR(std::string url, uint32_t pos = 0);
extern std::vector<std::byte> H265_ExtractVPSSPSPPSIDR(std::string url, uint32_t pos = 0);
extern std::vector<std::byte> H264_ExtractSPSPPSIDR(uint8_t *data, uint32_t dataLen, uint32_t pos = 0);
extern std::vector<std::byte> H265_ExtractVPSSPSPPSIDR(uint8_t *data, uint32_t dataLen, uint32_t pos = 0);

extern void H26X_DumpNalusToURL(std::string url, std::vector<std::byte> &buffer);

#endif /* __H26XPARSEC_H */