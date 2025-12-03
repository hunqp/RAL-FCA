#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "app.h"
#include "fca_audio.hpp"

VV_RB_MEDIA_HANDLE_T AudioHelpers::AUDIO0_PRODUCER = NULL;
VV_RB_MEDIA_HANDLE_T AudioHelpers::AUDIO1_PRODUCER = NULL;

AudioHelpers::AudioHelpers() {
	AudioHelpers::AUDIO0_PRODUCER = ringBufferCreateProducer(200 * 1024);
	AudioHelpers::AUDIO1_PRODUCER = ringBufferCreateProducer(200 * 1024);
}

AudioHelpers::~AudioHelpers() {
	ringBufferDelete(AudioHelpers::AUDIO0_PRODUCER);
	ringBufferDelete(AudioHelpers::AUDIO1_PRODUCER);
}

extern uint64_t nowInUs();

static int captureStream0Channels(const fca_audio_frame_t* Frame) {
	VV_RB_MEDIA_RESOURCE_S resource = {
		.id = Frame->sequence,
		.pData = Frame->data,
		.dataLen = Frame->size,
		.timestamp = nowInUs(),
		.framePerSeconds = 13,
		.type = VV_RB_MEDIA_FRAME_HDR_TYPE_AUDIO,
		.encoder = VV_RB_MEDIA_ENCODER_FRAME_ALAW
	};
	ringBufferSendTo(AudioHelpers::AUDIO0_PRODUCER, &resource);

	return 0;
}

static int captureStream1Channels(const fca_audio_frame_t* Frame) {
	VV_RB_MEDIA_RESOURCE_S resource = {
		.id = Frame->sequence,
		.pData = Frame->data,
		.dataLen = Frame->size,
		.timestamp = nowInUs(),
		.framePerSeconds = (Frame->size * 1000000) / 8000,
		.type = VV_RB_MEDIA_FRAME_HDR_TYPE_AUDIO,
		.encoder = VV_RB_MEDIA_ENCODER_FRAME_AAC
	};
	ringBufferSendTo(AudioHelpers::AUDIO1_PRODUCER, &resource);

	return 0;
}

void AudioHelpers::initialise() {
	FCA_API_ASSERT(fca_audio_in_init()  == 0);
	FCA_API_ASSERT(fca_audio_out_init() == 0);
}

void AudioHelpers::deinitialise() {
	FCA_API_ASSERT(fca_audio_in_uninit() == 0);
    FCA_API_ASSERT(fca_audio_out_uninit() == 0);
}

void AudioHelpers::startStreamChannels() {
	const int DEFAULT_VOLUME_IN = 85;
	const int DEFAULT_VOLUME_OU = 100;

	fca_audio_config_t configure;
	
	FCA_API_ASSERT(fca_set_audio_in_volume(DEFAULT_VOLUME_IN) == 0);

	/* Initialise ALaw Stream */
    configure.channel = 1;
    configure.codec = FCA_AUDIO_CODEC_G711_ALAW;
    configure.format = 16;
    configure.rate = FCA_AUDIO_SAMPLE_RATE_8_KHZ;
    FCA_API_ASSERT(fca_audio_in_set_config(0, &configure) == 0);
    FCA_API_ASSERT(fca_audio_in_start_callback(0, captureStream0Channels) == 0);

	/* Initialise AAC Stream */
	if (0) {
		configure.channel = 1;
		configure.codec = FCA_AUDIO_CODEC_AAC;
		configure.format = 16;
		configure.rate = FCA_AUDIO_SAMPLE_RATE_8_KHZ;
		FCA_API_ASSERT(fca_audio_in_set_config(1, &configure) == 0);
		FCA_API_ASSERT(fca_audio_in_start_callback(1, captureStream1Channels) == 0);
	}

	/* Initialise audio output */
    configure.channel = 1;
    configure.codec = FCA_AUDIO_CODEC_PCM;
    configure.format = 16;
    configure.rate = FCA_AUDIO_SAMPLE_RATE_8_KHZ;
    FCA_API_ASSERT(fca_audio_out_set_config(&configure) == 0);
    FCA_API_ASSERT(fca_audio_out_set_volume(DEFAULT_VOLUME_OU) == 0);
}

