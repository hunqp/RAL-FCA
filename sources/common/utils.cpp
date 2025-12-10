#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/wait.h>
#include <vector>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>	  //For errno - the error number
#include <netdb.h>	  //hostent
#include <curl/curl.h>

#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "cmd_line.h"
#include "utils.h"

size_t tmp;

#define APP_GET_MAC_ADDRESS_PATH "/sys/class/net/eth0/address"

AtomicString::AtomicString() {
	pthread_mutex_init(&mutex, nullptr);
}

AtomicString::AtomicString(const std::string &str) : value(str) {
	pthread_mutex_init(&mutex, nullptr);
}

AtomicString::~AtomicString() {
	pthread_mutex_destroy(&mutex);
}

void AtomicString::set(const std::string &str) {
	pthread_mutex_lock(&mutex);
	value = str;
	pthread_mutex_unlock(&mutex);
}

std::string AtomicString::get() const {
	pthread_mutex_lock(&mutex);
	std::string val = value;
	pthread_mutex_unlock(&mutex);
	return val;
}

bool AtomicString::empty() const {
	pthread_mutex_lock(&mutex);
	bool isEmpty = value.empty();
	pthread_mutex_unlock(&mutex);
	return isEmpty;
}

void AtomicString::clear() {
	pthread_mutex_lock(&mutex);
	value.clear();
	pthread_mutex_unlock(&mutex);
}

bool write_raw_file(const string data, const string &filename) {
	FILE *file = fopen(filename.data(), "w");
	if (file == NULL) {
		return false;
	}
	fwrite(data.data(), 1, data.length(), file);
	fsync(fileno(file));
	posix_fadvise(fileno(file), 0, 0, POSIX_FADV_DONTNEED);
	fclose(file);
	return true;
}

bool write_json_file(const json data, const string &filename) {
	return write_raw_file(data.dump(), filename);
}

bool read_raw_file(string &data, const string &filename) {
	struct stat file_info;
	int file_obj = -1;
	char *buffer;

	file_obj = open(filename.data(), O_RDONLY);
	if (file_obj < 0) {
		return false;
	}
	fstat(file_obj, &file_info);
	buffer = (char *)malloc(file_info.st_size);
	if (buffer == NULL) {
		close(file_obj);
		return false;
	}
	ssize_t n = pread(file_obj, buffer, file_info.st_size, 0);
	(void)n;
	posix_fadvise(file_obj, 0, 0, POSIX_FADV_DONTNEED);
	close(file_obj);

	data.assign((const char *)buffer, file_info.st_size);
	free(buffer);

	return true;
}

bool read_json_file(json &data, const string &filename) {
	string content;
	if (read_raw_file(content, filename)) {
		try {
			data = json::parse(content);
			return true;
		}
		catch (const exception &e) {
			(void)e;
			SYS_DBG("json::parse()\n");
		}
	}
	return false;
}

static size_t write_string_function(void *contents, size_t size, size_t nmemb, void *userp) {
	((string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

static size_t write_file_function(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

bool download_json(const string &link, json &response, unsigned long timeout) {
	CURL *curl_handle;
	string http_respone_string;
	long http_response_code = 0;
	bool return_state		= false;

	/* Initialise the library */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);

	// curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl_handle, CURLOPT_URL, link.data());
	if (timeout > 0) {
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
	}
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_string_function);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &http_respone_string);

	CURLcode curl_code = curl_easy_perform(curl_handle);
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_response_code);

	if (http_response_code == 200 && curl_code != CURLE_ABORTED_BY_CALLBACK) {
		try {
			response	 = json::parse(http_respone_string.data());
			return_state = true;
		}
		catch (const exception &e) {
			(void)e;
			SYS_DBG("json::parse(): %s\n", http_respone_string.data());
		}
	}
	else {
		SYS_DBG("http_response_code: %ld\n", http_response_code);
		SYS_DBG("link: %s\n", link.data());
		SYS_DBG("error: %s\n", curl_easy_strerror(curl_code));
	}

	curl_easy_cleanup(curl_handle);
	// curl_global_cleanup();

	return return_state;
}

