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

#define LOG_TAG "ProcessGlobal"

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <android/binder_status.h>
#include <nuttx/android/binder.h>
#include <nuttx/tls.h>

#include "AidlServiceManager.h"
#include "IPCThreadState.h"
#include "IServiceManager.h"
#include "ProcessGlobal.h"
#include "ProcessState.h"
#include "utils/Binderlog.h"
#include "utils/Thread.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void ServiceManager_global_dtor(ServiceManager_global* this)
{
}

static void ServiceManager_global_ctor(ServiceManager_global* this)
{
    this->gSmOnce = false;
    this->gDefaultServiceManager = NULL;

    this->dtor = ServiceManager_global_dtor;
}

static void IAIDLServiceManager_global_dtor(IAIDLServiceManager_global* this)
{
}

static void IAIDLServiceManager_global_ctor(IAIDLServiceManager_global* this)
{
    String_init(&this->descriptor, 0);
    this->default_impl = NULL;

    this->dtor = IAIDLServiceManager_global_dtor;
}

static void BpBinder_global_dtor(BpBinder_global* this)
{
    this->sTrackingMap.dtor(&this->sTrackingMap);
    this->sLastLimitCallbackMap.dtor(&this->sLastLimitCallbackMap);
    pthread_mutex_destroy(&this->sTrackingLock);
}

static void BpBinder_global_ctor(BpBinder_global* this)
{
    HashMap_ctor(&this->sTrackingMap);
    HashMap_ctor(&this->sLastLimitCallbackMap);
    pthread_mutex_init(&this->sTrackingLock, NULL);

    this->sNumTrackedUids = 0;
    this->sCountByUidEnabled = false;
    this->sLimitCallback = NULL;
    this->sBinderProxyThrottleCreate = false;
    this->sBinderProxyCountHighWatermark = 2500;
    this->sBinderProxyCountLowWatermark = 2000;

    this->dtor = BpBinder_global_dtor;
}

static void Parcel_global_dtor(Parcel_global* this)
{
    this->gParcelGlobalAllocCount = 0;
    this->gParcelGlobalAllocSize = 0;
}

static void Parcel_global_ctor(Parcel_global* this)
{
    this->dtor = Parcel_global_dtor;
}

static void ProcessState_global_dtor(ProcessState_global* this)
{
    if (this->gProcessState) {
        ProcessState_delete(this->gProcessState);
    }

    this->gProcessState = NULL;
    this->gProcessInit = false;
    pthread_mutex_destroy(&this->gProcessMutex);
}

static void ProcessState_global_ctor(ProcessState_global* this)
{
    this->gProcessState = NULL;
    this->gProcessInit = false;
    pthread_mutex_init(&this->gProcessMutex, NULL);

    this->dtor = ProcessState_global_dtor;
}

static void ProcessGlobal_dtor(ProcessGlobal* this)
{
    this->gProcessState_global.dtor(&this->gProcessState_global);
    this->gParcel_global.dtor(&this->gParcel_global);
    this->gBpBinder_global.dtor(&this->gBpBinder_global);
    this->gServiceManager_global.dtor(&this->gServiceManager_global);
    this->gIAIDLServiceManager_global.dtor(&this->gIAIDLServiceManager_global);
}

static void ProcessGlobal_ctor(ProcessGlobal* this)
{
    ProcessState_global_ctor(&this->gProcessState_global);
    Parcel_global_ctor(&this->gParcel_global);
    BpBinder_global_ctor(&this->gBpBinder_global);
    ServiceManager_global_ctor(&this->gServiceManager_global);
    IAIDLServiceManager_global_ctor(&this->gIAIDLServiceManager_global);

    this->dtor = ProcessGlobal_dtor;
}

static ProcessGlobal* ProcessGlobal_new(void)
{
    ProcessGlobal* this;

    this = zalloc(sizeof(ProcessGlobal));

    ProcessGlobal_ctor(this);
    return this;
}

static void ProcessGlobal_free(void* global)
{
    if (global) {
        ProcessGlobal* this = (ProcessGlobal*)global;
        this->dtor(this);
        free(this);
    }
}

ProcessGlobal* ProcessGlobal_get(void)
{
    static int index = -1;
    ProcessGlobal* global = NULL;

    if (index < 0) {
        index = task_tls_alloc(ProcessGlobal_free);
    }

    if (index >= 0) {
        global = (struct ProcessGlobal*)task_tls_get_value(index);
        if (global == NULL) {
            global = ProcessGlobal_new();
            if (global) {
                task_tls_set_value(index, (uintptr_t)global);
            }
        }
    }

    ASSERT(global != NULL);
    return global;
}
