#ifndef SDCARD_HH
#define SDCARD_HH

#include <vector>
#include <stdbool.h>
#include <pthread.h>
#include <functional>

#include "recorder.h"

#define DEFAULT_HARDDISK	"/dev/mmcblk0"
#define DEFAULT_HARDDISK_ID	"/dev/mmcblk0p%d"
#define DEFAULT_MOUNTPOINT	"/mnt/sd"
#define DEFAULT_TYPE_FORMAT "vfat"

#define DEFAULT_FOLDER_FORMATTED 	"%Y-%m-%d"
#define DEFAULT_STORAGE_FOLDER		DEFAULT_MOUNTPOINT "/" DEFAULT_FOLDER_FORMATTED
#define DEFAULT_DATABASE_INDEX		"index.db"

typedef enum {
	QRY_REGULAR = FILE_RECORD_TYPE_247,
	QRY_MOTION	= FILE_RECORD_TYPE_MOTION_DETECTED,
	QRY_HUMAN	= FILE_RECORD_TYPE_HUMAN_DETECTED,
	QRY_ALLLIST = 0xFFFF,
} SD_QUERY_PLAYLIST;

typedef enum {
	SD_STATE_REMOVED = 0,
	SD_STATE_INSERTED,
	SD_STATE_MOUNTED,
	SD_STATE_UNDEFINED,
} SD_STATUS;

class SdCard {
public:
	 SdCard();
	~SdCard();

	void initialise(void);
	void deinitialse(void);

	void scans(void);
	void clean(void);
	void eraseOldestFolder();
	SD_STATUS currentState();
	void updateIndexDatabase(RECORDER_INDEX_S *infor);
	std::vector<RECORDER_INDEX_S> queryPlaylists(uint32_t beginTs, uint32_t endTs);
	std::string prepareContainerFolder();

	void onStatusChange(std::function<void(SD_STATUS)> callback);

public: 
	/* Function operates with Ext-Disks */
	int ForceFormat(void);
	int ForceUmount(void);
	int GetCapacity(uint64_t *total, uint64_t *free, uint64_t *used);

	static pthread_mutex_t GeneralMutex;

private:
	int mounts();
	bool hasMounted();
	bool verifyMountHealth();
	void directoriesSynchronise();
	static void *scanningPlugOrUnplug(void *args);
	
private:
	pthread_t mScannerPlugOrUnplugId = (pthread_t)NULL;
	volatile bool mEnablePollingScanner = false;

	std::string mHardDisk;
	std::string mMountPoint = DEFAULT_MOUNTPOINT;

	SD_STATUS mState;
	bool mPerformTestWrite = true;
	std::string mContainerFolder;

	std::function<void(SD_STATUS)> mImplStatusChange;
};

extern SdCard SDCARD;

#endif /* _SDCARD_H_ */