bool download_file(const string &link, const string &filename, unsigned long timeout) {
	CURL *curl_handle;
	FILE *file;
	long http_response_code = 0;
	bool return_state		= false;

	/* Initialise the library */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);

	// curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl_handle, CURLOPT_URL, link.data());
	if (timeout > 0) {
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
	}
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file_function);

	file = fopen(filename.data(), "wb");
	if (file) {
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);

		CURLcode curl_code = curl_easy_perform(curl_handle);
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_response_code);

		if (http_response_code == 200 && curl_code != CURLE_ABORTED_BY_CALLBACK) {
			return_state = true;
		}
		else {
			SYS_DBG("http_response_code: %ld\n", http_response_code);
			SYS_DBG("link: %s\n", link.data());
			SYS_DBG("error: %s\n", curl_easy_strerror(curl_code));
		}

		fclose(file);
	}

	curl_easy_cleanup(curl_handle);
	// curl_global_cleanup();

	return return_state;
}

long download_file_with_data_req(const string &link, const string &filename, string &datasend, unsigned long timeout) {
	CURL *curl_handle;
	FILE *file;
	long http_response_code = 0;
	long return_state		= -1;

	/* Initialise the library */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);

	// curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl_handle, CURLOPT_URL, link.data());

	/* size of the POST data */
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, datasend.length());

	/* pass in a pointer to the data - libcurl will not copy */
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, datasend.data());

	if (timeout > 0) {
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
	}
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file_function);

	file = fopen(filename.data(), "wb");
	if (file) {
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);

		CURLcode curl_code = curl_easy_perform(curl_handle);
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_response_code);

		if (http_response_code == 200 && curl_code != CURLE_ABORTED_BY_CALLBACK) {
			/* OK nothing */
		}
		else {
			SYS_DBG("http_response_code: %ld\n", http_response_code);
			SYS_DBG("link: %s\n", link.data());
			SYS_DBG("error: %s\n", curl_easy_strerror(curl_code));
		}
		return_state = http_response_code;
		fclose(file);
	}

	curl_easy_cleanup(curl_handle);
	// curl_global_cleanup();

	return return_state;
}

void erase_all_string(string &main_string, string &erase_string) {
	size_t pos = string::npos;
	while ((pos = main_string.find(erase_string)) != string::npos) {
		main_string.erase(pos, erase_string.length());
	}
}

void replace_all_string(string &main_string, string &find_string, string &repl_string) {
	size_t pos = string::npos;
	while ((pos = main_string.find(find_string)) != string::npos) {
		main_string.replace(pos, find_string.length(), repl_string);
	}
}

void bytetoHexChar(uint8_t ubyte, uint8_t *uHexChar) {
	uHexChar[1] = ((ubyte & 0x0F) < 10) ? ((ubyte & 0x0F) + '0') : (((ubyte & 0x0F) - 10) + 'a');
	uHexChar[0] = ((ubyte >> 4 & 0x0F) < 10) ? ((ubyte >> 4 & 0x0F) + '0') : (((ubyte >> 4 & 0x0F) - 10) + 'a');
}

void bytestoHexChars(uint8_t *ubyte, int32_t len, uint8_t *uHexChar) {
	for (int8_t i = 0; i < len; i++) {
		bytetoHexChar(ubyte[i], (uint8_t *)&uHexChar[i * 2]);
	}
}

void hexChartoByte(uint8_t *uHexChar, uint8_t *ubyte) {
	*ubyte = 0;
	*ubyte = ((uHexChar[0] <= '9' && uHexChar[0] >= '0') ? ((uHexChar[0] - '0') << 4) : *ubyte);
	*ubyte = ((uHexChar[0] <= 'f' && uHexChar[0] >= 'a') ? ((uHexChar[0] - 'a' + 10) << 4) : *ubyte);

	*ubyte = ((uHexChar[1] <= '9' && uHexChar[1] >= '0') ? *ubyte | (uHexChar[1] - '0') : *ubyte);
	*ubyte = ((uHexChar[1] <= 'f' && uHexChar[1] >= 'a') ? *ubyte | ((uHexChar[1] - 'a') + 10) : *ubyte);
}

void hexCharsToBytes(uint8_t *uHexChar, int32_t len, uint8_t *ubyte) {
	for (int8_t i = 0; i < len; i += 2) {
		hexChartoByte((uint8_t *)&uHexChar[i], (uint8_t *)&ubyte[i / 2]);
	}
}

