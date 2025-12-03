#ifndef _SDCARD_H_
#define _SDCARD_H_

#include <stdbool.h>
#include <pthread.h>
#include <unordered_map>
#include <vector>

#include "recorder.h"

#define DEFAULT_HARDDISK	(const char*)"/dev/mmcblk0p2"
#define DEFAULT_MOUNTPOINT	(const char*)"/tmp/sd"
#define DEFAULT_TYPE_FORMAT (const char*)"vfat"

#define DATE_FOLDER_FORMATTED "%Y.%m.%d"
#define DEFAULT_VIDEO_FOLDER  DEFAULT_MOUNTPOINT "/video"
#define DEFAULT_AUDIO_FOLDER  DEFAULT_MOUNTPOINT "/audio"

#define SDCARD_RECORD_DURATION_DEFAULT (900)	 // 15 mins
#define SDCARD_RECORD_DURATION_MAX	   (3600)	 // 60 mins
#define SDCARD_RECORD_DURATION_MIN	   (60)		 // 01 min

enum {
	SD_OPER_SUCCESS		 = 0,
	SD_OPER_FAILURE		 = -1,
	SD_OPER_READ_FAILED	 = -2,
	SD_OPER_WRITE_FAILED = -3,
};

typedef enum {
    QRY_TYPE_EVT = -1,
    QRY_TYPE_ALL = 0,
    QRY_TYPE_REG = 1,
    QRY_TYPE_MDT = 2,
    QRY_TYPE_HMD = 3,
} SD_QUERY_PLAYLIST;

typedef enum {
	SD_STATE_REMOVED = 0,
	SD_STATE_INSERTED,
	SD_STATE_MOUNTED,
} SD_STATUS;

typedef enum {
	OPER_RECORDING_INITIAL,
	OPER_RECORDING_WRITING,
} SD_OPER_RECORDING_STATUS;

typedef struct {
    int regCounts;
    int hmdCounts;
    int mdtCounts;
} RecorderCounter;

class SdCard {
public:
	 SdCard();
	~SdCard();

	void scan();
	void clean();
	void eraseOldestFolder();
	void recorderChangeType(RECORDER_TYPE typeChanged);
    void loadingStatistic();
    void erasingStatistic();
    const std::unordered_map<std::string, RecorderCounter> getStatistic();

	SD_STATUS currentState();
	uint32_t recorderStartTs();
	RECORDER_TYPE recorderCurType();
	SD_OPER_RECORDING_STATUS recorderCurOPERState();
    uint32_t u32MaskCalenderRecorder(RECORDER_TYPE qrTypeMask, int qrYear, int qrMonth);

public:
	int doOPER_Format();
	int doOPER_GetCapacity(uint64_t *total, uint64_t *free, uint64_t *used);
	int doOPER_BeginRecord(RECORDER_TYPE type, uint32_t ts = 0);
	int doOPER_WriteRecord(uint8_t *data, uint32_t dataLen, bool isVideo);
	int doOPER_CloseRecord(uint32_t ts = 0);

    /* TWO WAY QUERY PLAYLISTS */
    std::vector<std::string> doOPER_GetPlaylists2(uint32_t beginTs, uint32_t endTs, RECORDER_TYPE qrTypeMask);
    std::vector<RecorderDescStructure> doOPER_GetPlaylists(uint32_t beginTs, uint32_t endTs, RECORDER_TYPE qrTypeMask);

	static pthread_mutex_t operMutex;

private:
	bool hasMounted();
	bool validFormat();
	int doOPER_Mount();
	int doOPER_Unmount();

private:
	const char *const mHardDisk	  = DEFAULT_HARDDISK;
	const char *const mMountPoint = DEFAULT_MOUNTPOINT;

	SD_STATUS mState;
	RECORDER_TYPE mType;
	std::string mVideoFolder;
	std::string mAudioFolder;

	Recorder mVideo;
	Recorder mAudio;
	uint32_t mU3StartTS;
	SD_OPER_RECORDING_STATUS fOperState;

    std::unordered_map<std::string, RecorderCounter> mStatistic;
};

extern SdCard sdCard;

extern void createNestFoler(std::string nestDir);

#endif /* _SDCARD_H_ */
