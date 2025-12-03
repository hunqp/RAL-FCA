#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <stdint.h>

using namespace std;

typedef struct {
	uint32_t bin_len;
	string checksum;
} fileInfo_t;

extern string calculateMD5(const string &filePath);
extern int firmware_get_info(fileInfo_t *fh, const char *bin_file_path);
extern int firmware_read(uint8_t *data, uint32_t cursor, uint32_t size, const char *bin_file_path);

#endif	  //__FIRMWARE_H__
