#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statfs.h>

#include "SdCard.h"

#if 0
#define SD_DBG(fmt, ...) 	printf("\x1B[36m[SD]\x1B[0m " fmt, ##__VA_ARGS__)
#else
#define SD_DBG(fmt, ...)
#endif

static std::string utlsTimestampToFormatted(uint32_t ts, char *formatted);

SdCard sdCard;

pthread_mutex_t SdCard::operMutex = PTHREAD_MUTEX_INITIALIZER;

SdCard::SdCard() {
	struct stat fStat;
	if (stat(DEFAULT_MOUNTPOINT, &fStat) == -1) {
		mkdir(DEFAULT_MOUNTPOINT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
}

SdCard::~SdCard() {
	pthread_mutex_destroy(&SdCard::operMutex);
}

/* ------------------------------ SD Operation ------------------------------ */
int SdCard::doOPER_Format() {
	int ret = SD_OPER_FAILURE;

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState != SD_STATE_REMOVED) {
		char cmd[256];
		memset(cmd, 0, sizeof(cmd));

		if (doOPER_Unmount() == SD_OPER_SUCCESS) {
			snprintf(cmd, sizeof(cmd), "mkfs.%s %s", DEFAULT_TYPE_FORMAT, mHardDisk);
		}
		ret = system(cmd) == 0 ? SD_OPER_SUCCESS : SD_OPER_FAILURE;
		mState = SD_STATE_REMOVED;
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

int SdCard::doOPER_GetCapacity(uint64_t *total, uint64_t *free, uint64_t *used) {
	int ret = SD_OPER_FAILURE;

	*total = *free = *used = 0;

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState == SD_STATE_MOUNTED) {
		struct statfs statFs;
		if (statfs(mMountPoint, &statFs) == 0) {
			size_t blockSize = (uint64_t)statFs.f_bsize;
			*total			 = (uint64_t)blockSize * (uint64_t)statFs.f_blocks;
			*free			 = (uint64_t)blockSize * (uint64_t)statFs.f_bfree;
			*used			 = (uint64_t)blockSize * ((uint64_t)statFs.f_blocks - (uint64_t)statFs.f_bfree);
			ret				 = SD_OPER_SUCCESS;
		}
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

void SdCard::scan() {
	pthread_mutex_lock(&SdCard::operMutex);

	mState = SD_STATE_REMOVED;

	if (access(mHardDisk, F_OK) == 0) {
		mState = SD_STATE_INSERTED;
		if (hasMounted()) {
			mState = SD_STATE_MOUNTED;
		}
		else {
			if (doOPER_Mount() == SD_OPER_SUCCESS) {
				mState = SD_STATE_MOUNTED;
			}
		}
	}

	pthread_mutex_unlock(&SdCard::operMutex);
}

SD_STATUS SdCard::currentState() {
	pthread_mutex_lock(&SdCard::operMutex);

	SD_STATUS ret = mState;

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

RECORDER_TYPE SdCard::recorderCurType() {
	pthread_mutex_lock(&SdCard::operMutex);

	RECORDER_TYPE ret = mType;

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

uint32_t SdCard::recorderStartTs() {
	pthread_mutex_lock(&SdCard::operMutex);

	uint32_t ts = mU3StartTS;

	pthread_mutex_unlock(&SdCard::operMutex);

	return ts;
}

void SdCard::recorderChangeType(RECORDER_TYPE typeChanged) {
	pthread_mutex_lock(&SdCard::operMutex);

	if (mType != typeChanged) {
		mType = typeChanged;
		mVideo.changeType(mType);
		mAudio.changeType(mType);
	}

	pthread_mutex_unlock(&SdCard::operMutex);
}

SD_OPER_RECORDING_STATUS SdCard::recorderCurOPERState() {
	pthread_mutex_lock(&SdCard::operMutex);

	SD_OPER_RECORDING_STATUS ret = fOperState;

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

static int countingRecorder(std::string pathStr, RECORDER_TYPE type) {
    FILE *fp;
    char result[5];
    char chars[96];

    std::string typeStr;
    if (type == RECORD_TYPE_REG) typeStr = RECORD_ST_TYPE_REG;
    else if (type == RECORD_TYPE_MDT) typeStr = RECORD_ST_TYPE_MDT;
    else typeStr = RECORD_ST_TYPE_HMD;

    memset(chars, 0, sizeof(chars));
    memset(result, 0, sizeof(result));
    snprintf(chars, sizeof(chars), "find %s -type f -name '*%s*' | wc -l", pathStr.c_str(), typeStr.c_str());

    fp = popen(chars, "r");
    if (fp != NULL) {
        fgets(result, sizeof(result), fp);
        pclose(fp);
        return atoi(result);
    }
    return 0;
}

void SdCard::loadingStatistic() {
	pthread_mutex_lock(&SdCard::operMutex);

	if (mState != SD_STATE_MOUNTED) {
		pthread_mutex_unlock(&SdCard::operMutex);
		return;
	}

	const std::string videoFolderPath = std::string(mMountPoint) + std::string("/video");
	DIR *dir = opendir(videoFolderPath.c_str());
	if (dir != NULL) {
		struct dirent *ent;
		while ((ent = readdir(dir)) != NULL) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
				continue;
			}

			const std::string subFolerStr = std::string(ent->d_name, strlen(ent->d_name));
			const std::string subFolderPathStr = videoFolderPath + std::string("/") + subFolerStr;

			SD_DBG("Sub folder path: %s\r\n", subFolderPathStr.c_str());

			RecorderCounter rcCounter = {
				.regCounts = countingRecorder(subFolderPathStr, RECORD_TYPE_REG),
				.hmdCounts = countingRecorder(subFolderPathStr, RECORD_TYPE_HMD),
				.mdtCounts = countingRecorder(subFolderPathStr, RECORD_TYPE_MDT),
			};
			mStatistic.emplace(subFolerStr, rcCounter);
		}
		closedir(dir);
	}

	SD_DBG("mStatistic.size() = %d\r\n", mStatistic.size());

	for (auto it : mStatistic) {
		SD_DBG("%s [REG: %d][MDT: %d][HMD: %d]\r\n", it.first.c_str(), it.second.regCounts, it.second.mdtCounts, it.second.hmdCounts);
	}
	
	pthread_mutex_unlock(&SdCard::operMutex);

}

void SdCard::erasingStatistic() {
	pthread_mutex_lock(&SdCard::operMutex);

	mStatistic = {};

	pthread_mutex_unlock(&SdCard::operMutex);
}

const std::unordered_map<std::string, RecorderCounter> SdCard::getStatistic() {
	std::unordered_map<std::string, RecorderCounter> ret = {};

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState == SD_STATE_MOUNTED) {
		ret = mStatistic;
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

static bool bAskFilesFormatExisted(std::string pathStr, std::string fmtStr) {
    char chars[128];
    memset(chars, 0, sizeof(chars));
    sprintf(chars, "find %s -type f -name \"*%s*\" -print -quit 2>/dev/null | grep -q .", 
            pathStr.c_str(), fmtStr.c_str());

    int r = system(chars);
    return (r == 0); /* Return 0 means existed */
}

uint32_t SdCard::u32MaskCalenderRecorder(RECORDER_TYPE qrTypeMask, int qrYear, int qrMonth) {
	uint32_t u32Mask = 0;

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState == SD_STATE_MOUNTED) {
		const std::string videoFolderPath = std::string(mMountPoint) + std::string("/video");
		DIR *d = opendir(videoFolderPath.c_str());
		if (d != NULL) {
			struct dirent *e;
			while ((e = readdir(d)) != NULL) {
				if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
					continue;
				}

				const RECORDER_TYPE arrType[] = {
					RECORD_TYPE_REG,
					RECORD_TYPE_MDT,
					RECORD_TYPE_HMD,
				};
				const std::string arrTypeStr[] = {
					RECORD_ST_TYPE_REG,
					RECORD_ST_TYPE_MDT,
					RECORD_ST_TYPE_HMD,
				};
				const std::string subFolerStr = std::string(e->d_name, strlen(e->d_name));
				const std::string subFolderPathStr = videoFolderPath + std::string("/") + subFolerStr;

				for (uint8_t id = 0; id < sizeof(arrType) / sizeof(arrType[0]); ++id) {
					if (qrTypeMask & arrType[id]) {
						if (bAskFilesFormatExisted(subFolderPathStr, arrTypeStr[id])) {
							/* PARSER: Year, month, day */
							int y = 0, m = 0, d = 0;
							sscanf(subFolerStr.c_str(), "%d.%2d.%2d", &y, &m, &d);
							if (y == qrYear && m == qrMonth) {
								u32Mask |= (1 << (d - 1));
							}
						}
					}
				}
			}
			closedir(d);
		}
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return u32Mask;
}

std::vector<RecorderDescStructure> SdCard::doOPER_GetPlaylists(uint32_t beginTs, uint32_t endTs, RECORDER_TYPE qrTypeMask) {
	std::vector<RecorderDescStructure> listRecorders;
	const std::string videoFolderPath = std::string(mMountPoint) + std::string("/video");
	const std::string audioFolderPath = std::string(mMountPoint) + std::string("/audio");

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState == SD_STATE_MOUNTED) {
		/* -- Do not admit query over 24 hours -- */
		if (endTs - beginTs < (24 * 3600)) {
			const std::string begFolder = utlsTimestampToFormatted(beginTs, (char *)DATE_FOLDER_FORMATTED);
			const std::string endFolder = utlsTimestampToFormatted(endTs, (char *)DATE_FOLDER_FORMATTED);
			const uint8_t nbQrFolder = (begFolder != endFolder) ? 2 : 1;

			for (uint8_t id = 0; id < nbQrFolder; ++id) {
				const std::string viSelectedFolder = videoFolderPath + std::string("/") + (id == 0 ? begFolder : endFolder);
				const std::string auSelectedFolder = audioFolderPath + std::string("/") + (id == 0 ? begFolder : endFolder);

				SD_DBG("Video folder selected: %s\r\n", viSelectedFolder.c_str());
				SD_DBG("Audio folder selected: %s\r\n", auSelectedFolder.c_str());

				DIR *d = opendir(viSelectedFolder.c_str());
				if (d == NULL) {
					continue;
				}
				struct dirent *e;
				while ((e = readdir(d)) != NULL) {
					if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
						continue;
					}

					struct tm tim;
					RecorderDescStructure desc;
					char sfx[5], typeStr[4];
					memset(sfx, 0, sizeof(sfx));
					memset(typeStr, 0, sizeof(typeStr));
					
					std::string viFileStr = std::string(e->d_name, strlen(e->d_name));
					std::string auFileStr = viFileStr;
					/* Change suffix into audio format */
					size_t pos = viFileStr.find(RECORD_VID_SUFFIX);
					if (pos != std::string::npos) {
						auFileStr.replace(pos, strlen(RECORD_VID_SUFFIX), RECORD_AUD_SUFFIX);
					}

					/* IGNORE only if video exist but audio does not */
					if (access(std::string(auSelectedFolder + std::string("/") + auFileStr).c_str(), F_OK) != 0) {
						continue;
					}

					/* Parser description name to get information */
					int r = sscanf(viFileStr.c_str(), RECORD_DESC_PARSER_FORMATTER,
							&tim.tm_year, &tim.tm_mon, &tim.tm_mday, &tim.tm_hour, &tim.tm_min, &tim.tm_sec, 
							&desc.begTs, &desc.endTs, typeStr, sfx);

					/* Valid parser recorder completed is 10 */
					if (r != 10) {
						/* IGNORE: File is recording */
						if (desc.begTs == mU3StartTS) {
							continue;
						}
						uint32_t lastVideoModifiedTs = Recorder::recover(viSelectedFolder, viFileStr);
						Recorder::recover(auSelectedFolder, auFileStr, lastVideoModifiedTs);
						SD_DBG("Video after recover: %s\r\n", viFileStr.c_str());
						SD_DBG("Audio after recover: %s\r\n", auFileStr.c_str());

						if (!Recorder::parser(viFileStr, &tim, &desc.begTs, &desc.endTs, &desc.type)) {
							SD_DBG("Can't parser video \"%s\"\r\n", viFileStr.c_str());
							continue;
						}
					}
					else {
						if (strcmp(typeStr, RECORD_ST_TYPE_REG) == 0) {
							desc.type = (int)RECORD_TYPE_REG;
						}
						else if (strcmp(typeStr, RECORD_ST_TYPE_MDT) == 0) {
							desc.type = (int)RECORD_TYPE_MDT;
						}
						else {
							desc.type = (int)RECORD_TYPE_HMD;
						}
					}

					size_t lastDotPos = viFileStr.find_last_of('.');
					if (lastDotPos != std::string::npos && viFileStr.substr(lastDotPos) == RECORD_VID_SUFFIX) {
						desc.name = viFileStr.substr(0, lastDotPos);
					}

					/* Compare timestamp range */
					if (desc.begTs < beginTs || desc.begTs > endTs) {
						SD_DBG("desc.begTs < beginTs || desc.begTs > endTs\r\n");
						continue;
					}

					/* Compare type query */
					if (!(qrTypeMask & desc.type)) {
						continue;
					}

					if (desc.endTs > desc.begTs) {
						desc.durationInSecs = desc.endTs - desc.begTs;
						listRecorders.push_back(desc);
						SD_DBG("[VALID] Video recorder \"%s\"\r\n", viFileStr.c_str());
					}
				}
				closedir(d);
			}
		}
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return listRecorders;
}

std::vector<std::string> SdCard::doOPER_GetPlaylists2(uint32_t beginTs, uint32_t endTs, RECORDER_TYPE qrTypeMask) {
	std::vector<std::string> listRecorders;
	const std::string videoFolderPath = std::string(mMountPoint) + std::string("/video");
	const std::string audioFolderPath = std::string(mMountPoint) + std::string("/audio");

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState == SD_STATE_MOUNTED) {
		/* -- Do not admit query over 24 hours -- */
		if (endTs - beginTs < (24 * 3600)) {
			const std::string begFolder = utlsTimestampToFormatted(beginTs, (char *)DATE_FOLDER_FORMATTED);
			const std::string endFolder = utlsTimestampToFormatted(endTs, (char *)DATE_FOLDER_FORMATTED);
			const uint8_t nbQrFolder = (begFolder != endFolder) ? 2 : 1;

			for (uint8_t id = 0; id < nbQrFolder; ++id) {
				const std::string viSelectedFolder = videoFolderPath + std::string("/") + (id == 0 ? begFolder : endFolder);
				const std::string auSelectedFolder = audioFolderPath + std::string("/") + (id == 0 ? begFolder : endFolder);

				SD_DBG("Video folder selected: %s\r\n", viSelectedFolder.c_str());
				SD_DBG("Audio folder selected: %s\r\n", auSelectedFolder.c_str());

				DIR *d = opendir(viSelectedFolder.c_str());
				if (d == NULL) {
					continue;
				}
				struct dirent *e;
				while ((e = readdir(d)) != NULL) {
					if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
						continue;
					}

					struct tm tim;
					RecorderDescStructure desc;

					char sfx[5], typeStr[4];
					memset(sfx, 0, sizeof(sfx));
					memset(typeStr, 0, sizeof(typeStr));
					
					std::string viFileStr = std::string(e->d_name, strlen(e->d_name));
					std::string auFileStr = viFileStr;
					/* Change suffix into audio format */
					size_t pos = viFileStr.find(RECORD_VID_SUFFIX);
					if (pos != std::string::npos) {
						auFileStr.replace(pos, strlen(RECORD_VID_SUFFIX), RECORD_AUD_SUFFIX);
					}

					/* IGNORE only if video exist but audio does not */
					if (access(std::string(auSelectedFolder + std::string("/") + auFileStr).c_str(), F_OK) != 0) {
						continue;
					}

					/* Parser description name to get information */
					int r = sscanf(viFileStr.c_str(), RECORD_DESC_PARSER_FORMATTER,
							&tim.tm_year, &tim.tm_mon, &tim.tm_mday, &tim.tm_hour, &tim.tm_min, &tim.tm_sec, 
							&desc.begTs, &desc.endTs, typeStr, sfx);

					/* Valid parser recorder completed is 10 */
					if (r != 10) {
						/* IGNORE: File is recording */
						if (desc.begTs == mU3StartTS) {
							continue;
						}
						uint32_t lastVideoModifiedTs = Recorder::recover(viSelectedFolder, viFileStr);
						Recorder::recover(auSelectedFolder, auFileStr, lastVideoModifiedTs);
						SD_DBG("Video after recover: %s\r\n", viFileStr.c_str());
						SD_DBG("Audio after recover: %s\r\n", viFileStr.c_str());

						if (!Recorder::parser(viFileStr, &tim, &desc.begTs, &desc.endTs, &desc.type)) {
							SD_DBG("Can't parser video \"%s\"\r\n", viFileStr.c_str());
							continue;
						}
					}
					else {
						if (strcmp(typeStr, RECORD_ST_TYPE_REG) == 0) {
							desc.type = (int)RECORD_TYPE_REG;
						}
						else if (strcmp(typeStr, RECORD_ST_TYPE_MDT) == 0) {
							desc.type = (int)RECORD_TYPE_MDT;
						}
						else {
							desc.type = (int)RECORD_TYPE_HMD;
						}
					}
					
					/* Get recorder name */
					size_t lastDotPos = viFileStr.find_last_of('.');
					if (lastDotPos != std::string::npos && viFileStr.substr(lastDotPos) == RECORD_VID_SUFFIX) {
						desc.name = viFileStr.substr(0, lastDotPos);
					}

					/* Compare timestamp range */
					if (desc.begTs < beginTs || desc.begTs > endTs) {
						continue;
					}

					/* Compare type query */
					if (!(qrTypeMask & desc.type)) {
						continue;
					}

					listRecorders.push_back(desc.name);
				}
				closedir(d);
			}
		}
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return listRecorders;
}

/* ------------------------------ Record Video/Audio ------------------------------ */
void createNestFoler(std::string nestDir) {
	char cmd[64];
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "mkdir -p %s", nestDir.c_str());
	SD_DBG("%s\r\n", cmd);
	system(cmd);
}

std::string utlsTimestampToFormatted(uint32_t ts, char *formatted) {
	char chars[64];
	time_t timeStamp = (time_t)ts;
	struct tm *timin = localtime(&timeStamp);
	strftime(chars, sizeof(chars), formatted, timin);
	return std::string(chars, strlen(chars));
}

int SdCard::doOPER_BeginRecord(RECORDER_TYPE type, uint32_t ts) {
	int ret = SD_OPER_FAILURE;

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState == SD_STATE_MOUNTED) {
		mType		 = type;
		mU3StartTS = (ts == 0) ? (uint32_t)time(NULL) : ts;
		std::string dateFolderStr;

		dateFolderStr = utlsTimestampToFormatted(mU3StartTS, (char *)DATE_FOLDER_FORMATTED);
		mVideoFolder = DEFAULT_VIDEO_FOLDER + std::string("/") + dateFolderStr;
		mAudioFolder = DEFAULT_AUDIO_FOLDER + std::string("/") + dateFolderStr;
		createNestFoler(mVideoFolder);
		createNestFoler(mAudioFolder);

		mVideo.initialise(mVideoFolder, mU3StartTS, mType, true);
		mAudio.initialise(mAudioFolder, mU3StartTS, mType, false);
		mVideo.Start(NULL, 0);
		mAudio.Start(NULL, 0);
		fOperState = OPER_RECORDING_INITIAL;

		/* Realtime add statistic */
		RecorderCounter *rcPtr = NULL;
		auto it = mStatistic.find(dateFolderStr);
		if (it == mStatistic.end()) {
			RecorderCounter rcCounter = {
				.regCounts = 0,
				.hmdCounts = 0,
				.mdtCounts = 0
			};
			mStatistic.emplace(dateFolderStr, rcCounter);
			rcPtr = &mStatistic[dateFolderStr];
		}
		else {
			rcPtr = &it->second;
		}

		if (type == RECORD_TYPE_REG) {
			++(rcPtr->regCounts);
		}
		else if (type == RECORD_TYPE_MDT) {
			++(rcPtr->mdtCounts);
		}
		else {
			++(rcPtr->hmdCounts);
		}
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

int SdCard::doOPER_WriteRecord(uint8_t *data, uint32_t dataLen, bool isVideo) {
	int ret = SD_OPER_WRITE_FAILED;

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState != SD_STATE_MOUNTED) {
		pthread_mutex_unlock(&SdCard::operMutex);
		return SD_OPER_FAILURE;
	}

	Recorder *fRecorder = (isVideo) ? &mVideo : &mAudio;
	if (fRecorder) {
		ret = (fRecorder->Write(data, dataLen) == 0) ? SD_OPER_SUCCESS : SD_OPER_WRITE_FAILED;
		fOperState = OPER_RECORDING_WRITING;
	}

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

int SdCard::doOPER_CloseRecord(uint32_t ts) {
	int ret = SD_OPER_WRITE_FAILED;

	pthread_mutex_lock(&SdCard::operMutex);

	if (mState == SD_STATE_MOUNTED) {
		uint32_t u32StopTS = (ts == 0) ? time(NULL) : ts;
		mVideo.Close(u32StopTS);
		mAudio.Close(u32StopTS);
		ret = SD_OPER_SUCCESS;
	}
	mType = RECORD_TYPE_INI;

	pthread_mutex_unlock(&SdCard::operMutex);

	return ret;
}

static void eraseFolder(std::string folderPathStr) {
	char cmd[128];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "rm -rf %s", folderPathStr.c_str());
	system(cmd);
}

static int eraseFolderRecursively(std::string folderPathStr) {
    DIR *dir = opendir(folderPathStr.c_str());
    if (!dir) {
		return -1;
	}

    struct dirent *ent;
    char chars[256];

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
			continue;
		}

        snprintf(chars, sizeof(chars), "%s/%s", folderPathStr.c_str(), ent->d_name);

        struct stat statbuf;
        if (stat(chars, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                eraseFolderRecursively(chars);
            }
			else {
                unlink(chars);
            }
        }
    }
    closedir(dir);

    return rmdir(folderPathStr.c_str());
}

