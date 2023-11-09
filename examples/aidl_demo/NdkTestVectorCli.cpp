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

#define LOG_TAG "NdkTestVectorCli"

#include <inttypes.h>
#include <stdio.h>

#include <aidl/INdkTestVector.h>
#include <android/binder_manager.h>
#include <utils/Log.h>

bool testCTypeVectorUsage(std::shared_ptr<aidl::INdkTestVector> proxy)
{
    ndk::ScopedAStatus status;

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
        ALOGE("RepeatBooleanVector error\n");
        return false;
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
        ALOGE("RepeatByteVector error\n");
        return false;
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
        ALOGE("RepeatCharVector error\n");
        return false;
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
        ALOGE("RepeatIntVector error\n");
        return false;
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
        ALOGE("RepeatLongVector error\n");
        return false;
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
        ALOGE("RepeatFloatVector error\n");
        return false;
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
        ALOGE("RepeatDoubleVector error\n");
        return false;
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
        ALOGE("RepeatStringVector error\n");
        return false;
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
        ALOGE("Repeat2StringList error\n");
        return false;
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

    return true;
}

bool testCTypeNullableVectorUsage(std::shared_ptr<aidl::INdkTestVector> proxy)
{
    ndk::ScopedAStatus status;

    // Test C type nullable bool vector usage
    int32_t nbin_len = 0;
    bool* nbin = nullptr;
    // int32_t nbin_len = 3;
    // bool nbin[3] = { false, false, true };
    int32_t nbret_len = 0;
    bool* nbret = nullptr;
    status = proxy->RepeatNullableBooleanVector(nbin, nbin_len, &nbret, &nbret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableBooleanVector error\n");
        return false;
    } else {
        printf("Nullable Bool ret is");
        for (int i = 0; i < nbret_len; i++) {
            if (nbret[i]) {
                printf(" true");
            } else {
                printf(" false");
            }
        }
        if (!nbret) {
            printf(" null");
        }
        printf("\n");
    }
    free(nbret);

    // Test C type nullable byte vector usage
    int32_t nbyin_len = 0;
    uint8_t* nbyin = nullptr;
    // int32_t nbyin_len = 3;
    // uint8_t nbyin[3] = { '1', '2', '3' };
    int32_t nbyret_len = 0;
    uint8_t* nbyret = nullptr;
    status = proxy->RepeatNullableByteVector(nbyin, nbyin_len, &nbyret, &nbyret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableByteVector error\n");
        return false;
    } else {
        printf("Nullable Byte ret is");
        for (int i = 0; i < nbyret_len; i++) {
            printf(" %c", nbyret[i]);
        }
        if (!nbyret) {
            printf(" null");
        }
        printf("\n");
    }
    free(nbyret);

    // Test C type nullable char vector usage
    int32_t ncin_len = 0;
    char16_t* ncin = nullptr;
    // int32_t ncin_len = 3;
    // char16_t ncin[3] = { '1', '2', '3' };
    int32_t ncret_len = 0;
    char16_t* ncret = nullptr;
    status = proxy->RepeatNullableCharVector(ncin, ncin_len, &ncret, &ncret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableCharVector error\n");
        return false;
    } else {
        printf("Nullable Char ret is");
        for (int i = 0; i < ncret_len; i++) {
            printf(" %c", ncret[i]);
        }
        if (!ncret) {
            printf(" null");
        }
        printf("\n");
    }
    free(ncret);

    // Test C type nullable int vector usage
    int32_t niin_len = 0;
    int32_t* niin = nullptr;
    // int32_t niin_len = 3;
    // int32_t niin[3] = { 1, 2, 3 };
    int32_t niret_len = 0;
    int32_t* niret = nullptr;
    status = proxy->RepeatNullableIntVector(niin, niin_len, &niret, &niret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableIntVector error\n");
        return false;
    } else {
        printf("Nullable Int ret is");
        for (int i = 0; i < niret_len; i++) {
            printf(" %" PRId32, niret[i]);
        }
        if (!niret) {
            printf(" null");
        }
        printf("\n");
    }
    free(niret);

    // Test C type nullable long vector usage
    int32_t nlin_len = 0;
    int64_t* nlin = nullptr;
    // int32_t nlin_len = 3;
    // int64_t nlin[3] = { 1, 2, 3 };
    int32_t nlret_len = 0;
    int64_t* nlret = nullptr;
    status = proxy->RepeatNullableLongVector(nlin, nlin_len, &nlret, &nlret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableLongVector error\n");
        return false;
    } else {
        printf("Nullable Long ret is");
        for (int i = 0; i < nlret_len; i++) {
            printf(" %" PRId64, nlret[i]);
        }
        if (!nlret) {
            printf(" null");
        }
        printf("\n");
    }
    free(nlret);

    // Test C type nullable float vector usage
    int32_t nfin_len = 0;
    float* nfin = nullptr;
    // int32_t nfin_len = 3;
    // float nfin[3] = { 1.0, 2.0, 3.0 };
    int32_t nfret_len = 0;
    float* nfret = nullptr;
    status = proxy->RepeatNullableFloatVector(nfin, nfin_len, &nfret, &nfret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableFloatVector error\n");
        return false;
    } else {
        printf("Nullable Float ret is");
        for (int i = 0; i < nfret_len; i++) {
            printf(" %f", nfret[i]);
        }
        if (!nfret) {
            printf(" null");
        }
        printf("\n");
    }
    free(nfret);

    // Test C type nullable double vector usage
    int32_t ndin_len = 0;
    double* ndin = nullptr;
    // int32_t ndin_len = 3;
    // double ndin[3] = { 1.0, 2.0, 3.0 };
    int32_t ndret_len = 0;
    double* ndret = nullptr;
    status = proxy->RepeatNullableDoubleVector(ndin, ndin_len, &ndret, &ndret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableDoubleVector error\n");
        return false;
    } else {
        printf("Nullable Double ret is");
        for (int i = 0; i < ndret_len; i++) {
            printf(" %lf", ndret[i]);
        }
        if (!ndret) {
            printf(" null");
        }
        printf("\n");
    }
    free(ndret);

    // Test C type nullable string vector usage
    int32_t nsin_len = 0;
    const char** nsin = nullptr;
    // int32_t nsin_len = 3;
    // const char* nsin[3] = { "abc", "def", "ghi" };
    int32_t nsret_len = 0;
    char** nsret = nullptr;
    status = proxy->RepeatNullableStringVector(nsin, nsin_len, &nsret, &nsret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatNullableStringVector error\n");
        return false;
    } else {
        printf("Nullable String ret is");
        for (int i = 0; i < nsret_len; i++) {
            printf(" %s", nsret[i]);
        }
        if (!nsret) {
            printf(" null");
        }
        printf("\n");
    }
    for (int i = 0; i < nsret_len; i++) {
        free(nsret[i]);
    }
    free(nsret);

    // Test C type nullable string List usage
    int32_t nslin_len = 0;
    const char** nslin = nullptr;
    // int32_t nslin_len = 3;
    // const char* nslin[3] = { "abc", "def", "ghi" };
    int32_t nslout_len = 0;
    char** nslout = nullptr;
    int32_t nslret_len = 0;
    char** nslret = nullptr;
    status = proxy->Repeat2NullableStringList(nslin, nslin_len, &nslout, &nslout_len, &nslret, &nslret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("Repeat2NullableStringList error\n");
        return false;
    } else {
        printf("Nullable String list out is");
        for (int i = 0; i < nslout_len; i++) {
            printf(" %s", nslout[i]);
        }
        if (!nslout) {
            printf(" null");
        }
        printf("\n");
        printf("Nullable String list ret is");
        for (int i = 0; i < nslret_len; i++) {
            printf(" %s", nslret[i]);
        }
        if (!nslret) {
            printf(" null");
        }
        printf("\n");
    }
    for (int i = 0; i < nslout_len; i++) {
        free(nslout[i]);
    }
    free(nslout);
    for (int i = 0; i < nslret_len; i++) {
        free(nslret[i]);
    }
    free(nslret);

    return true;
}

