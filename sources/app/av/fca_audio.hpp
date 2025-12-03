#ifndef __FCA_AUDIO_HPP__
#define __FCA_AUDIO_HPP__

#include "g711.h"
#include "mediabuffer.h"
#include "dispatchqueue.hpp"
#include "fca_parameter.hpp"

class AudioHelpers {
public:
	 AudioHelpers();
	~AudioHelpers();

	void initialise();
	void deinitialise();
	void startStreamChannels(void);
	void closeStreamChannels(void);

	void silent();
	void playSiren(int collapseAfterSecs);
    void playBuffers(char *data, int len);
	int  setMicVolume(int value);
	int  setSpeakerVolume(int value);

	void notifySalutation();
    void notifyFactoryReset();
    void notifyWiFiIsConnecting();
    void notifyWiFiHasConnected();
	void notifyBluetoothHasConnected();
	void notifyBluetoothHasDisconnected();
	void notifyChangeBluetoothMode();
	void notifyChangeWiFiMode();

public:
	static VV_RB_MEDIA_HANDLE_T AUDIO0_PRODUCER;
	static VV_RB_MEDIA_HANDLE_T AUDIO1_PRODUCER;

private:
	bool mSirenBoolean;
	std::string mFolder;
	DispatchQueue mSoundQueue = DispatchQueue("SoundQueue");
};

#endif /* __FCA_AUDIO_HPP__ */
