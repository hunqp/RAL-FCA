#ifndef __DATACHANNEL_HDL_H__
#define __DATACHANNEL_HDL_H__

#include <stdio.h>
#include <stdint.h>

#include <iostream>

#define DC_CMD_PANTILT				 "PanTilt"
#define DC_CMD_STREAM				 "Stream"
#define DC_CMD_DISCONNECT_REPORT	 "ForceDisconnect"
#define DC_CMD_PUSH_TO_TALK			 "PushToTalk"
#define DC_CMD_EMERGENCY_ALARM		 "EmergencyAlarm"

#define DC_CMD_PLAYLIST				 "Playlist"
#define DC_CMD_PLAYLIST2			 "Playlist2"
#define DC_CMD_PLAYBACK				 "Playback"
#define DC_CMD_DOWNLOAD				 "Download"
#define DC_CMD_FORMAT_STORAGE		 "FormatStorage"
#define DC_CMD_RECORD_CALENDAR		 "RecordCalendar"
#define DC_CMD_COUNT_RECORD_CALENDAR "CountRecordCalendar"

extern void onDataChannelHdl(const std::string clId, const std::string &req, std::string &resp);

#endif /* __DATACHANNEL_HDL_H__ */
