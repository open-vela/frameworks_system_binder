/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "Utils.Thread"

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <nuttx/android/binder.h>
#include <nuttx/clock.h>
#include <nuttx/tls.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils/Binderlog.h"
#include "utils/Thread.h"
#include <android/binder_status.h>

#include "base/IPCThreadState.h"
#include "base/ProcessState.h"

typedef void* (*binder_pthread_entry)(void*);

struct thread_data_t;
typedef struct thread_data_t thread_data_t;

struct thread_data_t {
    binder_thread_func_t entryFunction;
    void* userData;
    int priority;
    char* threadName;
};

static inline uint32_t getThreadId(void)
{
    return (uint32_t)pthread_self();
}

static void SetThreadName(const char* name)
{
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
}

static int thread_trampoline(thread_data_t* t)
{
    binder_thread_func_t f = t->entryFunction;
    void* u = t->userData;
    int prio = t->priority;
    char* name = t->threadName;
    free(t);

    setpriority(PRIO_PROCESS, 0, prio);

    if (name) {
        SetThreadName(name);
        free(name);
    }
    return f(u);
}

static int CreateRawBinderThreadEtc(binder_thread_func_t entryFunction,
    void* userData, const char* threadName, int32_t threadPriority,
    size_t threadStackSize, uint32_t* threadId)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (threadPriority != PRIORITY_DEFAULT || threadName != NULL) {
        thread_data_t* t = malloc(sizeof(thread_data_t));
        t->priority = threadPriority;
        t->threadName = threadName ? strdup(threadName) : NULL;
        t->entryFunction = entryFunction;
        t->userData = userData;
        entryFunction = (binder_thread_func_t)&thread_trampoline;
        userData = t;
    }

    if (threadStackSize) {
        pthread_attr_setstacksize(&attr, threadStackSize);
    }

    errno = 0;
    pthread_t thread;
    int result = pthread_create(&thread, &attr,
        (binder_pthread_entry)entryFunction, userData);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        BINDER_LOGE(" failed (entry=%p, res=%d, %s)\n"
                    "(android threadPriority=%" PRIi32 ")",
            entryFunction, result, strerror(errno), threadPriority);
        return 0;
    }
    /* Note that *threadID is directly available to the parent only, as it is
     * assigned after the child starts.  Use memory barrier / lock if the child
     * or other threads also need access.
     */

    if (threadId != NULL) {
        *threadId = (uint32_t)thread;
    }
    return 1;
}

static int BinderThread_threadLoop(void* user)
{
    BinderThread* self = (BinderThread*)(user);
    self->mTid = gettid();
    bool first = true;

    do {
        bool result;
        if (first) {
            first = false;
            self->mStatus = self->readyToRun(self);
            result = (self->mStatus == OK);

            if (result && !self->exitPending(self)) {
                result = self->threadLoop(self);
            }
        } else {
            result = self->threadLoop(self);
        }

        pthread_mutex_lock(&self->mLock);
        if (result == false || self->mExitPending) {
            self->mExitPending = true;
            self->mRunning = false;
            self->mThread = -1;
            pthread_cond_broadcast(&self->mThreadExitedCondition);
            break;
        }
        pthread_mutex_unlock(&self->mLock);

    } while (1);

    return 0;
}

static uint32_t BinderThread_run(BinderThread* this, const char* name, int32_t priority, size_t stack)
{
    LOG_FATAL_IF(name == NULL, "thread name not provided to Thread::run\n");

    pthread_mutex_lock(&this->mLock);
    if (this->mRunning) {
        /* thread already started */
        pthread_mutex_unlock(&this->mLock);
        return STATUS_INVALID_OPERATION;
    }

    /* reset status and exitPending to their default value, so we can
     * try again after an error happened (either below, or in readyToRun())
     */

    this->mStatus = STATUS_OK;
    this->mExitPending = false;
    this->mThread = -1;
    /* hold a strong reference on ourself */
    //  this->mHoldSelf = this;
    this->mRunning = true;
    bool res;
    res = CreateRawBinderThreadEtc(BinderThread_threadLoop,
        this, name, priority, stack, &this->mThread);

    if (res == false) {
        this->mStatus = STATUS_UNKNOWN_ERROR; // something happened!
        this->mRunning = false;
        this->mThread = -1;
        // mHoldSelf.clear();  // "this" may have gone away after this.
        pthread_mutex_unlock(&this->mLock);
        return STATUS_UNKNOWN_ERROR;
    }
    pthread_mutex_unlock(&this->mLock);
    return STATUS_OK;
}

