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

#ifndef __BINDER_INCLUDE_BINDER_PROCESSGLOBAL_H__
#define __BINDER_INCLUDE_BINDER_PROCESSGLOBAL_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <android/binder_status.h>
#include <pthread.h>
#include <stdatomic.h>

#include "AidlServiceManager.h"
#include "IBinder.h"
#include "IClientCallback.h"
#include "IPCThreadState.h"
#include "IServiceManager.h"
#include "ProcessState.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* For Linux, static values will allocated for every process
 * For FLAT_BUILD NuttX, every task will hold BpBinder_global for itself
 */

/* Global data for ServiceManager */

struct ServiceManager_global;
typedef struct ServiceManager_global ServiceManager_global;

struct ServiceManager_global {
    void (*dtor)(ServiceManager_global* this);

    bool gSmOnce;
    IServiceManager* gDefaultServiceManager;
};

/* Global data for IAIDLServiceManager */

struct IAIDLServiceManager_global;
typedef struct IAIDLServiceManager_global IAIDLServiceManager_global;

struct IAIDLServiceManager_global {
    void (*dtor)(IAIDLServiceManager_global* this);

    String descriptor;
    IAIDLServiceManager* default_impl;
};

/* Global data for BpBinder */

struct BpBinder_global;
typedef struct BpBinder_global BpBinder_global;

struct BpBinder_global {
    void (*dtor)(BpBinder_global* this);

    pthread_mutex_t sTrackingLock;
    int sNumTrackedUids;
    atomic_bool sCountByUidEnabled;
    uint32_t sBinderProxyCountHighWatermark;
    uint32_t sBinderProxyCountLowWatermark;
    bool sBinderProxyThrottleCreate;
    HashMap sTrackingMap;
    HashMap sLastLimitCallbackMap;
    binder_proxy_limit_callback sLimitCallback;
};

struct ProcessState_global;
typedef struct ProcessState_global ProcessState_global;

/* Global data for ProcessState */

struct ProcessState_global {
    void (*dtor)(ProcessState_global* this);

    ProcessState* gProcessState;
    pthread_mutex_t gProcessMutex;
    bool gProcessInit;
};

struct Parcel_global;
typedef struct Parcel_global Parcel_global;

/* Global data for Parcel */

struct Parcel_global {
    void (*dtor)(Parcel_global* this);
    size_t gParcelGlobalAllocCount;
    size_t gParcelGlobalAllocSize;
};

/* NuttX Process Binderlib Global Data */

struct ProcessGlobal;
typedef struct ProcessGlobal ProcessGlobal;

struct ProcessGlobal {
    void (*dtor)(ProcessGlobal* this);

    ProcessState_global gProcessState_global;
    Parcel_global gParcel_global;
    BpBinder_global gBpBinder_global;
    ServiceManager_global gServiceManager_global;
    IAIDLServiceManager_global gIAIDLServiceManager_global;

    IClientCallback* IClientCallback_impl;
    IServiceCallback* IServiceCallback_impl;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: ProcessGlobal_get
 *
 * Description:
 *
 ****************************************************************************/
ProcessGlobal* ProcessGlobal_get(void);

static inline ProcessState_global* ProcessState_global_get(void)
{
    return &(ProcessGlobal_get()->gProcessState_global);
}

static inline Parcel_global* Parcel_global_get(void)
{
    return &(ProcessGlobal_get()->gParcel_global);
}

static inline BpBinder_global* BpBinder_global_get(void)
{
    return &(ProcessGlobal_get()->gBpBinder_global);
}

static inline ServiceManager_global* ServiceManager_global_get(void)
{
    return &(ProcessGlobal_get()->gServiceManager_global);
}

static inline IAIDLServiceManager_global* IAIDLServiceManager_global_get(void)
{
    return &(ProcessGlobal_get()->gIAIDLServiceManager_global);
}

#endif /* __BINDER_INCLUDE_BINDER_PROCESSGLOBAL_H__ */
