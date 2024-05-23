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

#include <android-base/logging.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/RpcServer.h>

#include <CpcServiceManager.h>

using android::CpcServiceManager;
using android::defaultServiceManager;
using android::IPCThreadState;
using android::IServiceManager;
using android::ProcessState;
using android::RpcServer;
using android::sp;
using android::status_t;
using android::String16;

extern "C" int main(int argc, char** argv)
{
    status_t ret;

    LOG(INFO) << "Start cpcservicemanager";

    sp<CpcServiceManager> manager = sp<CpcServiceManager>::make();
    {
        sp<IServiceManager> sm(defaultServiceManager());
        ret = sm->addService(String16("cpcmanager"), manager);
        LOG(INFO) << "add cpcservicemanager to servicemanager, ret: " << ret;
    }

    {
        ret = ProcessState::self()->registerRemoteService("cpcmanager", manager);
        LOG(INFO) << "add cpcservicemanager to rpmsg socket, ret: " << ret;
    }

    IPCThreadState::self()->joinThreadPool();

    // should not be reached
    return EXIT_SUCCESS;
}
