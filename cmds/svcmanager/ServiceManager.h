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

#ifndef __BINDER_CMDS_SVCMANAGER_SERVICEMANAGER_H__
#define __BINDER_CMDS_SVCMANAGER_SERVICEMANAGER_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "base/BnServiceManager.h"
#include "base/IClientCallback.h"
#include "base/IServiceCallback.h"
#include "base/Status.h"
#include "utils/HashMap.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct BinderService;
typedef struct BinderService BinderService;

struct BinderService {
    IBinder* binder;
    bool allowIsolated;
    int32_t dumpPriority;
    bool hasClients;
    bool guaranteeClient;
    pid_t debugPid;

    void (*dtor)(BinderService* this);
    ssize_t (*getNodeStrongRefCount)(BinderService* this);
};

BinderService* BinderService_new(void);

struct ServiceManager;
typedef struct ServiceManager ServiceManager;

struct ServiceManager {
    BnAIDLServiceManager m_BnServiceManager;
    DeathRecipient m_DeathRecipient;

    void (*dtor)(ServiceManager* this);

    /* Virtual function override at BnServiceManager::AidlServiceManager */
    int32_t (*getService)(ServiceManager* this, String* name, IBinder** aidl_return);
    int32_t (*checkService)(ServiceManager* this, String* name, IBinder** aidl_return);
    int32_t (*addService)(ServiceManager* this, String* name, IBinder* service,
        bool allowIsolated, int32_t dumpPriority);
    int32_t (*listServices)(ServiceManager* this, int32_t dumpPriority,
        VectorString* aidl_return);
    int32_t (*registerForNotifications)(ServiceManager* this, String* name, const IServiceCallback* callback);
    int32_t (*unregisterForNotifications)(ServiceManager* this, String* name, const IServiceCallback* callback);
    int32_t (*isDeclared)(ServiceManager* this, String* name, bool* _aidl_return);
    int32_t (*getDeclaredInstances)(ServiceManager* this, String* iface, VectorString* aidl_return);
    int32_t (*registerClientCallback)(ServiceManager* this, String* name, const IBinder* service,
        const IClientCallback* callback);
    int32_t (*tryUnregisterService)(ServiceManager* this, String* name, const IBinder* service);
    int32_t (*getServiceDebugInfo)(ServiceManager* this, VectorString* aidl_return);

    /* Virtual function override at IBinder::DeathRecipient */
    void (*binderDied)(ServiceManager* this, const IBinder* who);

    /* Virtual function */
    void (*tryStartService)(ServiceManager* this, String* name);

    /* Member function */
    void (*handleClientCallbacks)(const ServiceManager* this);
    void (*removeRegistrationCallback)(ServiceManager* this, const IBinder* who,
        IServiceCallback* it, bool* found);
    ssize_t (*handleServiceClientCallback)(const ServiceManager* this, String* serviceName, bool isCalledOnInterval);
    void (*sendClientCallbackNotifications)(ServiceManager* this, String* serviceName, bool hasClients);
    void (*removeClientCallback)(ServiceManager* this, const IBinder* who, IClientCallback* it);
    IBinder* (*tryGetService)(ServiceManager* this, String* name, bool startIfNotFound);

    HashMap mNameToService;
    HashMap mNameToRegistrationCallback;
    HashMap mNameToClientCallback;
};

ServiceManager* ServiceManager_new(void);

#endif /* __BINDER_CMDS_SVCMANAGER_SERVICEMANAGER_H__ */