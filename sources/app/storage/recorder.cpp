#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <iostream>

#include "recorder.h"

#if 0
#define RC_DBG(fmt, ...) 	printf("\x1B[36m[RECORDER]\x1B[0m " fmt, ##__VA_ARGS__)
#else
#define RC_DBG(fmt, ...)
#endif

Recorder::Recorder() {
    
}

Recorder::~Recorder() {
    close(time(NULL));
}

void Recorder::initialise(std::string folder, uint32_t ts, RECORDER_TYPE type, bool isVideo) {
    char chars[64];

    /* Convert epoch time (i.e., time_t, seconds since Jan 1, 1970) into a local time */
    struct tm *tmPtr = localtime((time_t*)&ts);
	tmPtr->tm_year += 1900;
	tmPtr->tm_mon += 1;
	memcpy(&mTm, tmPtr, sizeof(struct tm));

    /* Generate recorder description format */
    if (type == RECORD_TYPE_REG) mTypeStr = RECORD_ST_TYPE_REG;
    else if (type == RECORD_TYPE_MDT) mTypeStr = RECORD_ST_TYPE_MDT;
    else mTypeStr = RECORD_ST_TYPE_HMD;

    mSfxStr = (isVideo) ? RECORD_VID_SUFFIX : RECORD_AUD_SUFFIX;
    mStartTs = ts;

    memset(chars, 0, sizeof(chars));
    (void)snprintf(chars, sizeof(chars), RECORD_DESC_TEMPORARY_FORMAT, 
        mTm.tm_year, mTm.tm_mon, mTm.tm_mday, mTm.tm_hour, mTm.tm_min, mTm.tm_sec,
        mStartTs, mTypeStr.c_str(), mSfxStr.c_str()
    );
    
    mDesc.assign(std::string(chars, strlen(chars)));
    mFolder.assign(folder);
}

void Recorder::changeType(RECORDER_TYPE type) {
    if (mFolder.empty() || mDesc.empty() || type == RECORD_TYPE_INI) {
        return;
    }

    if (type == RECORD_TYPE_REG) mTypeStr = RECORD_ST_TYPE_REG;
    else if (type == RECORD_TYPE_MDT) mTypeStr = RECORD_ST_TYPE_MDT;
    else mTypeStr = RECORD_ST_TYPE_HMD;

    char chars[64];
    memset(chars, 0, sizeof(chars));
    (void)snprintf(chars, sizeof(chars), RECORD_DESC_TEMPORARY_FORMAT, 
        mTm.tm_year, mTm.tm_mon, mTm.tm_mday, mTm.tm_hour, mTm.tm_min, mTm.tm_sec,
        mStartTs, mTypeStr.c_str(), mSfxStr.c_str()
    );

    const std::string srcPath = mFolder + std::string("/") + mDesc;
    const std::string desPath = mFolder + std::string("/") + std::string(chars, strlen(chars));
    rename(srcPath.c_str(), desPath.c_str());
    mDesc.assign(std::string(chars, strlen(chars)));

    RC_DBG("[CHANGE-TYPE] %s -> %s\r\n", srcPath.c_str(), desPath.c_str());
}

int Recorder::Start(uint8_t *dataPtr, uint32_t u32Len) {
	const std::string fullPath = mFolder + std::string("/") + mDesc;

	int fd = open(fullPath.c_str(), O_RDWR | O_CREAT | O_DSYNC, 0777);
	if (fd >= 0) {
		if (dataPtr && u32Len > 0) {
			write(fd, dataPtr, u32Len);
		}
		close(fd);
		return 0;
	}

	return -1;
}

int Recorder::Write(uint8_t *dataPtr, uint32_t u32Len) {
    if (!dataPtr || u32Len == 0) {
        return 0;
    }

    if (mDesc.empty() || mFolder.empty()) {
		return 0;
	}
    const std::string fullPath = mFolder + std::string("/") + mDesc;

    int fd = open(fullPath.c_str(), O_WRONLY | O_APPEND | O_DSYNC, 0777);
    if (fd >= 0) {
        size_t r = write(fd, dataPtr, u32Len);

		/* This line tell the kernel that buffers is no need to keep, remove it from cache memory */
		posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
        close(fd);
		if (r == u32Len) {
			return 0;
		}
    }

	return -1;
}

