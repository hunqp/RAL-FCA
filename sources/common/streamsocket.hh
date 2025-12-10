#ifndef LW_CGI_HH
#define LW_CGI_HH

#include <string>
#include <pthread.h>
#include <functional>

typedef struct {
    std::string str;
} SS_INGOING_S;

typedef struct {
    std::string str;
} SS_OUGOING_S;

class SocketStream {
public:
     SocketStream();
    ~SocketStream();

    virtual bool doOpen(int usPort);
    virtual void doClose(void);
    virtual void onOpened(std::function<void(void)> FuncCallback);
    virtual void onClosed(std::function<void(void)> FuncCallback);
    virtual void onIncome(std::function<void(SS_INGOING_S&,SS_OUGOING_S&)> FuncCallback);

private:
    static void *socketStreamListenerCalls(void *args);
    static void *readFrom(void *args);

protected:
    int mFd;
    int mPort;
    volatile bool mLoop;
    pthread_t mPid = (pthread_t)NULL;
    std::function<void(void)> mImplOpened;
	std::function<void(void)> mImplClosed;
    std::function<void(SS_INGOING_S&,SS_OUGOING_S&)> mImplImcome;
};

#endif /* LW_CGI_HH */