int get_net_info(const char *ifname, string &ip, string &mac) {
	string cmd = "ifconfig " + string(ifname) + " | grep \"inet addr\" | cut -d ':' -f 2 | cut -d ' ' -f 1";
	ip		   = systemStrCmd("ifconfig %s | grep \"inet addr\" | cut -d ':' -f 2 | cut -d ' ' -f 1", ifname);
	mac		   = systemStrCmd("cat /sys/class/net/%s/address", ifname);
	ip.erase(ip.find_last_not_of("\n") + 1);
	mac.erase(mac.find_last_not_of("\n") + 1);
	return 0;
}

bool isLocalIP(const string &ipAddress) {
	stringstream ss(ipAddress);
	vector<int> ipParts;
	string part;
	while (getline(ss, part, '.')) {
		ipParts.push_back(stoi(part));
	}

	if (ipParts.size() == 4) {
		if (ipParts[0] == 127 || ipParts[0] == 10 || (ipParts[0] == 172 && ipParts[1] >= 16 && ipParts[1] <= 31) || (ipParts[0] == 192 && ipParts[1] == 168)) {
			return true;
		}
	}
	return false;
}

bool isIpValid(const string &ipAddress) {
	stringstream ss(ipAddress);
	vector<int> ipParts;
	string part;

	try {
	while (getline(ss, part, '.')) {
		ipParts.push_back(stoi(part));
		}
	}
	catch (const std::exception &e) {
		APP_DBG("Invalid IP address format: %s\n", e.what());
		return false;
	}

	for (size_t i = 0; i < ipParts.size(); ++i) {
		if (ipParts[i] < 0 || ipParts[i] > 255) {
			return false;
		}
	}

	return (ipParts.size() == 4) ? true : false;
}

string get_md5_file(const char *path) {
	string md5 = systemStrCmd("md5sum %s | awk '{ print $1 }'", path);
	md5.erase(md5.find_last_not_of("\n") + 1);
	return md5;
}

int get_list_music(const char *path, json &list) {
	DIR *dir;
	struct dirent *ent;

	int song_index = 0;

	/* clean list */
	list = json::array();

	if ((dir = opendir(path)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			int len = strlen(ent->d_name);
			if ((len > 4) && ((ent->d_name[len - 1] == '3' && (ent->d_name[len - 2] == 'p' || ent->d_name[len - 2] == 'P') &&
							   (ent->d_name[len - 3] == 'm' || ent->d_name[len - 3] == 'M') && ent->d_name[len - 4] == '.') ||
							  ((ent->d_name[len - 1] == 'v' || ent->d_name[len - 2] == 'V') && (ent->d_name[len - 2] == 'a' || ent->d_name[len - 2] == 'A') &&
							   (ent->d_name[len - 3] == 'w' || ent->d_name[len - 3] == 'W') && ent->d_name[len - 4] == '.'))) {
				SYS_DBG("song[%d] : [%d]%s\n", song_index, len, ent->d_name);
				song_index++;

				list.push_back(string(ent->d_name));
			}
		}

		closedir(dir);
	}
	return song_index;
}

#define APP_GET_DISK_USAGE		   "df /mnt/sda1 | sed -n '2 p' | awk '{ print $3}'"
#define APP_GET_DISK_USAGE_ENCRYPT "df /mnt/sda1 | sed -n '3 p' | awk '{ print $2}'"
uint32_t get_usb_usage(const char *path) {
	FILE *fp	= NULL;
	uint8_t ret = 0;
	char resp[20];

	struct stat st = {0};
	if (stat(path, &st) == 0) {
		memset(resp, 0, 20);

		if (check_usb_encrypted("/etc/hotplug.d/block/80-decrypt-external-drive")) {
			fp = popen(APP_GET_DISK_USAGE_ENCRYPT, "r");
		}
		else {
			fp = popen(APP_GET_DISK_USAGE, "r");
		}

		if (fp == NULL) {
			SYS_DBG("Failed to run command\n");
		}
		else {
			while (fgets(resp, sizeof(resp) - 1, fp) != NULL) {
				ret = 1;
				SYS_DBG("%s", resp);
			}
		}

		/* close */
		pclose(fp);
	}

	if (ret == 0 || strlen(resp) == 1) {
		return 0;
	}

	return stoi(string(resp, strlen(resp) - 1));
}

