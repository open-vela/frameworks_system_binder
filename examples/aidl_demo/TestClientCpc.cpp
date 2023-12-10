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

#define LOG_TAG "TestClient"

#include "ITestStuff.h"

#include <binder/ICpcServiceManager.h>
#include <utils/Log.h>

using namespace android;
using android::binder::Status;

extern "C" int main(int argc, char** argv)
{
    sp<IServiceManager> sm(defaultCpcServiceManager());

    auto remoteBinder = sm->getService(String16("cpctest"));

    // auto remoteBinder = sm->waitForService(String16("cpctest"));

    if (!remoteBinder) {
        ALOGI("remoteBinder get nullptr error");
        return 0;
    }
    // interface_cast restore ITest interface
    sp<ITestStuff> service = interface_cast<ITestStuff>(remoteBinder);
    if (service) {
        // callback server service interface
        service->write(123);
        service->read(456);
    } else {
        ALOGI("remoteBinder get is null error\n");
    }
    return 0;
}
