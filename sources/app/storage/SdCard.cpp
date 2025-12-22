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
#include "sys_log.h"

SdCard SDCARD;
pthread_mutex_t SdCard::GeneralMutex = PTHREAD_MUTEX_INITIALIZER;

SdCard::SdCard() {
	mImplStatusChange = [](SD_STATUS){};
}

SdCard::~SdCard() {
	deinitialse();
	pthread_mutex_destroy(&SdCard::GeneralMutex);
}

int SdCard::ForceFormat() {
	int rc = -1;

	pthread_mutex_lock(&SdCard::GeneralMutex);

	if (mState != SD_STATE_REMOVED) {
		char cmd[128] = {0};
		snprintf(cmd, sizeof(cmd), "umount -l %s && mkfs.%s %s > /dev/null 2>&1", mMountPoint.c_str(), DEFAULT_TYPE_FORMAT, mHardDisk.c_str());
		rc = system(cmd);
		mState = SD_STATE_REMOVED;
	}

	pthread_mutex_unlock(&SdCard::GeneralMutex);

	return rc;
}

int SdCard::ForceUmount() {
	int rc = -1;

	pthread_mutex_lock(&SdCard::GeneralMutex);

	rc = umount2(mMountPoint.c_str(), MNT_DETACH);

	pthread_mutex_unlock(&SdCard::GeneralMutex);

	return rc;
}

int SdCard::GetCapacity(uint64_t *total, uint64_t *free, uint64_t *used) {
	int rc = -1;

	*total = *free = *used = 0;

	pthread_mutex_lock(&SdCard::GeneralMutex);

	if (mState == SD_STATE_MOUNTED) {
		struct statfs statFs;
		if (statfs(mMountPoint.c_str(), &statFs) == 0) {
			size_t blockSize = (uint64_t)statFs.f_bsize;
			*total = (uint64_t)blockSize * (uint64_t)statFs.f_blocks;
			*free = (uint64_t)blockSize * (uint64_t)statFs.f_bfree;
			*used = (uint64_t)blockSize * ((uint64_t)statFs.f_blocks - (uint64_t)statFs.f_bfree);
			rc = 0;
		}
	}

	pthread_mutex_unlock(&SdCard::GeneralMutex);

	return rc;
}

static std::string timestampToFormat(uint32_t ts, char *formatted) {
	char chars[64];
	time_t timeStamp = (time_t)ts;
	struct tm *timin = localtime(&timeStamp);
	strftime(chars, sizeof(chars), formatted, timin);
	return std::string(chars, strlen(chars));
}

static uint64_t partitionSize(const char *partition) {
	int fd = -1;
    uint64_t size = 0;
    
    fd = open(partition, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    /* Use BLKGETSIZE64 ioctl to get size in bytes */
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        close(fd);
        return 0;
    }
    close(fd);
    return size;
}

static inline bool is_FAT(const uint8_t *b) {
    if (memcmp(b + 54, "FAT", 3) == 0) {
		return true; /* FAT12/16 */
	}
    if (memcmp(b + 82, "FAT32", 5) == 0) {
		return true; /* FAT32 */
	}
    return false;
}

static inline bool is_EXFAT(const uint8_t *b) {
    return (memcmp(b + 3, "EXFAT   ", 8) == 0) ? true : false;
}

static inline bool is_EXT234(const uint8_t *b) {
    uint16_t magic = *(uint16_t *)(b + 1024 + 56);
    return (magic == 0xEF53) ? true : false;
}

static inline bool is_F2FS(const uint8_t *b) {
    /* F2FS magic: 0x10 0x20 0xF5 0xF2 */
    const uint8_t magic[4] = {0x10, 0x20, 0xF5, 0xF2};
    return (memcmp(b, magic, 4) == 0) ? true : false;
}

static inline bool is_NTFS(const uint8_t *b) {
    return (memcmp(b + 3, "NTFS    ", 8) == 0) ? true : false;
}

