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

#define LOG_TAG "TestSocketQueueServer"

#include <binder/ICpcServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <binder/AidlSocketQueue.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include "BnTestAidlSocketQ.h"

using namespace android;
using android::binder::Status;

#define BUFSZ (10 * 1024 * 1024)

static bool checkData(int* data, int count)
{
    for (int i = 0; i < count; ++i) {
        if (data[i] != i) {
            ALOGE("check error data[%d] = %d\n", i, data[i]);
            return false;
        }
    }
    return true;
}

namespace android {
class ITestAidlSocketQServer : public BnTestAidlSocketQ {
    typedef AidlSocketQueue<int> SocketQueue;

public:
    // ITestAidlSocketQServer::createSocketQ
    Status createSocketQ(const ::binder::SocketDescriptor& desc)
    {
        mSocketQ = new (std::nothrow)
            ITestAidlSocketQServer::SocketQueue(desc, true, 3000);
        if ((mSocketQ == nullptr) || (mSocketQ->isValid() == false)) {
            ALOGE("ITestAidlSocketQ: Failed to configure SocketQ!\n");
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }
        return Status::ok();
    }

    // ITestAidlSocketQServer::destroySocketQ
    Status destroySocketQ()
    {
        delete mSocketQ;
        return Status::ok();
    }

    // ITestAidlSocketQServer::requestReadSocketQAync
    Status requestReadSocketQAsync(int32_t count)
    {
        int* data = new int[count];
        bool result = mSocketQ->read(&data[0], count);
        if (!result) {
            delete[] data;
            return Status::fromExceptionCode(Status::EX_TRANSACTION_FAILED);
        }
        result = checkData(data, count);
        if (result) {
            ALOGE("Client write Server read case pass!\n");
        } else {
            ALOGE("Client write Server read case fail!\n");
        }
        delete[] data;
        return Status::ok();
    }

    // ITestAidlSocketQServer::requestWriteSocketQ
    Status requestWriteSocketQAsync(int32_t count)
    {
        int* data = new int[count];
        for (int i = 0; i < count; ++i) {
            data[i] = i;
        }
        bool result = mSocketQ->write(&data[0], count);
        delete[] data;
        if (!result) {
            return Status::fromExceptionCode(Status::EX_TRANSACTION_FAILED);
        }
        return Status::ok();
    }

private:
    SocketQueue* mSocketQ;
};
} // namespace android

extern "C" int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("usage: %s [socket type net/local/rpmsg]\n", argv[0]);
        return -1;
    }

    // obtain service manager
    IServiceManager* sm;
    sp<IServiceManager> sm_ipc(defaultServiceManager());
    sp<IServiceManager> sm_rpc(defaultCpcServiceManager());

    if (!strcmp(argv[1], "net") || !strcmp(argv[1], "local")) {
        sm = sm_ipc.get();
    } else if (!strcmp(argv[1], "rpmsg")) {
        sm = sm_rpc.get();
    } else {
        printf("usage: %s [socket type net/local/rpmsg]\n", argv[0]);
        return -1;
    }

    sp<ITestAidlSocketQServer> testSockServer = new ITestAidlSocketQServer;

    // add service
    sm->addService(String16("socketQ.service"), testSockServer);
    ALOGI("add socketQ.service to service manager");

    // start worker thread
    ProcessState::self()->startThreadPool();

    // join the main thread
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
