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

#define LOG_TAG "CpcFsqJniServer.cpp"

#include <binder/ICpcServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <binder/AidlSocketQueue.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include "BnCpcFsqJni.h"

using namespace android;
using android::binder::Status;

#define BUFSZ (1024)

static bool checkData(uint8_t* data, int count)
{
    for (int i = 0; i < count; ++i) {
        if (data[i] != (uint8_t)(i % 255)) {
            ALOGE("check error data[%d] = %02x\n", i, data[i]);
            return false;
        }
    }
    return true;
}

namespace android {
class ICpcFsqJniServer : public BnCpcFsqJni {
    typedef AidlSocketQueue<uint8_t> SocketQueue;

public:
    ICpcFsqJniServer()
        : mSocketQ(nullptr)
        , mFd(nullptr)
        , mFileLen(0)
    {
    }

    // ICpcFsqJniServer::createSocketQ
    Status createSocketQ(const ::binder::SocketDescriptor& desc)
    {
        mSocketQ = new (std::nothrow)
            ICpcFsqJniServer::SocketQueue(desc, true);
        if ((mSocketQ == nullptr) || (mSocketQ->isValid() == false)) {
            ALOGE("ICpcFsqJni: Failed to configure SocketQ!\n");
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }
        return Status::ok();
    }

    // ICpcFsqJniServer::destroySocketQ
    Status destroySocketQ()
    {
        pthread_mutex_lock(&mFileMutex);
        while (mFileBusy != false) {
            pthread_cond_wait(&mFileCondition, &mFileMutex);
        }
        pthread_mutex_unlock(&mFileMutex);
        pthread_mutex_destroy(&mFileMutex);
        pthread_cond_destroy(&mFileCondition);
        delete mSocketQ;
        return Status::ok();
    }

    // ICpcFsqJniServer::requestReadSocketQAync
    Status requestReadSocketQAsync(int32_t count)
    {
        uint8_t* data = new uint8_t[count];
        bool result = mSocketQ->read(&data[0], count);
        if (!result) {
            delete[] data;
            return Status::fromExceptionCode(Status::EX_TRANSACTION_FAILED);
        }

        if (mFd != nullptr) {
            int ret = fwrite(&data[0], 1, count, mFd);
            mFileLen -= ret;
            if (mFileLen <= 0) {
                pthread_mutex_lock(&mFileMutex);
                fclose(mFd);
                mFd = nullptr;
                mFileBusy = false;
                pthread_cond_broadcast(&mFileCondition);
                pthread_mutex_unlock(&mFileMutex);
            }
            delete[] data;
        } else {
            result = checkData(data, count);
            if (result) {
                ALOGE("Client write Server read case pass!\n");
            } else {
                ALOGE("Client write Server read case fail!\n");
            }
            delete[] data;
        }
        return Status::ok();
    }

    // ICpcFsqJniServer::requestWriteSocketQ
    Status requestWriteSocketQAsync(int32_t count)
    {
        uint8_t* data = new uint8_t[count];
        for (int i = 0; i < count; ++i) {
            data[i] = (uint8_t)(i % 255);
        }
        bool result = mSocketQ->write(&data[0], count);
        delete[] data;
        if (!result)
            return Status::fromExceptionCode(Status::EX_TRANSACTION_FAILED);
        return Status::ok();
    }

    // ICpcFsqJniServer::requestWriteFilePath
    Status requestWriteFilePath(const String16& path, int32_t count)
    {
        mFileLen = count;
        mFd = fopen((String8(path) + String8("_copy")).c_str(), "w+");
        if (mFd == nullptr) {
            return Status::fromExceptionCode(Status::EX_TRANSACTION_FAILED);
        }
        pthread_mutex_init(&mFileMutex, NULL);
        pthread_cond_init(&mFileCondition, NULL);
        mFileBusy = true;
        return Status::ok();
    }

private:
    SocketQueue* mSocketQ;
    pthread_mutex_t mFileMutex;
    pthread_cond_t mFileCondition;
    bool mFileBusy;
    FILE* mFd;
    int mFileLen;
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
        printf("server here is %s\n", argv[1]);
        sm = sm_ipc.get();
    } else if (!strcmp(argv[1], "rpmsg")) {
        sm = sm_rpc.get();
    } else {
        printf("usage: %s [socket type net/local/rpmsg]\n", argv[0]);
        return -1;
    }

    sp<ICpcFsqJniServer> testSockServer = new ICpcFsqJniServer;

    // add service
    sm->addService(String16("socketQ.service"), testSockServer);
    ALOGI("add socketQ.service to service manager");

    // start worker thread
    ProcessState::self()->startThreadPool();

    // join the main thread
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
