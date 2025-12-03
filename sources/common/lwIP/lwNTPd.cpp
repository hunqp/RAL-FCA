#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "lwNTPd.hh"

#define VERSION_3 (3)
#define VERSION_4 (4)

#define MODE_CLIENT (3)
#define MODE_SERVER (4)

#define NTP_LI 			(0)
#define NTP_VN 			VERSION_3
#define NTP_MODE 		MODE_CLIENT
#define NTP_STRATUM 	(0)
#define NTP_POLL 		(4)
#define NTP_PRECISION 	(-6)

#define NTP_HLEN 	(48)
#define NTP_PORT 	(123)
#define TIMEOUT 	(5)
#define BUFSIZE 	(1500)

#define JAN_1970 	(0x83AA7E80)

#define NTP_CONV_FRAC32(x) (uint64_t)((x) * ((uint64_t)1 << 32))
#define NTP_REVE_FRAC32(x) ((double)((double)(x) / ((uint64_t)1 << 32)))

#define NTP_CONV_FRAC16(x) (uint32_t)((x) * ((uint32_t)1 << 16))
#define NTP_REVE_FRAC16(x) ((double)((double)(x) / ((uint32_t)1 << 16)))

#define USEC2FRAC(x) ((uint32_t)NTP_CONV_FRAC32((x) / 1000000.0))
#define FRAC2USEC(x) ((uint32_t)NTP_REVE_FRAC32((x) * 1000000.0))

#define NTP_LFIXED2DOUBLE(x) ((double)(ntohl(((struct LFixedPt_S *)(x))->intpart) - JAN_1970 + FRAC2USEC(ntohl(((struct LFixedPt_S *)(x))->fracpart)) / 1000000.0))

struct SFixedPt_S {
	uint16_t intpart;
	uint16_t fracpart;
};

struct LFixedPt_S {
	uint32_t intpart;
	uint32_t fracpart;
};

struct NTP_HDR_S {
#if __BYTE_ORDER == __BID_ENDIAN
	unsigned int li : 2;
	unsigned int vn : 3;
	unsigned int mode : 3;
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int mode : 3;
	unsigned int vn : 3;
	unsigned int li : 2;
#endif
	uint8_t stratum;
	uint8_t poll;
	int8_t precision;
	struct SFixedPt_S rtdelay;
	struct SFixedPt_S rtdispersion;
	uint32_t refid;
	struct LFixedPt_S refts;
	struct LFixedPt_S orits;
	struct LFixedPt_S recvts;
	struct LFixedPt_S transts;
};

static in_addr_t domainToIP4(const char *host) {
	in_addr_t saddr;
	struct hostent *hostent = NULL;

	if ((saddr = inet_addr(host)) == INADDR_NONE) {
		if ((hostent = gethostbyname(host)) == NULL) {
            return INADDR_NONE;
        }
		memmove(&saddr, hostent->h_addr, hostent->h_length);
	}
	return saddr;
}

static int buildPackagesSender(void *p, size_t *len) {
	struct timeval tv;
    struct NTP_HDR_S *hdr;

	if (!len || *len < NTP_HLEN) {
        return -1;
    }
	memset(p, 0, *len);
	hdr = (struct NTP_HDR_S *)p;
	hdr->li = NTP_LI;
	hdr->vn = NTP_VN;
	hdr->mode = NTP_MODE;
	hdr->stratum = NTP_STRATUM;
	hdr->poll = NTP_POLL;
	hdr->precision = NTP_PRECISION;
	gettimeofday(&tv, NULL);
	hdr->transts.intpart = htonl(tv.tv_sec + JAN_1970);
	hdr->transts.fracpart = htonl(USEC2FRAC(tv.tv_usec));
	*len = NTP_HLEN;

	return 0;
}

#if 0
static double calculateRoundTripDelay(const struct NTP_HDR_S *hdr, const struct timeval *recvtv) {
	double t1, t2, t3, t4;

	t1 = NTP_LFIXED2DOUBLE(&hdr->orits);
	t2 = NTP_LFIXED2DOUBLE(&hdr->recvts);
	t3 = NTP_LFIXED2DOUBLE(&hdr->transts);
	t4 = recvtv->tv_sec + recvtv->tv_usec / 1000000.0;

	return (t4 - t1) - (t3 - t2);
}
#endif

