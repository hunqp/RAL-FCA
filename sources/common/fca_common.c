#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "app_dbg.h"

int systemCmd(const char *fmt, ...) {
	if (fmt == NULL) {
		APP_DBG_CMD("cmd is null\n");
		return -1;
	}

	FILE *fp;
	int ret = 0;
	char buf[256];
	memset(buf, 0, sizeof(buf));
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	APP_DBG_CMD("cmd: %s\n", buf);

	if ((fp = popen(buf, "r")) == NULL) {
		perror("popen");
		APP_DBG_CMD("popen error: %s\n", strerror(errno));
		return -1;
	}

	ret = pclose(fp);
	if (ret == -1) {
		APP_DBG_CMD("close open file pointer fp error\n");
		return ret;
	}
	else if (ret == 0) {
		return ret;
	}
	else {
		APP_DBG_CMD("popen ret is: %d\n", ret);
		return ret;
	}
}