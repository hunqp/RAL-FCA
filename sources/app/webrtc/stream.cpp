#include <string.h>
#include <sys/time.h>
#include "stream.hpp"

StreamSource::StreamSource(VV_RB_MEDIA_HANDLE_T producer, uint32_t preallocateBufferSize) {
	consumer = ringBufferCreateConsumer(producer, preallocateBufferSize);
}

StreamSource::~StreamSource() {
	ringBufferDelete(consumer);
}

void StreamSource::stop() {

}

void StreamSource::start() {

}

void StreamSource::loadNextSample() {
	int rc = -1;
	VV_RB_MEDIA_FRAMED_S Frame;
	
	sample = {};
	rc = ringBufferReadFrom(consumer, &Frame);
	if (rc == 0) {
		sample.insert(sample.end(), reinterpret_cast<std::byte*>(Frame.pData), reinterpret_cast<std::byte*>(Frame.pData) + Frame.dataLen);
	}
}

rtc::binary StreamSource::getSample() {
	return sample;
}

uint64_t StreamSource::getSampleTime_us() {
	return sampleTime_us;
}

uint64_t StreamSource::getSampleDuration_us() {
	return sampleDuration_us;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

Stream::Stream(
    std::shared_ptr<StreamSource> video0, 
    std::shared_ptr<StreamSource> video1, 
    std::shared_ptr<StreamSource> audio0)
	: std::enable_shared_from_this<Stream>(), video0(video0), video1(video1), audio0(audio0)
{
	mMutex = PTHREAD_MUTEX_INITIALIZER;
}


Stream::~Stream() {
    stop();
}

void Stream::start() {
    pthread_mutex_lock(&mMutex);

	audio0->start();
	video0->start();
	video1->start();

	pthread_mutex_unlock(&mMutex);
}

void Stream::stop() {
    pthread_mutex_lock(&mMutex);

	audio0->stop();
	video0->stop();
	video1->stop();

	pthread_mutex_unlock(&mMutex);
};

uint64_t currentTimeInMicroSeconds() {
    struct timeval time;
	gettimeofday(&time, NULL);
	return uint64_t(time.tv_sec) * 1000 * 1000 + time.tv_usec;
}
