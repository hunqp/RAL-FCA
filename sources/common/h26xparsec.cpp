#include <fstream>
#include <cstdint>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "h26xparsec.h"

static std::vector<uint8_t> loadDataFromTrack(std::string &url, uint32_t cursor, const uint32_t DATA_SIZE = 4096) {
	int fd		  = -1;
	uint8_t *data = NULL;
	std::vector<uint8_t> buffer;

	fd = open(url.c_str(), O_RDONLY);
	if (fd < 0) {
		return {};
	}

	data = (uint8_t *)malloc(DATA_SIZE * sizeof(uint8_t));
	if (data == NULL) {
		return {};
	}

	int read = pread(fd, data, DATA_SIZE, cursor);
	if (read) {
		buffer.insert(buffer.end(), data, data + read);
	}

	close(fd);
	free(data);

	return buffer;
}

rtc::binary H26X_ReadOneNaluFromBuffer(uint8_t *data, uint32_t dataLen, uint32_t *cursor) {
	rtc::binary nalu;

	/*-------------------------------------/
	 * Find NALU Start code (0x00000001)
	 *------------------------------------*/
	while (*cursor + 3 < dataLen) {
		uint8_t *p = data + (*cursor);
		if (IS_NALU4_START(p)) {
			nalu.insert(nalu.end(), reinterpret_cast<std::byte *>(data + *cursor), reinterpret_cast<std::byte *>(data + *cursor) + 4);
			*cursor += 4;
			break;
		}
		(*cursor)++;
	}

	/*------------------------------------------------/
	 * Read until the next start code or end of data
	 *-----------------------------------------------*/
	while (*cursor + 3 < dataLen) {
		uint8_t *p = data + (*cursor);
		if (IS_NALU4_START(p)) {
			break;
		}
		nalu.push_back(static_cast<std::byte>(data[*cursor]));
		(*cursor)++;
	}

	return nalu;
}

