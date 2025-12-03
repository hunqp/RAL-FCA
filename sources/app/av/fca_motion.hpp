#ifndef __FCA_MOTION_H__
#define __FCA_MOTION_H__

typedef enum {
    DETECT_TYPE_NONE = 0,
    DETECT_TYPE_MOTION,
    DETECT_TYPE_HUMAN,
	DETECT_TYPE_BABY_CRYING, /* Support in future */
} EVENT_DETECT_TYPE;

typedef enum {
    LOW_LEVEL 		= 0,
    MEDIUM_LEVEL	= 1,
    HIGH_LEVEL		= 2,
	VERY_HIGH_LEVEL,
	EXTREME_LEVEL,
	MAX_LEVEL,
} EVENT_LEVEL_SENSITIVITY;

typedef enum {
    DETECTED_MOTION = 0,
    DETECTED_HUMAN_FACE,
    DETECTED_HUMAN_BODY,
	DETECTED_BABY_CRYING,
} EVENT_DETECT_RESULT;

class EventDetection {
public:
	EventDetection();
	~EventDetection();

	int initialise();
	void enableDectection();
	void disableDetection();
	int start();
	int stop();
	int validConfig(fca_motionSetting_t *config);
	int setConfig(fca_motionSetting_t *config);
	bool isDetected();
	void setStatus(bool boolean);
	void setPaused(bool paused);
	bool isPaused();

	void print();
public:
	fca_motionSetting_t * const config = &mConfig;

private:
	fca_motionSetting_t mConfig;
	EVENT_DETECT_TYPE mType;
	bool mDetected;
	pthread_mutex_t mMutex;
	bool mPaused;
};

extern EventDetection eventDetection;

#endif	  // __FCA_MOTION_H__