bool testSTLVectorUsage(std::shared_ptr<aidl::INdkTestVector> proxy)
{
    ndk::ScopedAStatus status;

    // @BEGIN SLT method
    // Test STL bool vector usage
    std::vector<bool> bin_1 = { false, false, true };
    std::vector<bool> bout_1(3);
    std::vector<bool> bret_1;
    status = proxy->RepeatBooleanVector(bin_1, &bout_1, &bret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatBooleanVector error\n");
        return false;
    } else {
        printf("STL Bool out is");
        for (auto a : bout_1) {
            if (a) {
                printf(" true");
            } else {
                printf(" false");
            }
        }
        printf("\n");
        printf("STL Bool ret is");
        for (auto a : bret_1) {
            if (a) {
                printf(" true");
            } else {
                printf(" false");
            }
        }
        printf("\n");
    }

    // Test STL byte vector usage
    std::vector<uint8_t> byin_1 = { '1', '2', '3' };
    std::vector<uint8_t> byout_1(3);
    std::vector<uint8_t> byret_1;
    status = proxy->RepeatByteVector(byin_1, &byout_1, &byret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatByteVector error\n");
        return false;
    } else {
        printf("STL Byte out is");
        for (auto a : byout_1) {
            printf(" %c", a);
        }
        printf("\n");
        printf("STL Byte ret is");
        for (auto a : byret_1) {
            printf(" %c", a);
        }
        printf("\n");
    }

    std::vector<std::string> sin_1 = { "abc", "def", "ghi" };
    std::vector<std::string> sout_1(3);
    std::vector<std::string> sret_1;
    status = proxy->RepeatStringVector(sin_1, &sout_1, &sret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatStringVector error\n");
        return false;
    } else {
        printf("STL string out is");
        for (auto a : sout_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
        printf("STL string ret is");
        for (auto a : sret_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
    }

    std::vector<std::string> slin_1 = { "abc", "def", "ghi" };
    std::vector<std::string> slout_1;
    std::vector<std::string> slret_1;
    status = proxy->Repeat2StringList(slin_1, &slout_1, &slret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL Repeat2StringList error\n");
        return false;
    } else {
        printf("STL list string out is");
        for (auto a : slout_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
        printf("STL list string ret is");
        for (auto a : slret_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
    }

    return true;
    // @END SLT method
}

bool testSTLNullableVectorUsage(std::shared_ptr<aidl::INdkTestVector> proxy)
{
    ndk::ScopedAStatus status;

    // Test STL nullable bool vector usage
    std::optional<std::vector<bool>> nbin_1;
    std::optional<std::vector<bool>> nbret_1;
    status = proxy->RepeatNullableBooleanVector(nbin_1, &nbret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatNullableBooleanVector error\n");
        return false;
    } else {
        printf("STL Nullable Bool ret is");
        for (auto a : *nbret_1) {
            if (a) {
                printf(" true");
            } else {
                printf(" false");
            }
        }
        if (!nbret_1) {
            printf(" null");
        }
        printf("\n");
    }

    // Test STL nullable byte vector usage
    std::optional<std::vector<uint8_t>> nbyin_1;
    std::optional<std::vector<uint8_t>> nbyret_1;
    status = proxy->RepeatNullableByteVector(nbyin_1, &nbyret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatNullableByteVector error\n");
        return false;
    } else {
        printf("STL Nullable Byte ret is");
        for (auto a : *nbyret_1) {
            printf(" %c", a);
        }
        if (!nbyret_1) {
            printf(" null");
        }
        printf("\n");
    }

    // Test STL nullable string vector usage
    std::optional<std::vector<std::optional<std::string>>> nsin_1 = std::vector<std::optional<std::string>> { "abc", "def", "ghi" };
    std::optional<std::vector<std::optional<std::string>>> nsret_1;
    status = proxy->RepeatNullableStringVector(nsin_1, &nsret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatNullableStringVector error\n");
        return false;
    } else {
        printf("STL string ret is");
        for (auto a : *nsret_1) {
            printf(" %s", a->c_str());
        }
        printf("\n");
    }

    // Test STL nullable string List usage
    std::optional<std::vector<std::optional<std::string>>> nslin_1 = std::vector<std::optional<std::string>> { "abc", "def", "ghi" };
    std::optional<std::vector<std::optional<std::string>>> nslout_1;
    std::optional<std::vector<std::optional<std::string>>> nslret_1;
    status = proxy->Repeat2NullableStringList(nslin_1, &nslout_1, &nslret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL Repeat2NullableStringList error\n");
        return false;
    } else {
        printf("STL Nullable list string out is");
        for (auto a : *nslout_1) {
            printf(" %s", a->c_str());
        }
        if (!nslout_1) {
            printf(" null");
        }
        printf("\n");
        printf("STL Nullable list string ret is");
        for (auto a : *nslret_1) {
            printf(" %s", a->c_str());
        }
        if (!nslret_1) {
            printf(" null");
        }
        printf("\n");
    }

    return true;
}

bool testInOutVectorUsage(std::shared_ptr<aidl::INdkTestVector> proxy)
{
    ndk::ScopedAStatus status;

    // @BEGIN INOUT method
    // Test C type inout string vector usage
    int32_t inout_sin_len = 3;
    char** inout_sin = (char**)(malloc(inout_sin_len * sizeof(char*)));
    inout_sin[0] = (char*)(malloc(4 * sizeof(char)));
    inout_sin[1] = (char*)(malloc(4 * sizeof(char)));
    inout_sin[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_sin[0], "abc", 4);
    memcpy(inout_sin[1], "def", 4);
    memcpy(inout_sin[2], "ghi", 4);
    int32_t inout_sout_len = 3;
    char** inout_sout = (char**)(malloc(inout_sout_len * sizeof(char*)));
    inout_sout[0] = (char*)(malloc(4 * sizeof(char)));
    inout_sout[1] = (char*)(malloc(4 * sizeof(char)));
    inout_sout[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_sout[0], "ABC", 4);
    memcpy(inout_sout[1], "DEF", 4);
    memcpy(inout_sout[2], "GHI", 4);
    int32_t inout_sret_len = 0;
    char** inout_sret = nullptr;
    status = proxy->RepeatInOutStringVector(const_cast<const char**>(inout_sin), inout_sin_len, &inout_sout, &inout_sout_len, &inout_sret, &inout_sret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatInOutStringVector error\n");
        return false;
    } else {
        printf("inout String out is");
        for (int i = 0; i < inout_sout_len; i++) {
            printf(" %s", inout_sout[i]);
        }
        printf("\n");
        printf("inout String ret is");
        for (int i = 0; i < inout_sret_len; i++) {
            printf(" %s", inout_sret[i]);
        }
        printf("\n");
    }
    for (int i = 0; i < inout_sin_len; i++) {
        free(inout_sin[i]);
    }
    free(inout_sin);
    for (int i = 0; i < inout_sout_len; i++) {
        free(inout_sout[i]);
    }
    free(inout_sout);
    for (int i = 0; i < inout_sret_len; i++) {
        free(inout_sret[i]);
    }
    free(inout_sret);

    // Test STL inout string vector usage
    std::vector<std::string> inout_sin_1 = { "abc", "def", "ghi" };
    std::vector<std::string> inout_sout_1 = { "ABC", "DEF", "GHI" };
    std::vector<std::string> inout_sret_1;
    status = proxy->RepeatInOutStringVector(inout_sin_1, &inout_sout_1, &inout_sret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatInOutStringVector error\n");
        return false;
    } else {
        printf("STL inout string out is");
        for (auto a : inout_sout_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
        printf("STL inout string ret is");
        for (auto a : inout_sret_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
    }

    // Test C type inout string list usage
    int32_t inout_slin_len = 3;
    char** inout_slin = (char**)(malloc(inout_slin_len * sizeof(char*)));
    inout_slin[0] = (char*)(malloc(4 * sizeof(char)));
    inout_slin[1] = (char*)(malloc(4 * sizeof(char)));
    inout_slin[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_slin[0], "abc", 4);
    memcpy(inout_slin[1], "def", 4);
    memcpy(inout_slin[2], "ghi", 4);
    int32_t inout_slout_len = 3;
    char** inout_slout = (char**)(malloc(inout_slout_len * sizeof(char*)));
    inout_slout[0] = (char*)(malloc(4 * sizeof(char)));
    inout_slout[1] = (char*)(malloc(4 * sizeof(char)));
    inout_slout[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_slout[0], "ABC", 4);
    memcpy(inout_slout[1], "DEF", 4);
    memcpy(inout_slout[2], "GHI", 4);
    int32_t inout_slret_len = 0;
    char** inout_slret = nullptr;
    status = proxy->Repeat2InOutStringList(const_cast<const char**>(inout_slin), inout_slin_len, &inout_slout, &inout_slout_len, &inout_slret, &inout_slret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("Repeat2InOutStringList error\n");
        return false;
    } else {
        printf("inout String out is");
        for (int i = 0; i < inout_slout_len; i++) {
            printf(" %s", inout_slout[i]);
        }
        printf("\n");
        printf("inout String ret is");
        for (int i = 0; i < inout_slret_len; i++) {
            printf(" %s", inout_slret[i]);
        }
        printf("\n");
    }
    for (int i = 0; i < inout_slin_len; i++) {
        free(inout_slin[i]);
    }
    free(inout_slin);
    for (int i = 0; i < inout_slout_len; i++) {
        free(inout_slout[i]);
    }
    free(inout_slout);
    for (int i = 0; i < inout_slret_len; i++) {
        free(inout_slret[i]);
    }
    free(inout_slret);

    // Test STL inout string List usage
    std::vector<std::string> inout_slin_1 = { "abc", "def", "ghi" };
    std::vector<std::string> inout_slout_1 = { "ABC", "DEF", "GHI" };
    std::vector<std::string> inout_slret_1;
    status = proxy->Repeat2InOutStringList(inout_slin_1, &inout_slout_1, &inout_slret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL Repeat2InOutStringList error\n");
        return false;
    } else {
        printf("STL inout list string out is");
        for (auto a : inout_slout_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
        printf("STL inout list string ret is");
        for (auto a : inout_slret_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
    }

    return true;
    // @END INOUT method
}

bool testInOutNullabeVectorUsage(std::shared_ptr<aidl::INdkTestVector> proxy)
{
    ndk::ScopedAStatus status;

    // Test C type inout nullable string list usage
    int32_t inout_nslin_len = 3;
    char** inout_nslin = (char**)(malloc(inout_nslin_len * sizeof(char*)));
    inout_nslin[0] = (char*)(malloc(4 * sizeof(char)));
    inout_nslin[1] = (char*)(malloc(4 * sizeof(char)));
    inout_nslin[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_nslin[0], "abc", 4);
    memcpy(inout_nslin[1], "def", 4);
    memcpy(inout_nslin[2], "ghi", 4);
    int32_t inout_nslout_len = 3;
    char** inout_nslout = (char**)(malloc(inout_nslout_len * sizeof(char*)));
    inout_nslout[0] = (char*)(malloc(4 * sizeof(char)));
    inout_nslout[1] = (char*)(malloc(4 * sizeof(char)));
    inout_nslout[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_nslout[0], "ABC", 4);
    memcpy(inout_nslout[1], "DEF", 4);
    memcpy(inout_nslout[2], "GHI", 4);
    int32_t inout_nslret_len = 0;
    char** inout_nslret = nullptr;
    status = proxy->Repeat2InOutNullableStringList(const_cast<const char**>(inout_nslin), inout_nslin_len, &inout_nslout, &inout_nslout_len, &inout_nslret, &inout_nslret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("Repeat2InOutNullableStringList error\n");
        return false;
    } else {
        printf("inout Nullable String out is");
        for (int i = 0; i < inout_nslout_len; i++) {
            printf(" %s", inout_nslout[i]);
        }
        printf("\n");
        printf("inout Nullable String ret is");
        for (int i = 0; i < inout_nslret_len; i++) {
            printf(" %s", inout_nslret[i]);
        }
        printf("\n");
    }
    for (int i = 0; i < inout_nslin_len; i++) {
        free(inout_nslin[i]);
    }
    free(inout_nslin);
    for (int i = 0; i < inout_nslout_len; i++) {
        free(inout_nslout[i]);
    }
    free(inout_nslout);
    for (int i = 0; i < inout_nslret_len; i++) {
        free(inout_nslret[i]);
    }
    free(inout_nslret);

    // Test STL inout nullable string List usage
    std::optional<std::vector<std::optional<std::string>>> inout_nslin_1 = std::vector<std::optional<std::string>> { "abc", "def", "ghi" };
    std::optional<std::vector<std::optional<std::string>>> inout_nslout_1 = std::vector<std::optional<std::string>> { "ABC", "DEF", "GHI" };
    std::optional<std::vector<std::optional<std::string>>> inout_nslret_1;
    status = proxy->Repeat2InOutNullableStringList(inout_nslin_1, &inout_nslout_1, &inout_nslret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL Repeat2InOutNullableStringList error\n");
        return false;
    } else {
        printf("STL inout Nullable list string out is");
        for (auto a : *inout_nslout_1) {
            printf(" %s", a->c_str());
        }
        if (!inout_nslout_1) {
            printf(" null");
        }
        printf("\n");
        printf("STL inout Nullable list string ret is");
        for (auto a : *inout_nslret_1) {
            printf(" %s", a->c_str());
        }
        if (!inout_nslret_1) {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type inout nullable string vector usage
    int32_t inout_nsin_len = 3;
    char** inout_nsin = (char**)(malloc(inout_nsin_len * sizeof(char*)));
    inout_nsin[0] = (char*)(malloc(4 * sizeof(char)));
    inout_nsin[1] = (char*)(malloc(4 * sizeof(char)));
    inout_nsin[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_nsin[0], "abc", 4);
    memcpy(inout_nsin[1], "def", 4);
    memcpy(inout_nsin[2], "ghi", 4);
    int32_t inout_nsout_len = 3;
    char** inout_nsout = (char**)(malloc(inout_nsout_len * sizeof(char*)));
    inout_nsout[0] = (char*)(malloc(4 * sizeof(char)));
    inout_nsout[1] = (char*)(malloc(4 * sizeof(char)));
    inout_nsout[2] = (char*)(malloc(4 * sizeof(char)));
    memcpy(inout_nsout[0], "ABC", 4);
    memcpy(inout_nsout[1], "DEF", 4);
    memcpy(inout_nsout[2], "GHI", 4);
    int32_t inout_nsret_len = 0;
    char** inout_nsret = nullptr;
    status = proxy->RepeatInOutNullableStringVector(const_cast<const char**>(inout_nsin), inout_nsin_len, &inout_nsout, &inout_nsout_len, &inout_nsret, &inout_nsret_len);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("RepeatInOutNullableStringVector error\n");
        return false;
    } else {
        printf("inout Nullable String out is");
        for (int i = 0; i < inout_nsout_len; i++) {
            printf(" %s", inout_nsout[i]);
        }
        printf("\n");
        printf("inout Nullable String ret is");
        for (int i = 0; i < inout_nsret_len; i++) {
            printf(" %s", inout_nsret[i]);
        }
        printf("\n");
    }
    for (int i = 0; i < inout_nsin_len; i++) {
        free(inout_nsin[i]);
    }
    free(inout_nsin);
    for (int i = 0; i < inout_nsout_len; i++) {
        free(inout_nsout[i]);
    }
    free(inout_nsout);
    for (int i = 0; i < inout_nsret_len; i++) {
        free(inout_nsret[i]);
    }
    free(inout_nsret);

    // Test STL inout nullable string vector usage
    std::optional<std::vector<std::optional<std::string>>> inout_nsin_1 = std::vector<std::optional<std::string>> { "abc", "def", "ghi" };
    std::optional<std::vector<std::optional<std::string>>> inout_nsout_1 = std::vector<std::optional<std::string>> { "ABC", "DEF", "GHI" };
    std::optional<std::vector<std::optional<std::string>>> inout_nsret_1;
    status = proxy->RepeatInOutNullableStringVector(inout_nsin_1, &inout_nsout_1, &inout_nsret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        ALOGE("STL RepeatInOutNullableStringVector error\n");
        return false;
    } else {
        printf("STL inout Nullable list string out is");
        for (auto a : *inout_nsout_1) {
            printf(" %s", a->c_str());
        }
        if (!inout_nsout_1) {
            printf(" null");
        }
        printf("\n");
        printf("STL inout Nullable list string ret is");
        for (auto a : *inout_nsret_1) {
            printf(" %s", a->c_str());
        }
        if (!inout_nsret_1) {
            printf(" null");
        }
        printf("\n");
    }

    return true;
}

extern "C" int main(int argc, char** argv)
{
    ALOGI("vector demo client start argc: %d, argv[0]: %s\n", argc, argv[0]);

    ndk::ScopedAStatus status;

    ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_checkService("ndktestvector.service"));
    if (binder.get() == nullptr) {
        ALOGE("binder is null\n");
        return 1;
    }

    std::shared_ptr<aidl::INdkTestVector> proxy = aidl::INdkTestVector::fromBinder(binder);
    if (proxy.get() == nullptr) {
        ALOGE("proxy is null\n");
        return 1;
    }

    if (!testCTypeVectorUsage(proxy)) {
        ALOGE("testCTypeVectorUsage NOT PASS");
    } else {
        ALOGI("testCTypeVectorUsage OK");
    }

    if (!testCTypeNullableVectorUsage(proxy)) {
        ALOGE("testNullableVectorUsage NOT PASS");
    } else {
        ALOGI("testNullableVectorUsage OK");
    }

    if (!testSTLVectorUsage(proxy)) {
        ALOGE("testSTLVectorUsage NOT PASS");
    } else {
        ALOGI("testSTLVectorUsage OK");
    }

    if (!testSTLNullableVectorUsage(proxy)) {
        ALOGE("testSTLNullableVectorUsage NOT PASS");
    } else {
        ALOGI("testSTLNullableVectorUsage OK");
    }

    if (!testInOutVectorUsage(proxy)) {
        ALOGE("testInOutVectorUsage NOT PASS");
    } else {
        ALOGI("testInOutVectorUsage OK");
    }

    if (!testInOutNullabeVectorUsage(proxy)) {
        ALOGE("testInOutNullabeVectorUsage NOT PASS");
    } else {
        ALOGI("testInOutNullabeVectorUsage OK");
    }

    return 0;
}
