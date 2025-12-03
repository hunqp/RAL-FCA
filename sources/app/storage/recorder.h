#ifndef RECORDER_H
#define RECORDER_H

#include <stdio.h>
#include <stdint.h>
#include <string>

/* -- [YYmmddHHMMSS]_[StartTimestamp]_[EndTimestamp]_[StringType].[SuffixString] -- */
#define RECORD_DESC_COMPLETED_FORMAT (const char*)"%04d%02d%02d%02d%02d%02d_%d_%d_%s%s"

/* -- [YYmmddHHMMSS]_[StartTimestamp]_[StringType].[SuffixString] -- */
#define RECORD_DESC_TEMPORARY_FORMAT (const char*)"%04d%02d%02d%02d%02d%02d_%d_%s%s"

#define RECORD_DESC_PARSER_FORMATTER (const char*)"%04d%02d%02d%02d%02d%02d_%d_%d_%[^.]%s"

#define RECORD_ST_TYPE_REG (const char*)"reg"
#define RECORD_ST_TYPE_HMD (const char*)"hmd"
#define RECORD_ST_TYPE_MDT (const char*)"mdt"

#define RECORD_VID_SUFFIX ".h264"
#define RECORD_AUD_SUFFIX ".g711"
#define RECORD_IMG_SUFFIX ".thmb"

#define RECORD_VID_TMP_SUFFIX RECORD_VID_SUFFIX RECORD_TMP_SUFFIX
#define RECORD_AUD_TMP_SUFFIX RECORD_AUD_SUFFIX RECORD_TMP_SUFFIX

#define RECORD_RETURN_SUCCESS               (1)
#define RECORD_RETURN_FAILURE               (-1)

#define IS_FORMAT_PARSED_VALID(nbParsed)    (nbParsed == 4 ? true : false)

#define RECORD_DATE_FORMAT                  "%Y.%m.%d"
#define RECORD_TIME_FORMAT                  "%H:%M:%S"
#define RECORD_DATETIME_FORMAT              "%Y.%m.%d %H:%M:%S"

typedef enum {
	RECORD_TYPE_REG = 1 << 0,
	RECORD_TYPE_MDT	= 1 << 1,
	RECORD_TYPE_HMD	= 1 << 2,

    RECORD_TYPE_INI	= 0xFF,
} RECORDER_TYPE;

typedef struct {
    int type;
    std::string name;
    uint32_t begTs;
    uint32_t endTs;
    uint32_t durationInSecs;
} RecorderDescStructure;

class Recorder {
public:
	 Recorder();
    ~Recorder();

    void changeType(RECORDER_TYPE type);
	void initialise(std::string folder, uint32_t ts, RECORDER_TYPE type, bool isVideo);
	int Start(uint8_t *dataPtr, uint32_t u32Len);
	int Write(uint8_t *dataPtr, uint32_t u32Len);
	int Close(uint32_t ts);

public:
    static uint32_t recover(std::string folder, std::string& brokenStr, uint32_t inputTs = 0);
    static bool parser(std::string descStr, struct tm *tmPtr, uint32_t *beginTsPtr, uint32_t *endTsPtr, int *type);

private:
    struct tm mTm;
    std::string mDesc;
    std::string mFolder;
    std::string mSfxStr;
    std::string mTypeStr;
    uint32_t mStartTs;
};


#endif /* __RECORDER_H */