#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"

std::string deviceSerialNumber;
std::string defaultConfigurationPath;
std::string vendorsConfigurationPath;

bool selectedBluetoothMode;
std::string vendorsHostapdSsid;
std::string vendorsHostapdPssk;

static void setupEnvirVariables(const char *params) {
	try {
		nlohmann::json js;
		if (read_json_file(js, std::string(params))) {
			deviceSerialNumber = js["SerialNumber"].get<std::string>();
			defaultConfigurationPath = js["DefaultConfigurationPath"].get<std::string>();
			vendorsConfigurationPath = js["VendorsConfigurationPath"].get<std::string>();
			vendorsHostapdSsid = js["HostapdSSID"].get<std::string>();
			vendorsHostapdPssk = js["HostapdPSSK"].get<std::string>();
			selectedBluetoothMode = js["SelectedBluetoothMode"].get<bool>();

			APP_PRINT("Serial number: %s\r\n", deviceSerialNumber.c_str());
			APP_PRINT("Default configuration path: %s\r\n", defaultConfigurationPath.c_str());
			APP_PRINT("Vendors configuration path: %s\r\n", vendorsConfigurationPath.c_str());
			return;
		}
	}
	catch (const std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	exit(EXIT_FAILURE);
}

static void testFuncs() {
	motors.initialise(NULL, NULL);

	while (1) {
		char let = getchar();
		switch (let) {
		case '0': {
			motors.run(FCA_PTZ_DIRECTION_UP);
		}
		break;

		case '1': {
			motors.run(FCA_PTZ_DIRECTION_DOWN);
		}
		break;

		case '2': {
			motors.run(FCA_PTZ_DIRECTION_LEFT);
		}
		break;

		case '3': {
			motors.run(FCA_PTZ_DIRECTION_RIGHT);
		}
		break;

		case '4': {
			int horizone, vertical;
			motors.getPosition(&horizone, &vertical);
			printf("Horizone: %d\r\n", horizone);
			printf("Vertical: %d\r\n", vertical);
		}
		break;

		case '5': {
			motors.run(FCA_PTZ_DIRECTION_UPRIGHT);
		}
		break;

		case '6': {
			motors.run(FCA_PTZ_DIRECTION_DOWNLEFT);
		}
		break;

		case '7': {
			motors.run(FCA_PTZ_DIRECTION_DOWNRIGHT);
		}
		break;

		case '8': {
			motors.run(FCA_PTZ_DIRECTION_STOP);
		}
		break;
		
		default:
		break;
		}
	}
}

#include "wifi.h"
#include "fca_bluetooth.h"

void task_init(const char *params) {
	signal(SIGPIPE, SIG_IGN);

	if (params) {
		setupEnvirVariables(params);
	}

	/* Setup logger for application */
	RUNTIME_LOGGER.filename = (const char*)"/tmp/runtime.log";
	RUNTIME_LOGGER.maxBytesCanBeHold = (50 * 1024);
	JOURNAL_LOGGER.filename = strdup(FCA_VENDORS_FILE_LOCATE("journal.log").c_str());
	RUNTIME_LOGGER.maxBytesCanBeHold = (10 * 1024);


	// int rc = fca_bluetooth_start(deviceSerialNumber.c_str(), vendorsHostapdSsid.c_str(), vendorsHostapdPssk.c_str());
	// if (rc != 0) {
	// 	SYSE("Can't start bluetooth mode\r\n");
	// }
	// else APP_PRINT("Bluetooth mode has started SN {%s}, SSID {%s}, PSSK {%s}\r\n", deviceSerialNumber.c_str(), vendorsHostapdSsid.c_str(), vendorsHostapdPssk.c_str());

	// while (1) {
	// 	char let = getchar();

	// 	printf("let: %c\r\n", let);

	// 	if (let == 'a') {
	// 		fca_wifi_stop_ap_mode(FCA_NET_WIFI_AP_IF_NAME);
	// 		fca_bluetooth_start(deviceSerialNumber.c_str(), vendorsHostapdSsid.c_str(), vendorsHostapdPssk.c_str());
			
	// 		// int rc = fca_bluetooth_start(deviceSerialNumber.c_str(), vendorsHostapdSsid.c_str(), vendorsHostapdPssk.c_str());
	// 		APP_PRINT("Bluetooth mode has started SN {%s}, SSID {%s}, PSSK {%s}\r\n", deviceSerialNumber.c_str(), vendorsHostapdSsid.c_str(), vendorsHostapdPssk.c_str());
	// 	}
	// 	else if (let == 'b') {
	// 		fca_bluetooth_close();
	// 		fca_wifi_start_ap_mode(FCA_NET_WIFI_AP_IF_NAME, vendorsHostapdSsid.c_str(), vendorsHostapdPssk.c_str(), NETWORK_AP_HOST_IP);
	// 		APP_PRINT("Hostapd mode has started SN {%s}, SSID {%s}, PSSK {%s}\r\n", deviceSerialNumber.c_str(), vendorsHostapdSsid.c_str(), vendorsHostapdPssk.c_str());
	// 	}
	// }
	// // testFuncs();

	// // create folder save jpeg motion tmp
	// struct stat st;
	// if (stat(FCA_JPEG_MOTION_TMP_PATH, &st) == -1) {
	// 	mkdir(FCA_JPEG_MOTION_TMP_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	// }

	// if (stat(FCA_MEDIA_MP4_PATH, &st) == -1) {
	// 	mkdir(FCA_MEDIA_MP4_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	// }

	// ledStatus.init();
	motors.initialise(NULL, NULL);

	caSslPath = FCA_VENDORS_FILE_LOCATE(FCA_NETWORK_CA_FILE);
	if (!doesFileExist(caSslPath.c_str())) {
		caSslPath = FCA_DEFAULT_FILE_LOCATE(FCA_NETWORK_CA_FILE);
	}

	offlineEventManager.setWaitTimeUpdate(true);
}