static void BinderThread_requestExit(BinderThread* this)
{
    pthread_mutex_lock(&this->mLock);
    this->mExitPending = true;
    pthread_mutex_unlock(&this->mLock);
}

static uint32_t BinderThread_readyToRun(BinderThread* this)
{
    return STATUS_OK;
}

static uint32_t BinderThread_requestExitAndWait(BinderThread* this)
{
    pthread_mutex_lock(&this->mLock);

    if (this->mThread == getThreadId()) {
        BINDER_LOGE("Thread (this=%p): don't call waitForExit() from this "
                    "Thread object's thread. It's a guaranteed deadlock!",
            this);
        pthread_mutex_unlock(&this->mLock);
        return STATUS_WOULD_BLOCK;
    }
    this->mExitPending = true;
    while (this->mRunning == true) {
        pthread_cond_wait(&this->mThreadExitedCondition, &this->mLock);
    }
    /* This next line is probably not needed any more, but is being left for
     * historical reference. Note that each interested party will clear flag.
     */

    this->mExitPending = false;

    pthread_mutex_unlock(&this->mLock);
    return this->mStatus;
}

static uint32_t BinderThread_join(BinderThread* this)
{
    pthread_mutex_lock(&this->mLock);
    if (this->mThread == getThreadId()) {
        BINDER_LOGE("Thread (this=%p): don't call join() from this "
                    "Thread object's thread. It's a guaranteed deadlock!",
            this);
        pthread_mutex_unlock(&this->mLock);
        return STATUS_WOULD_BLOCK;
    }
    while (this->mRunning == true) {
        pthread_cond_wait(&this->mThreadExitedCondition, &this->mLock);
    }
    pthread_mutex_unlock(&this->mLock);
    return this->mStatus;
}

static bool BinderThread_isRunning(BinderThread* this)
{
    bool ret;
    pthread_mutex_lock(&this->mLock);
    ret = this->mRunning;
    pthread_mutex_unlock(&this->mLock);
    return ret;
}

static pid_t BinderThread_getTid(BinderThread* this)
{
    pthread_mutex_lock(&this->mLock);
    pid_t tid;
    if (this->mRunning) {
        // pthread_t pthread = (pthread_t)this->mThread;
        tid = gettid();
    } else {
        BINDER_LOGE("Thread (this=%p): getTid() is undefined before run()", this);
        tid = -1;
    }
    pthread_mutex_unlock(&this->mLock);
    return tid;
}

static bool BinderThread_exitPending(BinderThread* this)
{
    bool ret;
    pthread_mutex_lock(&this->mLock);
    ret = this->mExitPending;
    pthread_mutex_unlock(&this->mLock);
    return ret;
}

static void BinderThread_dtor(BinderThread* this)
{
    this->m_refbase.dtor(&this->m_refbase);
}

void BinderThread_ctor(BinderThread* this)
{
    RefBase_ctor(&this->m_refbase);

    pthread_mutex_init(&this->mLock, NULL);
    pthread_cond_init(&this->mThreadExitedCondition, NULL);
    this->mStatus = STATUS_OK;
    this->mExitPending = false;
    this->mRunning = false;
    this->mThread = -1;
    this->mTid = -1;
    this->mHoldSelf = NULL;

    this->run = BinderThread_run;
    this->requestExit = BinderThread_requestExit;
    this->readyToRun = BinderThread_readyToRun;
    this->requestExitAndWait = BinderThread_requestExitAndWait;
    this->join = BinderThread_join;
    this->isRunning = BinderThread_isRunning;
    this->getTid = BinderThread_getTid;
    this->exitPending = BinderThread_exitPending;

    this->dtor = BinderThread_dtor;
}

bool IPCThreadPool_threadLoop(void* v_this)
{
    IPCThreadPool* this = (IPCThreadPool*)v_this;
    IPCThreadState* self = IPCThreadState_self();

    self->joinThreadPool(self, this->mIsMain);
    return false;
}

static void IPCThreadPool_dtor(IPCThreadPool* this)
{
    this->m_Thread.dtor(&this->m_Thread);
}

static void IPCThreadPool_ctor(IPCThreadPool* this, bool isMain)
{
    BinderThread_ctor(&this->m_Thread);

    this->m_Thread.threadLoop = IPCThreadPool_threadLoop;

    this->mIsMain = isMain;
    this->dtor = IPCThreadPool_dtor;
}

IPCThreadPool* IPCThreadPool_new(bool isMain)
{
    IPCThreadPool* this;
    this = zalloc(sizeof(IPCThreadPool));

    IPCThreadPool_ctor(this, isMain);
    return this;
}
