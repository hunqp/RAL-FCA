#ifndef _COMMON_HH_
#define _COMMON_HH_

#include <stdio.h>
#include <stdint.h>

typedef struct {
	int type;
	int size;
	unsigned char *data;
} NaluUnit;

struct H264NalUnitHeader {
	uint8_t First = 0;
	bool forbiddenBit() const { return First >> 7; }
	uint8_t nri() const { return First >> 5 & 0x03; }
	uint8_t idc() const { return First & 0x60; }
	uint8_t unitType() const { return First & 0x1F; }
	void setForbiddenBit(bool isSet) { First = (First & 0x7F) | (isSet << 7); }
	void setNRI(uint8_t nri) { First = (First & 0x9F) | ((nri & 0x03) << 5); }
	void setUnitType(uint8_t type) { First = (First & 0xE0) | (type & 0x1F); }
};

struct H265NalUnitHeader {
	uint8_t First  = 0;
	uint8_t Second = 0;

	bool forbiddenBit() const { return First >> 7; }
	uint8_t unitType() const { return (First & 0b01111110) >> 1; }
	uint8_t nuhLayerId() const { return ((First & 0x1) << 5) | ((Second & 0b11111000) >> 3); }
	uint8_t nuhTempIdPlus1() const { return Second & 0b111; }
	void setForbiddenBit(bool isSet) { First = (First & 0x7F) | (isSet << 7); }
	void setUnitType(uint8_t type) { First = (First & 0b10000001) | ((type & 0b111111) << 1); }
	void setNuhLayerId(uint8_t nuhLayerId) {
		First = (First & 0b11111110) | ((nuhLayerId & 0b100000) >> 5);
		Second = (Second & 0b00000111) | ((nuhLayerId & 0b011111) << 3);
	}
	void setNuhTempIdPlus1(uint8_t nuhTempIdPlus1) { Second = (Second & 0b11111000) | (nuhTempIdPlus1 & 0b111); }
};

extern size_t readOneNaluFromBuffer(const unsigned char *buffer, unsigned int nBufferSize, unsigned int offSet, NaluUnit &nalu);

#endif /* _COMMON_HH_ */
