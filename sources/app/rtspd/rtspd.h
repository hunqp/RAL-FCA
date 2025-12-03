#ifndef _RTSPD_H_
#define _RTSPD_H_

#include "rtspd555.hh"
#include "mediabuffer.h"

#define configDEFAULT_RESOURCE0	        (const char*)"/profile1" /* rtsp://<IP_ADDRESS>:<PORT>/profile1 */
#define configDEFAULT_RESOURCE1	        (const char*)"/profile2" /* rtsp://<IP_ADDRESS>:<PORT>/profile2 */

class RTSPD : public RTSPD555 {
public:
     RTSPD();
    ~RTSPD();

    bool begin(int usPortnum = 554) override;
    void addMediaSession0(std::string streamName, std::string description, 
        int videoEncoder, int audioEncoder, int channels, int bitsamples, int samplerate);
    void addMediaSession1(std::string streamName, std::string description, 
        int videoEncoder, int audioEncoder, int channels, int bitsamples, int samplerate);

private:
    VV_RB_MEDIA_HANDLE_T mConsumerGettingVIDEO0;
    VV_RB_MEDIA_HANDLE_T mConsumerGettingVIDEO1;
    VV_RB_MEDIA_HANDLE_T mConsumerGettingAUDIO0;
};

#endif /* _RTSPD_H_ */