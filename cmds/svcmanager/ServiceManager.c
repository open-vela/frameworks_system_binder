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

#define LOG_TAG "ServiceManager"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/android/binder.h>
#include <nuttx/tls.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/IPCThreadState.h"
#include "base/IServiceManager.h"
#include "base/ProcessState.h"
#include "base/Status.h"
#include "utils/Binderlog.h"
#include <android/binder_status.h>

#include "ServiceManager.h"

static ssize_t BinderService_getNodeStrongRefCount(BinderService* this)
{
    IBinder* binder = this->binder;
    BpBinder* bpBinder;
    ProcessState* self;

    binder->incStrongRequireStrong(binder, (const void*)binder);
    bpBinder = binder->remoteBinder(binder);

    if (bpBinder == NULL) {
        return -1;
    }
    self = ProcessState_self();
    return self->getStrongRefCountForNode(self, bpBinder);
}

static void BinderService_dtor(BinderService* this)
{
}

void BinderService_ctor(BinderService* this)
{
    this->hasClients = false;
    this->guaranteeClient = false;
    this->debugPid = 0;

    this->getNodeStrongRefCount = BinderService_getNodeStrongRefCount;
    this->dtor = BinderService_dtor;
}

BinderService* BinderService_new()
{
    BinderService* this;
    this = zalloc(sizeof(BinderService));

    BinderService_ctor(this);
    return this;
}