static const char *partitionType(const char *partition) {
	uint8_t buffers[4096] = {0};

    int fd = open(partition, O_RDONLY);
    if (fd < 0) {
		return "UNKNOWN";
	}

    ssize_t readBytes = read(fd, buffers, sizeof(buffers));
    close(fd);

    if (readBytes < 2048) {
		return "UNKNOWN";
	}
    if (is_FAT(buffers)) {
		return "VFAT";
	}
    if (is_EXFAT(buffers)) {
		return "EXFAT";
	}
    if (is_EXT234(buffers)) {
		return "EXT4";
	}
    if (is_F2FS(buffers)) {
		return "F2FS";
	}
    if (is_NTFS(buffers)) {
		return "NTFS";
	}
    return "UNKNOWN";
}

void SdCard::initialise(void) {
	mHardDisk.clear();
	mPerformTestWrite = true;
	mState = SD_STATE_REMOVED;
	mEnablePollingScanner = true;

	if (!mScannerPlugOrUnplugId) {
		pthread_create(&mScannerPlugOrUnplugId, NULL, SdCard::scanningPlugOrUnplug, this);
	}
}

void SdCard::deinitialse(void) {
	mEnablePollingScanner = false;

	if (mScannerPlugOrUnplugId) {
		pthread_join(mScannerPlugOrUnplugId, NULL);
		mScannerPlugOrUnplugId = (pthread_t)NULL;
	}
}

void SdCard::scans() {
	pthread_mutex_lock(&SdCard::GeneralMutex);

	/*
		Step 1 -> Polling SD Card insertion.
		Step 2 -> Find the largest partition in disks.
		Step 3 -> Check partiton format type, if NOT VFAT -> Let format to VFAT.
		Step 4 -> Mount as VFAT type command.
	*/

	if (mState == SD_STATE_REMOVED) {
		if (access(DEFAULT_HARDDISK, F_OK) != 0) {
			pthread_mutex_unlock(&SdCard::GeneralMutex);
			return;
		}

		uint64_t previousLargestSize = 0;

		for (int8_t id = 10; id >= 0; id--) {
			char parts[18] = {0};
			snprintf(parts, sizeof(parts), "%sp%d", DEFAULT_HARDDISK, id);
			if (access(parts, F_OK) != 0) {
				continue;
			}
			uint64_t partSize = partitionSize(parts);
			if (partSize != 0 && partSize > previousLargestSize) {
				previousLargestSize = partSize;
				mHardDisk.assign(std::string(parts, strlen(parts)));
			}
		}
		if (previousLargestSize == 0) {
			mHardDisk.assign(DEFAULT_HARDDISK);
		}
	}

	if (!mHardDisk.empty() && (access(mHardDisk.c_str(), F_OK) == 0)) {
		mState = SD_STATE_INSERTED;
		const char *typeOfParts = partitionType(mHardDisk.c_str());
		if (strcmp(typeOfParts, (const char*)"VFAT") != 0) {
			SYSW("Found \'%s\' type %s size %llu (bytes) -> MUST BE formatted to \'VFAT\'\r\n", mHardDisk.c_str(), typeOfParts, partitionSize(mHardDisk.c_str()));
			char cmd[128] = {0};
			snprintf(cmd, sizeof(cmd), "mkfs.%s %s > /dev/null 2>&1", DEFAULT_TYPE_FORMAT, mHardDisk.c_str());
			system(cmd);
		}

		if (hasMounted()) {
			mState = SD_STATE_MOUNTED;
		}
		else {
			if (mounts() == 0) {
				mState = SD_STATE_MOUNTED;
				/*	We need to perform a write data test the 
					first time the card is inserted
				*/
				if (mPerformTestWrite) {
					/* 	Ext-Disk malfunctioned during installation (Some errors that we can't write data), 
						we need to unmount and remount it from scratch
					*/
					if (!verifyMountHealth()) {
						mState = SD_STATE_REMOVED;
						SYSE("Ext-Disk malfunctioned during installation\r\n");
						umount2(mMountPoint.c_str(), MNT_DETACH);
					}
					else {
						/* 	We've just verified one time,
							no need to check for the next time 
						*/
						SYSI("Ext-Disk checks healthy good\r\n");
						mPerformTestWrite = false;
					}
				}
			}
		}
	}
	else {
		mHardDisk.clear();
		mPerformTestWrite = true;
		mState = SD_STATE_REMOVED;
	}

	pthread_mutex_unlock(&SdCard::GeneralMutex);
}

