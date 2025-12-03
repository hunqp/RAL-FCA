#include <string>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>

#include "BasicUsageEnvironment.hh"
#include "H265LiveSources.hh"
#include "H264LiveSources.hh"
#include "ULAWLiveSources.hh"
#include "ALAWLiveSources.hh"
#include "announceURL.hh"
#include "rtspd555.hh"

typedef struct {
    pthread_t pid;
    std::function<void(void)> implOpened = []() {};
    std::function<void(void)> implClosed = []() {};

    /* LIVE555 */
    EventLoopWatchVariable watchVariable;
    UsageEnvironment* env = NULL;
    TaskScheduler* scheduler = NULL;
    RTSPServer *rtspSrvr = NULL;
    UserAuthenticationDatabase* authDB = NULL;
} RTSPD555_INSTANCE_S;

static ServerMediaSession * createSMSChannels(UsageEnvironment &env, RSTPD555_MEDIA_SESSION_S &ms) {
	ServerMediaSession *sms = ServerMediaSession::createNew(env, ms.streamName.c_str(), NULL, ms.description.c_str());
	if (sms) {
		if (ms.video.encoder != RTSPD555_MEDIA_ENCODER_NONE_ID) {
			ServerMediaSubsession *viss = NULL;
			if (ms.video.encoder == RTSPD555_VIDEO_ENCODER_H265_ID) {
				viss = VideoH265Subsession::createNew(env, NULL, True, ms.video.doGettingFramed);
			}
			else if (ms.video.encoder == RTSPD555_VIDEO_ENCODER_H264_ID) {
				viss = VideoH264Subsession::createNew(env, NULL, True, ms.video.doGettingFramed);
			}
			assert(viss != NULL);
			sms->addSubsession(viss);
		}
		
		if (ms.audio.encoder != RTSPD555_MEDIA_ENCODER_NONE_ID) {
			if (ms.audio.encoder == RTSPD555_AUDIO_ENCODER_ALAW_ID) {
				AudioALAWSubsession *auss = AudioALAWSubsession::createNew(env, NULL, True, ms.audio.doGettingFramed);
				assert(auss != NULL);
				auss->setParameters(ms.audio.channels, ms.audio.samplerate, ms.audio.bitsamples);
				sms->addSubsession(static_cast<ServerMediaSubsession*>(auss));
			}
			else if (ms.audio.encoder == RTSPD555_AUDIO_ENCODER_ULAW_ID) {
				AudioULAWSubsession *auss = AudioULAWSubsession::createNew(env, NULL, True, ms.audio.doGettingFramed);
				assert(auss != NULL);
				auss->setParameters(ms.audio.channels, ms.audio.samplerate, ms.audio.bitsamples);
				sms->addSubsession(static_cast<ServerMediaSubsession*>(auss));
			}
		}
	}

	return sms;
}

static void *doEventLoop(void *args) {
    RTSPD555_INSTANCE_S *ins = (RTSPD555_INSTANCE_S*)args;

    ins->implOpened();
    ins->env->taskScheduler().doEventLoop(&ins->watchVariable);
    ins->implClosed();

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
RTSPD555::RTSPD555() {

}

RTSPD555::~RTSPD555() {

}

bool RTSPD555::begin(int usPortnum) {
    if (usPortnum < 0) {
        return false;
    }
    RTSPD555_INSTANCE_S *newInstance = new RTSPD555_INSTANCE_S();
    if (!newInstance) {
        return false;
    }
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
    RTSPServer *rtspSrvr = RTSPServer::createNew(*env, usPortnum, NULL);
    if (!rtspSrvr) {
        env->reclaim();
        delete scheduler;
        delete newInstance;
        return false;
	}
    newInstance->env = env;
    newInstance->scheduler = scheduler;
    newInstance->rtspSrvr = rtspSrvr;
    /* Assignee instance */
    mInstance = newInstance;
    return true;
}

void RTSPD555::close() {
    if (!mInstance) {
        return;
    }
    RTSPD555_INSTANCE_S *ins = (RTSPD555_INSTANCE_S*)mInstance;

    ins->watchVariable = 1;
    pthread_join(ins->pid, NULL);
    /* Proper LIVE555 cleanup */
    Medium::close(ins->rtspSrvr);
    if (ins->authDB) {
        delete ins->authDB;
    }
    ins->env->reclaim();
    delete ins->scheduler;
    delete ins;
}

void RTSPD555::run() {
    if (!mInstance) {
        return;
    }
    RTSPD555_INSTANCE_S *ins = (RTSPD555_INSTANCE_S*)mInstance;
    ins->watchVariable = 0;
    pthread_create(&ins->pid, NULL, doEventLoop, mInstance);
}

void RTSPD555::addMediaSubsession(RSTPD555_MEDIA_SESSION_S &ms) {
    if (!mInstance) {
        return;
    }
    RTSPD555_INSTANCE_S *ins = (RTSPD555_INSTANCE_S*)mInstance;
    ServerMediaSession *sms = createSMSChannels(*ins->env, ms);
    ins->rtspSrvr->addServerMediaSession(sms);
}

void RTSPD555::onOpened(std::function<void(void)> callback) {
    if (!mInstance) {
        return;
    }
    ((RTSPD555_INSTANCE_S*)mInstance)->implOpened = callback;
}

void RTSPD555::onClosed(std::function<void(void)> callback) {
    if (!mInstance) {
        return;
    }
    ((RTSPD555_INSTANCE_S*)mInstance)->implClosed = callback;
}
