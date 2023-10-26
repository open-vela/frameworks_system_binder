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

#include <utils/Log.h>
#include <utils/String8.h>
#include <uv.h>

using namespace android;
using android::binder::Status;

namespace android {
class ILibuvServer : public BnTestStuff {
public:
    // ILibuvServer::read()
    Status read(int32_t sample)
    {
        ALOGI("ITestLibuv::read() called %" PRIi32 ", start do hard work", sample);
        return Status::ok();
    }

    // ILibuvServer::write()
    Status write(int32_t index)
    {
        ALOGI("ITestLibuv::write() called %" PRIi32 ", start do hard work", index);
        return Status::ok();
    }

    uv_poll_t* getHandle()
    {
        return &binder_handle;
    }

private:
    uv_poll_t binder_handle;
};
} // namespace android

static void uv_binder_cb(uv_poll_t* handle, int status, int events)
{
    ALOGI("process binder transaction in libuv cb");
    IPCThreadState::self()->handlePolledCommands();
    IPCThreadState::self()->flushCommands(); // flush BC_FREE_BUFFER
}

extern "C" int main(int argc, char** argv)
{
    int fd;

    ALOGI("sample service start count: %d, argv[0]: %s", argc, argv[0]);

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());
    sp<ILibuvServer> testServer = new ILibuvServer;

    // add service
    sm->addService(String16("aidldemo.service"), testServer);
    ALOGI("add aidldemo.service to service manager");

    IPCThreadState::self()->setupPolling(&fd);
    if (fd < 0) {
        ALOGE("failed to open binder device:%d", errno);
        return fd;
    }

    IPCThreadState::self()->flushCommands(); // flush BC_ENTER_LOOPER
    uv_poll_init(uv_default_loop(), testServer->getHandle(), fd);
    uv_poll_start(testServer->getHandle(), UV_READABLE, uv_binder_cb);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)(testServer->getHandle()), NULL);
    IPCThreadState::self()->stopProcess();

    return 0;
}
