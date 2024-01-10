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

#pragma once

#include <android/os/BnServiceManager.h>
#include <android/os/IClientCallback.h>
#include <android/os/IServiceCallback.h>

#include <map>

namespace android {

using os::ConnectionInfo;
using os::IClientCallback;
using os::IServiceCallback;
using os::ServiceDebugInfo;

class CpcServiceManager : public os::BnServiceManager, public IBinder::DeathRecipient {
public:
    binder::Status getService(const std::string& name, sp<IBinder>* outBinder) override;
    binder::Status checkService(const std::string& name, sp<IBinder>* outBinder) override;
    binder::Status addService(const std::string& name, const sp<IBinder>& binder,
        bool allowIsolated, int32_t dumpPriority) override;
    binder::Status listServices(int32_t dumpPriority, std::vector<std::string>* outList) override;
    binder::Status registerForNotifications(const std::string& name,
        const sp<IServiceCallback>& callback) override;
    binder::Status unregisterForNotifications(const std::string& name,
        const sp<IServiceCallback>& callback) override;

    binder::Status isDeclared(const std::string& name, bool* outReturn) override;
    binder::Status getDeclaredInstances(const std::string& interface, std::vector<std::string>* outReturn) override;
    binder::Status updatableViaApex(const std::string& name,
        std::optional<std::string>* outReturn) override;
    binder::Status getConnectionInfo(const std::string& name,
        std::optional<ConnectionInfo>* outReturn) override;
    binder::Status registerClientCallback(const std::string& name, const sp<IBinder>& service,
        const sp<IClientCallback>& cb) override;
    binder::Status tryUnregisterService(const std::string& name, const sp<IBinder>& binder) override;
    binder::Status getServiceDebugInfo(std::vector<ServiceDebugInfo>* outReturn) override;

    void binderDied(const wp<IBinder>& who) override;

private:
    struct Service {
        sp<IBinder> binder;
        std::string cpuname;
    };
    using ServiceMap = std::map<std::string, Service>;
    using ServiceCallbackMap = std::map<std::string, std::vector<sp<IServiceCallback>>>;

    ServiceMap mNameToService;
    ServiceCallbackMap mNameToCallback;
};

} // namespace android
