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

#include <benchmark/benchmark.h>

#include <binder/Binder.h>
#include <binder/IBinder.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <utils/String16.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
using namespace android;

const char kServiceName[] = "android.tests.binder.IBenchmark";

enum BenchmarkServiceCode {
    BINDER_ECHO_VECTOR = IBinder::FIRST_CALL_TRANSACTION,
};

class BenchmarkService : public BBinder {
public:
    BenchmarkService()
    {
    }
    ~BenchmarkService()
    {
    }
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
        uint32_t flags = 0)
    {
        (void)flags;
        switch (code) {
        case BINDER_ECHO_VECTOR: {
            std::vector<uint8_t> vector;
            auto err = data.readByteVector(&vector);
            if (err != NO_ERROR)
                return err;
            reply->writeByteVector(vector);
            return NO_ERROR;
        }
        default:
            return UNKNOWN_TRANSACTION;
        };
    }
};

static void startServer()
{
    sp<BenchmarkService> service = new BenchmarkService();
    // Tells the kernel to spawn zero threads, but startThreadPool() below will still spawn one.
    defaultServiceManager()->addService(String16(kServiceName),
        service);
    ProcessState::self()->startThreadPool();
}

void BM_sendVec(benchmark::State& state)
{
    sp<IBinder> service;
    sp<IServiceManager> sm = defaultServiceManager();
    Parcel data, reply;
    // Prepare data to IPC
    vector<uint8_t> data_vec;
    data_vec.resize(state.range(0));
    for (int i = 0; i < state.range(0); i++) {
        data_vec[i] = i % 256;
    }

    data.writeByteVector(data_vec);
    // printf("%s: pid %d, get service\n", __func__, m_pid);
    service = sm->getService(String16(kServiceName));
    // getService automatically retries
    if (service == NULL) {
        state.SkipWithError("Failed to retrieve benchmark service.");
    }
    // Start running
    while (state.KeepRunning()) {
        service->transact(BINDER_ECHO_VECTOR, data, &reply);
    }
}

BENCHMARK(BM_sendVec)->RangeMultiplier(2)->Range(4, 65536);

extern "C" int main(int argc, char* argv[])
{
    pid_t pid = fork();

    if (pid == 0) {
        ::benchmark::RunSpecifiedBenchmarks();
        exit(EXIT_SUCCESS);
    } else {
        startServer();
        // Wait for child to finish
        while (true) {
            int stat, retval;
            retval = wait(&stat);
            if (retval == -1 && errno == ECHILD) {
                break;
            }
        }
    }

    return 0;
}
