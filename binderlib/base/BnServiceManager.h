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

#ifndef __BINDER_INCLUDE_BINDER_BNSERVICEMANAGER_H__
#define __BINDER_INCLUDE_BINDER_BNSERVICEMANAGER_H__

#include "AidlServiceManager.h"
#include "IBinder.h"
#include "IInterface.h"
#include "Parcel.h"

enum {
    BnServiceManager_TRANSACTION_getService = FIRST_CALL_TRANSACTION + 0,
    BnServiceManager_TRANSACTION_checkService = FIRST_CALL_TRANSACTION + 1,
    BnServiceManager_TRANSACTION_addService = FIRST_CALL_TRANSACTION + 2,
    BnServiceManager_TRANSACTION_listServices = FIRST_CALL_TRANSACTION + 3,
    BnServiceManager_TRANSACTION_registerForNotifications = FIRST_CALL_TRANSACTION + 4,
    BnServiceManager_TRANSACTION_unregisterForNotifications = FIRST_CALL_TRANSACTION + 5,
    BnServiceManager_TRANSACTION_isDeclared = FIRST_CALL_TRANSACTION + 6,
    BnServiceManager_TRANSACTION_getDeclaredInstances = FIRST_CALL_TRANSACTION + 7,
    BnServiceManager_TRANSACTION_updatableViaApex = FIRST_CALL_TRANSACTION + 8,
    BnServiceManager_TRANSACTION_getConnectionInfo = FIRST_CALL_TRANSACTION + 9,
    BnServiceManager_TRANSACTION_registerClientCallback = FIRST_CALL_TRANSACTION + 10,
    BnServiceManager_TRANSACTION_tryUnregisterService = FIRST_CALL_TRANSACTION + 11,
    BnServiceManager_TRANSACTION_getServiceDebugInfo = FIRST_CALL_TRANSACTION + 12,
};

struct BnInterface_IAIDLServiceManager;
typedef struct BnInterface_IAIDLServiceManager BnInterface_IAIDLServiceManager;

struct BnInterface_IAIDLServiceManager {
    BBinder m_BBinder;
    IAIDLServiceManager m_IAIDLServiceManager;

    void (*dtor)(BnInterface_IAIDLServiceManager* this);

    IInterface* (*queryLocalInterface)(BnInterface_IAIDLServiceManager* this,
        const String* _descriptor);
    String* (*getInterfaceDescriptor)(BnInterface_IAIDLServiceManager* this);
    IBinder* (*onAsBinder)(BnInterface_IAIDLServiceManager* this);
};

void BnInterface_IAIDLServiceManager_ctor(BnInterface_IAIDLServiceManager* this);

struct BnAIDLServiceManager;
typedef struct BnAIDLServiceManager BnAIDLServiceManager;

struct BnAIDLServiceManager {
    BnInterface_IAIDLServiceManager m_BnIAidlServiceManager;

    void (*dtor)(BnAIDLServiceManager* this);

    /* Virtual Function for IAIDLServiceManager */

    void (*getService)(BnAIDLServiceManager* this, String* name, IBinder** aidl_return, Status* retStatus);
    void (*checkService)(BnAIDLServiceManager* this, String* name, IBinder** aidl_return, Status* retStatus);
    void (*addService)(BnAIDLServiceManager* this, String* name, IBinder* service,
        bool allowIsolated, int32_t dumpPriority, Status* retStatus);
    void (*listServices)(BnAIDLServiceManager* this, int32_t dumpPriority, VectorString* aidl_return, Status* retStatus);
    void (*registerForNotifications)(BnAIDLServiceManager* this, String* name, const IServiceCallback* callback, Status* retStatus);
    void (*unregisterForNotifications)(BnAIDLServiceManager* this, String* name, const IServiceCallback* callback, Status* retStatus);
    void (*isDeclared)(BnAIDLServiceManager* this, String* name, bool* _aidl_return, Status* retStatus);
    void (*getDeclaredInstances)(BnAIDLServiceManager* this, String* iface, VectorString* aidl_return, Status* retStatus);
    void (*registerClientCallback)(BnAIDLServiceManager* this, String* name, const IBinder* service, const IClientCallback* callback, Status* retStatus);
    void (*tryUnregisterService)(BnAIDLServiceManager* this, String* name, const IBinder* service, Status* retStatus);
    void (*getServiceDebugInfo)(BnAIDLServiceManager* this, VectorString* aidl_return, Status* retStatus);

    /* Virtual function */
    uint32_t (*onTransact)(BnAIDLServiceManager* this, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags);
};

void BnAIDLServiceManager_ctor(BnAIDLServiceManager* this);

#endif /* __BINDER_INCLUDE_BINDER_BNSERVICEMANAGER_H__ */
