/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#define LOG_TAG "SendVec"

#include "BnBinderLatency.h"
#include "latency_time.h"

#include <cinttypes>

#include <binder/ICpcServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

using namespace android;
using android::binder::Status;

#define SIZE 1024

static void latency_test(int iterations, const char* binder_type, bool oneway)
{
    sp<IServiceManager> sm_ipc(defaultServiceManager());
    sp<IServiceManager> sm_rpc(defaultCpcServiceManager());
    sp<IBinder> remoteBinder;
    IServiceManager* sm;

    printf("thread sched_priority: %d\n", thread_pri());

    if (!strcmp(binder_type, "rpc")) {
        sm = sm_rpc.get();
    } else {
        sm = sm_ipc.get();
    }

    remoteBinder = sm->getService(String16("sendvec"));

    // auto
    if (!remoteBinder) {
        printf("remoteBinder get nullptr error");
        return;
    }

    // interface_cast restore IBinderLatency interface
    sp<IBinderLatency> service = interface_cast<IBinderLatency>(remoteBinder);
    if (service) {
        Results results_fifo(false);
        Tick begin, end;
        // Prepare data to IPC
        std::vector<uint8_t> data_vec;
        std::vector<uint8_t> data_return;

        data_vec.resize(SIZE);
        for (int i = 0; i < SIZE; ++i) {
            data_vec[i] = i % 256;
        }

        for (int i = 0; i < iterations; ++i) {
            begin = tickNow();
            if (!oneway) {
                service->sendVec(data_vec, &data_return);
            } else {
                service->sendVecOneWay(data_vec);
            }
            end = tickNow();
            results_fifo.add_time(tickNano(begin, end));
        }

        printf("%s: latency results (ms):\n", oneway ? "oneway" : "dualway");
        results_fifo.dump();
    } else {
        printf("remoteBinder get is null error\n");
    }
}

extern "C" int main(int argc, char** argv)
{
    if (argc != 4) {
        printf("[usage]: latency_sendvec_client [test_count] [ipc/rpc] [oneway]\n");
        return 0;
    }

    int iterations = atoi(argv[1]);

    latency_test(iterations, argv[2], !strcmp(argv[3], "oneway"));

    return 0;
}
