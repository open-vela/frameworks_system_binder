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

#ifndef __BINDER_INCLUDE_BINDER_ISERVERMANAGER_H__
#define __BINDER_INCLUDE_BINDER_ISERVERMANAGER_H__

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

struct IServiceManager;
typedef struct IServiceManager IServiceManager;

struct LocalRegistrationCallback;
typedef struct LocalRegistrationCallback LocalRegistrationCallback;

struct LocalRegistrationCallback {
    RefBase m_refbase;
    void (*dtor)(LocalRegistrationCallback* this);

    void (*onServiceRegistration)(LocalRegistrationCallback* this, const String* instance,
        const IBinder* binder);
};

struct ServiceDebugInfo;
typedef struct ServiceDebugInfo ServiceDebugInfo;

struct ServiceDebugInfo {
    String name;
    int pid;
};

/**
 * Service manager for C++ services.
 */
struct IServiceManager {
    IInterface m_iface;
    void (*dtor)(IServiceManager* this);

    /* Virtual Function */
    String* (*getInterfaceDescriptor)(IServiceManager* this);

    /* Pure Virtual Function */

    IBinder* (*getService)(IServiceManager* this, String* name);
    IBinder* (*checkService)(IServiceManager* this, String* name);
    int32_t (*addService)(IServiceManager* this, String* name, IBinder* service,
        bool allowIsolated, int dumpsysFlags);
    int32_t (*listServices)(IServiceManager* this, int dumpsysFlags, VectorString* list);
    IBinder* (*waitForService)(IServiceManager* this, String* name);
    bool (*isDeclared)(IServiceManager* this, String* name);
    int32_t (*getDeclaredInstances)(IServiceManager* this, String* interface, VectorString* strvector);
    int32_t (*registerForNotifications)(IServiceManager* this, String* name,
        LocalRegistrationCallback* callback);
    int32_t (*unregisterForNotifications)(IServiceManager* this, String* name,
        LocalRegistrationCallback* callback);
    int32_t (*getServiceDebugInfo)(IServiceManager* this, VectorImpl* infvector);
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

IServiceManager* defaultServiceManager(void);
void setDefaultServiceManager(IServiceManager* sm);
void destoryServiceManager(void);

#endif /* __BINDER_INCLUDE_BINDER_ISERVERMANAGER_H__ */
