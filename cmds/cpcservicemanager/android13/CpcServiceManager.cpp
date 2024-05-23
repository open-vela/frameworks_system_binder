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

#include "CpcServiceManager.h"

using android::binder::Status;

namespace android {

Status CpcServiceManager::getService(const std::string& name, sp<IBinder>* outBinder)
{
    if (auto it = mNameToService.find(name); it != mNameToService.end()) {
        *outBinder = it->second.binder;
    } else {
        *outBinder = nullptr;
    }

    LOG(INFO) << "getService name: " << name << ", return " << (*outBinder).get();
    return Status::ok();
}

Status CpcServiceManager::checkService(const std::string& name, sp<IBinder>* outBinder)
{
    return getService(name, outBinder);
}

Status CpcServiceManager::addService(const std::string& name, const sp<IBinder>& binder,
    bool allowIsolated, int32_t dumpPriority)
{
    LOG(INFO) << "addService name: " << name;

    if (binder->linkToDeath(sp<CpcServiceManager>::fromExisting(this)) != OK) {
        LOG(ERROR) << "Could not linkToDeath when adding " << name;
    }

    auto sep = name.find("/");
    std::string cpuname = name.substr(0, sep);
    std::string servname = name.substr(sep + 1);

    if (auto it = mNameToService.find(servname); it != mNameToService.end()) {
        it->second.binder->unlinkToDeath(sp<CpcServiceManager>::fromExisting(this));
    }

    mNameToService[servname] = Service {
        .binder = binder,
        .cpuname = cpuname,
    };

    if (auto it = mNameToCallback.find(servname); it != mNameToCallback.end()) {
        for (const sp<IServiceCallback>& cb : it->second) {
            cb->onRegistration(servname, IInterface::asBinder(this));
        }
    }

    return Status::ok();
}

Status CpcServiceManager::listServices(int32_t dumpPriority, std::vector<std::string>* outList)
{
    outList->reserve(mNameToService.size());
    for (const auto& [servname, service] : mNameToService) {
        outList->push_back(service.cpuname + "/" + servname);
    }

    return Status::ok();
}

Status CpcServiceManager::registerForNotifications(const std::string& name,
    const sp<IServiceCallback>& callback)
{
    if (callback == nullptr) {
        return Status::fromExceptionCode(Status::EX_NULL_POINTER);
    }

    mNameToCallback[name].push_back(callback);

    if (auto it = mNameToService.find(name); it != mNameToService.end()) {
        callback->onRegistration(name, IInterface::asBinder(this));
    }

    return Status::ok();
}

Status CpcServiceManager::unregisterForNotifications(const std::string& name,
    const sp<IServiceCallback>& callback)
{
    if (auto it = mNameToCallback.find(name); it != mNameToCallback.end()) {
        std::vector<sp<IServiceCallback>>& listeners = it->second;

        for (auto lit = listeners.begin(); lit != listeners.end();) {
            if (*lit == callback) {
                lit = listeners.erase(lit);
                return Status::ok();
            } else {
                lit++;
            }
        }
    }

    return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE);
}

void CpcServiceManager::binderDied(const wp<IBinder>& who)
{
    LOG(DEBUG) << "binderDied: " << who.get_refs();
    for (auto it = mNameToService.begin(); it != mNameToService.end();) {
        if (who == it->second.binder) {
            it = mNameToService.erase(it);
        } else {
            it++;
        }
    }
}

Status CpcServiceManager::isDeclared(const std::string& name, bool* outReturn)
{
    return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
}

Status CpcServiceManager::getDeclaredInstances(const std::string& interface, std::vector<std::string>* outReturn)
{
    return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
}

Status CpcServiceManager::updatableViaApex(const std::string& name,
    std::optional<std::string>* outReturn)
{
    return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
}

Status CpcServiceManager::getConnectionInfo(const std::string& name,
    std::optional<ConnectionInfo>* outReturn)
{
    return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
}

Status CpcServiceManager::registerClientCallback(const std::string& name, const sp<IBinder>& service,
    const sp<IClientCallback>& cb)
{
    return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
}

Status CpcServiceManager::tryUnregisterService(const std::string& name, const sp<IBinder>& binder)
{
    return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
}

Status CpcServiceManager::getServiceDebugInfo(std::vector<ServiceDebugInfo>* outReturn)
{
    return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
}

} // namespace android