static double calculateOffset(const struct NTP_HDR_S *hdr, const struct timeval *recvtv) {
	double t1, t2, t3, t4;

	t1 = NTP_LFIXED2DOUBLE(&hdr->orits);
	t2 = NTP_LFIXED2DOUBLE(&hdr->recvts);
	t3 = NTP_LFIXED2DOUBLE(&hdr->transts);
	t4 = recvtv->tv_sec + recvtv->tv_usec / 1000000.0;

	return ((t2 - t1) + (t3 - t4)) / 2;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static pthread_t pid = (pthread_t)NULL;
static volatile bool boolean = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int LwNTPd::netTimeProtocolsUpdate(const char *domain) {	
	int sfd, mfd;
	size_t nbytes;
	double offset;
    fd_set readfds;
	struct tm *local = NULL;
	char chars[BUFSIZE] = {0};
    struct sockaddr_in servaddr;
	struct timeval timeout, recvtv, tv;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(NTP_PORT);
	servaddr.sin_addr.s_addr = domainToIP4(domain);

	if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		mImplDoUpdateFailure((char*)domain, (char*)strerror(errno));
		return -1;
	}

	if (connect(sfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) != 0) {
		mImplDoUpdateFailure((char*)domain, (char*)strerror(errno));
		close(sfd);
		return -2;
	}

	nbytes = BUFSIZE;
	if (buildPackagesSender(chars, &nbytes) != 0) {
		mImplDoUpdateFailure((char*)domain, (char*)"Build packages sender has failed");
		close(sfd);
		return -3;
	}
	send(sfd, chars, nbytes, 0);

	FD_ZERO(&readfds);
	FD_SET(sfd, &readfds);
	mfd = sfd + 1;

	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;

	if (select(mfd, &readfds, NULL, NULL, &timeout) > 0) {
		if (FD_ISSET(sfd, &readfds)) {
			if ((nbytes = recv(sfd, chars, BUFSIZE, 0)) < 0) {
				mImplDoUpdateFailure((char*)domain, (char*)strerror(errno));
				close(sfd);
				return -4;
			}

			gettimeofday(&recvtv, NULL);
			offset = calculateOffset((struct NTP_HDR_S *)chars, &recvtv);

			gettimeofday(&tv, NULL);
			tv.tv_sec += (int)offset;
			tv.tv_usec += offset - (int)offset;

			local = localtime((time_t *)&tv.tv_sec);

			mImplDoUpdateSuccess(local);

			close(sfd);
			return 0;
		}
	}
	close(sfd);
	mImplDoUpdateFailure((char*)domain, (char*)"select() or recv() failed");
	return -5;
}

void* LwNTPd::lwTimeServerPollingCalls(void *args) {
	LwNTPd *me = (LwNTPd*)args;
	
	char *listDomains[] = {
		"0.pool.ntp.org", 
		"1.pool.ntp.org", 
		"2.pool.ntp.org", 
		"3.pool.ntp.org", 
		"time.google.com"
	};
	const int totalDomains = sizeof(listDomains) / sizeof(listDomains[0]);

	me->mImplOpened();

	while (boolean) {
		for (uint8_t id = 0; id < totalDomains; ++id) {
			if (me->netTimeProtocolsUpdate((const char*)listDomains[id]) == 0) {
				break;
			}
		}
		usleep(me->mTimePeriodicUpdate * 1000);
	}

	me->mImplClosed();

	return (void *)0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LwNTPd::LwNTPd() {
	mImplOpened = [](){};
	mImplClosed = [](){};
	mImplDoUpdateSuccess = [](struct tm*){};
	mImplDoUpdateFailure = [](char*,char*){};
}

LwNTPd::~LwNTPd() {
	
}
void LwNTPd::begin() {
	boolean = true;
	if (!pid) {
		pthread_create(&pid, NULL, lwTimeServerPollingCalls, this);
	}
}

void LwNTPd::ForceClose() {
	boolean = false;
	if (pid) {
		pthread_join(pid, NULL);
		pid = (pthread_t)NULL;
	}
}

void LwNTPd::setTimePeriodicUpdate(uint32_t milliseconds) {
	mTimePeriodicUpdate = milliseconds;
}

uint32_t LwNTPd::getTimePeriodicUpdate(void) {
	return mTimePeriodicUpdate;
}

void LwNTPd::onOpened(std::function<void(void)> callback) {
	mImplOpened = callback;
}

void LwNTPd::onClosed(std::function<void(void)> callback) {
	mImplClosed = callback;
}

void LwNTPd::onDoUpdateSuccess(std::function<void(struct tm*)> callback) {
	mImplDoUpdateSuccess = callback;
}

void LwNTPd::onDoUpdateFailure(std::function<void(char*,char*)> callback) {
	mImplDoUpdateFailure = callback;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LwNTPd ntpd;