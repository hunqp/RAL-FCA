#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <curl/curl.h>

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"

#include "ota.h"
#include "firmware.h"
#include "task_list.h"

#define UPGRADE_PID_FILE			 "/var/run/fota.pid"
#define DISTANCE_RESPOND_PROCESS_OTA (10)

static void otaRespondProcess(int otaProcess) {
	json resJSon				  = json::object();
	resJSon["Id"]				  = fca_getSerialInfo();
	resJSon["Command"]			  = "OTAfirmware";
	resJSon["Type"]				  = "Respond";
	resJSon["Content"]["Process"] = otaProcess;
	APP_DBG("data ota process: %s", resJSon.dump().data());
	sendMsgRespondProcess(resJSon.dump());
}

double fca_otaNextProcess = 0;
int fca_otaProcess(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	(void)clientp;
	(void)ultotal;
	(void)ulnow;
	if (dltotal == 0)
		return 0;

	double process = (dlnow / dltotal) * 100.0;
	APP_DBG("Process OTA: %.2f%%\n", process);

	if ((fca_otaNextProcess + DISTANCE_RESPOND_PROCESS_OTA) <= process || fca_otaNextProcess > process || process == 100) {
		if (fca_otaNextProcess != process) {
			fca_otaNextProcess = process;
			APP_DBG("\rSend message json process: %d \n", (int)fca_otaNextProcess);
			otaRespondProcess((int)fca_otaNextProcess);
		}
	}

	return 0;
}

static size_t write_file(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

int fca_otaDownloadFile(const string &url, const string &file, uint32_t timeout) {
	CURLcode curl_ret = CURLE_OK;
	CURL *handle	  = NULL;
	FILE *f			  = NULL;
	long http_ret	  = 0;
	int state		  = FCA_OTA_FAIL;

	/* curl init */
	handle = curl_easy_init();
	if (!handle) {
		APP_DBG("[ota-download] curl_easy_init failed: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
		return FCA_OTA_FAIL;
	}

	do {
		f = fopen(file.data(), "wb");
		if (!f) {
			APP_DBG("[ota-download] open file %s failed: %s\n", file.data(), strerror(errno));
			state = FCA_OTA_FAIL;
			break;
		}

		curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, fca_otaProcess);
		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);

		curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(handle, CURLOPT_URL, url.data());
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, f);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_file);

		/* set bundle file for download from https */
		curl_easy_setopt(handle, CURLOPT_CAINFO, caSslPath.c_str());

		curl_ret = curl_easy_perform(handle);
		if (curl_ret != CURLE_OK) {
			APP_DBG("[ota-download] curl_easy_perform failed: %s - %d\n", curl_easy_strerror(curl_ret), curl_ret);
			if (curl_ret == CURLE_OPERATION_TIMEDOUT)
				state = FCA_OTA_DL_TIMEOUT;
			else
				state = FCA_OTA_FAIL;
			break;
		}

		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_ret);
		if (http_ret == 200 && curl_ret != CURLE_ABORTED_BY_CALLBACK) {
			state = FCA_OTA_OK;
			break;
		}
		else {
			APP_DBG("[ota-download] https return code: %ld\n", http_ret);
			state = FCA_OTA_FAIL;
			break;
		}
	} while (0);

	if (f) {
		fclose(f);
	}
	if (handle) {
		curl_easy_cleanup(handle);
	}

	return state;
}

const char *fca_otaStatusUpgrade(int state) {
	switch (state) {
	case FCA_OTA_STATE_DL_FAIL: {
		return "download-failed";
	} break;
	case FCA_OTA_STATE_DL_SUCCESS: {
		return "download-success";
	} break;
	case FCA_OTA_STATE_UPGRADE_SUCCESS: {
		return "upgrade-success";
	} break;
	default: {
		return "idle";
	} break;
	}
}
