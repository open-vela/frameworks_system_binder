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

#include <inttypes.h>
#include <stdio.h>

#include <aidl/INdkTestVector.h>
#include <android/binder_manager.h>

extern "C" int main(int argc, char** argv)
{
    printf("vector demo client start argc: %d, argv[0]: %s\n", argc, argv[0]);

    ndk::ScopedAStatus status;

    ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_checkService("ndktestvector.service"));
    if (binder.get() == nullptr) {
        printf("binder is null\n");
        return 1;
    }

    std::shared_ptr<aidl::INdkTestVector> proxy = aidl::INdkTestVector::fromBinder(binder);
    if (proxy.get() == nullptr) {
        printf("proxy is null\n");
        return 1;
    }

    // Test C type bool vector usage
    int32_t bin_len = 3;
    bool bin[3] = { false, false, true };
    // bool* bin = nullptr;
    int32_t bout_len = 0;
    bool* bout = nullptr;
    int32_t bret_len = 0;
    bool* bret = nullptr;
    status = proxy->RepeatBooleanVector(bin, bin_len, &bout, &bout_len, &bret, &bret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatBooleanVector error\n");
        return 1;
    } else {
        printf("Bool out is");
        if (!bout) {
            printf(" null");
        } else {
            for (int i = 0; i < bout_len; i++) {
                if (bout[i]) {
                    printf(" true");
                } else {
                    printf(" false");
                }
            }
        }
        printf("\n");
        printf("Bool ret is");
        if (!bret) {
            printf(" null");
        } else {
            for (int i = 0; i < bret_len; i++) {
                if (bret[i]) {
                    printf(" true");
                } else {
                    printf(" false");
                }
            }
        }
        printf("\n");
    }
    free(bout);
    free(bret);

    // Test C type byte vector usage
    int32_t byin_len = 3;
    uint8_t byin[3] = { '1', '2', '3' };
    // uint8_t* byin = nullptr;
    int32_t byout_len = 0;
    uint8_t* byout = nullptr;
    int32_t byret_len = 0;
    uint8_t* byret = nullptr;
    status = proxy->RepeatByteVector(byin, byin_len, &byout, &byout_len, &byret, &byret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatByteVector error\n");
        return 1;
    } else {
        printf("Byte out is");
        if (!byout) {
            printf(" null");
        } else {
            for (int i = 0; i < byout_len; i++) {
                printf(" %c", byout[i]);
            }
        }
        printf("\n");
        printf("Byte ret is");
        if (!byret) {
            printf(" null");
        } else {
            for (int i = 0; i < byret_len; i++) {
                printf(" %c", byret[i]);
            }
        }
        printf("\n");
    }
    free(byout);
    free(byret);

    // Test C type char vector usage
    int32_t cin_len = 3;
    char16_t cin[3] = { '1', '2', '3' };
    int32_t cout_len = 0;
    char16_t* cout = nullptr;
    int32_t cret_len = 0;
    char16_t* cret = nullptr;
    status = proxy->RepeatCharVector(cin, cin_len, &cout, &cout_len, &cret, &cret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatCharVector error\n");
        return 1;
    } else {
        printf("Char out is");
        for (int i = 0; i < cout_len; i++) {
            printf(" %c", cout[i]);
        }
        printf("\n");
        printf("Char ret is");
        for (int i = 0; i < cret_len; i++) {
            printf(" %c", cret[i]);
        }
        printf("\n");
    }
    free(cout);
    free(cret);

    // Test C type int vector usage
    int32_t iin_len = 3;
    int32_t iin[3] = { 1, 2, 3 };
    int32_t iout_len = 0;
    int32_t* iout = nullptr;
    int32_t iret_len = 0;
    int32_t* iret = nullptr;
    status = proxy->RepeatIntVector(iin, iin_len, &iout, &iout_len, &iret, &iret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatIntVector error\n");
        return 1;
    } else {
        printf("Int out is");
        for (int i = 0; i < iout_len; i++) {
            printf(" %" PRId32, iout[i]);
        }
        printf("\n");
        printf("Int ret is");
        for (int i = 0; i < iret_len; i++) {
            printf(" %" PRId32, iret[i]);
        }
        printf("\n");
    }
    free(iout);
    free(iret);

    // Test C type long vector usage
    int32_t lin_len = 3;
    int64_t lin[3] = { 1, 2, 3 };
    int32_t lout_len = 0;
    int64_t* lout = nullptr;
    int32_t lret_len = 0;
    int64_t* lret = nullptr;
    status = proxy->RepeatLongVector(lin, lin_len, &lout, &lout_len, &lret, &lret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatLongVector error\n");
        return 1;
    } else {
        printf("Long out is");
        for (int i = 0; i < lout_len; i++) {
            printf(" %" PRId64, lout[i]);
        }
        printf("\n");
        printf("Long ret is");
        for (int i = 0; i < lret_len; i++) {
            printf(" %" PRId64, lret[i]);
        }
        printf("\n");
    }
    free(lout);
    free(lret);

    // Test C type float vector usage
    int32_t fin_len = 3;
    float fin[3] = { 1.0, 2.0, 3.0 };
    int32_t fout_len = 0;
    float* fout = nullptr;
    int32_t fret_len = 0;
    float* fret = nullptr;
    status = proxy->RepeatFloatVector(fin, fin_len, &fout, &fout_len, &fret, &fret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatFloatVector error\n");
        return 1;
    } else {
        printf("Float out is");
        for (int i = 0; i < fout_len; i++) {
            printf(" %f", fout[i]);
        }
        printf("\n");
        printf("Float ret is");
        for (int i = 0; i < fret_len; i++) {
            printf(" %f", fret[i]);
        }
        printf("\n");
    }
    free(fout);
    free(fret);

    // Test C type double vector usage
    int32_t din_len = 3;
    double din[3] = { 1.0, 2.0, 3.0 };
    int32_t dout_len = 0;
    double* dout = nullptr;
    int32_t dret_len = 0;
    double* dret = nullptr;
    status = proxy->RepeatDoubleVector(din, din_len, &dout, &dout_len, &dret, &dret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatDoubleVector error\n");
        return 1;
    } else {
        printf("Double out is");
        for (int i = 0; i < dout_len; i++) {
            printf(" %lf", dout[i]);
        }
        printf("\n");
        printf("Double ret is");
        for (int i = 0; i < dret_len; i++) {
            printf(" %lf", dret[i]);
        }
        printf("\n");
    }
    free(dout);
    free(dret);

    // Test C type string vector usage
    int32_t sin_len = 3;
    const char* sin[3] = { "abc", "def", "ghi" };
    // const char** sin = nullptr;
    int32_t sout_len = 0;
    char** sout = nullptr;
    int32_t sret_len = 0;
    char** sret = nullptr;
    status = proxy->RepeatStringVector(sin, sin_len, &sout, &sout_len, &sret, &sret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatStringVector error\n");
        return 1;
    } else {
        printf("String out is");
        if (!sout) {
            printf(" null");
        } else {
            for (int i = 0; i < sout_len; i++) {
                printf(" %s", sout[i]);
            }
        }
        printf("\n");
        printf("String ret is");
        if (!sret) {
            printf(" null");
        } else {
            for (int i = 0; i < sret_len; i++) {
                printf(" %s", sret[i]);
            }
        }
        printf("\n");
    }
    for (int i = 0; i < sout_len; i++) {
        free(sout[i]);
    }
    free(sout);
    for (int i = 0; i < sret_len; i++) {
        free(sret[i]);
    }
    free(sret);

    // Test C type string List usage
    int32_t slin_len = 3;
    const char* slin[3] = { "abc", "def", "ghi" };
    int32_t slout_len = 0;
    char** slout = nullptr;
    int32_t slret_len = 0;
    char** slret = nullptr;
    status = proxy->Repeat2StringList(slin, slin_len, &slout, &slout_len, &slret, &slret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("Repeat2StringList error\n");
        return 1;
    } else {
        printf("String out is");
        for (int i = 0; i < slout_len; i++) {
            printf(" %s", slout[i]);
        }
        printf("\n");
        printf("String ret is");
        for (int i = 0; i < slret_len; i++) {
            printf(" %s", slret[i]);
        }
        printf("\n");
    }
    for (int i = 0; i < slout_len; i++) {
        free(slout[i]);
    }
    free(slout);
    for (int i = 0; i < slret_len; i++) {
        free(slret[i]);
    }
    free(slret);

    return 0;
}
