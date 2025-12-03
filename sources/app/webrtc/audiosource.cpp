#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "audiosource.hpp"

AudioSource::AudioSource(VV_RB_MEDIA_HANDLE_T producer, uint64_t sampleDurationUs) : StreamSource(producer, 312) {
	sampleDuration_us = sampleDurationUs;
}

AudioSource::~AudioSource() {

}

void AudioSource::start() {
	sampleTime_us = std::numeric_limits<uint64_t>::max() - sampleDuration_us + 1;
	sampleTime_us += sampleDuration_us;
}

void AudioSource::stop() {
	sample = {};
	sampleTime_us = 0;
}

void AudioSource::loadNextSample() {
	StreamSource::loadNextSample();
	if (sample.size()) {
		sampleTime_us += sampleDuration_us;
	}
}
