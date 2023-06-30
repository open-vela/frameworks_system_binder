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

#include <stdio.h>
#include <string.h>

#include <aidl/BnNdkTest.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

class MyService : public aidl::BnNdkTest {
    ndk::ScopedAStatus repeatString(const char* in, const char* in2, char** out)
    {
        printf("%s\n", __func__);
        *out = (char*)malloc((strlen(in) + strlen(in2) + 1) * sizeof(char));
        strcpy(*out, in);
        strcat(*out, in2);
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus repeatNullableString(const char* in, const char* in2, char** out)
    {
        printf("%s\n", __func__);
        if (!in || !in2) {
            printf("in in2 has nullptr.\n");
            return ndk::ScopedAStatus::ok();
        }
        *out = (char*)malloc((strlen(in) + strlen(in2) + 1) * sizeof(char));
        strcpy(*out, in);
        strcat(*out, in2);
        return ndk::ScopedAStatus::ok();
    }
};

extern "C" int main(int argc, char** argv)
{
    printf("sample service start count: %d, argv[0]: %s\n", argc, argv[0]);

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto service = ndk::SharedRefBase::make<MyService>();

    binder_status_t status = AServiceManager_addService(service->asBinder().get(), "ndktest.service");

    if (status != STATUS_OK) {
        printf("could not register service\n");
        return 1;
    }

    ABinderProcess_joinThreadPool();

    return 0;
}
