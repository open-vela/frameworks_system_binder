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

#ifndef __BINDER_INCLUDE_BINDER_BPSERVICEMANAGER_H__
#define __BINDER_INCLUDE_BINDER_BPSERVICEMANAGER_H__

#include "AidlServiceManager.h"
#include "IBinder.h"
#include "IInterface.h"

struct BpInterface_IAIDLServiceManager;
typedef struct BpInterface_IAIDLServiceManager BpInterface_IAIDLServiceManager;

struct BpInterface_IAIDLServiceManager {
    IAIDLServiceManager m_IAIDLServiceManager;
    BpRefBase m_BpRefBase;

    void (*dtor)(BpInterface_IAIDLServiceManager* this);

    IBinder* (*onAsBinder)(BpInterface_IAIDLServiceManager* this);
};

void BpInterface_IAIDLServiceManager_ctor(BpInterface_IAIDLServiceManager* this,
    IBinder* remote);

struct BpAIDLServiceManager;
typedef struct BpAIDLServiceManager BpAIDLServiceManager;

struct BpAIDLServiceManager {
    BpInterface_IAIDLServiceManager m_BpIAidlServiceManager;

    void (*dtor)(BpAIDLServiceManager* this);

    /* Virtual Function for IAIDLServiceManager */
    String* (*getInterfaceDescriptor)(BpAIDLServiceManager* this);

    /* Virtual Function for BpRefBase */
    IBinder* (*remoteStrong)(BpAIDLServiceManager* this);
    IBinder* (*remote)(BpAIDLServiceManager* this);

    /* Virtual function override at BpAIDLServiceManager::AidlServiceManager */
    void (*getService)(BpAIDLServiceManager* this, String* name, IBinder** aidl_return, Status* retStatus);
    void (*checkService)(BpAIDLServiceManager* this, String* name, IBinder** aidl_return, Status* retStatus);
    void (*addService)(BpAIDLServiceManager* this, String* name, IBinder* service,
        bool allowIsolated, int32_t dumpPriority, Status* retStatus);
    void (*listServices)(BpAIDLServiceManager* this, int32_t dumpPriority, VectorString* aidl_return, Status* retStatus);
    void (*registerForNotifications)(BpAIDLServiceManager* this, String* name, const IServiceCallback* callback, Status* retStatus);
    void (*unregisterForNotifications)(BpAIDLServiceManager* this, String* name, const IServiceCallback* callback, Status* retStatus);
    void (*isDeclared)(BpAIDLServiceManager* this, String* name, bool* _aidl_return, Status* retStatus);
    void (*getDeclaredInstances)(BpAIDLServiceManager* this, String* iface, VectorString* aidl_return, Status* retStatus);
    void (*registerClientCallback)(BpAIDLServiceManager* this, String* name, const IBinder* service,
        const IClientCallback* callback, Status* retStatus);
    void (*tryUnregisterService)(BpAIDLServiceManager* this, String* name, IBinder* service, Status* retStatus);
    void (*getServiceDebugInfo)(BpAIDLServiceManager* this, VectorString* aidl_return, Status* retStatus);
};

BpAIDLServiceManager* BpAIDLServiceManager_new(IBinder* impl);
void BpAIDLServiceManager_delete(BpAIDLServiceManager* this);

#endif /* __BINDER_INCLUDE_BINDER_BPSERVICEMANAGER_H__ */
