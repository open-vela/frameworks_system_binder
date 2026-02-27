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
#include <murmurhash.h>
#include <netpacket/rpmsg.h>
#include <sys/socket.h>
#ifdef AF_VSOCK
#ifdef __linux__
#include <linux/vm_sockets.h>
#else
#include <sys/vm_sockets.h>
#endif
#endif

namespace android {

class CpcServiceManagerShim : public IServiceManager {
public:
    CpcServiceManagerShim(const sp<os::IServiceManager>& impl,
        const char* cpuname, uv_loop_t* loop = nullptr);

    sp<IBinder> getService(const String16& name) const override;
    status_t addService(const String16& name, const sp<IBinder>& service,
        bool allowIsolated, int dumpsysPriority) override;
#ifdef __ANDROID__
    Vector<String16> listServices(int dumpsysPriority) override;
#else
    std::vector<String16> listServices(int dumpsysPriority) override;
#endif

    sp<IBinder> checkService(const String16& name) const override;
    sp<IBinder> waitForService(const String16& name) override;
    bool isDeclared(const String16& name) override;
#ifdef __ANDROID__
    Vector<String16> getDeclaredInstances(const String16& interface) override;
#else
    std::vector<String16> getDeclaredInstances(const String16& interface) override;
#endif
    std::optional<String16> updatableViaApex(const String16& name) override;
#if CONFIG_ANDROID_BINDER_VERSION == 14 || CONFIG_ANDROID_BINDER_VERSION == 15
#ifdef __ANDROID__
    Vector<String16> getUpdatableNames(const String16& apexName) override;
#else
    std::vector<String16> getUpdatableNames(const String16& apexName) override;
#endif
#endif
    std::optional<IServiceManager::ConnectionInfo> getConnectionInfo(const String16& name) override;
    status_t registerForNotifications(const String16& service,
        const sp<LocalRegistrationCallback>& cb) override;
    status_t unregisterForNotifications(const String16& service,
        const sp<LocalRegistrationCallback>& cb) override;
    std::vector<IServiceManager::ServiceDebugInfo> getServiceDebugInfo() override;
#if CONFIG_ANDROID_BINDER_VERSION == 15
    void enableAddServiceCache(bool value) override
    {
    }
#endif

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
    uv_loop_t* mLoop;
};

CpcServiceManagerShim::CpcServiceManagerShim(const sp<os::IServiceManager>& impl,
    const char* cpuname, uv_loop_t* loop)
    : mTheRealServiceManager(impl)
    , mLocalCpuName(cpuname)
    , mLoop(loop)
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

#ifdef CONFIG_CPC_MAX_INCOMING_THREADS
    session->setMaxIncomingThreads(CONFIG_CPC_MAX_INCOMING_THREADS);
#else
    session->setMaxIncomingThreads(1);
#endif

#ifdef AF_VSOCK
    unsigned int remote_cid = 0;
    strncpy((char*)&remote_cid, cpuname.c_str(), std::min(sizeof(remote_cid), cpuname.length()));
    if (status_t status = session->setupVsockClient(remote_cid, murmurhash(servname.c_str()));
        status == OK) {
        return session->getRootObject();
    }
#endif
#ifdef AF_RPMSG
    if (status_t status = session->setupRpmsgSockClient(cpuname.c_str(), servname.c_str());
        status == OK) {
        return session->getRootObject();
    }
#endif
    return nullptr;
}

static bool isValidServiceName(const std::string& name)
{
    if (name.size() == 0)
        return false;
    if (name.size() > 127)
        return false;

    for (char c : name) {
        if (c == '_' || c == '-' || c == '.' || c == '/')
            continue;
        if (c >= 'a' && c <= 'z')
            continue;
        if (c >= 'A' && c <= 'Z')
            continue;
        if (c >= '0' && c <= '9')
            continue;
        return false;
    }

    return true;
}

