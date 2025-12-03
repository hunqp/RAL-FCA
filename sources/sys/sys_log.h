/**
 * @file  sys_log.h
 * @brief System journal log writes file, auto clear half 
 * content when reach to full size setting.
 *
 * Version: 1.0
 * Date: 27-08-2025
 * Author: Hunqp
 */
#ifndef SYS_LOG_H
#define SYS_LOG_H

#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SYS_LOG_INFO,
    SYS_LOG_WARN,
    SYS_LOG_DEBUG,
    SYS_LOG_ERROR,
};

#if 0
#define SYSI(fmt, ...)
#define SYSW(fmt, ...)
#define SYSD(fmt, ...)
#define SYSE(fmt, ...)

#define RAM_SYSI(fmt, ...)
#define RAM_SYSW(fmt, ...)
#define RAM_SYSD(fmt, ...)
#define RAM_SYSE(fmt, ...)
#else
#define SYSI(fmt, ...) vv_sys_logger_write(&JOURNAL_LOGGER, SYS_LOG_INFO , fmt, ##__VA_ARGS__)
#define SYSW(fmt, ...) vv_sys_logger_write(&JOURNAL_LOGGER, SYS_LOG_WARN , fmt, ##__VA_ARGS__)
#define SYSD(fmt, ...) vv_sys_logger_write(&JOURNAL_LOGGER, SYS_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define SYSE(fmt, ...) vv_sys_logger_write(&JOURNAL_LOGGER, SYS_LOG_ERROR, fmt, ##__VA_ARGS__)

#define RAM_SYSI(fmt, ...) vv_sys_logger_write(&RUNTIME_LOGGER, SYS_LOG_INFO , fmt, ##__VA_ARGS__)
#define RAM_SYSW(fmt, ...) vv_sys_logger_write(&RUNTIME_LOGGER, SYS_LOG_WARN , fmt, ##__VA_ARGS__)
#define RAM_SYSD(fmt, ...) vv_sys_logger_write(&RUNTIME_LOGGER, SYS_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define RAM_SYSE(fmt, ...) vv_sys_logger_write(&RUNTIME_LOGGER, SYS_LOG_ERROR, fmt, ##__VA_ARGS__)
#endif

typedef struct {
    const char *filename;
    size_t maxBytesCanBeHold;
    pthread_mutex_t mt;
} VV_SYS_LOGGER_S;

extern VV_SYS_LOGGER_S RUNTIME_LOGGER;
extern VV_SYS_LOGGER_S JOURNAL_LOGGER;

extern void vv_sys_logger_write(VV_SYS_LOGGER_S *me, int level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* SYS_LOG_H */