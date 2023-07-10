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

#define LOG_TAG "ProcessState"

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

#include "IPCThreadState.h"
#include "ProcessGlobal.h"
#include "ProcessState.h"
#include "Stability.h"
#include "utils/Binderlog.h"
#include "utils/Thread.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_BINDER_VM_SIZE (4 * 1024)
#define DEFAULT_MAX_BINDER_THREADS 2

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int open_driver(ProcessState* this, const char* driver)
{
    int fd;
    int result;

    fd = open(driver, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        BINDER_LOGD("Opening %s failed, errno=%d\n", driver, fd);
        return fd;
    }
    result = ioctl(fd, BINDER_SET_MAX_THREADS, &this->mMaxThreads);
    if (result < 0) {
        BINDER_LOGD("Binder ioctl to set max threads failed: %d", result);
        return result;
    }

    return fd;
}

IBinder* ProcessState_getContextObject(ProcessState* this, const IBinder* caller)
{
    IBinder* context = this->getStrongProxyForHandle(this, 0);

    if (context) {
        /* The root object is special since we get it directly from the driver, it is never
         * written by Parcell_writeStrongBinder.
         */

        Stability_markCompilationUnit(context);
    } else {
        BINDER_LOGW("Not able to get context object on %s.", this->mDriverName);
    }
    return context;
}

static void ProcessState_startThreadPool(ProcessState* this)
{
    pthread_mutex_lock(&this->mLock);
    if (!this->mThreadPoolStarted) {
        if (this->mMaxThreads == 0) {
            BINDER_LOGE("Extra binder thread started, but 0 threads requested. Do not use "
                        "*startThreadPool when zero threads are requested.");
        }
        this->mThreadPoolStarted = true;
        this->spawnPooledThread(this, true);
    }
    pthread_mutex_unlock(&this->mLock);
}

static bool ProcessState_becomeContextManager(ProcessState* this)
{
    int unused = 0;
    int32_t result = 0;

    pthread_mutex_lock(&this->mLock);

    result = ioctl(this->mDriverFD, BINDER_SET_CONTEXT_MGR, &unused);
    if (result != 0) {
        BINDER_LOGD("Binder ioctl to become context manager failed: %s\n",
            strerror(errno));
    }

    pthread_mutex_unlock(&this->mLock);
    return result == 0;
}

/* Queries the driver for the current strong reference count of the node
 * that the handle points to. Can only be used by the servicemanager.
 *
 * Returns -1 in case of failure, otherwise the strong reference count.
 */
static ssize_t ProcessState_getStrongRefCountForNode(ProcessState* this, BpBinder* binder)
{
    struct binder_node_info_for_ref info;
    memset(&info, 0, sizeof(struct binder_node_info_for_ref));
    // info.handle = binder->getPrivateAccessor().binderHandle();
    info.handle = binder->binderHandle(binder);

    int32_t result = ioctl(this->mDriverFD, BINDER_GET_NODE_INFO_FOR_REF, &info);
    if (result != OK) {
        BINDER_LOGE("Kernel does not support BINDER_GET_NODE_INFO_FOR_REF.");
        return -1;
    }
    return info.strong_count;
}

static void ProcessState_setCallRestriction(ProcessState* this, int restriction)
{
    LOG_FATAL_IF(IPCThreadState_selfOrNull() != NULL,
        "Call restrictions must be set before the threadpool is started.");

    this->mCallRestriction = restriction;
}

static handle_entry* ProcessState_lookupHandleLocked(ProcessState* this, int32_t handle)
{
    const size_t N = this->mHandleToObject.size(&this->mHandleToObject);
    if (N <= (size_t)handle) {
        handle_entry* e = zalloc(sizeof(handle_entry));
        e->binder = NULL;
        e->refs = NULL;
        int32_t err = this->mHandleToObject.append(&this->mHandleToObject, (void*)e, handle + 1 - N);
        if (err < STATUS_OK)
            return NULL;
    }
    return this->mHandleToObject.editItemAt(&this->mHandleToObject, handle);
}

