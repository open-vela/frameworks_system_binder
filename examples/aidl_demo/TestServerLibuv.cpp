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
class ITestServer : public BnTestStuff {
public:
    // ITestServer::read()
    Status read(int32_t sample)
    {
        ALOGD("ITest::read() called %" PRIi32 ", start do hard work", sample);
        return Status::ok();
    }

    // ITestServer::write()
    Status write(int32_t index)
    {
        ALOGD("ITest::write() called %" PRIi32 ", start do hard work", index);
        return Status::ok();
    }
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
    uv_poll_t binder_handle;
    int fd;

    ALOGI("sample service start count: %d, argv[0]: %s", argc, argv[0]);

    // create ProcessState
    sp<ProcessState> proc(ProcessState::self());

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());
    sp<ITestServer> testServer = new ITestServer;

    // add service
    sm->addService(String16("aidldemo.service"), testServer);
    ALOGI("add aidldemo.service to service manager");

    IPCThreadState::self()->setupPolling(&fd);
    if (fd < 0) {
        ALOGE("failed to open binder device:%d", errno);
        return fd;
    }

    IPCThreadState::self()->flushCommands(); // flush BC_ENTER_LOOPER
    uv_poll_init(uv_default_loop(), &binder_handle, fd);
    uv_poll_start(&binder_handle, UV_READABLE, uv_binder_cb);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&binder_handle, NULL);
    IPCThreadState::self()->stopProcess();

    return 0;
}