int Recorder::Close(uint32_t ts) {
    if (mDesc.empty() || mFolder.empty()) {
        return -1;
    }

    char chars[64];
    memset(chars, 0, sizeof(chars));
    snprintf(chars, sizeof(chars), RECORD_DESC_COMPLETED_FORMAT, 
            mTm.tm_year, mTm.tm_mon, mTm.tm_mday, mTm.tm_hour, mTm.tm_min, mTm.tm_sec,
            mStartTs, ts, mTypeStr.c_str(), mSfxStr.c_str()
    );

    std::string result;
    result.assign(std::string(chars, strlen(chars)));
    std::string srcPath = mFolder + std::string("/") + mDesc;
    std::string desPath = mFolder + std::string("/") + result;
    int r = rename(srcPath.c_str(), desPath.c_str());
    RC_DBG("[CLOSE] %s\r\n", desPath.c_str());
    
    mDesc.clear();
    mSfxStr.clear();
    mFolder.clear();
    mTypeStr.clear();
    memset(&mTm, 0, sizeof(mTm));
    return r;
}

static uint32_t lastModifiedTs(std::string pathStr) {
	struct stat fstat;
	if (stat(pathStr.c_str(), &fstat) != 0) {
		return 0;
	}

	if (fstat.st_size > 0) {
		return (uint32_t)(fstat.st_mtime);
	}
	return 0;
}

uint32_t Recorder::recover(std::string folder, std::string& brokenStr, uint32_t inputTs) {
    std::string srcPath = folder + std::string("/") + brokenStr;

    if (access(srcPath.c_str(), F_OK) != 0) {
        RC_DBG("%s doesn't exist\r\n", srcPath.c_str());
        return 0;
    }

    struct tm Tm;
    uint32_t startTs, stopTs = 0;
    char sfxStr[5], typStr[4];
    memset(sfxStr, 0, sizeof(sfxStr));
    memset(typStr, 0, sizeof(typStr));

    int r = sscanf(brokenStr.c_str(), "%04d%02d%02d%02d%02d%02d_%d_%[^.]%s",
            &Tm.tm_year, &Tm.tm_mon, &Tm.tm_mday, &Tm.tm_hour, &Tm.tm_min, &Tm.tm_sec, 
            &startTs, typStr, sfxStr);

    if (r == 9) {
        char chars[64];
        memset(chars, 0, sizeof(chars));
        stopTs = (inputTs == 0) ? lastModifiedTs(srcPath) : inputTs;
        snprintf(chars, sizeof(chars), RECORD_DESC_COMPLETED_FORMAT, 
                Tm.tm_year, Tm.tm_mon, Tm.tm_mday, Tm.tm_hour, Tm.tm_min, Tm.tm_sec,
                startTs, stopTs, typStr, sfxStr
        );

        std::string desPath = folder + std::string("/") + std::string(chars, strlen(chars));
        rename(srcPath.c_str(), desPath.c_str());
        RC_DBG("[RECOVER] \"%s\" into \"%s\"\r\n", srcPath.c_str(), desPath.c_str());
        brokenStr.assign(std::string(chars, strlen(chars)));
    }
    return stopTs;
}

bool Recorder::parser(std::string descStr, struct tm *tmPtr, uint32_t *beginTsPtr, uint32_t *endTsPtr, int *type) {
    if (descStr.empty()) {
        return false;
    }

    char sfx[5], typeStr[4];
    memset(sfx, 0, sizeof(sfx));
    memset(typeStr, 0, sizeof(typeStr));

    int r = sscanf(descStr.c_str(), RECORD_DESC_PARSER_FORMATTER,
            &tmPtr->tm_year, &tmPtr->tm_mon, &tmPtr->tm_mday, &tmPtr->tm_hour, &tmPtr->tm_min, &tmPtr->tm_sec, 
            beginTsPtr, endTsPtr, typeStr, sfx);

    if (r == 10) {
        if (strcmp(typeStr, RECORD_ST_TYPE_REG) == 0) {
            *type = (int)RECORD_TYPE_REG;
        }
        else if (strcmp(typeStr, RECORD_ST_TYPE_MDT) == 0) {
            *type = (int)RECORD_TYPE_MDT;
        }
        else {
            *type = (int)RECORD_TYPE_HMD;
        }
        return true;
    }
    return false;
}

