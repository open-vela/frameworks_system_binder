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

#define LOG_TAG "BBinder"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <nuttx/sched.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Binder.h"
#include "BpBinder.h"
#include "IBinder.h"
#include "IInterface.h"
#include "IPCThreadState.h"
#include "Parcel.h"
#include "utils/Binderlog.h"
#include <android/binder_status.h>

/* Log any reply transactions for which the data exceeds this size */
#define LOG_REPLIES_OVER_SIZE (2 * 1024)

static void BBinder_Extras_dtor(BBinder_Extras* this)
{
    this->mObjects.dtor(&this->mObjects);
    pthread_mutex_destroy(&this->mLock);
}

static void BBinder_Extras_ctor(BBinder_Extras* this)
{
    ObjectManager_ctor(&this->mObjects);
    pthread_mutex_init(&this->mLock, NULL);
    this->mPolicy = SCHED_FIFO;
    this->mPriority = 100;
    this->mRequestingSid = false;
    this->mInheritRt = false;
    this->mExtension = NULL;

    this->dtor = BBinder_Extras_dtor;
}

BBinder_Extras* BBinder_Extras_new(void)
{
    BBinder_Extras* this;

    this = zalloc(sizeof(BBinder_Extras));

    BBinder_Extras_ctor(this);
    return this;
}

void BBinder_Extras_delete(BBinder_Extras* this)
{
    this->dtor(this);
    free(this);
}

static bool BBinder_isBinderAlive(BBinder* this)
{
    return true;
}

static uint32_t BBinder_pingBinder(BBinder* this)
{
    return STATUS_OK;
}

static String* BBinder_getInterfaceDescriptor(BBinder* this)
{
    /* This is a local static rather than a global static,
     * to avoid static initializer ordering issues.
     */

    static String sEmptyDescriptor;

    return &sEmptyDescriptor;
}

static uint32_t BBinder_transact(BBinder* this, uint32_t code,
    const Parcel* in_data, Parcel* reply,
    uint32_t flags)
{
    Parcel data;

    Parcel_initState(&data);
    Parcel_dup(&data, in_data);

    Parcel_setDataPosition(&data, 0);

    if (reply != NULL && (flags & FLAG_CLEAR_BUF)) {
        Parcel_markSensitive(reply);
    }

    uint32_t err = STATUS_OK;

    switch (code) {
    case PING_TRANSACTION: {
        err = this->pingBinder(this);
        break;
    }

    case EXTENSION_TRANSACTION: {
        CHECK(reply == NULL);
        err = Parcel_writeStrongBinder(reply, this->getExtension(this));
        break;
    }

    case DEBUG_PID_TRANSACTION: {
        CHECK(reply == NULL);
        err = Parcel_writeInt32(reply, this->getDebugPid(this));
        break;
    }

    default: {
        err = this->onTransact(this, code, &data, reply, flags);
        break;
    }
    }

    /* In case this is being transacted on in the same process. */
    if (reply != NULL) {
        Parcel_setDataPosition(reply, 0);
        if (Parcel_dataSize(reply) > LOG_REPLIES_OVER_SIZE) {
            BINDER_LOGD("Large reply transaction of %zu bytes, "
                        " interface descriptor %s, code %" PRIu32 "",
                Parcel_dataSize(reply),
                String_data(this->getInterfaceDescriptor(this)), code);
        }
    }

    return err;
}

static uint32_t BBinder_linkToDeath(BBinder* this,
    DeathRecipient* recipient,
    void* cookie, uint32_t flags)
{
    return STATUS_INVALID_OPERATION;
}

static uint32_t BBinder_unlinkToDeath(BBinder* this,
    DeathRecipient* recipient,
    void* cookie, uint32_t flags,
    DeathRecipient** outRecipient)
{
    return STATUS_INVALID_OPERATION;
}

static uint32_t BBinder_dump(BBinder* this, int fd,
    const VectorString* args)
{
    return STATUS_OK;
}

static void* BBinder_attachObject(BBinder* this, const void* objectID,
    void* object, void* cleanupCookie,
    object_cleanup_func func)
{
    BBinder_Extras* e = this->getOrCreateExtras(this);
    void* pRet;

    CHECK_FAIL(!e);

    pthread_mutex_lock(&e->mLock);
    pRet = e->mObjects.attach(&e->mObjects, objectID, object, cleanupCookie,
        func);
    pthread_mutex_unlock(&e->mLock);

    return pRet;
}

