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

#include <aidl/BnNdkTestVector.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

class NdkTestVectorSrv : public aidl::BnNdkTestVector {
    ndk::ScopedAStatus RepeatBooleanVector(const bool* in_input, const int32_t in_input_length, bool** out_repeated, int32_t* out_repeated_length, bool** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (bool*)malloc((*_aidl_return_length) * sizeof(bool));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (bool*)malloc((*out_repeated_length) * sizeof(bool));
        }
        for (int i = 0; i < in_input_length; i++) {
            (*_aidl_return)[i] = (*out_repeated)[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus RepeatByteVector(const uint8_t* in_input, const int32_t in_input_length, uint8_t** out_repeated, int32_t* out_repeated_length, uint8_t** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (uint8_t*)malloc((*_aidl_return_length) * sizeof(uint8_t));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (uint8_t*)malloc((*out_repeated_length) * sizeof(uint8_t));
        }
        for (int i = 0; i < in_input_length; i++) {
            (*_aidl_return)[i] = (*out_repeated)[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus RepeatCharVector(const char16_t* in_input, const int32_t in_input_length, char16_t** out_repeated, int32_t* out_repeated_length, char16_t** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (char16_t*)malloc((*_aidl_return_length) * sizeof(char16_t));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (char16_t*)malloc((*out_repeated_length) * sizeof(char16_t));
        }
        for (int i = 0; i < in_input_length; i++) {
            (*_aidl_return)[i] = (*out_repeated)[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus RepeatIntVector(const int32_t* in_input, const int32_t in_input_length, int32_t** out_repeated, int32_t* out_repeated_length, int32_t** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (int32_t*)malloc((*_aidl_return_length) * sizeof(int32_t));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (int32_t*)malloc((*out_repeated_length) * sizeof(int32_t));
        }
        for (int i = 0; i < in_input_length; i++) {
            (*_aidl_return)[i] = (*out_repeated)[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus RepeatLongVector(const int64_t* in_input, const int32_t in_input_length, int64_t** out_repeated, int32_t* out_repeated_length, int64_t** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (int64_t*)malloc((*_aidl_return_length) * sizeof(int64_t));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (int64_t*)malloc((*out_repeated_length) * sizeof(int64_t));
        }
        for (int i = 0; i < in_input_length; i++) {
            (*_aidl_return)[i] = (*out_repeated)[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus RepeatFloatVector(const float* in_input, const int32_t in_input_length, float** out_repeated, int32_t* out_repeated_length, float** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (float*)malloc((*_aidl_return_length) * sizeof(float));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (float*)malloc((*out_repeated_length) * sizeof(float));
        }
        for (int i = 0; i < in_input_length; i++) {
            (*_aidl_return)[i] = (*out_repeated)[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus RepeatDoubleVector(const double* in_input, const int32_t in_input_length, double** out_repeated, int32_t* out_repeated_length, double** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (double*)malloc((*_aidl_return_length) * sizeof(double));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (double*)malloc((*out_repeated_length) * sizeof(double));
        }
        for (int i = 0; i < in_input_length; i++) {
            (*_aidl_return)[i] = (*out_repeated)[i] = in_input[i];
        }
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus RepeatStringVector(const char** in_input, const int32_t in_input_length, char*** out_repeated, int32_t* out_repeated_length, char*** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *_aidl_return_length = *out_repeated_length = in_input_length;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (char**)malloc((*_aidl_return_length) * sizeof(char*));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (char**)malloc((*out_repeated_length) * sizeof(char*));
        }
        for (int i = 0; i < in_input_length; i++) {
            size_t input_len = strlen(in_input[i]) + 1;
            (*_aidl_return)[i] = (char*)malloc(input_len);
            (*out_repeated)[i] = (char*)malloc(input_len);
            memcpy((*_aidl_return)[i], in_input[i], input_len);
            memcpy((*out_repeated)[i], in_input[i], input_len);
        }

        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus Repeat2StringList(const char** in_input, const int32_t in_input_length, char*** out_repeated, int32_t* out_repeated_length, char*** _aidl_return, int32_t* _aidl_return_length)
    {
        printf("%s\n", __func__);
        *out_repeated_length = *_aidl_return_length = in_input_length * 2;
        if (*_aidl_return_length > 0) {
            *_aidl_return = (char**)malloc((*_aidl_return_length) * sizeof(char*));
        }
        if (*out_repeated_length > 0) {
            *out_repeated = (char**)malloc((*out_repeated_length) * sizeof(char*));
        }
        for (int i = 0; i < in_input_length; i++) {
            size_t input_len = strlen(in_input[i]) + 1;
            (*_aidl_return)[i] = (char*)malloc(input_len);
            (*_aidl_return)[i + in_input_length] = (char*)malloc(input_len);
            (*out_repeated)[i] = (char*)malloc(input_len);
            (*out_repeated)[i + in_input_length] = (char*)malloc(input_len);
            memcpy((*_aidl_return)[i], in_input[i], input_len);
            memcpy((*_aidl_return)[i + in_input_length], in_input[i], input_len);
            memcpy((*out_repeated)[i], in_input[i], input_len);
            memcpy((*out_repeated)[i + in_input_length], in_input[i], input_len);
        }

        return ndk::ScopedAStatus::ok();
    }
};

extern "C" int main(int argc, char** argv)
{
    printf("sample service start count: %d, argv[0]: %s\n", argc, argv[0]);

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto service = ndk::SharedRefBase::make<NdkTestVectorSrv>();

    binder_status_t status = AServiceManager_addService(service->asBinder().get(), "ndktestvector.service");

    if (status != STATUS_OK) {
        printf("could not register service\n");
        return 1;
    }

    ABinderProcess_joinThreadPool();

    return 0;
}
