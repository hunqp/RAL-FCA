#ifndef __OSD_H__
#define __OSD_H__

#include "app_dbg.h"
#include "app_data.h"
#include "app_config.h"
#include "parser_json.h"

class OSDCtrl {
public:
	OSDCtrl(/* args */);
	~OSDCtrl();

	int loadConfigFromFile(FCA_OSD_S *watermark);
	int verifyConfig(FCA_OSD_S *watermark);
	int updateWatermark(FCA_OSD_S *watermark);

	int getWatermarkConfJs(json &json_in);

	FCA_OSD_S watermarkConf() const;
	void setWatermarkConf(const FCA_OSD_S *newWatermarkConf);
	void printWatermarkConfig();

private:
	FCA_OSD_S mWatermarkConf;
};

#endif