std::string findOldestFolder(std::string rootPath) {
	DIR *d = opendir(rootPath.c_str());
	if (d == NULL) {
		return "";
	}

	struct dirent *e;
	struct stat fStat;
	std::string oldestFolder = "";
	time_t oldestTime		 = time(NULL);

	while ((e = readdir(d)) != NULL) {
		if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
			continue;
		}

		const std::string folder	 = std::string(e->d_name, strlen(e->d_name));
		const std::string folderPath = rootPath + std::string("/") + folder;
		if (stat(folderPath.c_str(), &fStat) == -1) {
			continue;
		}

		if (difftime(fStat.st_mtime, oldestTime) < 0) {
			oldestTime = fStat.st_mtime;
			oldestFolder.assign(folder);
		}
	}
	closedir(d);

	return oldestFolder;
}

void SdCard::eraseOldestFolder() {
	pthread_mutex_lock(&SdCard::operMutex);

	std::string oldestVideoPathStr;
	std::string oldestAudioPathStr;

	if (mState == SD_STATE_MOUNTED) {
		const std::string videoFolderPath = std::string(mMountPoint) + std::string("/video");
		const std::string audioFolderPath = std::string(mMountPoint) + std::string("/audio");
		std::string oldestFolder		  = findOldestFolder(videoFolderPath);

		if (!oldestFolder.empty()) {
			oldestVideoPathStr = videoFolderPath + std::string("/") + oldestFolder;
			oldestAudioPathStr = audioFolderPath + std::string("/") + oldestFolder;
		}

		if (!oldestVideoPathStr.empty() && !oldestAudioPathStr.empty()) {
			eraseFolder(oldestVideoPathStr);
			eraseFolder(oldestAudioPathStr);
			
			/* Realtime remove statistic */
			auto it = mStatistic.find(oldestFolder);
			if (it != mStatistic.end()) {
				mStatistic.erase(it);
			}
		}
	}

	pthread_mutex_unlock(&SdCard::operMutex);
}

