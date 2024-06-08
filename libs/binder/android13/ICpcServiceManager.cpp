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

#define LOG_TAG "CpcServiceManager"

#include <android-base/stringprintf.h>
#include <android/os/BnServiceCallback.h>
#include <android/os/IServiceManager.h>
#include <binder/ICpcServiceManager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/RpcSession.h>
#include <utils/Log.h>

#include <inttypes.h>
#include <sys/socket.h>
#ifdef AF_RPMSG
#include <netpacket/rpmsg.h>
#endif

namespace android {

class CpcServiceManagerShim : public IServiceManager {
public:
    CpcServiceManagerShim(const sp<os::IServiceManager>& impl, const char* cpuname);

    sp<IBinder> getService(const String16& name) const override;
    status_t addService(const String16& name, const sp<IBinder>& service,
        bool allowIsolated, int dumpsysPriority) override;
    Vector<String16> listServices(int dumpsysPriority) override;

    sp<IBinder> checkService(const String16& name) const override;
    sp<IBinder> waitForService(const String16& name) override;
    bool isDeclared(const String16& name) override;
    Vector<String16> getDeclaredInstances(const String16& interface) override;
    std::optional<String16> updatableViaApex(const String16& name) override;
    std::optional<IServiceManager::ConnectionInfo> getConnectionInfo(const String16& name) override;
    status_t registerForNotifications(const String16& service,
        const sp<LocalRegistrationCallback>& cb) override;
    status_t unregisterForNotifications(const String16& service,
        const sp<LocalRegistrationCallback>& cb) override;
    std::vector<IServiceManager::ServiceDebugInfo> getServiceDebugInfo() override;

    IBinder* onAsBinder() override
    {
        return IInterface::asBinder(mTheRealServiceManager).get();
    }

    class RegistrationWaiter : public os::BnServiceCallback {
    public:
        explicit RegistrationWaiter(const sp<LocalRegistrationCallback>& callback)
            : mImpl(callback)
        {
        }

        binder::Status onRegistration(const std::string& name, const sp<IBinder>& binder) override
        {
            mImpl->onServiceRegistration(String16(name.c_str()), binder);
            return binder::Status::ok();
        }

    private:
        sp<LocalRegistrationCallback> mImpl;
    };

private:
    using LocalRegistrationAndWaiter = std::pair<sp<LocalRegistrationCallback>, sp<RegistrationWaiter>>;
    using ServiceCallbackMap = std::map<std::string, std::vector<LocalRegistrationAndWaiter>>;

