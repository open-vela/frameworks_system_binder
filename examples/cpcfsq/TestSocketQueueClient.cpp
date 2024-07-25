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

#define LOG_TAG "TestSocketQueueClient"

#include <binder/ICpcServiceManager.h>
#include <binder/IServiceManager.h>

#include <binder/AidlSocketQueue.h>
#include <binder/SocketDescriptor.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include "ITestAidlSocketQ.h"

using namespace android;
using android::binder::Status;

typedef android::AidlSocketQueue<int> AidlSocketQ;

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

static bool remoteWriteLocalReadTest(sp<ITestAidlSocketQ>& service,
    AidlSocketQ* mQueue, int dataLen)
{
    auto ret = service->requestWriteSocketQAsync(dataLen);
    if (!ret.isOk()) {
        ALOGE("Failed to call mService->requestWriteSocketQ!\n");
        return false;
    }

    int* readData = new int[dataLen];

    auto result = mQueue->read(readData, dataLen);
    if (!result) {
        goto end;
    }

    result = checkData(readData, dataLen);

end:
    delete[] readData;
    return result;
}

static bool remoteReadLocalWriteTest(sp<ITestAidlSocketQ>& service,
    AidlSocketQ* mQueue, int dataLen)
{
    auto ret = service->requestReadSocketQAsync(dataLen);
    if (!ret.isOk()) {
        ALOGE("Failed to call mService->requestReadSocketQ!\n");
        return false;
    }

    int* writeData = new int[dataLen];

    for (int i = 0; i < dataLen; ++i) {
        writeData[i] = i;
    }

    auto result = mQueue->write(writeData, dataLen);
    delete[] writeData;
    return result;
}

extern "C" int main(int argc, char** argv)
{
    AidlSocketQ* mQueue = nullptr;
    constexpr int dataLen = 1024 * 1024;
    ::binder::SocketDescriptor desc;

    if (argc != 2) {
        printf("usage: %s [socket type net/local/rpmsg]\n", argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "local")) {
        ::binder::SocketDescriptor::LocalSockAddr local_sock_addr;
        local_sock_addr.sun_path = String16("./socket_server");
        desc.sock_addr = local_sock_addr;
    } else if (!strcmp(argv[1], "rpmsg")) {
        ::binder::SocketDescriptor::RpmsgSockAddr rpmsg_sock_addr;
        rpmsg_sock_addr.rp_cpu = String16("droid");
        rpmsg_sock_addr.rp_name = String16("Hello");
        desc.sock_addr = rpmsg_sock_addr;
        ;
    } else if (!strcmp(argv[1], "net")) {
        ::binder::SocketDescriptor::NetSockAddr net_sock_addr;
        net_sock_addr.net_port = 12345;
        net_sock_addr.net_addr = String16("127.0.0.1");
        desc.sock_addr = net_sock_addr;
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

    // obtain socketQ.service
    sp<IBinder> binder = sm->getService(String16("socketQ.service"));
    if (binder == NULL) {
        ALOGE("socketQ service binder is null, abort...");
        return -1;
    }
    ALOGI("socketQ service binder is %p", binder.get());

    // by interface_cast restore ITest
    sp<ITestAidlSocketQ> service = interface_cast<ITestAidlSocketQ>(binder);
    ALOGI("socketQ service is %p", service.get());

    auto ret = service->createSocketQ(desc);
    if (!ret.isOk()) {
        ALOGI("Call mService->configureSocketQReadWrite Failed!\n");
        return -1;
    }

    mQueue = new (std::nothrow) AidlSocketQ(desc, false);

    // remote write, local read test
    auto result = remoteWriteLocalReadTest(service, mQueue, dataLen);
    if (!result) {
        ALOGE("Server write Client read fail!\n");
        goto end;
    } else {
        ALOGE("Server write Client read pass!\n");
    }

    // remote read, local write test
    result = remoteReadLocalWriteTest(service, mQueue, dataLen);
    if (!result) {
        ALOGE("Request Server read Client write fail !\n");
    } else {
        ALOGE("Request Server read Client write pass !\n");
    }

end:
    service->destroySocketQ();
    delete mQueue;
    return 0;
}
