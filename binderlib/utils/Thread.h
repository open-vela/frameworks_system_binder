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

#ifndef __BINDER_INCLUDE_UTILS_THREAD_H__
#define __BINDER_INCLUDE_UTILS_THREAD_H__

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include "utils/RefBase.h"

#define PRIORITY_DEFAULT 100

typedef int (*binder_thread_func_t)(void*);

struct BinderThread;
typedef struct BinderThread BinderThread;

struct BinderThread {
    struct RefBase m_refbase;
    void (*dtor)(BinderThread* this);

    uint32_t (*run)(BinderThread* this, const char* name, int32_t priority, size_t stack);
    void (*requestExit)(BinderThread* this);
    uint32_t (*readyToRun)(BinderThread* this);
    uint32_t (*requestExitAndWait)(BinderThread* this);
    uint32_t (*join)(BinderThread* this);
    bool (*isRunning)(BinderThread* this);
    pid_t (*getTid)(BinderThread* this);
    bool (*exitPending)(BinderThread* this);

    /* virtual function */
    bool (*threadLoop)(void* v_this);

    BinderThread* mHoldSelf;
    pthread_mutex_t mLock;
    pthread_cond_t mThreadExitedCondition;
    uint32_t mStatus;
    volatile bool mExitPending;
    volatile bool mRunning;
    uint32_t mThread;
    pid_t mTid;
};

void BinderThread_ctor(BinderThread* this);

struct IPCThreadPool;
typedef struct IPCThreadPool IPCThreadPool;

struct IPCThreadPool {
    BinderThread m_Thread;

    void (*dtor)(IPCThreadPool* this);

    bool mIsMain;
};

IPCThreadPool* IPCThreadPool_new(bool isMain);

#endif // _LIBS_UTILS_THREAD_H
