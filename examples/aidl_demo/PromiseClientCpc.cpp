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

#include <binder/ICpcServiceManager.h>

#include "BnTestPromiseCallback.h"
#include "ITestPromise.h"

#include <utils/Log.h>

#include <future>

using namespace android;

class CallbackPromise : public BnTestPromiseCallback, public std::promise<int32_t> {
public:
    binder::Status result(int32_t res)
    {
        this->set_value(res);
        return binder::Status::ok();
    }
};

extern "C" int main(int argc, char** argv)
{
    int32_t callback_result;

    ALOGD("promise client start argc: %d, argv[0]: %s", argc, argv[0]);

    // obtain service manager
    sp<IServiceManager> sm(defaultCpcServiceManager());

    // obtain service
    sp<IBinder> binder = sm->getService(String16("promise.srv"));
    if (binder == NULL) {
        ALOGE("service binder is null, abort...");
        return -1;
    }

    sp<ITestPromise> service = interface_cast<ITestPromise>(binder);

    if (argc == 2 && strcmp(argv[1], "exit") == 0) {
        service->requestExit();
        return 0;
    }

    sp<CallbackPromise> callback = sp<CallbackPromise>::make();
    service->add(102, 302, callback);

    std::future<int32_t> f = callback->get_future();
    callback_result = f.get();
    ALOGI("result %" PRId32, callback_result);

    return 0;
}
