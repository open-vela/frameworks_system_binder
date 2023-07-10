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

#ifndef __BINDER_INCLUDE_BINDER_AIDLSERVERMANAGER_H__
#define __BINDER_INCLUDE_BINDER_AIDLSERVERMANAGER_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "Binder.h"
#include "BpBinder.h"
#include "IBinder.h"
#include "IClientCallback.h"
#include "IInterface.h"
#include "IServiceCallback.h"
#include "Status.h"
#include "utils/BinderString.h"
#include "utils/RefBase.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum {
    DUMP_FLAG_PRIORITY_CRITICAL = 1 << 0,
    DUMP_FLAG_PRIORITY_HIGH = 1 << 1,
    DUMP_FLAG_PRIORITY_NORMAL = 1 << 2,
    DUMP_FLAG_PRIORITY_DEFAULT = 1 << 3,
    DUMP_FLAG_PROTO = 1 << 4,
    DUMP_FLAG_PRIORITY_ALL = DUMP_FLAG_PRIORITY_CRITICAL | DUMP_FLAG_PRIORITY_HIGH | DUMP_FLAG_PRIORITY_NORMAL | DUMP_FLAG_PRIORITY_DEFAULT,
};

/* IAIDLServiceManager = android::os::IServiceManager
 * android::os::IServiceManager != IServiceManager
 * android::os::IServiceManager is generate with aidl,
 * real implement for ServiceManager pair
 *  (Base class for BnServiceManager/BpServiceManager)
 */

struct IAIDLServiceManager;
typedef struct IAIDLServiceManager IAIDLServiceManager;

struct IAIDLServiceManager {
    IInterface m_iface;
    void (*dtor)(IAIDLServiceManager* this);

    /* Virtual Function */

    String* (*getInterfaceDescriptor)(IAIDLServiceManager* this);

    /* Pure Virtual Function */

    void (*getService)(IAIDLServiceManager* this, String* name, IBinder** aidl_return, Status* retStatus);
    void (*checkService)(IAIDLServiceManager* this, String* name, IBinder** aidl_return, Status* retStatus);
    void (*addService)(IAIDLServiceManager* this, String* name, IBinder* service,
        bool allowIsolated, int32_t dumpPriority, Status* retStatus);
    void (*listServices)(IAIDLServiceManager* this, int32_t dumpPriority,
        VectorString* aidl_return, Status* retStatus);
    void (*registerForNotifications)(IAIDLServiceManager* this, String* name,
        const IServiceCallback* callback, Status* retStatus);
    void (*unregisterForNotifications)(IAIDLServiceManager* this, String* name,
        const IServiceCallback* callback, Status* retStatus);
    void (*isDeclared)(IAIDLServiceManager* this, String* name, bool* _aidl_return, Status* retStatus);
    void (*getDeclaredInstances)(IAIDLServiceManager* this, String* iface, VectorString* aidl_return,
        Status* retStatus);
    void (*registerClientCallback)(IAIDLServiceManager* this, String* name, const IBinder* service,
        const IClientCallback* callback, Status* retStatus);
    void (*tryUnregisterService)(IAIDLServiceManager* this, String* name, IBinder* service,
        Status* retStatus);
    void (*getServiceDebugInfo)(IAIDLServiceManager* this, VectorString* aidl_return,
        Status* retStatus);
};

void IAIDLServiceManager_ctor(IAIDLServiceManager* this);

String* IAIDLServiceManager_getInterfaceDescriptor(IAIDLServiceManager* this);
IAIDLServiceManager* IAIDLServiceManager_asInterface(IBinder* obj);
bool IAIDLServiceManager_setDefaultImpl(IAIDLServiceManager* impl);
IAIDLServiceManager* IAIDLServiceManager_getDefaultImpl(void);

#endif /* __BINDER_INCLUDE_BINDER_AIDLSERVERMANAGER_H__ */