#define APP_GET_DISK_AVAI		  "df /mnt/sda1 | sed -n '2 p' | awk '{ print $4}'"
#define APP_GET_DISK_AVAI_ENCRYPT "df /mnt/sda1 | sed -n '3 p' | awk '{ print $3}'"
uint32_t get_usb_remain(const char *path) {
	FILE *fp	= NULL;
	uint8_t ret = 0;
	char resp[20];

	struct stat st = {0};
	if (stat(path, &st) == 0) {
		memset(resp, 0, 20);

		if (check_usb_encrypted("/etc/hotplug.d/block/80-decrypt-external-drive")) {
			fp = popen(APP_GET_DISK_AVAI_ENCRYPT, "r");
		}
		else {
			fp = popen(APP_GET_DISK_AVAI, "r");
		}

		if (fp == NULL) {
			SYS_DBG("Failed to run command\n");
		}
		else {
			while (fgets(resp, sizeof(resp) - 1, fp) != NULL) {
				ret = 1;
				SYS_DBG("%s", resp);
			}
		}

		/* close */
		pclose(fp);
	}

	if (ret == 0 || strlen(resp) == 1) {
		return 0;
	}

	return stoi(string(resp, strlen(resp) - 1));
}

bool check_usb_encrypted(const char *script) {
	bool ret = false;
	if (access(script, F_OK) != -1) {
		ret = true;
	}
	else {
		ret = false;
	}
	return ret;
}

int touch_full_file(const char *file_name) {
	int readed_size, read_size;
	uint8_t temp_data[256];
	struct stat file_info;
	int file_name_f = -1;

	file_name_f = open(file_name, O_RDONLY);
	if (file_name_f < 0) {
		return -1;
	}

	fstat(file_name_f, &file_info);

	for (readed_size = 0; readed_size < static_cast<int>(file_info.st_size); readed_size += 256) {
		if (readed_size <= file_info.st_size) {
			read_size = 256;
		}
		else {
			read_size = file_info.st_size - (readed_size - 256);
		}

		tmp = pread(file_name_f, &temp_data, read_size, readed_size);
	}

	(void)temp_data;

	close(file_name_f);
	return 0;
}

bool doesFileExist(const char *filename) {
	return (access(filename, F_OK) == 0) ? true : false;
}

int exec_prog(int timeout_sec, const char **argv) {
	pid_t my_pid;

	if (0 == (my_pid = fork())) {
		if (-1 == execve(argv[0], (char **)argv, NULL)) {
			printf("[exec_prog] child process execve failed [%m]\n");
			return -1;
		}
	}

#define WAIT_FOR_COMPLETION
#ifdef WAIT_FOR_COMPLETION
	int status;

	while (0 == waitpid(my_pid, &status, WNOHANG)) {
		if (--timeout_sec < 0) {
			printf("[exec_prog] timeout");
			return -1;
		}
		sleep(1);
	}

	printf("[exec_prog] %s WEXITSTATUS %d WIFEXITED %d [status %d]\n", (const char *)argv[0], (int)WEXITSTATUS(status), (int)WIFEXITED(status), status);

	if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
		printf("[exec_prog] %s failed, halt system\n", (const char *)argv[0]);
		return -1;
	}

#endif
	return 0;
}

vector<string> string_split(string &s, string delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	string token;
	vector<string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
		token	  = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		if (token.length()) {
			res.push_back(token);
		}
	}

	res.push_back(s.substr(pos_start));
	return res;
}

size_t sizeOfFile(const char *url) {
	struct stat fStat;
	int fd = -1;

	fd = open(url, O_RDONLY);
	if (fd < 0) {
		return 0;
	}
	fstat(fd, &fStat);
	close(fd);

	return fStat.st_size;
}

std::string systemStrCmd(const char *fmt, ...) {
	std::array<char, 128> buffer;
	std::string result = "";
	char cmd[256];
	memset(cmd, 0, sizeof(cmd));
	va_list args;
	va_start(args, fmt);
	vsprintf(cmd, fmt, args);
	va_end(args);
	APP_DBG_CMD("cmd: %s\n", cmd);
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

	if (!pipe) {
		// throw std::runtime_error("popen() failed!");
		APP_DBG("popen() failed!");
		return "";
	}

	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	result.erase(result.find_last_not_of("\n") + 1);
	return result;
}

char *safe_strcpy(char *dest, const char *src, size_t dest_size) {
	strncpy(dest, src, dest_size - 1);	  // Leave space for null terminator
	dest[dest_size - 1] = '\0';			  // Ensure null termination
	return dest;
}