static void* BBinder_findObject(BBinder* this, const void* objectID)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);
    void* pRet;

    if (!e) {
        return NULL;
    }

    pthread_mutex_lock(&e->mLock);
    pRet = e->mObjects.find(&e->mObjects, objectID);
    pthread_mutex_unlock(&e->mLock);

    return pRet;
}

static void* BBinder_detachObject(BBinder* this, const void* objectID)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);
    void* pRet;

    if (!e) {
        return NULL;
    }

    pthread_mutex_lock(&e->mLock);
    pRet = e->mObjects.detach(&e->mObjects, objectID);
    pthread_mutex_unlock(&e->mLock);

    return pRet;
}

static BBinder* BBinder_localBinder(BBinder* this)
{
    return this;
}

static BBinder_Extras* BBinder_getOrCreateExtras(BBinder* this)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    if (!e) {
        e = BBinder_Extras_new();
        if (e == NULL) {
            return NULL; /* out of memory */
        }
    }

    return e;
}

static IBinder* BBinder_getExtension(BBinder* this)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    if (e == NULL) {
        return NULL;
    }
    return e->mExtension;
}

static pid_t BBinder_getDebugPid(BBinder* this)
{
    return gettid();
}

uint32_t BBinder_onTransact(BBinder* this, uint32_t code,
    const struct Parcel* in_data,
    struct Parcel* reply, uint32_t flags)
{
    Parcel data;

    Parcel_initState(&data);
    Parcel_dup(&data, in_data);

    switch (code) {
    case INTERFACE_TRANSACTION: {
        CHECK(reply != NULL);
        String desc;
        String_dup(&desc, this->getInterfaceDescriptor(this));
        Parcel_writeString16(reply, &desc);
        return STATUS_OK;
    }

    case DUMP_TRANSACTION: {
        int fd = Parcel_readFileDescriptor(&data);
        int32_t argc;
        uint32_t ret;

        Parcel_readInt32(&data, &argc);
        VectorString args;
        VectorString_ctor(&args);
        String str;
        for (int i = 0; i < argc && Parcel_dataAvail(&data) > 0; i++) {
            Parcel_readString16_to(&data, &str);
            args.add(&args, &str);
        }
        ret = this->dump(this, fd, &args);
        args.dtor(&args);
        return ret;
    }

    case SHELL_COMMAND_TRANSACTION: {
        BINDER_LOGD("Unsupport: SHELL_COMMAND_TRANSACTION\n");
        return STATUS_OK;
    }

    case SYSPROPS_TRANSACTION: {
        BINDER_LOGD("Unsupport: SYSPROPS_TRANSACTION\n");
        return STATUS_OK;
    }

    default: {
        return STATUS_UNKNOWN_TRANSACTION;
    }
    }
}

static bool BBinder_isRequestingSid(BBinder* this)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    return e && e->mRequestingSid;
}

static void BBinder_setRequestingSid(BBinder* this, bool requestingSid)
{
    LOG_FATAL_IF(this->mParceled,
        "setRequestingSid() should not be called after a binder object "
        "is parceled/sent to another process");

    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    if (!e) {
        /* default is false. Most things don't need sids, so avoiding
         * allocations when possible. */
        if (!requestingSid) {
            return;
        }

        e = this->getOrCreateExtras(this);
        if (!e) {
            return; /* out of memory */
        }
    }

    e->mRequestingSid = requestingSid;
}

static void BBinder_setMinSchedulerPolicy(BBinder* this, int policy,
    int priority)
{
    LOG_FATAL_IF(this->mParceled,
        "setMinSchedulerPolicy() should not be called after a binder object "
        "is parceled/sent to another process");

    switch (policy) {
    case SCHED_RR:
    case SCHED_FIFO: {
        LOG_FATAL_IF(
            priority < SCHED_PRIORITY_MIN || priority > SCHED_PRIORITY_MAX,
            "Invalid priority for sched %d: %d", policy, priority);
        break;
    }

    default: {
        LOG_FATAL("Unrecognized scheduling policy: %d", policy);
    }
    }

    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    if (e == NULL) {
        /* Avoid allocations if called with default. */
        if (policy == SCHED_FIFO && priority == SCHED_PRIORITY_DEFAULT) {
            return;
        }

        e = this->getOrCreateExtras(this);
        if (!e) {
            return; /* out of memory */
        }
    }

    e->mPolicy = policy;
    e->mPriority = priority;
}

