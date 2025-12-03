#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "lwcgi.hh"

typedef struct {
    int cFd;
    SocketStream *ss;
} SS_INGOING_CONTEXT_S;

static int createSocketStream(int usPort) {
    int sFd = -1;
    struct sockaddr_in saddrin = {0};

    sFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sFd < 0) {
        perror("socket()");
        return -1;
    }
    int reuse = 1;
    setsockopt(sFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    saddrin.sin_family = AF_INET;
    saddrin.sin_addr.s_addr = INADDR_ANY;
    saddrin.sin_port = htons(usPort);

    if (bind(sFd, (struct sockaddr*)&saddrin, sizeof(saddrin)) < 0) {
        perror("bind()");
        close(sFd);
        return -1;
    }
    if (listen(sFd, 32) < 0) { 
        perror("listen()");
        close(sFd);
        return -1;
    }

    int flags = fcntl(sFd, F_GETFL, 0);
    fcntl(sFd, F_SETFL, flags | O_NONBLOCK);

    return sFd;
}

static void closeSocketStream(int *sFd) {
    if (sFd && (*sFd) > 0) {
        close(*sFd);
        *sFd = -1;
    }
}

void* SocketStream::readFrom(void *args) {
    char buffers[4096];
    size_t readBytes = 0;
    SS_OUGOING_S ougoing;
    SS_INGOING_S ingoing;

    int cFd = ((SS_INGOING_CONTEXT_S*)args)->cFd;
    SocketStream *ss = (SocketStream*)((SS_INGOING_CONTEXT_S*)args)->ss;

    /* Read all available data until EOF or catching ERROR */
    if ((readBytes = recv(cFd, buffers, sizeof(buffers), 0)) > 0) {
        ingoing.str = std::string(buffers, readBytes);
    }

    if (ss->mImplImcome) {
        ss->mImplImcome(ingoing, ougoing);
    }

    if (ougoing.str.length()) {
        size_t totalBytes = ougoing.str.length();
        const char *ptr = ougoing.str.data();
        while (totalBytes > 0) {
            ssize_t sentBytes = send(cFd, ptr, totalBytes, MSG_NOSIGNAL);
            if (sentBytes <= 0) {
                break;
            }
            ptr += sentBytes;
            totalBytes -= sentBytes;
        }
    }
    close(cFd);
    free(args);

    return NULL;
}

void* SocketStream::socketStreamListenerCalls(void *args) {
    SocketStream *ss = (SocketStream*)args;

    ss->mImplOpened();

    int cFd = -1;
    struct sockaddr_in saddrin = {0};
    socklen_t saddrLen = sizeof(saddrin);

    while (ss->mLoop) {
        cFd = accept(ss->mFd, (struct sockaddr*)&saddrin, &saddrLen);
        if (cFd < 0) {
            if (errno == EINTR) continue;
            if (errno == EBADF || errno == ENOTSOCK) break;
            usleep(10000);
            continue;
        }

        SS_INGOING_CONTEXT_S *income = (SS_INGOING_CONTEXT_S*)malloc(sizeof(SS_INGOING_CONTEXT_S));
        if (!income) {
            close(cFd);
            continue;
        }
        income->ss = ss;
        income->cFd = cFd;
        
        pthread_t tid;
        if (pthread_create(&tid, NULL, readFrom, income) != 0) {
            close(cFd);
            free(income);
            continue;
        }
        pthread_detach(tid);
    }

    ss->mImplClosed();

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////
SocketStream::SocketStream() {
    mImplOpened = [](void){};
    mImplClosed = [](void){};
    mImplImcome = [](SS_INGOING_S&,SS_OUGOING_S&){};
}

SocketStream::~SocketStream() {
    doClose();
}

bool SocketStream::doOpen(int usPort) {
    if (mFd > 0) {
        printf("CGI on port %d already exist\r\n", mPort);
        return true;
    }

    mFd = createSocketStream(usPort);
    if (mFd > 0) {
        mLoop = true;
        pthread_create(&mPid, NULL, SocketStream::socketStreamListenerCalls, this);
        return true;
    }
    return false;
}

void SocketStream::doClose() {
    if (mPid) {
        shutdown(mFd, SHUT_RDWR);
        closeSocketStream(&mFd);
        mLoop = false;
        pthread_join(mPid, NULL);
        mPid = 0;
    }
}

void SocketStream::onOpened(std::function<void(void)> FuncCallback) {
    mImplOpened = FuncCallback;
}

void SocketStream::onClosed(std::function<void(void)> FuncCallback) {
    mImplClosed = FuncCallback;
}

void SocketStream::onIncome(std::function<void(SS_INGOING_S&,SS_OUGOING_S&)> FuncCallback) {
    mImplImcome = FuncCallback;
}
 