void SdCard::clean() {
	pthread_mutex_lock(&SdCard::operMutex);

#if 1
	/* -- Erase all logs auto generating by system -- */
	DIR *d = opendir(mMountPoint);
	struct dirent *e = NULL;
	if (d) {
		while ((e = readdir(d)) != NULL) {
			if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
				continue;
			}

			std::string trash(e->d_name, strlen(e->d_name));
			if (trash != std::string("video") && trash != std::string("audio") && trash != std::string("mp4")) {
				eraseFolder(std::string(mMountPoint) + std::string("/") + trash);
			}
		}
		closedir(d);
	}
#endif

	pthread_mutex_unlock(&SdCard::operMutex);
}

/* ------------------------------ Private method ------------------------------ */
bool SdCard::hasMounted() {
	int pos = 0;
	char chars[256];
	bool isRO	 = true;
	bool boolean = false;
	ssize_t n;

	int fd = open("/proc/mounts", O_RDONLY);
	if (fd == -1) {
		return false;
	}

	while ((n = read(fd, &chars[pos], 1)) > 0) {
		if (chars[pos] == '\n' || chars[pos] == '\r') {
			chars[pos] = '\0';
			if (strstr(chars, mMountPoint)) {
				boolean = true;

				// Find the start of mount options
				// (after the third space).
				char *options = strchr(chars, ' ');
				if (options != NULL) {
					options++;
					options = strchr(options, ' ');
					if (options != NULL) {
						options++;

						// Query Sd is mounted as read-only or not
						// (contains "rw" means mounted as read-write).
						if (strstr(options, "rw") != NULL) {
							isRO = false;
						}
					}
				}
				break;
			}
			pos = 0;
		}
		else {
			pos++;
			if (pos >= (int)((sizeof(chars) - 1))) {
				pos = 0;
			}
		}
	}
	close(fd);

	if (isRO) {
		SD_DBG("Sd mounted type RO -> remount to RW\r\n");

		char cmds[64];
		memset(cmds, 0, sizeof(cmds));
		snprintf(cmds, sizeof(cmds), "mount -o remount,rw %s", mMountPoint);
		system(cmds);
	}

	return boolean;
}

int SdCard::doOPER_Mount() {
	char cmd[64];
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "mount %s %s", mHardDisk, mMountPoint);
	return (system(cmd) == 0) ? SD_OPER_SUCCESS : SD_OPER_FAILURE;
}

int SdCard::doOPER_Unmount() {
	return (umount2(mMountPoint, MNT_FORCE) == 0) ? SD_OPER_SUCCESS : SD_OPER_FAILURE;
}
