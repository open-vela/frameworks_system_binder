/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "TestService"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "BnTestStuff.h"

#include <sys/epoll.h>
#include <utils/Log.h>
#include <utils/String8.h>

using namespace android;
using android::binder::Status;

namespace android {
class IEpollServer : public BnTestStuff {
public:
    // IEpollServer::read()
    Status read(int32_t sample)
    {
        ALOGI("ITestEpoll::read() called %" PRIi32 ", start do hard work", sample);
        return Status::ok();
    }

    // IEpollServer::write()
    Status write(int32_t index)
    {
        ALOGI("ITestEpoll::write() called %" PRIi32 ", start do hard work", index);
        return Status::ok();
    }
};
} // namespace android

extern "C" int main(int argc, char** argv)
{
    struct epoll_event ev;
    int epoll_fd;
    int fd;
    int ret;

    ALOGI("sample service start count: %d, argv[0]: %s", argc, argv[0]);

    // create ProcessState
    sp<ProcessState> proc(ProcessState::self());

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());
    sp<IEpollServer> testServer = new IEpollServer;

    // add service
    sm->addService(String16("aidldemo.service"), testServer);
    ALOGI("add aidldemo.service to service manager");

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        ALOGE("failed to create epoll:%d", errno);
        return epoll_fd;
    }

    IPCThreadState::self()->setupPolling(&fd);
    if (fd < 0) {
        ALOGE("failed to open binder device:%d", errno);
        epoll_close(epoll_fd);
        return fd;
    }

    IPCThreadState::self()->flushCommands(); // flush BC_ENTER_LOOPER

    ev.events = EPOLLIN;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        ALOGE("failed to add fd to epoll:%d", errno);
        IPCThreadState::self()->stopProcess();
        epoll_close(epoll_fd);
        return ret;
    }

    while (1) {
        struct epoll_event events[1];
        int numEvents = epoll_wait(epoll_fd, events, 1, -1);
        if (numEvents < 0) {
            if (errno == EINTR) {
                continue;
            }

            break;
        }

        if (numEvents > 0 && (events[0].events & EPOLLIN)) {
            ALOGI("process binder transaction");
            IPCThreadState::self()->handlePolledCommands();
            IPCThreadState::self()->flushCommands(); // flush BC_FREE_BUFFER
        }
    }

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    IPCThreadState::self()->stopProcess();
    epoll_close(epoll_fd);

    return 0;
}
