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

#include <aidl/INdkTestArray.h>
#include <android/binder_manager.h>

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
    return 0;
}