static IBinder* ProcessState_getStrongProxyForHandle(ProcessState* this, int32_t handle)
{
    IBinder* result;

    pthread_mutex_lock(&this->mLock);

    result = (IBinder*)this->mContextObject;
    if (handle == 0 && result != NULL) {
        pthread_mutex_unlock(&this->mLock);
        return result;
    }

    handle_entry* e = this->lookupHandleLocked(this, handle);
    if (e != NULL) {
        /* We need to create a new BpBinder if there isn't currently one, OR we
         * are unable to acquire a weak reference on this current one.  The
         * attemptIncWeak() is safe because we know the BpBinder destructor will always
         * call expungeHandle(), which acquires the same lock we are holding now.
         * We need to do this because there is a race condition between someone
         * releasing a reference on this BpBinder, and a new reference on its handle
         * arriving from the driver.
         */

        IBinder* b = e->binder;
        if (b == NULL || !e->refs->attemptIncWeak(e->refs, this)) {
            if (handle == 0) {
                /* Special case for context manager...
                 * The context manager is the only object for which we create
                 * a BpBinder proxy without already holding a reference.
                 * Perform a dummy transaction to ensure the context manager
                 * is registered before we create the first local reference
                 * to it (which will occur when creating the BpBinder).
                 * If a local reference is created for the BpBinder when the
                 * context manager is not present, the driver will fail to
                 * provide a reference to the context manager, but the
                 * driver API does not return status.
                 *
                 * Note that this is not race-free if the context manager
                 * dies while this code runs.
                 */
                IPCThreadState* ipc = IPCThreadState_self();
                enum CallRestriction originalCallRestriction = ipc->getCallRestriction(ipc);
                ipc->setCallRestriction(ipc, CALL_RESTRICTION_NONE);
                Parcel data;
                Parcel_initState(&data);
                int32_t status = ipc->transact(ipc, 0, PING_TRANSACTION, &data, NULL, 0);
                ipc->setCallRestriction(ipc, originalCallRestriction);
                if (status == STATUS_DEAD_OBJECT) {
                    pthread_mutex_unlock(&this->mLock);
                    return NULL;
                }
            }
            BpBinder* bpbinder = PrivateAccessor_create(handle);
            e->binder = (IBinder*)bpbinder;
            if (bpbinder) {
                e->refs = bpbinder->getWeakRefs(bpbinder);
            }
            result = (IBinder*)bpbinder;
        } else {
            /* This little bit of nastyness is to allow us to add a primary
             * reference to the remote proxy when this team doesn't have one
             * but another team is sending the handle to us.
             */
            b->forceIncStrong(b, (const void*)b);
            result = b;

            // result.force_set(b);
            e->refs->decWeak(e->refs, this);
        }
    }
    pthread_mutex_unlock(&this->mLock);
    return result;
}

static void ProcessState_expungeHandle(ProcessState* this, int32_t handle, IBinder* binder)
{
    pthread_mutex_lock(&this->mLock);
    handle_entry* e = this->lookupHandleLocked(this, handle);

    /* This handle may have already been replaced with a new BpBinder
     * (if someone failed the AttemptIncWeak() above); we don't want
     * to overwrite it.
     */
    if (e && e->binder == binder) {
        e->binder = NULL;
    }
    pthread_mutex_unlock(&this->mLock);
}

static void ProcessState_makeBinderThreadName(ProcessState* this, char* name, int namelen)
{
    int32_t s = this->mThreadPoolSeq++;
    pid_t pid = gettid();

    snprintf(name, namelen, "binder:%d_%" PRIi32 "", pid, s);
}

static void ProcessState_spawnPooledThread(ProcessState* this, bool isMain)
{
    if (this->mThreadPoolStarted) {
        char name[32];
        this->makeBinderThreadName(this, name, 32);
        BinderThread* t = (BinderThread*)IPCThreadPool_new(isMain);
        t->run(t, name, SCHED_PRIORITY_DEFAULT, CONFIG_DEFAULT_TASK_STACKSIZE);
    }
}

static int32_t ProcessState_setThreadPoolMaxThreadCount(ProcessState* this, int maxThreads)
{
    LOG_FATAL_IF(this->mThreadPoolStarted && maxThreads < this->mMaxThreads,
        "Binder threadpool cannot be shrunk after starting");
    int32_t result = STATUS_OK;

    if (ioctl(this->mDriverFD, BINDER_SET_MAX_THREADS, &maxThreads) != -1) {
        this->mMaxThreads = maxThreads;
    } else {
        result = -errno;
        BINDER_LOGE("Binder ioctl to set max threads failed: %s", strerror(-result));
    }
    return result;
}

static const char* ProcessState_getDriverName(ProcessState* this)
{
    return this->mDriverName;
}

void ProcessState_incStrong(ProcessState* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->incStrong(refbase, id);
}

void ProcessState_incStrongRequireStrong(ProcessState* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->incStrongRequireStrong(refbase, id);
}

void ProcessState_decStrong(ProcessState* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->decStrong(refbase, id);
}

