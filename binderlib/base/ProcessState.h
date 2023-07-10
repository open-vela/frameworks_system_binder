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

#ifndef __BINDER_INCLUDE_BINDER_PROCESSSTATE_H__
#define __BINDER_INCLUDE_BINDER_PROCESSSTATE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <pthread.h>
#include <stdatomic.h>

#include "IBinder.h"

enum CallRestriction {
    /* all calls okay */
    CALL_RESTRICTION_NONE,
    /* log when calls are blocking */
    CALL_ERROR_IF_NOT_ONEWAY,
    /* abort process on blocking calls */
    CALL_FATAL_IF_NOT_ONEWAY,
};

/****************************************************************************
 * Public Types
 ****************************************************************************/
struct handle_entry {
    IBinder* binder;
    RefBase_weakref* refs;
};

typedef struct handle_entry handle_entry;

struct ProcessState;
typedef struct ProcessState ProcessState;

struct ProcessState {
    RefBase m_refbase;

    void (*dtor)(ProcessState* this);

    /* Virtual Function for RefBase */
    void (*incStrong)(ProcessState* this, const void* id);
    void (*incStrongRequireStrong)(ProcessState* this, const void* id);
    void (*decStrong)(ProcessState* this, const void* id);
    void (*forceIncStrong)(ProcessState* this, const void* id);

    /* public function */
    IBinder* (*getContextObject)(ProcessState* this, const IBinder* caller);
    void (*startThreadPool)(ProcessState* this);
    bool (*becomeContextManager)(ProcessState* this);
    IBinder* (*getStrongProxyForHandle)(ProcessState* this, int32_t handle);
    void (*expungeHandle)(ProcessState* this, int32_t handle, IBinder* binder);
    void (*spawnPooledThread)(ProcessState* this, bool isMain);
    int32_t (*setThreadPoolMaxThreadCount)(ProcessState* this, int maxThreads);
    const char* (*getDriverName)(ProcessState* this);
    ssize_t (*getStrongRefCountForNode)(ProcessState* this, BpBinder* binder);
    void (*setCallRestriction)(ProcessState* this, int restriction);
    void (*makeBinderThreadName)(ProcessState* this, char* name, int namelen);
    handle_entry* (*lookupHandleLocked)(ProcessState* this, int32_t handle);

    /* data for process context */

    BBinder* mContextObject;

    /* private data */

    const char* mDriverName;
    int mDriverFD;
    void* mVMStart;
    pthread_mutex_t mThreadCountLock;
    pthread_cond_t mThreadCountDecrement;
    size_t mExecutingThreadsCount;
    size_t mWaitingForThreads;
    size_t mMaxThreads;
    size_t mCurrentThreads;
    size_t mKernelStartedThreads;
    int64_t mStarvationStartTimeMs;

    /*mLock: protects everything below */

    pthread_mutex_t mLock;
    VectorImpl mHandleToObject;
    pthread_key_t mTLS;
    bool mThreadPoolStarted;

    /* atomic opertion value */
    atomic_bool mShutdown;
    atomic_bool mDisableBackgroundScheduling;
    volatile int32_t mThreadPoolSeq;
    enum CallRestriction mCallRestriction;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: ProcessState_self
 *
 * Description:
 *     ProcessState_self get binder process object for currect process
 *  pointer, if the binder object is not available, create for current
 *  process and return it
 *
 ****************************************************************************/

ProcessState* ProcessState_self(void);

/****************************************************************************
 * Name: ProcessState_initWithDriver
 *
 * Description:
 *     ProcessState_initWithDriver() can be used to configure libbinder
 * to use a different binder driver dev node. It must be called *before*
 * any call to ProcessState::self(). The default is:
 *
 *  /dev/vndbinder: for processes built with the VNDK
 *  /dev/binder for those which are not.
 *
 ****************************************************************************/

ProcessState* ProcessState_initWithDriver(const char* driver);

/****************************************************************************
 * Name: ProcessState_delete
 *
 * Description:
 *
 ****************************************************************************/

void ProcessState_delete(ProcessState* this);

#endif /* __BINDER_INCLUDE_BINDER_PROCESSSTATE_H__ */