SD_STATUS SdCard::currentState() {
	pthread_mutex_lock(&SdCard::GeneralMutex);

	SD_STATUS ret = mState;

	pthread_mutex_unlock(&SdCard::GeneralMutex);

	return ret;
}

std::vector<RECORDER_INDEX_S> SdCard::queryPlaylists(uint32_t beginTs, uint32_t endTs) {
	std::vector<RECORDER_INDEX_S> playlist {};

	pthread_mutex_lock(&SdCard::GeneralMutex);

	if (mState == SD_STATE_MOUNTED) {
		/* Do not admit query over 24 hours */
		if (endTs - beginTs < (24 * 3600)) {
			std::string folder0 = timestampToFormat(beginTs, (char *)DEFAULT_FOLDER_FORMATTED);
			std::string folder1 = timestampToFormat(endTs, (char *)DEFAULT_FOLDER_FORMATTED);
			uint8_t numFolders = (folder0 != folder1) ? 2 : 1;

			for (uint8_t id = 0; id < numFolders; ++id) {
				std::string selected = (id == 0 ? folder0 : folder1);
				std::string indexDB = mMountPoint + std::string("/") + (selected) + std::string("/") + DEFAULT_DATABASE_INDEX;
				FILE *fp = fopen(indexDB.c_str(), "rb");
				if (!fp) {
					continue;
				}
				
				do {
					RECORDER_INDEX_S item = {0};
					size_t readBytes = fread(&item, sizeof(char), sizeof(RECORDER_INDEX_S), fp);
					if (readBytes == sizeof(RECORDER_INDEX_S)) {
						if (item.startTs < beginTs || item.startTs > endTs) {
							continue;
						}
						playlist.push_back(item);
					}
					else break;
				}
				while (1);
				fclose(fp);
			}
		}
	}

	pthread_mutex_unlock(&SdCard::GeneralMutex);

	return playlist;
}

void SdCard::onStatusChange(std::function<void(SD_STATUS)> callback) {
	mImplStatusChange = callback;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void createNestFolder(std::string nestDir) {
	char cmd[64] = {0};
	snprintf(cmd, sizeof(cmd), "mkdir -p %s", nestDir.c_str());
	system(cmd);
}

std::string SdCard::prepareContainerFolder() {
	pthread_mutex_lock(&SdCard::GeneralMutex);

	if (mState == SD_STATE_MOUNTED) {
		uint32_t ts = (uint32_t)time(NULL);
		mContainerFolder = timestampToFormat(ts, (char*)DEFAULT_STORAGE_FOLDER);
		createNestFolder(mContainerFolder);
	}

	pthread_mutex_unlock(&SdCard::GeneralMutex);

	return mContainerFolder;
}

static void eraseFolder(std::string folderPath) {
	char cmd[128];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "rm -rf %s > /dev/null 2>&1", folderPath.c_str());
	system(cmd);
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

		const std::string folder = std::string(e->d_name, strlen(e->d_name));
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
	pthread_mutex_lock(&SdCard::GeneralMutex);

	if (mState == SD_STATE_MOUNTED) {
		const std::string rootFolderPath = std::string(mMountPoint);
		std::string oldestDate = findOldestFolder(rootFolderPath);

		if (!oldestDate.empty()) {
			std::string delFolderPath = rootFolderPath + std::string("/") + oldestDate;
			eraseFolder(delFolderPath);
			directoriesSynchronise();
		}
	}

	pthread_mutex_unlock(&SdCard::GeneralMutex);
}

void SdCard::clean() {
	pthread_mutex_lock(&SdCard::GeneralMutex);

#if 0
	/* -- Erase all logs auto generating by system -- */
	DIR *d = opendir(mMountPoint.c_str());
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

	pthread_mutex_unlock(&SdCard::GeneralMutex);
}