void ProcessState_forceIncStrong(ProcessState* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->forceIncStrong(refbase, id);
}

static void ProcessState_dtor(ProcessState* this)
{
    pthread_key_delete(this->mTLS);

    if (this->mDriverFD >= 0) {
        munmap(this->mVMStart, DEFAULT_BINDER_VM_SIZE);
        close(this->mDriverFD);
    }
    this->mDriverFD = -1;

    this->m_refbase.dtor(&this->m_refbase);
    this->mHandleToObject.dtor(&this->mHandleToObject);
}

static void ProcessState_ctor(ProcessState* this, const char* driver)
{
    int fd;

    RefBase_ctor(&this->m_refbase);
    VectorImpl_ctor(&this->mHandleToObject);

    this->mDriverName = driver;
    this->mDriverFD = -1;
    this->mVMStart = MAP_FAILED;
    pthread_mutex_init(&this->mThreadCountLock, NULL);
    pthread_cond_init(&this->mThreadCountDecrement, NULL);
    this->mExecutingThreadsCount = 0;
    this->mWaitingForThreads = 0;
    this->mMaxThreads = DEFAULT_MAX_BINDER_THREADS;
    this->mCurrentThreads = 0;
    this->mKernelStartedThreads = 0;
    this->mContextObject = 0;

    atomic_init(&this->mShutdown, false);
    atomic_init(&this->mDisableBackgroundScheduling, false);
    pthread_mutex_init(&this->mLock, NULL);

    /* Virtual Function for RefBase */
    this->incStrong = ProcessState_incStrong;
    this->incStrongRequireStrong = ProcessState_incStrongRequireStrong;
    this->decStrong = ProcessState_decStrong;
    this->forceIncStrong = ProcessState_forceIncStrong;

    this->getContextObject = ProcessState_getContextObject;
    this->spawnPooledThread = ProcessState_spawnPooledThread;
    this->getDriverName = ProcessState_getDriverName;
    this->makeBinderThreadName = ProcessState_makeBinderThreadName;
    this->expungeHandle = ProcessState_expungeHandle;
    this->getStrongProxyForHandle = ProcessState_getStrongProxyForHandle;
    this->lookupHandleLocked = ProcessState_lookupHandleLocked;
    this->setCallRestriction = ProcessState_setCallRestriction;
    this->getStrongRefCountForNode = ProcessState_getStrongRefCountForNode;
    this->becomeContextManager = ProcessState_becomeContextManager;
    this->startThreadPool = ProcessState_startThreadPool;
    this->setThreadPoolMaxThreadCount = ProcessState_setThreadPoolMaxThreadCount;

    pthread_key_create(&this->mTLS, IPCThreadState_threadDestructor);

    fd = open_driver(this, driver);

    if (fd >= 0) {
        /* mmap the binder, providing a memory region
         * to receive transactions.
         * Notes:
         *   For Linux, it's necessary to set flag to MAP_PRIVATE | MAP_NORESERVE,
         * but for NuttX, the driver will allocate a real memory region rather than
         * virtual memory map like Linux, so the MAP_PRIVATE | MAP_NORESERVE is
         * useless for NuttX
         */

        this->mVMStart = mmap(NULL, DEFAULT_BINDER_VM_SIZE, PROT_READ, 0, fd, 0);

        if (this->mVMStart == MAP_FAILED) {
            close(fd);
            this->mDriverName = NULL;
            LOG_FATAL("Using %s failed: unable to mmap transaction memory.\n",
                driver);
        }
    } else {
        LOG_FATAL("Binder driver %s could not be opened. Terminating: %d\n",
            driver, fd);
    }
    this->mDriverFD = fd;
    this->dtor = ProcessState_dtor;
}

ProcessState* ProcessState_new(const char* driver)
{
    ProcessState* this;

    this = zalloc(sizeof(ProcessState));

    ProcessState_ctor(this, driver);
    return this;
}

static ProcessState* ProcessState_init(const char* driver)
{
    ProcessState_global* global = ProcessState_global_get();

    pthread_mutex_lock(&global->gProcessMutex);
    if (global->gProcessInit == false) {
        global->gProcessState = ProcessState_new(driver);
        global->gProcessInit = true;
    }
    pthread_mutex_unlock(&global->gProcessMutex);

    return global->gProcessState;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void ProcessState_delete(ProcessState* this)
{
    this->dtor(this);
    free(this);
}

ProcessState* ProcessState_initWithDriver(const char* driver)
{
    return ProcessState_init(driver);
}

ProcessState* ProcessState_self(void)
{
    return ProcessState_init("/dev/binder");
}
