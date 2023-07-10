/*
 * Copyright (C) 2023 Xiaomi Corperation
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

#define LOG_TAG "IServiceManager"

#include <inttypes.h>
#include <nuttx/tls.h>
#include <unistd.h>

#include <android/binder_status.h>

#include "BnServiceManager.h"
#include "BpServiceManager.h"
#include "IPCThreadState.h"
#include "IServiceCallback.h"
#include "IServiceManager.h"
#include "Parcel.h"
#include "ProcessGlobal.h"
#include "Status.h"
#include "utils/BinderString.h"
#include "utils/Binderlog.h"
#include "utils/HashMap.h"
#include "utils/Timers.h"
#include "utils/Vector.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct RegistrationWaiter;
typedef struct RegistrationWaiter RegistrationWaiter;

struct RegistrationWaiter {
    BnServiceCallback m_BnServiceCallback;
    void (*dtor)(RegistrationWaiter* this);

    int32_t (*onRegistration)(RegistrationWaiter* this, const String* name,
        const IBinder* binder);

    LocalRegistrationCallback* mImpl;
};

struct LocalRegistrationAndWaiter {
    LocalRegistrationCallback* mLocalRegistrationCallback;
    RegistrationWaiter* mRegistrationWaiter;
};

typedef struct LocalRegistrationAndWaiter LocalRegistrationAndWaiter;

struct ServiceManagerShim;
typedef struct ServiceManagerShim ServiceManagerShim;

struct ServiceManagerShim {
    IServiceManager m_IServiceManager;

    void (*dtor)(ServiceManagerShim* this);

    /* Public function */

    IBinder* (*getService)(ServiceManagerShim* this, String* name);
    IBinder* (*checkService)(ServiceManagerShim* this, String* name);
    int32_t (*addService)(ServiceManagerShim* this, String* name, IBinder* service,
        bool allowIsolated, int dumpsysFlags);
    int32_t (*listServices)(ServiceManagerShim* this, int dumpsysFlags, VectorString* list);
    IBinder* (*waitForService)(ServiceManagerShim* this, String* name);
    bool (*isDeclared)(ServiceManagerShim* this, String* name);
    int32_t (*getDeclaredInstances)(ServiceManagerShim* this, String* interface, VectorString* strvector);
    int32_t (*registerForNotifications)(ServiceManagerShim* this, String* name,
        const LocalRegistrationCallback* callback);
    int32_t (*unregisterForNotifications)(ServiceManagerShim* this, String* name,
        const LocalRegistrationCallback* callback);
    int32_t (*getServiceDebugInfo)(ServiceManagerShim* this, VectorImpl* infvector);

    /* Member function */
    int32_t (*realGetService)(ServiceManagerShim* this, String* name, IBinder** aidl_return);
    void (*removeRegistrationCallbackLocked)(ServiceManagerShim* this, const LocalRegistrationCallback* cb,
        VectorImpl* it, RegistrationWaiter* waiter);
    String* (*getInterfaceDescriptor)(ServiceManagerShim* this);
    IBinder* (*onAsBinder)(ServiceManagerShim* this);

    /* Member value */
    IAIDLServiceManager* mTheRealServiceManager;
    HashMap mNameToRegistrationCallback;
    pthread_mutex_t mNameToRegistrationLock;
};

/****************************************************************************
 * Function define for IServiceManager
 ****************************************************************************/

static String* IServiceManager_getInterfaceDescriptor(IServiceManager* this)
{
    IAIDLServiceManager_global* global = IAIDLServiceManager_global_get();
    return &global->descriptor;
}

static void IServiceManager_dtor(IServiceManager* this)
{
    this->m_iface.dtor(&this->m_iface);
}

static void IServiceManager_ctor(IServiceManager* this)
{
    IInterface_ctor(&this->m_iface);

    this->getInterfaceDescriptor = IServiceManager_getInterfaceDescriptor;

    this->dtor = IServiceManager_dtor;
}

