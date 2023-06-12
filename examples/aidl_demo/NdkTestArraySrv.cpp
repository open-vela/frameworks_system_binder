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

#include <aidl/BnNdkTestArray.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

class NdkTestArraySrv : public aidl::BnNdkTestArray {
    ndk::ScopedAStatus RepeatBooleanArray(const bool* in_input, bool* out_repeated, bool* _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            _aidl_return[i] = out_repeated[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatByteArray(const uint8_t* in_input, uint8_t* out_repeated, uint8_t* _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            _aidl_return[i] = out_repeated[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatCharArray(const char16_t* in_input, char16_t* out_repeated, char16_t* _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            _aidl_return[i] = out_repeated[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatIntArray(const int32_t* in_input, int32_t* out_repeated, int32_t* _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            _aidl_return[i] = out_repeated[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatLongArray(const int64_t* in_input, int64_t* out_repeated, int64_t* _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            _aidl_return[i] = out_repeated[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatFloatArray(const float* in_input, float* out_repeated, float* _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            _aidl_return[i] = out_repeated[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatDoubleArray(const double* in_input, double* out_repeated, double* _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            _aidl_return[i] = out_repeated[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatStringArray(const char** in_input, char** out_repeated, char** _aidl_return)
    {
        printf("%s\n", __func__);
        for (int i = 0; i < 3; i++) {
            size_t input_len = strlen(in_input[i]) + 1;
            _aidl_return[i] = (char*)malloc(input_len);
            out_repeated[i] = (char*)malloc(input_len);
            memcpy(_aidl_return[i], in_input[i], input_len);
            memcpy(out_repeated[i], in_input[i], input_len);
        }
        return ndk::ScopedAStatus::ok();
    }
};

extern "C" int main(int argc, char** argv)
{
    printf("sample service start count: %d, argv[0]: %s\n", argc, argv[0]);

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto service = ndk::SharedRefBase::make<NdkTestArraySrv>();

    binder_status_t status = AServiceManager_addService(service->asBinder().get(), "ndktestarray.service");

    if (status != STATUS_OK) {
        printf("could not register service\n");
        return 1;
    }

    ABinderProcess_joinThreadPool();

    return 0;
}