status_t CpcServiceManagerShim::addService(const String16& name, const sp<IBinder>& binder,
    bool allowIsolated, int dumpsysPriority)
{
    std::string servname = String8(name).c_str();

    if (!isValidServiceName(servname)) {
        ALOGE("addService: Invalid service name: %s", servname.c_str());
        return BAD_VALUE;
    }

#ifdef AF_VSOCK
#ifdef __ANDROID__
    if (status_t status = ProcessState::self()->registerRemoteService(murmurhash(servname.c_str()), binder);
#else
    if (status_t status = ProcessState::self()->registerRemoteService(murmurhash(servname.c_str()), binder, mLoop);
#endif
        status != android::OK) {
        ALOGI("failed to reigister %s for vsock status=%" PRId32, servname.c_str(), status);
    }
#endif
#ifdef AF_RPMSG
#ifdef __ANDROID__
    if (status_t status = ProcessState::self()->registerRemoteService(servname.c_str(), binder);
#else
    if (status_t status = ProcessState::self()->registerRemoteService(servname.c_str(), binder, mLoop);
#endif
        status != android::OK) {
        ALOGI("failed to register %s for rpmsg status=%" PRId32, servname.c_str(), status);
    }
#endif

    defaultServiceManager()->addService(name, binder);

    binder::Status status = mTheRealServiceManager->addService(
        mLocalCpuName + "/" + String8(name).c_str(), binder,
        allowIsolated, dumpsysPriority);
    return status.exceptionCode();
}

#ifdef __ANDROID__
Vector<String16> CpcServiceManagerShim::listServices(int dumpsysPriority)
#else
std::vector<String16> CpcServiceManagerShim::listServices(int dumpsysPriority)
#endif
{
    std::vector<std::string> ret;
    if (!mTheRealServiceManager->listServices(dumpsysPriority, &ret).isOk()) {
        return {};
    }

#ifdef __ANDROID__
    Vector<String16> res;
    res.setCapacity(ret.size());
    for (const std::string& name : ret)
        res.push(String16(name.c_str()));
#else
    std::vector<String16> res;
    res.reserve(ret.size());
    for (const std::string& name : ret)
        res.push_back(String16(name.c_str()));
#endif
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

#ifdef __ANDROID__
Vector<String16> CpcServiceManagerShim::getDeclaredInstances(const String16& interface)
#else
std::vector<String16> CpcServiceManagerShim::getDeclaredInstances(const String16& interface)
#endif
{
    (void)interface;
    return {};
}

std::optional<String16> CpcServiceManagerShim::updatableViaApex(const String16& name)
{
    (void)name;
    return std::nullopt;
}

#if CONFIG_ANDROID_BINDER_VERSION == 14 || CONFIG_ANDROID_BINDER_VERSION == 15
#ifdef __ANDROID__
Vector<String16> CpcServiceManagerShim::getUpdatableNames(const String16& apexName)
#else
std::vector<String16> CpcServiceManagerShim::getUpdatableNames(const String16& apexName)
#endif
{
    (void)apexName;
    return {};
}
#endif

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

    if (!isValidServiceName(nameStr)) {
        ALOGE("registerForNotifications: Invalid service name: %s", nameStr.c_str());
        return BAD_VALUE;
    }

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

sp<IServiceManager> defaultCpcServiceManager(uv_loop_t* loop)
{
    sp<IServiceManager> sm(defaultServiceManager());
    sp<IBinder> binder = sm->checkService(String16("cpcmanager"));

    if (binder != nullptr) {
#ifdef __ANDROID__
        return sp<CpcServiceManagerShim>::make(interface_cast<os::IServiceManager>(binder), CONFIG_CPC_SERVICEMANAGER_CPUNAME);
#else
        return sp<CpcServiceManagerShim>::make(interface_cast<os::IServiceManager>(binder), CONFIG_CPC_SERVICEMANAGER_CPUNAME, loop);
#endif
    }

    auto session = RpcSession::make();
    session->setMaxIncomingThreads(1);
#ifdef AF_VSOCK
    unsigned int remote_cid = 0;
    memcpy(&remote_cid, CONFIG_CPC_SERVICEMANAGER_CPUNAME, std::min(strlen(CONFIG_CPC_SERVICEMANAGER_CPUNAME), sizeof(remote_cid)));
    if (session->setupVsockClient(remote_cid, murmurhash("cpcmanager")) == OK) {
        struct sockaddr_vm addr = {
            .svm_family = AF_VSOCK,
            .svm_port = murmurhash("cpcmanager"),
            .svm_cid = remote_cid
        };

        binder = session->getRootObject();
        if (binder == nullptr) {
            ALOGE("Failed to getRootObject!\n");
            return nullptr;
        }

        struct sockaddr* sa = reinterpret_cast<struct sockaddr*>(&addr);
        socklen_t len = sizeof(addr);
        int fd = socket(AF_VSOCK, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        connect(fd, sa, len);
        getsockname(fd, sa, &len);
        close(fd);
        char cpuname[16] = { 0 };
        memcpy(cpuname, &addr.svm_cid, sizeof(addr.svm_cid));
        return sp<CpcServiceManagerShim>::make(interface_cast<os::IServiceManager>(binder), cpuname, loop);
    }
#endif

#ifdef AF_RPMSG
    if (session->setupRpmsgSockClient(CONFIG_CPC_SERVICEMANAGER_CPUNAME, "cpcmanager") == OK) {
        struct sockaddr_rpmsg addr = {
            .rp_family = AF_RPMSG,
            .rp_cpu = CONFIG_CPC_SERVICEMANAGER_CPUNAME,
            .rp_name = "cpcmanger",
        };

        binder = session->getRootObject();
        if (binder == nullptr) {
            ALOGE("Failed to getRootObject!\n");
            return nullptr;
        }

        struct sockaddr* sa = reinterpret_cast<struct sockaddr*>(&addr);
        socklen_t len = sizeof(addr);
        int fd = socket(AF_RPMSG, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        connect(fd, sa, len);
        getsockname(fd, sa, &len);
        close(fd);
        return sp<CpcServiceManagerShim>::make(interface_cast<os::IServiceManager>(binder), addr.rp_cpu, loop);
    }
#endif

    return nullptr;
}

} // namespace android
