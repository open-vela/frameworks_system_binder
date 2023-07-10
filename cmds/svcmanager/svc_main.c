/*
 * Copyright (C) 2023 Xiaomi Corperation
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

#define LOG_TAG "SVCManager"

#include <ctype.h>
#include <debug.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <time.h>

#include "utils/Binderlog.h"
#include <android/binder_status.h>

#include "base/IPCThreadState.h"
#include "base/IServiceManager.h"
#include "base/ProcessState.h"

#include "ServiceManager.h"

int main(int argc, char** argv)
{
    int32_t status;
    String name;
    ProcessState* ps;
    IPCThreadState* self;
    int binder_fd;
    int epoll_fd;
    struct epoll_event ev;

    if (argc > 2) {
        LOG_FATAL_IF(1, "usage: %s [binder driver]\n", argv[0]);
    }

    const char* driver = argc == 2 ? argv[1] : "/dev/binder";

    ps = ProcessState_initWithDriver(driver);
    ps->setThreadPoolMaxThreadCount(ps, 0);
    ps->setCallRestriction(ps, CALL_FATAL_IF_NOT_ONEWAY);

    ServiceManager* manager = ServiceManager_new();
    String_init(&name, "manager");

    status = manager->addService(manager, &name, (IBinder*)manager, false,
        DUMP_FLAG_PRIORITY_DEFAULT);
    if (status != STATUS_OK) {
        BINDER_LOGE("Could not self register servicemanager\n");
        return EXIT_FAILURE;
    }

    self = IPCThreadState_self();
    self->setTheContextObject(self, (BBinder*)manager);
    ps->becomeContextManager(ps);

    self->setupPolling(self, &binder_fd);
    if (binder_fd < 0) {
        return EXIT_FAILURE;
    }

    /* flush BC_ENTER_LOOPER */
    self->flushCommands(self);

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        return EXIT_FAILURE;
    }

    ev.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, binder_fd, &ev) == -1) {
        return EXIT_FAILURE;
    }
    while (1) {
        struct epoll_event events[1];
        int numEvents = epoll_wait(epoll_fd, events, 1, 1000);
        if (numEvents < 0) {
            if (errno == EINTR) {
                continue;
            }
            return EXIT_FAILURE;
        }

        if (numEvents > 0) {
            self->handlePolledCommands(self);
        }
    }
    // should not be reached
    return EXIT_FAILURE;
}
