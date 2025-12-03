#include "task_list.h"

ak_task_t task_list[] = {
	/* SYSTEM TASKS */
	{	AK_TASK_TIMER_ID,	   	TASK_PRI_LEVEL_1, 	TaskTimerEntry,			  	&timerMailbox,			  		"AK_TASK_TIMER_ID" 	},

 	/* APP TASKS */
	{	GW_TASK_CLOUD_ID,	   	TASK_PRI_LEVEL_1, 	gw_task_cloud_entry,	  	&gw_task_cloud_mailbox,	  		"GW_TASK_CLOUD_ID  "},
	{	GW_TASK_SYS_ID,		 	TASK_PRI_LEVEL_1, 	gw_task_sys_entry,		  	&gw_task_sys_mailbox,			"GW_TASK_SYS_ID    "},
	{	GW_TASK_AV_ID,			TASK_PRI_LEVEL_1, 	gw_task_av_entry,			&gw_task_av_mailbox,		 	"GW_TASK_AV_ID     "},
	{	GW_TASK_DETECT_ID,		TASK_PRI_LEVEL_1, 	gw_task_detect_entry,		&gw_task_detect_mailbox,	 	"GW_TASK_DETECT_ID "},
	{	GW_TASK_CONFIG_ID,		TASK_PRI_LEVEL_1, 	gw_task_config_entry,		&gw_task_config_mailbox,	 	"GW_TASK_CONFIG_ID "},
	{	GW_TASK_UPLOAD_ID,		TASK_PRI_LEVEL_1, 	gw_task_upload_entry,		&gw_task_upload_mailbox,	 	"GW_TASK_UPLOAD_ID "},
	{	GW_TASK_NETWORK_ID,	 	TASK_PRI_LEVEL_1, 	gw_task_network_entry,	  	&gw_task_network_mailbox,		"GW_TASK_NETWORK_ID"},
	{	GW_TASK_FW_ID,			TASK_PRI_LEVEL_1, 	gw_task_fw_entry,			&gw_task_fw_mailbox,		 	"GW_TASK_FW_ID     "},
	{	GW_TASK_WEBRTC_ID,		TASK_PRI_LEVEL_1, 	gw_task_webrtc_entry,		&gw_task_webrtc_mailbox,	 	"GW_TASK_WEBRTC_ID "},
	{	GW_TASK_RECORD_ID,		TASK_PRI_LEVEL_1, 	gw_task_record_entry,		&gw_task_record_mailbox,	   	"GW_TASK_RECORD_ID "},
};