void AudioHelpers::closeStreamChannels(void) {
	FCA_API_ASSERT(fca_audio_in_stop_callback(0) == 0);
}

void AudioHelpers::silent() {
	mSirenBoolean = false;
    mSoundQueue.removePending();
	FCA_API_ASSERT(fca_audio_out_stop() == 0);
}

void AudioHelpers::playSiren(int collapseAfterSecs) {
	mSirenBoolean = true;
    const std::string filename = FCA_SOUND_MOTION_ALARM_FILE;

    mSoundQueue.dispatch([filename, collapseAfterSecs, this]() {
        FILE *fp = NULL;
        int readBytes = 0;
        unsigned char bytes[640];
        uint32_t durationSecs = time(NULL);

		while (mSirenBoolean && (time(NULL) - durationSecs < (uint32_t)collapseAfterSecs)) {
            fp = fopen(filename.c_str(), "r");
            if (!fp) {
                break;
            }
            do {
                memset(bytes, 0, sizeof(bytes));
                readBytes = fread(bytes, sizeof(char), sizeof(bytes), fp);
                if (readBytes > 0) {
                    fca_audio_out_play_frame((unsigned char*)bytes, readBytes);
                }
                usleep(10000);
            }
            while (readBytes > 0);
            fclose(fp);
        }
        mSirenBoolean = false;
	});
}

void AudioHelpers::playBuffers(char *data, int len) {
	std::vector<uint8_t> frame(data, data + len);

    mSoundQueue.dispatch([frame = std::move(frame)]() mutable {
        size_t aLawBufferSize = frame.size();
        std::vector<uint8_t> pcmBuffer(aLawBufferSize * 2);
        aLaw2PCM(frame.data(), pcmBuffer.data(), aLawBufferSize);

		int sentBytes = 0;
		int remainBytes = pcmBuffer.size();

		while (remainBytes > 0) {
			int size = (remainBytes > 640) ? 640 : remainBytes;
			fca_audio_out_play_frame(pcmBuffer.data() + sentBytes, size); /* This api only support max frame 640 */
			remainBytes -= size;
			sentBytes += size;
		}	
    });
}

int AudioHelpers::setMicVolume(int value) {
	return fca_set_audio_in_volume(value);
}

int AudioHelpers::setSpeakerVolume(int value) {
	return fca_audio_out_set_volume(value);
}

void AudioHelpers::notifySalutation() {
	const std::string filename = FCA_SOUND_HELLO_DEVICE_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}

void AudioHelpers::notifyFactoryReset() {
	const std::string filename = FCA_SOUND_REBOOT_DEVICE_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}

void AudioHelpers::notifyWiFiIsConnecting() {
	const std::string filename = FCA_SOUND_WAIT_CONNECT_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}

void AudioHelpers::notifyWiFiHasConnected() {
	const std::string filename = FCA_SOUND_NET_CONNECT_SUCCESS_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}

void AudioHelpers::notifyBluetoothHasConnected() {
	const std::string filename = FCA_SOUND_BLE_CONNECTED_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}

void AudioHelpers::notifyBluetoothHasDisconnected() {
	const std::string filename = FCA_SOUND_BLE_DISCONNECTED_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}

void AudioHelpers::notifyChangeBluetoothMode() {
	const std::string filename = FCA_SOUND_CHANGE_BLE_MODE_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}

void AudioHelpers::notifyChangeWiFiMode() {
	const std::string filename = FCA_SOUND_CHANGE_WIFI_MODE_FILE;
	if (access(filename.c_str(), F_OK) != 0) {
		return;
	}
    mSoundQueue.dispatch([filename]() {
		FCA_API_ASSERT(fca_audio_out_play_file_block((char*)filename.c_str()) == 0);
        sleep(2);
	});
}
