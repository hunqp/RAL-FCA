#ifndef __FCA_VIDEO_HPP__
#define __FCA_VIDEO_HPP__

#include "mediabuffer.h"
#include "fca_parameter.hpp"

class VideoHelpers {
public:
	 VideoHelpers();
	~VideoHelpers();

	void initialise(void);
	void deinitialise(void);
	void startStreamChannels(void);
	void closeStreamChannels(void);

	FCA_ENCODE_S getEncoders();
	bool setEncoders(FCA_ENCODE_S *encoders);
	void dummy(void);

	/* OSD */
	bool setOsd(FCA_OSD_S *osds);

private:
	bool validEncoders(FCA_ENCODE_S *encoders);

public:
	static VV_RB_MEDIA_HANDLE_T VIDEO0_PRODUCER;
	static VV_RB_MEDIA_HANDLE_T VIDEO1_PRODUCER;

private:
	FCA_ENCODE_S mEncoders;
};

#endif /* __FCA_VIDEO_HPP__ */
