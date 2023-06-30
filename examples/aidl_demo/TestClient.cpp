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

#include <binder/IServiceManager.h>

#include "ITestStuff.h"

#include <utils/Log.h>
#include <utils/String8.h>

using namespace android;
using android::binder::Status;

extern "C" int main(int argc, char** argv)
{
    ALOGI("demo client start count: %d, argv[0]: %s", argc, argv[0]);

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());

    // obtain demo.service
    sp<IBinder> binder = sm->getService(String16("aidldemo.service"));
    if (binder == NULL) {
        ALOGE("aidldemo service binder is null, abort...");
        return -1;
    }
    ALOGI("aidldemo service binder is %p", binder.get());

    // by interface_cast restore ITest
    sp<ITestStuff> service = interface_cast<ITestStuff>(binder);
    ALOGI("aidldemo service is %p", service.get());

    // use service provided by demo.service
    service->write(123);
    service->read(456);
    return 0;
}