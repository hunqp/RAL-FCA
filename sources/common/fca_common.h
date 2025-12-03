#ifndef __FCA_COMMON_H__
#define __FCA_COMMON_H__

#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_threadHdl {
	pthread_t id;
	volatile bool is_running;
	pthread_mutex_t mtx;
	pthread_cond_t cond;
} threadHdl_t;

extern int systemCmd(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif