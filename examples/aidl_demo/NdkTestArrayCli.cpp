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

#define LOG_TAG "NdkTestArrayCli"

#include <inttypes.h>
#include <stdio.h>

#include <aidl/INdkTestArray.h>
#include <android/binder_manager.h>
#include <utils/Log.h>

bool testCTypeArrayUsage(std::shared_ptr<aidl::INdkTestArray> proxy)
{
    ndk::ScopedAStatus status;

    // Test C type bool array usage
    bool bin[3] = { true, true, false };
    bool bout[3];
    bool bret[3];
    status = proxy->RepeatBooleanArray(bin, bout, bret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatBooleanArray error\n");
        return 1;
    } else {
        printf("Bool out is");
        for (int i = 0; i < 3; i++) {
            if (bout[i]) {
                printf(" true");
            } else {
                printf(" false");
            }
        }
        printf("\n");
        printf("Bool ret is");
        for (int i = 0; i < 3; i++) {
            if (bret[i]) {
                printf(" true");
            } else {
                printf(" false");
            }
        }
        printf("\n");
    }

    // Test C type byte array usage
    uint8_t byin[3] = { '1', '2', '3' };
    uint8_t byout[3];
    uint8_t byret[3];
    status = proxy->RepeatByteArray(byin, byout, byret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatByteArray error\n");
        return 1;
    } else {
        printf("Byte out is %c %c %c\n", byout[0], byout[1], byout[2]);
        printf("Byte ret is %c %c %c\n", byret[0], byret[1], byret[2]);
    }

    // Test C type char array usage
    char16_t cin[3] = { '1', '2', '3' };
    char16_t cout[3];
    char16_t cret[3];
    status = proxy->RepeatCharArray(cin, cout, cret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatCharArray error\n");
        return 1;
    } else {
        printf("Char out is %c %c %c\n", cout[0], cout[1], cout[2]);
        printf("Char ret is %c %c %c\n", cret[0], cret[1], cret[2]);
    }

    // Test C type int array usage
    int32_t iin[3] = { 1, 2, 3 };
    int32_t iout[3];
    int32_t iret[3];
    status = proxy->RepeatIntArray(iin, iout, iret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatIntArray error\n");
        return 1;
    } else {
        printf("Int out is %" PRId32 " %" PRId32 " %" PRId32 "\n", iout[0], iout[1], iout[2]);
        printf("Int ret is %" PRId32 " %" PRId32 " %" PRId32 "\n", iret[0], iret[1], iret[2]);
    }

    // Test C type long array usage
    int64_t lin[3] = { 1, 2, 3 };
    int64_t lout[3];
    int64_t lret[3];
    status = proxy->RepeatLongArray(lin, lout, lret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatLongArray error\n");
        return 1;
    } else {
        printf("Long out is %" PRId64 " %" PRId64 " %" PRId64 "\n", lout[0], lout[1], lout[2]);
        printf("Long ret is %" PRId64 " %" PRId64 " %" PRId64 "\n", lret[0], lret[1], lret[2]);
    }

    // Test C type float array usage
    float fin[3] = { 1.0, 2.0, 3.0 };
    float fout[3];
    float fret[3];
    status = proxy->RepeatFloatArray(fin, fout, fret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatFloatArray error\n");
        return 1;
    } else {
        printf("Float out is %f %f %f\n", fout[0], fout[1], fout[2]);
        printf("Float ret is %f %f %f\n", fret[0], fret[1], fret[2]);
    }

    // Test C type double array usage
    double din[3] = { 1.0, 2.0, 3.0 };
    double dout[3];
    double dret[3];
    status = proxy->RepeatDoubleArray(din, dout, dret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatDoubleArray error\n");
        return 1;
    } else {
        printf("Double out is %lf %lf %lf\n", dout[0], dout[1], dout[2]);
        printf("Double ret is %lf %lf %lf\n", dret[0], dret[1], dret[2]);
    }

    // Test C type string array usage
    const char* sin[3] = { "abc", "def", "ghi" };
    char* sout[3];
    char* sret[3];
    status = proxy->RepeatStringArray(sin, sout, sret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatStringArray error\n");
        return 1;
    } else {
        printf("String out is");
        for (int i = 0; i < 3; i++) {
            printf(" %s", sout[i]);
        }
        printf("\n");
        printf("String ret is");
        for (int i = 0; i < 3; i++) {
            printf(" %s", sret[i]);
        }
        printf("\n");
    }
    for (int i = 0; i < 3; i++) {
        free(sout[i]);
        free(sret[i]);
    }

    return true;
}

bool testCTypeNullableArrayUsage(std::shared_ptr<aidl::INdkTestArray> proxy)
{
    ndk::ScopedAStatus status;

    // Test C type nullable bool array usage
    // bool nbin[3] = { true, true, false };
    bool* nbin = nullptr;
    bool nbret[3];
    bool* nbret_p = nbret;
    status = proxy->RepeatNullableBooleanArray(nbin, &nbret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableBooleanArray error\n");
        return 1;
    } else {
        printf("Nullable Bool ret is");
        if (nbret_p) {
            for (int i = 0; i < 3; i++) {
                if (nbret[i]) {
                    printf(" true");
                } else {
                    printf(" false");
                }
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type nullable byte array usage
    // uint8_t nbyin[3] = { '1', '2', '3' };
    uint8_t* nbyin = nullptr;
    uint8_t nbyret[3];
    uint8_t* nbyret_p = nbyret;
    status = proxy->RepeatNullableByteArray(nbyin, &nbyret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableByteArray error\n");
        return 1;
    } else {
        printf("Nullable Byte ret is");
        if (nbyret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %c", nbyret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type nullable char array usage
    // char16_t ncin[3] = { '1', '2', '3' };
    char16_t* ncin = nullptr;
    char16_t ncret[3];
    char16_t* ncret_p = ncret;
    status = proxy->RepeatNullableCharArray(ncin, &ncret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableCharArray error\n");
        return 1;
    } else {
        printf("Nullable Char ret is");
        if (ncret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %c", ncret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type nullable int array usage
    // int32_t niin[3] = { 1, 2, 3 };
    int32_t* niin = nullptr;
    int32_t niret[3];
    int32_t* niret_p = niret;
    status = proxy->RepeatNullableIntArray(niin, &niret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableIntArray error\n");
        return 1;
    } else {
        printf("Nullable Int ret is");
        if (niret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %" PRId32, niret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type nullable long array usage
    // int64_t nlin[3] = { 1, 2, 3 };
    int64_t* nlin = nullptr;
    int64_t nlret[3];
    int64_t* nlret_p = nlret;
    status = proxy->RepeatNullableLongArray(nlin, &nlret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableLongArray error\n");
        return 1;
    } else {
        printf("Nullable Long ret is");
        if (nlret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %" PRId64, nlret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type nullable float array usage
    // float nfin[3] = { 1.0, 2.0, 3.0 };
    float* nfin = nullptr;
    float nfret[3];
    float* nfret_p = nfret;
    status = proxy->RepeatNullableFloatArray(nfin, &nfret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableFloatArray error\n");
        return 1;
    } else {
        printf("Nullable Float ret is");
        if (nfret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %f", nfret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type nullable double array usage
    // double ndin[3] = { 1.0, 2.0, 3.0 };
    double* ndin = nullptr;
    double ndret[3];
    double* ndret_p = ndret;
    status = proxy->RepeatNullableDoubleArray(ndin, &ndret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableDoubleArray error\n");
        return 1;
    } else {
        printf("Nullable Double ret is");
        if (ndret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %lf", ndret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }

    // Test C type nullable string array usage
    // const char* nsin[3] = { "abc", "def", "ghi" };
    const char** nsin = nullptr;
    char* nsret[3];
    char** nsret_p = nsret;
    status = proxy->RepeatNullableStringArray(nsin, &nsret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatNullableStringArray error\n");
        return 1;
    } else {
        printf("String ret is");
        if (nsret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %s", nsret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }
    if (nsret_p) {
        for (int i = 0; i < 3; i++) {
            free(nsret[i]);
        }
    }

    return true;
}

bool testSTLArrayUsage(std::shared_ptr<aidl::INdkTestArray> proxy)
{
    ndk::ScopedAStatus status;

    // @BEGIN SLT method
    // Test STL bool array usage
    std::array<bool, 3> bin_1 = { true, true, false };
    std::array<bool, 3> bout_1;
    std::array<bool, 3> bret_1;
    status = proxy->RepeatBooleanArray(bin_1, &bout_1, &bret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL RepeatBooleanArray error\n");
        return 1;
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

    // Test STL byte array usage
    std::array<uint8_t, 3> byin_1 = { '1', '2', '3' };
    std::array<uint8_t, 3> byout_1;
    std::array<uint8_t, 3> byret_1;
    status = proxy->RepeatByteArray(byin_1, &byout_1, &byret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL RepeatByteArray error\n");
        return 1;
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

    // Test STL string array usage
    std::array<std::string, 3> sin_1 = { "abc", "def", "ghi" };
    std::array<std::string, 3> sout_1;
    std::array<std::string, 3> sret_1;
    status = proxy->RepeatStringArray(sin_1, &sout_1, &sret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL RepeatStringArray error\n");
        return 1;
    } else {
        printf("STL String out is");
        for (auto a : sout_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
        printf("STL String ret is");
        for (auto a : sret_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
    }

    return true;
}

bool testSTLNullableArrayUsage(std::shared_ptr<aidl::INdkTestArray> proxy)
{
    ndk::ScopedAStatus status;

    // Test STL nullable bool array usage
    // std::optional<std::array<bool,3>> nbin_1 = std::array<bool,3>{true, false, true};
    std::optional<std::array<bool, 3>> nbin_1;
    std::optional<std::array<bool, 3>> nbret_1;
    status = proxy->RepeatNullableBooleanArray(nbin_1, &nbret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL RepeatNullableBooleanArray error\n");
        return 1;
    } else {
        printf("STL Nullable Bool ret is");
        if (!nbret_1) {
            printf(" null");
        } else {
            for (auto a : *nbret_1) {
                if (a) {
                    printf(" true");
                } else {
                    printf(" false");
                }
            }
        }
        printf("\n");
    }

    // Test STL nullable byte array usage
    // std::optional<std::array<uint8_t,3>> nbyin_1 = std::array<uint8_t,3>{'1', '2', '3'};
    std::optional<std::array<uint8_t, 3>> nbyin_1;
    std::optional<std::array<uint8_t, 3>> nbyret_1;
    status = proxy->RepeatNullableByteArray(nbyin_1, &nbyret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL RepeatNullableByteArray error\n");
        return 1;
    } else {
        printf("STL Nullable Byte ret is");
        if (!nbyret_1) {
            printf(" null");
        } else {
            for (auto a : *nbyret_1) {
                printf(" %c", a);
            }
        }
        printf("\n");
    }

    // Test STL nullable string array usage
    // std::optional<std::array<std::optional<std::string>, 3>> nsin_1 = std::array<std::optional<std::string>, 3>{"abc", "def", "ghi"};
    std::optional<std::array<std::optional<std::string>, 3>> nsin_1;
    std::optional<std::array<std::optional<std::string>, 3>> nsret_1;
    status = proxy->RepeatNullableStringArray(nsin_1, &nsret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL RepeatNullableStringArray error\n");
        return 1;
    } else {
        printf("STL string ret is");
        if (!nsret_1) {
            printf(" null");
        } else {
            for (auto a : *nsret_1) {
                printf(" %s", a->c_str());
            }
        }
        printf("\n");
    }
    // @END SLT method

    return true;
}

bool testInOutArrayUsage(std::shared_ptr<aidl::INdkTestArray> proxy)
{
    ndk::ScopedAStatus status;

    // @BEGIN INOUT method
    // Test C type inout string array usage
    const char* inout_sin[3] = { "abc", "def", "ghi" };
    char* inout_sout[3];
    inout_sout[0] = (char*)malloc(4 * sizeof(char));
    inout_sout[1] = (char*)malloc(4 * sizeof(char));
    inout_sout[2] = (char*)malloc(4 * sizeof(char));
    strcpy(inout_sout[0], "ABC");
    strcpy(inout_sout[1], "DEF");
    strcpy(inout_sout[2], "GHI");
    char* inout_sret[3];
    status = proxy->RepeatInOutStringArray(inout_sin, inout_sout, inout_sret);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatInOutStringArray error\n");
        return 1;
    } else {
        printf("inout String out is");
        for (int i = 0; i < 3; i++) {
            printf(" %s", inout_sout[i]);
        }
        printf("\n");
        printf("inout String ret is");
        for (int i = 0; i < 3; i++) {
            printf(" %s", inout_sret[i]);
        }
        printf("\n");
    }
    for (int i = 0; i < 3; i++) {
        free(inout_sout[i]);
    }
    for (int i = 0; i < 3; i++) {
        free(inout_sret[i]);
    }

    // Test C type inout nullable string array usage
    const char* inout_nsin[3] = { "abc", "def", "ghi" };
    char* inout_nsout[3];
    inout_nsout[0] = (char*)malloc(4 * sizeof(char));
    inout_nsout[1] = (char*)malloc(4 * sizeof(char));
    inout_nsout[2] = (char*)malloc(4 * sizeof(char));
    strcpy(inout_nsout[0], "ABC");
    strcpy(inout_nsout[1], "DEF");
    strcpy(inout_nsout[2], "GHI");
    char** inout_nsout_p = inout_nsout;
    char* inout_nsret[3];
    char** inout_nsret_p = inout_nsret;
    status = proxy->RepeatInOutNullableStringArray(inout_nsin, &inout_nsout_p, &inout_nsret_p);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("RepeatInOutNullableStringArray error\n");
        return 1;
    } else {
        printf("inout nullable String out is");
        if (inout_nsout_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %s", inout_nsout[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
        printf("inout nullable String ret is");
        if (inout_nsret_p) {
            for (int i = 0; i < 3; i++) {
                printf(" %s", inout_nsret[i]);
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }
    if (inout_nsout_p) {
        for (int i = 0; i < 3; i++) {
            free(inout_nsout[i]);
        }
    }
    if (inout_nsret_p) {
        for (int i = 0; i < 3; i++) {
            free(inout_nsret[i]);
        }
    }

    // Test STL inout string array usage
    std::array<std::string, 3> inout_sin_1 = { "abc", "def", "ghi" };
    std::array<std::string, 3> inout_sout_1 = { "ABC", "DEF", "GHI" };
    std::array<std::string, 3> inout_sret_1;
    status = proxy->RepeatInOutStringArray(inout_sin_1, &inout_sout_1, &inout_sret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL inout RepeatInOutStringArray error\n");
        return 1;
    } else {
        printf("STL inout String out is");
        for (auto a : inout_sout_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
        printf("STL inout String ret is");
        for (auto a : inout_sret_1) {
            printf(" %s", a.c_str());
        }
        printf("\n");
    }

    // Test STL inout nullable string array usage
    std::optional<std::array<std::optional<std::string>, 3>> inout_nsin_1 = std::array<std::optional<std::string>, 3> { "abc", "def", "ghi" };
    std::optional<std::array<std::optional<std::string>, 3>> inout_nsout_1 = std::array<std::optional<std::string>, 3> { "ABC", "DEF", "GHI" };
    std::optional<std::array<std::optional<std::string>, 3>> inout_nsret_1;
    status = proxy->RepeatInOutNullableStringArray(inout_nsin_1, &inout_nsout_1, &inout_nsret_1);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("STL RepeatInOutNullableStringArray error\n");
        return 1;
    } else {
        printf("STL inout nullable string out is");
        if (inout_nsout_1) {
            for (auto a : *inout_nsout_1) {
                printf(" %s", a->c_str());
            }
        } else {
            printf(" null");
        }
        printf("\n");
        printf("STL inout nullable string ret is");
        if (inout_nsret_1) {
            for (auto a : *inout_nsret_1) {
                printf(" %s", a->c_str());
            }
        } else {
            printf(" null");
        }
        printf("\n");
    }
    // @END INOUT method

    return true;
}

extern "C" int main(int argc, char** argv)
{
    printf("vector demo client start argc: %d, argv[0]: %s\n", argc, argv[0]);

    ndk::ScopedAStatus status;

    ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_checkService("ndktestarray.service"));
    if (binder.get() == nullptr) {
        printf("binder is null\n");
        return 1;
    }

    std::shared_ptr<aidl::INdkTestArray> proxy = aidl::INdkTestArray::fromBinder(binder);
    if (proxy.get() == nullptr) {
        printf("proxy is null\n");
        return 1;
    }

    if (!testCTypeArrayUsage(proxy)) {
        ALOGE("testCTypeArrayUsage NOT PASS");
    } else {
        ALOGI("testCTypeArrayUsage OK");
    }

    if (!testCTypeNullableArrayUsage(proxy)) {
        ALOGE("testCTypeNullableArrayUsage NOT PASS");
    } else {
        ALOGI("testCTypeNullableArrayUsage OK");
    }

    if (!testSTLArrayUsage(proxy)) {
        ALOGE("testSTLArrayUsage NOT PASS");
    } else {
        ALOGI("testSTLArrayUsage OK");
    }

    if (!testSTLNullableArrayUsage(proxy)) {
        ALOGE("testSTLNullableArrayUsage NOT PASS");
    } else {
        ALOGI("testSTLNullableArrayUsage OK");
    }

    if (!testInOutArrayUsage(proxy)) {
        ALOGE("testInOutArrayUsage NOT PASS");
    } else {
        ALOGI("testInOutArrayUsage OK");
    }

    return 0;
}
