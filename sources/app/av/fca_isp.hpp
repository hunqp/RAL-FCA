#ifndef __ISPCTRL_H__
#define __ISPCTRL_H__

#include "fca_parameter.hpp"

#define DAY_NIGHT_TRIGGER_INTERVAL (3000)	 // 3s
#define DN_VIDEO_DEV VIDEO_DEV0
#define COLOR_ADJUSTMENT_MAX_THRESHOLD (50)

using namespace std;

class IspCtrl {
public:
	IspCtrl();
	~IspCtrl();

	int setDayNightConfig(const fca_dayNightParam_t &conf);
	int loadConfigFromFile(fca_cameraParam_t *camParam);
	int verifyConfig(fca_cameraParam_t *ispParam);
	int controlAntiFlickerMode(const int &mode);
	int controlFlipMirror(bool bFlip, bool bMirror);
	int imageAdjustments(int brightness, int contrast, int saturation, int sharpness);
	int controlLighting(bool b_Light);
	int controlCamWithNewParam(const string &type, fca_cameraParam_t *param);

	int getCamParamJs(json &json_in);
	fca_cameraParam_t camParam() const;
	void setCamParam(const fca_cameraParam_t *newCamParam);
	void printCameraparamConfig();
	int getDayNightState();
	void getDayNightData(json &data);

	int controlSmartLight(int smartLightMode);
	int controlWDR();
	int controlDNR();
	int controlAWB();

	std::atomic<int> dayNightState; 
	uint8_t lightingChangeCount;
	uint32_t lightingCheckStartTime;
	fca_cameraParam_t &CamParam = mCamParam;
	
private:
	fca_cameraParam_t mCamParam;
};

extern IspCtrl ispCtrl;

#endif