void SdCard::updateIndexDatabase(RECORDER_INDEX_S *infor) {
	pthread_mutex_lock(&SdCard::GeneralMutex);

	if (mState == SD_STATE_MOUNTED) {
		std::string filename = mContainerFolder + std::string("/") + DEFAULT_DATABASE_INDEX;
		int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_DSYNC, 0777);
        if (fd >= 0) {
            write(fd, infor, sizeof(RECORDER_INDEX_S));
            fsync(fd);
            posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
            close(fd);
        }
	}

	pthread_mutex_unlock(&SdCard::GeneralMutex);
}

/* ------------------------------ Private method ------------------------------ */
void SdCard::directoriesSynchronise() {
	/* Ensure operation is commited to disk */
	int dfd = open(mMountPoint.c_str(), O_DIRECTORY | O_RDONLY);
	if (dfd >= 0) {
		fsync(dfd);
		close(dfd);
	}
}

bool SdCard::verifyMountHealth() {
	#if 1
	const char rw[] = "v";
	std::string filename = mMountPoint + std::string("/.rw");
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if (fd < 0) {
		return false;
	}
	bool boolean = ((write(fd, rw, 1) == 1) && (fsync(fd) == 0)) ? true : false;
    close(fd);
    unlink(filename.c_str());
    return boolean;
	#else
	char cmd[64] = {0};
	snprintf(cmd, sizeof(cmd), "echo 123 > %s/123 > /dev/null 2>&1", mMountPoint.c_str());
	return (system(cmd) == 0) ? true : false;
	#endif
}

bool SdCard::hasMounted() {
	int fd = -1, pos = 0;
	bool boolean = false;
	char line[256] = {0};
	ssize_t readBytes;

	fd = open("/proc/mounts", O_RDONLY);
	if (fd == -1) {
		return false;
	}

	while ((readBytes = read(fd, &line[pos], 1)) > 0) {
		if (line[pos] == '\n') {
			line[pos] = '\0';
			if (strstr(line, mMountPoint.c_str())) {
				boolean = true;
				break;
			}
			pos = 0;
		}
		else {
			pos++;
			if (pos >= (int)sizeof(line) - 1) {
				pos = 0;
			}
		}
	}
	close(fd);
	return (boolean) ? true : false;
}

int SdCard::mounts() {
	#if 0
	char cmd[64] = {0};
	snprintf(cmd, sizeof(cmd), "mount %s %s", mHardDisk.c_str(), mMountPoint.c_str());
	return system(cmd);
	#endif

	if (mHardDisk.empty()) {
		return -1;
	}

	if (mount(mHardDisk.c_str(), mMountPoint.c_str(), DEFAULT_TYPE_FORMAT, 0, NULL) != 0) {
        if (errno == EIO) {
            RAM_SYSE("Detected: Input/output error (possibly bad SD card)\r\n");
        }
		else if (errno == ENODEV) {
            RAM_SYSE("No such device - controller or slot missing\r\n");
        }
		else if (errno == ENXIO) {
            RAM_SYSE("Device not ready / no medium present\r\n");
        }
		else if (errno == EINVAL) {
            RAM_SYSE("Invalid parameters or unsupported filesystem\r\n");
        }
		else if (errno == EBUSY) {
            RAM_SYSE("Device or mountpoint already in use\r\n");
        }
        return -2;
    }
	return 0;
}

void *SdCard::scanningPlugOrUnplug(void *args) {
	uint32_t ts = 0;
	SD_STATUS lastState = SD_STATE_REMOVED;
	SdCard *me = static_cast<SdCard*>(args);

	while (me->mEnablePollingScanner) {
		me->scans();
		SD_STATUS state = me->currentState();

		if (state != lastState) {
			lastState = state;
			me->mImplStatusChange(lastState);
		}

		if (time(NULL) - ts > 60) {
			ts = time(NULL);
			/* Periodic maintain capacity at least 20% free space */
			if (lastState == SD_STATE_MOUNTED) {
				uint64_t total, free, used;
				if (me->GetCapacity(&total, &free, &used) == 0) {
					int percent = (int)(((double)used / (double)total) * 100.0);
					if (percent > 80) {
						me->eraseOldestFolder();
					}
					printf("SD/EMMC total: %llu, used: %llu, free: %llu\r\n", total, used, free);
				}
				if (0) {
					me->clean();
				}
			}
		}
		sleep(1);
	}

	return NULL;
}