static int BBinder_getMinSchedulerPolicy(BBinder* this)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    if (e == NULL) {
        return SCHED_FIFO;
    }
    return e->mPolicy;
}

static int BBinder_getMinSchedulerPriority(BBinder* this)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    if (e == NULL) {
        return 0;
    }
    return e->mPriority;
}

static bool BBinder_isInheritRt(BBinder* this)
{
    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    return e && e->mInheritRt;
}

static void BBinder_setInheritRt(BBinder* this, bool inheritRt)
{
    LOG_FATAL_IF(this->mParceled,
        "setInheritRt() should not be called after a binder object "
        "is parceled/sent to another process");

    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_acquire);

    if (!e) {
        if (!inheritRt) {
            return;
        }

        e = this->getOrCreateExtras(this);
        if (!e) {
            return; /* out of memory */
        }
    }

    e->mInheritRt = inheritRt;
}

static void BBinder_setExtension(BBinder* this, IBinder* extension)
{
    LOG_FATAL_IF(this->mParceled,
        "setExtension() should not be called after a binder object "
        "is parceled/sent to another process");

    BBinder_Extras* e = this->getOrCreateExtras(this);
    e->mExtension = extension;
}

static void BBinder_withLock(BBinder* this, withLockcallback doWithLock)
{
    BBinder_Extras* e = this->getOrCreateExtras(this);

    LOG_FATAL_IF(!e, "no memory");

    pthread_mutex_lock(&e->mLock);
    doWithLock((IBinder*)this);
    pthread_mutex_unlock(&e->mLock);
}

static bool BBinder_wasParceled(BBinder* this)
{
    return this->mParceled;
}

static void BBinder_setParceled(BBinder* this)
{
    this->mParceled = true;
}
void BBinder_incStrong(BBinder* this, const void* id)
{
    RefBase* refbase = &this->m_IBinder.m_refbase;
    refbase->incStrong(refbase, id);
}

void BBinder_decStrong(BBinder* this, const void* id)
{
    RefBase* refbase = &this->m_IBinder.m_refbase;
    refbase->decStrong(refbase, id);
}

RefBase_weakref* BBinder_createWeak(BBinder* this, const void* id)
{
    RefBase* refbase = &this->m_IBinder.m_refbase;
    return refbase->createWeak(refbase, id);
}

RefBase_weakref* BBinder_getWeakRefs(BBinder* this)
{
    RefBase* refbase = &this->m_IBinder.m_refbase;
    return refbase->getWeakRefs(refbase);
}

void BBinder_printRefs(BBinder* this)
{
    RefBase* refbase = &this->m_IBinder.m_refbase;
    return refbase->printRefs(refbase);
}

static String* BBinder_Vfun_getInterfaceDescriptor(IBinder* v_this)
{
    BBinder* this = (BBinder*)v_this;
    return this->getInterfaceDescriptor(this);
}

static bool BBinder_Vfun_isBinderAlive(IBinder* v_this)
{
    return true;
}

static uint32_t BBinder_Vfun_pingBinder(IBinder* v_this)
{
    return STATUS_OK;
}

static uint32_t BBinder_Vfun_transact(IBinder* v_this, uint32_t code,
    const Parcel* in_data, Parcel* reply,
    uint32_t flags)
{
    BBinder* this = (BBinder*)v_this;
    return this->transact(this, code, in_data, reply, flags);
}

static uint32_t BBinder_Vfun_linkToDeath(IBinder* v_this,
    DeathRecipient* recipient,
    void* cookie, uint32_t flags)
{
    return STATUS_INVALID_OPERATION;
}

static uint32_t BBinder_Vfun_unlinkToDeath(IBinder* v_this,
    DeathRecipient* recipient,
    void* cookie, uint32_t flags,
    DeathRecipient** outRecipient)
{
    return STATUS_INVALID_OPERATION;
}

static uint32_t BBinder_Vfun_dump(IBinder* v_this, int fd,
    const VectorString* args)
{
    return STATUS_OK;
}

