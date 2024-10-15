/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#include "BnTestStuff.h"

#include <cinttypes>

#include <binder/ICpcServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/Log.h>
#include <uv.h>

using namespace android;
using android::binder::Status;

namespace android {
class ILibuvCpcServer : public BnTestStuff {
public:
    Status read(int32_t sample)
    {
        ALOGI("ITestLibuvCpc::read() called %" PRIi32 ", server start do hard work", sample);
        return Status::ok();
    }
    Status write(int32_t index)
    {
        ALOGI("ITestLibuvCpc::write() called %" PRIi32 ", server start do hard work", index);
        return Status::ok();
    }
};
}

extern "C" int main(int argc, char** argv)
{
    ALOGI("sample service start count: %d, argv[0]: %s", argc, argv[0]);
    sp<ILibuvCpcServer> testServer = new ILibuvCpcServer;

    sp<IServiceManager> sm(defaultCpcServiceManager(uv_default_loop()));

    auto remoteBinder = sm->getService(String16("cpctest"));
    if (remoteBinder != NULL) {
        ALOGE("Service cpctest has been already added!\n");
        return 0;
    }

    sm->addService(String16("cpctest"), testServer);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    return 0;
}