void addNewlineToString(std::string &str, const std::string &newString) {
	if (!newString.empty()) {
		if (!str.empty()) {
			str += "\n" + newString;
		}
		else {
			str = newString;
		}
	}
}

string getFileName(const string &s) {
	char sep = '/';
	size_t i = s.rfind(sep, s.length());
	if (i != string::npos) {
		return (s.substr(i + 1, s.length() - i));
	}

	return ("");
}

bool appendStringToFile(const char *path, const string str) {
	FILE *file = fopen(path, "a");
	if (!file) {
		APP_DBG("Cannot open file: %s\n", path);
		return false;
	}

	if (fwrite(str.c_str(), sizeof(char), str.size(), file) != str.size()) {
		APP_DBG("Failed to write to file: %s\n", path);
		fclose(file);
		return false;
	}

	fputc('\n', file);
	fclose(file);
	return true;
}

string getLinesNotContainingWord(const char *path, const string str) {
	std::ifstream resolvFile(path);
	std::string line, content = "";
	while (std::getline(resolvFile, line)) {
		if (line.find(str) == std::string::npos) {
			content += line + "\n";
		}
	}
	resolvFile.close();
	return content;
}

// Function to check if a string ends with another string
bool endsWith(const string &fullString, const string &ending) {
	// Check if the ending string is longer than the full
	// string
	if (ending.size() > fullString.size())
		return false;

	// Compare the ending of the full string with the target
	// ending
	return fullString.compare(fullString.size() - ending.size(), ending.size(), ending) == 0;
}

// Function to apply XOR operation between the message and a key
std::string xorEncryptDecrypt(const std::string &text, const std::string &key) {
	std::string result = text;	  // Start with the original text

	for (size_t i = 0; i < text.length(); ++i) {
		result[i] = text[i] ^ key[i % key.length()];	// Apply XOR with the key, cycling through the key if necessary
	}

	return result;
}

int CURL_Upload(const std::string cer, const std::string url, const std::string file, string &http_respone_string) {
	CURL *curl;
	CURLcode res;
	struct stat fStat;
	curl_off_t speed, totalTime;
	FILE *fp = NULL;
	int ret	 = -1;

	fp = fopen(file.c_str(), "rb");
	if (!fp) {
		return -1;
	}

	if (fstat(fileno(fp), &fStat) != 0) {
		fclose(fp);
		return -1;
	}

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		if (!cer.empty()) {
			curl_easy_setopt(curl, CURLOPT_CAINFO, cer.c_str());
		}
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_READDATA, fp);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fStat.st_size);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_function);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http_respone_string);

		/* enable verbose for easier tracing */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			APP_DBG("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else {
			curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &speed);
			curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &totalTime);

			APP_DBG("Speed: %lu bytes/sec during %lu.%06lu seconds\n", (unsigned long)speed, (unsigned long)(totalTime / 1000000), (unsigned long)(totalTime % 1000000));

			long http_ret;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_ret);
			if (http_ret == 200 || http_ret == 201) {
				ret = 0;
			}
			APP_DBG("curl ret: %ld, data: %s\n", http_ret, http_respone_string.data());
		}
	}
	curl_easy_cleanup(curl);
	fclose(fp);

	return ret;
}

string getLastLine(const string &filename) {
	ifstream file(filename, ios::ate);
	if (!file.is_open()) {
		APP_DBG("Cannot open file: %s\n", filename.c_str());
		return "";
	}

	// Start at the end of the file
	file.seekg(-1, ios::end);
	char ch;

	// find the last line
	while (file.tellg() > 1) {
		file.seekg(-2, ios::cur);
		file.get(ch);
		if (ch == '\n') {
			break;
		}
	}

	string last_line;
	getline(file, last_line);
	file.close();
	return last_line;
}