/****************************************************************************
 * Function define for RegistrationWaiter
 ****************************************************************************/

static int32_t RegistrationWaiter_onRegistration(RegistrationWaiter* this, const String* name,
    const IBinder* binder)
{
    this->mImpl->onServiceRegistration(this->mImpl, name, binder);
    return STATUS_OK;
}

static void RegistrationWaiter_dtor(RegistrationWaiter* this)
{
    this->m_BnServiceCallback.dtor(&this->m_BnServiceCallback);
}

void RegistrationWaiter_ctor(RegistrationWaiter* this, LocalRegistrationCallback* callback)
{
    BnServiceCallback_ctor(&this->m_BnServiceCallback);

    this->mImpl = callback;

    this->onRegistration = RegistrationWaiter_onRegistration;
    this->dtor = RegistrationWaiter_dtor;
}

/****************************************************************************
 * Function define for ServiceManagerShim
 ****************************************************************************/

static String* IServiceManager_Vfun_getInterfaceDescriptor(IServiceManager* v_this)
{
    ServiceManagerShim* this = (ServiceManagerShim*)v_this;
    return this->getInterfaceDescriptor(this);
}

static IBinder* IServiceManager_Vfun_getService(IServiceManager* v_this, String* name)
{
    ServiceManagerShim* this = (ServiceManagerShim*)v_this;
    return this->getService(this, name);
}

static IBinder* IServiceManager_Vfun_checkService(IServiceManager* v_this, String* name)
{
    ServiceManagerShim* this = (ServiceManagerShim*)v_this;
    return this->checkService(this, name);
}

static int32_t IServiceManager_Vfun_addService(IServiceManager* v_this, String* name, IBinder* service,
    bool allowIsolated, int dumpPriority)
{
    ServiceManagerShim* this = (ServiceManagerShim*)v_this;
    return this->addService(this, name, service, allowIsolated, dumpPriority);
}

static int32_t IServiceManager_Vfun_listServices(IServiceManager* v_this, int dumpsysPriority, VectorString* list)
{
    ServiceManagerShim* this = (ServiceManagerShim*)v_this;
    return this->listServices(this, dumpsysPriority, list);
}

static String* ServiceManagerShim_getInterfaceDescriptor(ServiceManagerShim* this)
{
    return this->mTheRealServiceManager->getInterfaceDescriptor(this->mTheRealServiceManager);
}

static int32_t ServiceManagerShim_realGetService(ServiceManagerShim* this, String* name, IBinder** aidl_return)
{
    Status status;
    this->mTheRealServiceManager->getService(this->mTheRealServiceManager, name, aidl_return, &status);
    return status.mErrorCode;
}

static IBinder* ServiceManagerShim_getService(ServiceManagerShim* this, String* name)
{
    ProcessState* self = ProcessState_self();

    IBinder* svc = this->checkService(this, name);
    UNUSED(self);

    if (svc != NULL) {
        return svc;
    }

    // retry interval in millisecond; note that vendor services stay at 100ms
    const useconds_t sleepTime = 100;
    const int64_t timeout = 5000;
    int64_t startTime = uptimeMillis();

    BINDER_LOGV("Waiting for service '%s' on '%s'...", String_data(name), self->getDriverName(self));

    int n = 0;
    while (uptimeMillis() - startTime < timeout) {
        n++;
        usleep(1000 * sleepTime);
        svc = this->checkService(this, name);
        if (svc != NULL) {
            BINDER_LOGV("Waiting for service '%s' on '%s' successful after waiting %" PRIi64 "ms",
                String_data(name), self->getDriverName(self), uptimeMillis() - startTime);
            return svc;
        }
    }
    BINDER_LOGV("Service %s didn't start. Returning NULL", String_data(name));
    return NULL;
}

