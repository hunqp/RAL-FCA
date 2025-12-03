#ifndef __TASK_CLOUD_H__
#define __TASK_CLOUD_H__

// #include <mosquittopp.h>
#include "message.h"

extern q_msg_t gw_task_cloud_mailbox;
extern void *gw_task_cloud_entry(void *);

#endif	  //__TASK_CLOUD_H__