static bool isValidServiceName(const char* name)
{
    int i;
    char c;

    if (strlen(name) == 0)
        return false;
    if (strlen(name) > 127)
        return false;

    for (i = 0; i < strlen(name); i++) {
        c = name[i];
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

static IBinder* ServiceManager_tryGetService(ServiceManager* this, String* name, bool startIfNotFound)
{
    uint32_t status;
    IBinder* out = NULL;
    BinderService* service = NULL;

    status = this->mNameToService.find(&this->mNameToService, (long)name, (long*)&service);
    if (status != STATUS_OK) {
        return NULL;
    }

    if (service != NULL) {
        out = service->binder;
    }

    if (!out && startIfNotFound) {
        this->tryStartService(this, name);
    }
    if (out) {
        service->guaranteeClient = true;
    }
    return out;
}

static int32_t ServiceManager_getService(ServiceManager* this, String* name, IBinder** aidl_return)
{
    *aidl_return = this->tryGetService(this, name, true);
    return STATUS_OK;
}

static int32_t ServiceManager_checkService(ServiceManager* this, String* name, IBinder** aidl_return)
{
    *aidl_return = this->tryGetService(this, name, false);
    return STATUS_OK;
}

static int32_t ServiceManager_addService(ServiceManager* this, String* name, IBinder* binder,
    bool allowIsolated, int32_t dumpPriority)
{
    IPCThreadState* ipc = IPCThreadState_self();
    pid_t callingPid = ipc->getCallingPid(ipc);

    if (binder == NULL) {
        return STATUS_BAD_VALUE;
    }

    if (!isValidServiceName(String_data(name))) {
        BINDER_LOGE("Invalid service name: %s\n", String_data(name));
        return STATUS_BAD_VALUE;
    }

    /* implicitly unlinked when the binder is removed */

    if (binder->remoteBinder(binder) != NULL && binder->linkToDeath((void*)this, (void*)(&this->m_DeathRecipient), NULL, 0) != OK) {
        BINDER_LOGE("Could not linkToDeath when adding %s\n", String_data(name));
        return STATUS_BAD_TYPE;
    }

    /* Overwrite the old service if it exists */

    BinderService* Service = BinderService_new();

    Service->binder = binder,
    Service->allowIsolated = allowIsolated,
    Service->dumpPriority = dumpPriority,
    Service->debugPid = callingPid,
    this->mNameToService.put(&this->mNameToService, (long)name, (long)Service);

    IServiceCallback* callback = NULL;

    this->mNameToRegistrationCallback.find(&this->mNameToRegistrationCallback,
        (long)name, (long*)&callback);
    if (callback != NULL) {
        callback->onRegistration(callback, name, binder);
    }

    return STATUS_OK;
}

static int32_t ServiceManager_listServices(ServiceManager* this, int32_t dumpPriority,
    VectorString* aidl_return)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static int32_t ServiceManager_registerForNotifications(ServiceManager* this, String* name,
    const IServiceCallback* callback)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static int32_t ServiceManager_unregisterForNotifications(ServiceManager* this, String* name,
    const IServiceCallback* callback)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static int32_t ServiceManager_isDeclared(ServiceManager* this, String* name, bool* _aidl_return)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static int32_t ServiceManager_getDeclaredInstances(ServiceManager* this, String* iface,
    VectorString* aidl_return)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static int32_t ServiceManager_registerClientCallback(ServiceManager* this, String* name,
    const IBinder* service,
    const IClientCallback* callback)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static int32_t ServiceManager_tryUnregisterService(ServiceManager* this, String* name,
    const IBinder* service)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static int32_t ServiceManager_getServiceDebugInfo(ServiceManager* this, VectorString* aidl_return)
{
    BINDER_LOGE("Not support still\n");
    return STATUS_OK;
}

static void ServiceManager_removeRegistrationCallback(ServiceManager* this,
    const IBinder* who, IServiceCallback* it, bool* found)
{
    BINDER_LOGE("Not support still\n");
}

static void ServiceManager_tryStartService(ServiceManager* this, String* name)
{
}

static void ServiceManager_removeClientCallback(ServiceManager* this,
    const IBinder* who, IClientCallback* it)
{
    BINDER_LOGE("Not support still\n");
}

static void ServiceManager_handleClientCallbacks(const ServiceManager* this)
{
    BINDER_LOGE("Not support still\n");
}

static ssize_t ServiceManager_handleServiceClientCallback(const ServiceManager* this,
    String* serviceName, bool isCalledOnInterval)
{
    BINDER_LOGE("Not support still\n");
    return 0;
}

static void ServiceManager_sendClientCallbackNotifications(ServiceManager* this,
    String* serviceName, bool hasClients)
{
    BINDER_LOGE("Not support still\n");
}

static void ServiceManager_binderDied(ServiceManager* this, const IBinder* who)
{
    BINDER_LOGE("Not support still\n");
}

static void ServiceManager_Vfun_getService(BnAIDLServiceManager* v_this, String* name, IBinder** aidl_return,
    Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->getService(this, name, aidl_return);
}

static void ServiceManager_Vfun_checkService(BnAIDLServiceManager* v_this, String* name, IBinder** aidl_return,
    Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->checkService(this, name, aidl_return);
}

static void ServiceManager_Vfun_addService(BnAIDLServiceManager* v_this, String* name, IBinder* service,
    bool allowIsolated, int32_t dumpPriority,
    Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->addService(this, name, service, allowIsolated, dumpPriority);
}

static void ServiceManager_Vfun_listServices(BnAIDLServiceManager* v_this, int32_t dumpPriority,
    VectorString* aidl_return, Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->listServices(this, dumpPriority, aidl_return);
}

static void ServiceManager_Vfun_registerForNotifications(BnAIDLServiceManager* v_this, String* name,
    const IServiceCallback* callback, Status* retStatus)

{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->registerForNotifications(this, name, callback);
}

static void ServiceManager_Vfun_unregisterForNotifications(BnAIDLServiceManager* v_this, String* name,
    const IServiceCallback* callback,
    Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->unregisterForNotifications(this, name, callback);
}

static void ServiceManager_Vfun_isDeclared(BnAIDLServiceManager* v_this, String* name,
    bool* _aidl_return, Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->isDeclared(this, name, _aidl_return);
}

static void ServiceManager_Vfun_getDeclaredInstances(BnAIDLServiceManager* v_this, String* iface,
    VectorString* aidl_return, Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->getDeclaredInstances(this, iface, aidl_return);
}

static void ServiceManager_Vfun_registerClientCallback(BnAIDLServiceManager* v_this, String* name,
    const IBinder* service,
    const IClientCallback* callback,
    Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->registerClientCallback(this, name, service, callback);
}

static void ServiceManager_Vfun_tryUnregisterService(BnAIDLServiceManager* v_this, String* name,
    const IBinder* service, Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->tryUnregisterService(this, name, service);
}

static void ServiceManager_Vfun_getServiceDebugInfo(BnAIDLServiceManager* v_this, VectorString* aidl_return,
    Status* retStatus)
{
    ServiceManager* this = (ServiceManager*)v_this;
    retStatus->mErrorCode = this->getServiceDebugInfo(this, aidl_return);
}

static void ServiceManager_dtor(ServiceManager* this)
{
    this->m_DeathRecipient.dtor(&this->m_DeathRecipient);
    this->mNameToService.dtor(&this->mNameToService);
    this->mNameToRegistrationCallback.dtor(&this->mNameToRegistrationCallback);
    this->mNameToClientCallback.dtor(&this->mNameToClientCallback);
}

static void ServiceManager_ctor(ServiceManager* this)
{
    BnAIDLServiceManager* aidl;

    BnAIDLServiceManager_ctor(&this->m_BnServiceManager);
    DeathRecipient_ctor(&this->m_DeathRecipient);
    HashMap_String_ctor(&this->mNameToService);
    HashMap_String_ctor(&this->mNameToRegistrationCallback);
    HashMap_String_ctor(&this->mNameToClientCallback);

    aidl = &this->m_BnServiceManager;

    /* Virtual function override at BnServiceManager::AidlServiceManager */
    aidl->getService = ServiceManager_Vfun_getService;
    aidl->checkService = ServiceManager_Vfun_checkService;
    aidl->addService = ServiceManager_Vfun_addService;
    aidl->listServices = ServiceManager_Vfun_listServices;
    aidl->registerForNotifications = ServiceManager_Vfun_registerForNotifications;
    aidl->unregisterForNotifications = ServiceManager_Vfun_unregisterForNotifications;
    aidl->isDeclared = ServiceManager_Vfun_isDeclared;
    aidl->getDeclaredInstances = ServiceManager_Vfun_getDeclaredInstances;
    aidl->registerClientCallback = ServiceManager_Vfun_registerClientCallback;
    aidl->tryUnregisterService = ServiceManager_Vfun_tryUnregisterService;
    aidl->getServiceDebugInfo = ServiceManager_Vfun_getServiceDebugInfo;

    this->getService = ServiceManager_getService;
    this->checkService = ServiceManager_checkService;
    this->addService = ServiceManager_addService;
    this->listServices = ServiceManager_listServices;
    this->registerForNotifications = ServiceManager_registerForNotifications;
    this->unregisterForNotifications = ServiceManager_unregisterForNotifications;
    this->isDeclared = ServiceManager_isDeclared;
    this->getDeclaredInstances = ServiceManager_getDeclaredInstances;
    this->registerClientCallback = ServiceManager_registerClientCallback;
    this->tryUnregisterService = ServiceManager_tryUnregisterService;
    this->getServiceDebugInfo = ServiceManager_getServiceDebugInfo;

    /* Virtual function override at IBinder::DeathRecipient */
    this->binderDied = ServiceManager_binderDied;

    /* Virtual function */
    this->tryStartService = ServiceManager_tryStartService;

    /* Member function */
    this->tryGetService = ServiceManager_tryGetService;
    this->sendClientCallbackNotifications = ServiceManager_sendClientCallbackNotifications;
    this->handleServiceClientCallback = ServiceManager_handleServiceClientCallback;
    this->handleClientCallbacks = ServiceManager_handleClientCallbacks;
    this->removeClientCallback = ServiceManager_removeClientCallback;
    this->removeRegistrationCallback = ServiceManager_removeRegistrationCallback;

    this->dtor = ServiceManager_dtor;
}

ServiceManager* ServiceManager_new()
{
    ServiceManager* this;
    this = zalloc(sizeof(ServiceManager));

    ServiceManager_ctor(this);
    return this;
}