uint64_t getTimeMs() /* get ms in clock */ {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

bool check_env(const char *env_name) {
	string cmd		 = "fw_printenv " + string(env_name) + " | cut -d '=' -f 2";
	string env_value = systemStrCmd(cmd.c_str());
	APP_DBG("env_value: %s\n", env_value.c_str());
	return (env_value == "1");
}

std::string reverseUTCOffset(const std::string &utc) {
	if (utc.size() < 4 || utc.substr(0, 3) != "UTC") {
		return utc;
	}

	std::string result = "UTC";
	char sign		   = utc[3];
	if (sign == '+') {
		result += '-';
	}
	else if (sign == '-') {
		result += '+';
	}
	else {
		return utc;
	}

	result += utc.substr(4);
	return result;
}

float get_memory_usage_percent() {
	FILE *fp = fopen("/proc/meminfo", "r");
	if (!fp) {
		perror("Failed to open /proc/meminfo");
		return -1;
	}

	char line[256];
	long total = 0, free = 0, buffers = 0, cached = 0;

	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "MemTotal: %ld kB", &total) == 1)
			continue;
		if (sscanf(line, "MemFree: %ld kB", &free) == 1)
			continue;
		if (sscanf(line, "Buffers: %ld kB", &buffers) == 1)
			continue;
		if (sscanf(line, "Cached: %ld kB", &cached) == 1)
			continue;
	}

	fclose(fp);

	long used = total - free - buffers - cached;
	return (float)used / total * 100.0;
}

#define FCA_LINE_MAX 256
void update_host(const char *hosts_file, const char *host, const char *ip) {
	int found = 0;
	// tmp file
	char temp_file[] = "/tmp/hosts.tmp";
	FILE *in = fopen(hosts_file, "r"), *out = fopen(temp_file, "w");
	if (!in || !out) {
		printf("Error opening files: %s or %s\n", hosts_file, temp_file);
		return;
	}
	char line[FCA_LINE_MAX], ipbuf[FCA_LINE_MAX], hostbuf[FCA_LINE_MAX];
	while (fgets(line, sizeof(line), in)) {
		if (line[0] == '#' || sscanf(line, "%255s %255s", ipbuf, hostbuf) != 2) {
			fputs(line, out);
			continue;
		}
		if (!found && strcmp(host, hostbuf) == 0) {
			fprintf(out, "%s\t%s\n", ip, host);
			found = 1;
		}
		else {
			fputs(line, out);
		}
	}
	if (!found)
		fprintf(out, "%s\t%s\n", ip, host);
	fclose(in);
	fclose(out);
	systemCmd("mv %s %s", temp_file, hosts_file);
}

void get_hostname_from_url(const char *url, char *out, size_t outlen) {
	const char *start = strstr(url, "://");
	if (start)
		start += 3;
	else
		start = url;
	const char *end = strchr(start, '/');
	size_t len		= end ? (size_t)(end - start) : strlen(start);
	if (len >= outlen)
		len = outlen - 1;
	strncpy(out, start, len);
	out[len] = '\0';
}

int add_line_not_exist_to_file(const char *path, const string &line) {
	if (line.empty()) {
		APP_DBG("Line is empty, nothing to add.\n");
		return -1;
	}

	std::string cur_line;
	std::ifstream fileIn(path);
	if (fileIn.is_open()) {
		while (std::getline(fileIn, cur_line)) {
			if (line == cur_line) {
				APP_DBG("Line already exists in file: %s\n", line.c_str());
				fileIn.close();
				return 0;
			}
		}
		fileIn.close();
	}
	return appendStringToFile(path, line) ? 0 : -1;	// If the line does
}


int systemCmd(const char *fmt, ...) {
	if (fmt == NULL) {
		APP_DBG_CMD("cmd is null\n");
		return -1;
	}

	FILE *fp;
	int ret = 0;
	char buf[256];
	memset(buf, 0, sizeof(buf));
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	APP_DBG_CMD("cmd: %s\n", buf);

	if ((fp = popen(buf, "r")) == NULL) {
		perror("popen");
		APP_DBG_CMD("popen error: %s\n", strerror(errno));
		return -1;
	}

	ret = pclose(fp);
	if (ret == -1) {
		APP_DBG_CMD("close open file pointer fp error\n");
		return ret;
	}
	else if (ret == 0) {
		return ret;
	}
	else {
		APP_DBG_CMD("popen ret is: %d\n", ret);
		return ret;
	}
}

int resolvDomain(const char *domain, uint8_t ip4[4], char ip4Text[16]) {
    int status;
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(domain, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo() return %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        if (ip4) {
            memcpy(ip4, &ipv4->sin_addr, 4);
        }
        if (ip4Text) {
            inet_ntop(AF_INET, &ipv4->sin_addr, ip4Text, 16);
        }
        freeaddrinfo(res);
        return 0;
    }

    freeaddrinfo(res);
    return -2;
}