static void* BBinder_Vfun_attachObject(IBinder* v_this, const void* objectID,
    void* object, void* cleanupCookie,
    object_cleanup_func func)
{
    BBinder* this = (BBinder*)v_this;
    return this->attachObject(this, objectID, object, cleanupCookie, func);
}

static void* BBinder_Vfun_findObject(IBinder* v_this, const void* objectID)
{
    BBinder* this = (BBinder*)v_this;
    return this->findObject(this, objectID);
}

static void* BBinder_Vfun_detachObject(IBinder* v_this, const void* objectID)
{
    BBinder* this = (BBinder*)v_this;
    return this->detachObject(this, objectID);
}

static BBinder* BBinder_Vfun_localBinder(IBinder* v_this)
{
    BBinder* this = (BBinder*)v_this;
    return this;
}

static void BBinder_dtor(BBinder* this)
{
    if (!this->wasParceled(this) && this->getExtension(this)) {
        BINDER_LOGD(
            "Binder %p destroyed with extension attached before being parceled.",
            this);
    }

    BBinder_Extras* e = atomic_load_explicit(&this->mExtras, memory_order_relaxed);

    if (e) {
        BBinder_Extras_delete(e);
    }

    this->m_IBinder.dtor(&this->m_IBinder);
}

void BBinder_ctor(BBinder* this)
{
    IBinder* ibinder = &this->m_IBinder;

    IBinder_ctor(&this->m_IBinder);

    /* Override Pure Virtual function in IBinder */
    ibinder->getInterfaceDescriptor = BBinder_Vfun_getInterfaceDescriptor;
    ibinder->isBinderAlive = BBinder_Vfun_isBinderAlive;
    ibinder->pingBinder = BBinder_Vfun_pingBinder;
    ibinder->transact = BBinder_Vfun_transact;
    ibinder->linkToDeath = BBinder_Vfun_linkToDeath;
    ibinder->unlinkToDeath = BBinder_Vfun_unlinkToDeath;
    ibinder->dump = BBinder_Vfun_dump;
    ibinder->localBinder = BBinder_Vfun_localBinder;
    ibinder->attachObject = BBinder_Vfun_attachObject;
    ibinder->findObject = BBinder_Vfun_findObject;
    ibinder->detachObject = BBinder_Vfun_detachObject;

    /* Virtual Function for RefBase */
    this->incStrong = BBinder_incStrong;
    this->decStrong = BBinder_decStrong;
    this->createWeak = BBinder_createWeak;
    this->getWeakRefs = BBinder_getWeakRefs;
    this->printRefs = BBinder_printRefs;

    /* Virtual Function */
    this->getInterfaceDescriptor = BBinder_getInterfaceDescriptor;
    this->onTransact = BBinder_onTransact;
    this->isBinderAlive = BBinder_isBinderAlive;
    this->pingBinder = BBinder_pingBinder;
    this->transact = BBinder_transact;
    this->linkToDeath = BBinder_linkToDeath;
    this->unlinkToDeath = BBinder_unlinkToDeath;
    this->dump = BBinder_dump;
    this->localBinder = BBinder_localBinder;
    this->attachObject = BBinder_attachObject;
    this->findObject = BBinder_findObject;
    this->detachObject = BBinder_detachObject;

    /* Member function of BBinder */
    this->isRequestingSid = BBinder_isRequestingSid;
    this->setRequestingSid = BBinder_setRequestingSid;
    this->getExtension = BBinder_getExtension;
    this->setExtension = BBinder_setExtension;
    this->setMinSchedulerPolicy = BBinder_setMinSchedulerPolicy;
    this->getMinSchedulerPolicy = BBinder_getMinSchedulerPolicy;
    this->getMinSchedulerPriority = BBinder_getMinSchedulerPriority;
    this->isInheritRt = BBinder_isInheritRt;
    this->setInheritRt = BBinder_setInheritRt;
    this->getDebugPid = BBinder_getDebugPid;
    this->wasParceled = BBinder_wasParceled;
    this->setParceled = BBinder_setParceled;
    this->getOrCreateExtras = BBinder_getOrCreateExtras;
    this->withLock = BBinder_withLock;

    this->mExtras = NULL;
    this->mStability = 0;
    this->mParceled = false;

    this->dtor = BBinder_dtor;
}