    sp<os::IServiceManager> mTheRealServiceManager;
    std::string mLocalCpuName;
    ServiceCallbackMap mNameToCallback;
};

CpcServiceManagerShim::CpcServiceManagerShim(const sp<os::IServiceManager>& impl, const char* cpuname)
    : mTheRealServiceManager(impl)
    , mLocalCpuName(cpuname)
{
}

sp<IBinder> CpcServiceManagerShim::getService(const String16& name) const
{
    std::string cpuname;
    std::string servname = String8(name).c_str();
    auto sep = servname.find('/');

    if (sep != std::string::npos) {
        cpuname = servname.substr(0, sep);
        servname = servname.substr(sep + 1);
    } else {
        std::vector<std::string> list;
        if (!mTheRealServiceManager->listServices(DUMP_FLAG_PRIORITY_ALL, &list).isOk())
            return nullptr;

        bool found = false;
        for (const auto& i : list) {
            sep = i.find('/');
            if (servname != i.substr(sep + 1))
                continue;
            cpuname = i.substr(0, sep);
            found = true;
            break;
        }

        if (!found)
            return nullptr;
    }

    if (cpuname == mLocalCpuName) {
        sp<IBinder> binder = defaultServiceManager()->getService(name);
        return binder;
    }

    auto session = RpcSession::make();
    session->setMaxIncomingThreads(1);
    auto status = session->setupRpmsgSockClient(cpuname.c_str(), servname.c_str());
    if (status != OK)
        return nullptr;

    return session->getRootObject();
}

status_t CpcServiceManagerShim::addService(const String16& name, const sp<IBinder>& binder,
    bool allowIsolated, int dumpsysPriority)
{
    std::string servname = String8(name).c_str();
    if (status_t ret = ProcessState::self()->registerRemoteService(servname.c_str(), binder);
        ret != android::OK) {
        return ret;
    }

    defaultServiceManager()->addService(name, binder);

    binder::Status status = mTheRealServiceManager->addService(
        mLocalCpuName + "/" + String8(name).c_str(), binder,
        allowIsolated, dumpsysPriority);
    return status.exceptionCode();
}

Vector<String16> CpcServiceManagerShim::listServices(int dumpsysPriority)
{
    std::vector<std::string> ret;
    if (!mTheRealServiceManager->listServices(dumpsysPriority, &ret).isOk()) {
        return {};
    }

    Vector<String16> res;
    res.setCapacity(ret.size());
    for (const std::string& name : ret)
        res.push(String16(name.c_str()));
    return res;
}

sp<IBinder> CpcServiceManagerShim::checkService(const String16& name) const
{
    return getService(name);
}

sp<IBinder> CpcServiceManagerShim::waitForService(const String16& name16)
{
    class Waiter : public LocalRegistrationCallback {
        void onServiceRegistration(const String16& instance, const sp<IBinder>& binder)
        {
            ALOGD("Waiter::onServiceRegistration: %s %p", String8(instance).c_str(), binder.get());
            std::unique_lock lock(mMutex);
            mReady = true;
            mCv.notify_one();
        }

    public:
        bool mReady = false;
        std::mutex mMutex;
        std::condition_variable mCv;
    };

    sp<Waiter> waiter = sp<Waiter>::make();
    if (status_t status = registerForNotifications(name16, waiter);
        status != android::OK) {
        ALOGE("Failed to registerForNotifications: %" PRId32, status);
        return nullptr;
    } else {
        std::unique_lock lock(waiter->mMutex);
        waiter->mCv.wait(lock, [&] { return waiter->mReady; });
    }

    if (status_t status = unregisterForNotifications(name16, waiter);
        status != android::OK) {
        ALOGE("Failed to unregisterForNotifications: %" PRId32, status);
    }
    return getService(name16);
}

bool CpcServiceManagerShim::isDeclared(const String16& name)
{
    (void)name;
    return false;
}

Vector<String16> CpcServiceManagerShim::getDeclaredInstances(const String16& interface)
{
    (void)interface;
    return {};
}

std::optional<String16> CpcServiceManagerShim::updatableViaApex(const String16& name)
{
    (void)name;
    return std::nullopt;
}

std::optional<IServiceManager::ConnectionInfo> CpcServiceManagerShim::getConnectionInfo(
    const String16& name)
{
    (void)name;
    return std::nullopt;
}

status_t CpcServiceManagerShim::registerForNotifications(const String16& name,
    const sp<LocalRegistrationCallback>& cb)
{
    std::string nameStr = String8(name).c_str();
    sp<RegistrationWaiter> registrationWaiter = sp<RegistrationWaiter>::make(cb);

    if (binder::Status status = mTheRealServiceManager->registerForNotifications(nameStr, registrationWaiter); !status.isOk()) {
        return status.exceptionCode();
    }

    mNameToCallback[nameStr].push_back(std::make_pair(cb, registrationWaiter));
    return OK;
}

status_t CpcServiceManagerShim::unregisterForNotifications(const String16& name,
    const sp<LocalRegistrationCallback>& cb)
{
    std::string nameStr = String8(name).c_str();
    if (auto it = mNameToCallback.find(nameStr); it != mNameToCallback.end()) {
        std::vector<LocalRegistrationAndWaiter>& localRegistrationAndWaiters = it->second;
        sp<RegistrationWaiter> registrationWaiter;

        for (auto lit = localRegistrationAndWaiters.begin(); lit != localRegistrationAndWaiters.end();) {
            if (lit->first == cb) {
                registrationWaiter = lit->second;
                lit = localRegistrationAndWaiters.erase(lit);
                mTheRealServiceManager->unregisterForNotifications(nameStr, registrationWaiter);
            } else {
                lit++;
            }
        }

        if (localRegistrationAndWaiters.empty()) {
            mNameToCallback.erase(it);
        }
        return OK;
    } else {
        return BAD_VALUE;
    }
}

std::vector<IServiceManager::ServiceDebugInfo> CpcServiceManagerShim::getServiceDebugInfo()
{
    return {};
}

sp<IServiceManager> defaultCpcServiceManager()
{
#ifdef AF_RPMSG
    sp<IServiceManager> sm(defaultServiceManager());
    sp<IBinder> binder = sm->checkService(String16("cpcmanager"));

    if (binder == nullptr) {
        auto session = RpcSession::make();
        session->setMaxIncomingThreads(1);
        auto status = session->setupRpmsgSockClient(CONFIG_CPC_SERVICEMANAGER_CPUNAME, "cpcmanager");
        if (status != OK)
            return nullptr;
        binder = session->getRootObject();
    }

    // read cpuname from kernel
    int fd = socket(AF_RPMSG, SOCK_STREAM | SOCK_CLOEXEC, 0);
    sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &addrLen);
    const char* cpuname = ((sockaddr_rpmsg*)(&addr))->rp_cpu;
    close(fd);

    return sp<CpcServiceManagerShim>::make(interface_cast<os::IServiceManager>(binder), cpuname);
#else
    return nullptr;
#endif
}

} // namespace android
