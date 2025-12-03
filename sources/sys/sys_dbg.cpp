#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>

#include "ak.h"
#include "app_dbg.h"
#include "app_config.h"
#include "sys_dbg.h"

int8_t g_sys_log_dbg_en = 0;

void sys_dbg_fatal(const char *s, uint8_t c) {
	printf("[SYS_UNIX] FATAL: %s \t 0x%02X\n", s, c);
	LOG_FILE_FATAL("FATAL: %s \t 0x%02X\n", s, c);
	exit(EXIT_FAILURE);
}

char *sys_dbg_get_time(void) {
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	static char return_val[64];
	strftime(return_val, sizeof(return_val), "%Y-%m-%d %H:%M:%S", timeinfo);

	struct timeval te;
	gettimeofday(&te, NULL);
	int millis = te.tv_sec * 1000 + te.tv_usec / 1000;

	char buf[32];
	sprintf(buf, " [%010d]", millis);
	strcat(return_val, buf);

	return (char *)return_val;
}

void startSyslog(const char *ip, const char *port) {
	static string syslogName;
	systemCmd("killall -9 syslogd &");
	usleep(500 * 1000);
	systemCmd("syslogd -R %s:%s &", ip, port);
	usleep(500 * 1000); // Wait for syslogd to start, 300ms is usually enough
	g_sys_log_dbg_en = 1;
	syslogName		 = fca_getSerialInfo();
	openlog(syslogName.c_str(), LOG_PID | LOG_CONS, LOG_USER);	  // Reopen syslog with desired options
	APP_DBG("open syslog with serial: %s\n", syslogName.c_str());
}

void stopSyslog() {
	closelog();	   // Close the current syslog connection
	g_sys_log_dbg_en = 0;
	APP_DBG("close syslog\n");
}