static IBinder* ServiceManagerShim_checkService(ServiceManagerShim* this, String* name)
{
    IBinder* ret = NULL;
    Status status;

    this->mTheRealServiceManager->checkService(this->mTheRealServiceManager,
        name, &ret, &status);
    if (status.mErrorCode != STATUS_OK) {
        return NULL;
    }
    return ret;
}

static int32_t ServiceManagerShim_addService(ServiceManagerShim* this, String* name, IBinder* service,
    bool allowIsolated, int dumpPriority)
{
    Status status;

    this->mTheRealServiceManager->addService(this->mTheRealServiceManager,
        name, service, allowIsolated,
        dumpPriority, &status);
    return status.mException;
}

static int32_t ServiceManagerShim_listServices(ServiceManagerShim* this, int dumpsysPriority, VectorString* list)
{
    Status status;

    this->mTheRealServiceManager->listServices(this->mTheRealServiceManager,
        dumpsysPriority, list, &status);
    return status.mErrorCode;
}

static void ServiceManagerShim_dtor(ServiceManagerShim* this)
{
    this->m_IServiceManager.dtor(&this->m_IServiceManager);
    this->mNameToRegistrationCallback.dtor(&this->mNameToRegistrationCallback);
}

static void ServiceManagerShim_ctor(ServiceManagerShim* this, IAIDLServiceManager* impl)
{
    IServiceManager* isvcmgr = &this->m_IServiceManager;

    IServiceManager_ctor(&this->m_IServiceManager);
    HashMap_String_ctor(&this->mNameToRegistrationCallback);
    pthread_mutex_init(&this->mNameToRegistrationLock, NULL);

    this->mTheRealServiceManager = impl;

    /* Override Virtual function for IServiceManager */

    isvcmgr->getInterfaceDescriptor = IServiceManager_Vfun_getInterfaceDescriptor;
    isvcmgr->getService = IServiceManager_Vfun_getService;
    isvcmgr->checkService = IServiceManager_Vfun_checkService;
    isvcmgr->addService = IServiceManager_Vfun_addService;
    isvcmgr->listServices = IServiceManager_Vfun_listServices;

    this->getInterfaceDescriptor = ServiceManagerShim_getInterfaceDescriptor;
    this->getService = ServiceManagerShim_getService;
    this->checkService = ServiceManagerShim_checkService;
    this->addService = ServiceManagerShim_addService;
    this->listServices = ServiceManagerShim_listServices;
    this->realGetService = ServiceManagerShim_realGetService;

    this->dtor = ServiceManagerShim_dtor;
}

static ServiceManagerShim* ServiceManagerShim_new(IAIDLServiceManager* impl)
{
    ServiceManagerShim* this;
    this = zalloc(sizeof(ServiceManagerShim));

    ServiceManagerShim_ctor(this, impl);
    return this;
}

void ServiceManagerShim_delete(ServiceManagerShim* this)
{
    this->dtor(this);
    free(this);
}

IServiceManager* defaultServiceManager()
{
    ServiceManager_global* global = ServiceManager_global_get();
    ProcessState* self;
    if (!global->gSmOnce) {
        IAIDLServiceManager* sm = NULL;
        while (sm == NULL) {
            self = ProcessState_self();
            sm = IAIDLServiceManager_asInterface(self->getContextObject(self, NULL));
            if (sm == NULL) {
                BINDER_LOGE("Waiting 1s on context object on %s.", self->getDriverName(self));
                sleep(1);
            }
        }
        global->gDefaultServiceManager = (IServiceManager*)ServiceManagerShim_new(sm);
        global->gSmOnce = true;
    }
    return global->gDefaultServiceManager;
}

void setDefaultServiceManager(IServiceManager* sm)
{
    ServiceManager_global* global = ServiceManager_global_get();
    bool called = false;
    if (!global->gSmOnce) {
        global->gDefaultServiceManager = sm;
        called = true;
        global->gSmOnce = true;
    }

    if (!called) {
        LOG_FATAL("setDefaultServiceManager() called after defaultServiceManager().\n");
    }
}
