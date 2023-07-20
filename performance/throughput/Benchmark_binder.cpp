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

bool startServer()
{
    sp<BenchmarkService> service = new BenchmarkService();
    // Tells the kernel to spawn zero threads, but startThreadPool() below will still spawn one.
    defaultServiceManager()->addService(String16(kServiceName),
        service);
    ProcessState::self()->startThreadPool();
    return 0;
}

static void BM_sendVec_binder(benchmark::State& state)
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

BENCHMARK(BM_sendVec_binder)->RangeMultiplier(2)->Range(4, 65536);

extern "C" int main(int argc, char* argv[])
{
    pid_t pid;
    int pipe_fd[2];
    pipe(pipe_fd);

    pid = fork();

    if (pid == 0) {
        // Child, close read end of the pipe
        close(pipe_fd[0]);
        ::benchmark::RunSpecifiedBenchmarks();
        // Notify parent that benchmarks are finished
        char buf[1] = { 1 };
        write(pipe_fd[1], buf, 1);
        close(pipe_fd[1]);
    } else {
        // Parent, close write end of the pipe
        close(pipe_fd[1]);
        startServer();
        // Wait for child to finish
        char buf[1];
        read(pipe_fd[0], buf, 1);
        close(pipe_fd[0]);
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
