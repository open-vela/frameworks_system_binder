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

#include <cinttypes>

#include <binder/ICpcServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

using namespace android;
using android::binder::Status;

namespace android {
class ISendVecServer : public BnBinderLatency {
public:
    Status sendVec(const std::vector<uint8_t>& data, std::vector<uint8_t>* _aidl_return)
    {
        *_aidl_return = data;
        return Status::ok();
    }

    Status sendVecOneWay(const std::vector<uint8_t>& data)
    {
        return Status::ok();
    }
};
}

extern "C" int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("[usage]: latency_sendvec_server [ipc/rpc]\n");
        return 0;
    }

    printf("SendVec Server Start!\n");

    sp<ISendVecServer> sendVecServer = new ISendVecServer;
    sp<IServiceManager> sm_ipc(defaultServiceManager());
    sp<IServiceManager> sm_rpc(defaultCpcServiceManager());
    IServiceManager* sm;

    if (!strcmp(argv[1], "rpc")) {
        sm = sm_rpc.get();
    } else {
        sm = sm_ipc.get();
    }

    auto remoteBinder = sm->getService(String16("sendvec"));
    if (remoteBinder != NULL) {
        printf("Service sendvec has been already added!\n");
        return 0;
    }

    sm->addService(String16("sendvec"), sendVecServer);

    IPCThreadState::self()->joinThreadPool();

    return 0;
}