rtc::binary H26X_ReadOneNaluFromURL(std::string url, uint32_t *cursor) {
#define STAGE_NALU_INITIAL	 0
#define STAGE_NALU_STORAGE	 1
#define STAGE_NALU_COMPLETED 2

	int stage		   = STAGE_NALU_INITIAL;
	bool naluCompleted = false;

	rtc::binary nalu;

	while (!naluCompleted) {
		auto buffer = loadDataFromTrack(url, *cursor);
		if (buffer.empty()) {
			break;
		}

		if (buffer.size() <= 4) {
			nalu.insert(nalu.end(), reinterpret_cast<std::byte *>(buffer.data()), reinterpret_cast<std::byte *>(buffer.data()) + buffer.size());
			cursor += buffer.size();
			break;
		}

		for (size_t id = 0; id < buffer.size(); ++id) {
			uint8_t *p = buffer.data() + id;
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

rtc::binary H264_ExtractSPSPPSIDR(std::string url, uint32_t pos) {
	/*
	+ + + + + + + + + + + + + + + + + + + +
	+   H264 Picture Codec order nalus
	+   -------------------
	+   | SPS | PPS | IDR |
	+   -------------------
	+ + + + + + + + + + + + + + + + + + + +
	*/

	uint32_t cursor = pos;
	rtc::binary nalus;

	while (true) {
		nalus = H26X_ReadOneNaluFromURL(url, &cursor);
		if (nalus.empty()) {
			break;
		}

		rtc::NalUnitHeader *header = (rtc::NalUnitHeader *)(nalus.data() + 4);

		if (header->unitType() == AVCNALUnitType::AVC_NAL_SPS) {
			auto pps = H26X_ReadOneNaluFromURL(url, &cursor); /* AVC_NAL_PPS */
			auto idr = H26X_ReadOneNaluFromURL(url, &cursor); /* AVC_NAL_CODED_SLICE_IDR */
			nalus.insert(nalus.end(), pps.begin(), pps.end());
			nalus.insert(nalus.end(), idr.begin(), idr.end());
			break;
		}
		nalus.clear();
	}

	return nalus;
}

rtc::binary H265_ExtractVPSSPSPPSIDR(std::string url, uint32_t pos) {
	/*
	+ + + + + + + + + + + + + + + + + + + +
	+   H265 Picture Codec order nalus
	+   -------------------------
	+   | VPS | SPS | PPS | IDR |
	+   -------------------------
	+ + + + + + + + + + + + + + + + + + + +
	*/

	uint32_t cursor = pos;
	rtc::binary nalus;

	while (true) {
		nalus = H26X_ReadOneNaluFromURL(url, &cursor);
		if (nalus.empty()) {
			break;
		}

		rtc::H265NalUnitHeader *header = (rtc::H265NalUnitHeader *)(nalus.data() + 4);

		if (header->unitType() == HEVCNALUnitType::HEVC_NAL_VPS) {
			auto sps = H26X_ReadOneNaluFromURL(url, &cursor); /* HEVC_NAL_SPS */
			auto pps = H26X_ReadOneNaluFromURL(url, &cursor); /* HEVC_NAL_PPS */
			auto idr = H26X_ReadOneNaluFromURL(url, &cursor); /* HEVC_NAL_IDR_W_RADL */
			nalus.insert(nalus.end(), sps.begin(), sps.end());
			nalus.insert(nalus.end(), pps.begin(), pps.end());
			nalus.insert(nalus.end(), idr.begin(), idr.end());
			break;
		}
		nalus.clear();
	}

	return nalus;
}

rtc::binary H264_ExtractSPSPPSIDR(uint8_t *data, uint32_t dataLen, uint32_t pos) {
	uint32_t cursor = pos;
	rtc::binary nalus;

	for (;;) {
		nalus = H26X_ReadOneNaluFromBuffer(data, dataLen, &cursor);
		if (nalus.empty()) {
			break;
		}

		rtc::NalUnitHeader *header = (rtc::NalUnitHeader *)(nalus.data() + 4);
		if (header->unitType() == AVCNALUnitType::AVC_NAL_SPS) {
			auto pps = H26X_ReadOneNaluFromBuffer(data, dataLen, &cursor); /* AVC_NAL_PPS */
			auto idr = H26X_ReadOneNaluFromBuffer(data, dataLen, &cursor); /* AVC_NAL_CODED_SLICE_IDR */
			nalus.insert(nalus.end(), pps.begin(), pps.end());
			nalus.insert(nalus.end(), idr.begin(), idr.end());
			break;
		}
		nalus.clear();
	}

	return nalus;
}

rtc::binary H265_ExtractVPSSPSPPSIDR(uint8_t *data, uint32_t dataLen, uint32_t pos) {
	uint32_t cursor = pos;
	rtc::binary nalus;

	for (;;) {
		nalus = H26X_ReadOneNaluFromBuffer(data, dataLen, &cursor);
		if (nalus.empty()) {
			break;
		}

		rtc::H265NalUnitHeader *header = (rtc::H265NalUnitHeader *)(nalus.data() + 4);
		if (header->unitType() == HEVCNALUnitType::HEVC_NAL_VPS) {
			auto sps = H26X_ReadOneNaluFromBuffer(data, dataLen, &cursor); /* HEVC_NAL_SPS */
			auto pps = H26X_ReadOneNaluFromBuffer(data, dataLen, &cursor); /* HEVC_NAL_PPS */
			auto idr = H26X_ReadOneNaluFromBuffer(data, dataLen, &cursor); /* HEVC_NAL_IDR_W_RADL */
			nalus.insert(nalus.end(), sps.begin(), sps.end());
			nalus.insert(nalus.end(), pps.begin(), pps.end());
			nalus.insert(nalus.end(), idr.begin(), idr.end());
			break;
		}
		nalus.clear();
	}

	return nalus;
}

void H26X_DumpNalusToURL(std::string url, rtc::binary &buffer) {
	std::ofstream ofs(url, std::ios::binary);
	if (ofs.is_open()) {
		ofs.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
		ofs.close();
	}
}