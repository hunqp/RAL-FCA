#ifndef LW_NTPD_HH
#define LW_NTPD_HH

#include <stdint.h>
#include <functional>

class LwNTPd {
public:
     LwNTPd();
    ~LwNTPd();

    void onOpened(std::function<void(void)> callback);
    void onClosed(std::function<void(void)> callback);
    void onDoUpdateSuccess(std::function<void(struct tm*)> callback);
    void onDoUpdateFailure(std::function<void(char*,char*)> callback);

    void begin();
    void ForceClose();
    void setTimePeriodicUpdate(uint32_t milliseconds);
    uint32_t getTimePeriodicUpdate(void);

private:
    int netTimeProtocolsUpdate(const char *domain);
    static void *lwTimeServerPollingCalls(void *args);

private:
    uint32_t mTimePeriodicUpdate = 1000;

    std::function<void(void)> mImplOpened;
    std::function<void(void)> mImplClosed;
    std::function<void(struct tm*)> mImplDoUpdateSuccess;
    std::function<void(char*,char*)> mImplDoUpdateFailure;
};

extern LwNTPd ntpd;

#endif /* LW_NTPD_HH */
