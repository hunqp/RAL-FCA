#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/md5.h>

#include "app_data.h"
#include "app_dbg.h"
#include "firmware.h"

using namespace std;

ssize_t ssize;

int firmware_get_info(fileInfo_t *fh, const char *bin_file_path) {
	struct stat file_info;
	int binary_file = -1;
	binary_file		= open(bin_file_path, O_RDONLY);
	if (binary_file < 0) {
		return -1;
	}
	fstat(binary_file, &file_info);
	close(binary_file);

	string checksumStr = calculateMD5(string(bin_file_path));
	APP_DBG("Checksum: %s\n", checksumStr.c_str());

	fh->bin_len	 = file_info.st_size;
	fh->checksum = checksumStr;

	return 0;
}

int firmware_read(uint8_t *data, uint32_t cursor, uint32_t size, const char *bin_file_path) {
	struct stat file_info;
	int binary_file = -1;

	binary_file = open(bin_file_path, O_RDONLY);
	if (binary_file < 0) {
		return -1;
	}

	fstat(binary_file, &file_info);

	if ((cursor + size) > (uint32_t)file_info.st_size) {
		return -1;
	}

	ssize = pread(binary_file, data, size, cursor);
	posix_fadvise(binary_file, 0, 0, POSIX_FADV_DONTNEED);
	close(binary_file); 

	return 0;
}

string calculateMD5(const string &filePath) {
	ifstream file(filePath, ios::binary);
	if (!file.is_open()) {
		APP_DBG("error open file\n");
		return "";
	}

	MD5_CTX md5Context;
	MD5_Init(&md5Context);

	char buffer[1024];
	while (!file.eof()) {
		file.read(buffer, sizeof(buffer));
		std::streamsize bytesRead = file.gcount();

		if (bytesRead > 0) {
			MD5_Update(&md5Context, buffer, bytesRead);
		}
	}

	unsigned char hash[MD5_DIGEST_LENGTH];
	MD5_Final(hash, &md5Context);

	stringstream ss;
	for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
		ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
	}
	file.close(); 
	return ss.str();
}