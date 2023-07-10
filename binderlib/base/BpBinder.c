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

#define LOG_TAG "BpBinder"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/android/binder.h>
#include <nuttx/tls.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "BpBinder.h"
#include "IPCThreadState.h"
#include "ProcessGlobal.h"
#include "ProcessState.h"
#include "utils/Binderlog.h"
#include <android/binder_status.h>

enum {
    LIMIT_REACHED_MASK = 0x80000000, // A flag denoting that the limit has been reached
    COUNTING_VALUE_MASK = 0x7FFFFFFF, // A mask of the remaining bits for the count value
};

static void* ObjectManager_attach(ObjectManager* this, const void* objectID, void* object,
    void* cleanupCookie, object_cleanup_func func)
{
    ObjectEntry e;
    ObjectEntry* pEntry = NULL;

    e.object = object;
    e.cleanupCookie = cleanupCookie;
    e.func = func;

    this->mObjects.find(&this->mObjects, (long)objectID, (long*)&pEntry);

    if (pEntry != NULL) {
        BINDER_LOGI("Trying to attach object ID %p to binder ObjectManager %p "
                    "with object %p, but object ID already in use",
            objectID, this, object);
        return pEntry->object;
    }

    this->mObjects.insert(&this->mObjects, (long)objectID, (long)&e);
    return NULL;
}

static void* ObjectManager_find(ObjectManager* this, const void* objectID)
{
    ObjectEntry* pEntry = NULL;

    this->mObjects.find(&this->mObjects, (long)objectID, (long*)&pEntry);
    if (pEntry == NULL) {
        return NULL;
    }
    return pEntry->object;
}

void* ObjectManager_detach(ObjectManager* this, const void* objectID)
{
    ObjectEntry* pEntry = NULL;

    this->mObjects.find(&this->mObjects, (long)objectID, (long*)&pEntry);
    if (pEntry == NULL) {
        return NULL;
    }
    void* value = pEntry->object;
    this->mObjects.erase(&this->mObjects, (long)objectID);
    return value;
}

static void ObjectEntry_Callback(const void* id, void* val)
{
    ObjectEntry* pEntry = (ObjectEntry*)val;

    if (pEntry->func != NULL) {
        pEntry->func(id, pEntry->object, pEntry->cleanupCookie);
    }
}

void ObjectManager_kill(ObjectManager* this)
{
    this->mObjects.iterator(&this->mObjects, ObjectEntry_Callback);
    this->mObjects.clear(&this->mObjects);
}

static void ObjectManager_dtor(ObjectManager* this)
{
    this->kill(this);
    this->mObjects.dtor(&this->mObjects);
}

void ObjectManager_ctor(ObjectManager* this)
{
    HashMap_ctor(&this->mObjects);

    this->attach = ObjectManager_attach;
    this->find = ObjectManager_find;
    this->detach = ObjectManager_detach;
    this->kill = ObjectManager_kill;

    this->dtor = ObjectManager_dtor;
}

static void PrivateAccessor_dtor(PrivateAccessor* this)
{
}

static int32_t PrivateAccessor_binderHandle(PrivateAccessor* this)
{
    return this->mBinder->binderHandle(this->mBinder);
}

static void PrivateAccessor_ctor(PrivateAccessor* this, BpBinder* binder)
{
    this->mBinder = binder;

    this->binderHandle = PrivateAccessor_binderHandle;

    this->dtor = PrivateAccessor_dtor;
}

void PrivateAccessor_delete(PrivateAccessor* this)
{
    this->dtor(this);
    free(this);
}

PrivateAccessor* PrivateAccessor_new(BpBinder* binder)
{
    PrivateAccessor* this;
    this = zalloc(sizeof(PrivateAccessor));
    PrivateAccessor_ctor(this, binder);
    return this;
}

void PrivateAccessor_delete(PrivateAccessor* this);

BpBinder* PrivateAccessor_create(int32_t handle)
{
    return BpBinder_create(handle);
}

RefBase_weakref* BpBinder_getWeakRefs(BpBinder* this)
{
    return this->m_IBinder.m_refbase.getWeakRefs(&this->m_IBinder.m_refbase);
}

static int32_t BpBinder_binderHandle(BpBinder* this)
{
    return this->binder_handle;
}

static int32_t BpBinder_getDebugBinderHandle(BpBinder* this)
{
    return this->binderHandle(this);
}

static bool BpBinder_isDescriptorCached(BpBinder* this)
{
    bool ret;

    pthread_mutex_lock(&this->mLock);
    ret = String_size(&this->mDescriptorCache) ? true : false;
    pthread_mutex_unlock(&this->mLock);
    return ret;
}

static String* BpBinder_getInterfaceDescriptor(BpBinder* this)
{
    if (this->isDescriptorCached(this) == false) {
        this->incStrongRequireStrong(this, (void*)this);
        Parcel data;
        Parcel reply;
        Parcel_initState(&data);
        Parcel_initState(&reply);

        Parcel_markForBinder(&data, (IBinder*)this);

        /* do the IPC without a lock held. */

        uint32_t err = this->transact(this, INTERFACE_TRANSACTION, &data, &reply, 0);
        if (err == STATUS_OK) {
            String res;
            String_init(&res, Parcel_readString16(&reply));
            pthread_mutex_lock(&this->mLock);
            if (String_size(&this->mDescriptorCache) == 0) {
                String_dup(&this->mDescriptorCache, &res);
            }
            pthread_mutex_unlock(&this->mLock);
        }
    }

    return &this->mDescriptorCache;
}

static bool BpBinder_isBinderAlive(BpBinder* this)
{
    return this->mAlive != 0;
}

static uint32_t BpBinder_pingBinder(BpBinder* this)
{
    Parcel data;
    Parcel_initState(&data);

    this->incStrongRequireStrong(this, (void*)this);
    Parcel_markForBinder(&data, (const IBinder*)this);
    Parcel reply;
    Parcel_initState(&reply);
    return this->transact(this, PING_TRANSACTION, &data, &reply, 0);
}

static uint32_t BpBinder_dump(BpBinder* this, int fd, const VectorString* args)
{
    Parcel send;
    Parcel reply;
    Parcel_initState(&send);
    Parcel_initState(&reply);
    String* str;

    Parcel_writeFileDescriptor(&send, fd, false);
    const size_t numArgs = args->size(args);
    Parcel_writeInt32(&send, numArgs);
    for (size_t i = 0; i < numArgs; i++) {
        str = args->get(args, i);
        Parcel_writeString16(&send, str);
    }
    return this->transact(this, DUMP_TRANSACTION, &send, &reply, 0);
}

static uint32_t BpBinder_transact(BpBinder* this, uint32_t code,
    const Parcel* data, Parcel* reply,
    uint32_t flags)
{
    IPCThreadState* self;

    /* Once a binder has died, it will never come back to life. */

    if (this->mAlive) {
        /* don't send userspace flags to the kernel */
        flags = flags & ~(FLAG_PRIVATE_VENDOR);

        uint32_t status;
        self = IPCThreadState_self();
        status = self->transact(self, this->binderHandle(this), code, data, reply, flags);

        if (Parcel_dataSize(data) > LOG_TRANSACTIONS_OVER_SIZE) {
            pthread_mutex_lock(&this->mLock);
            BINDER_LOGD("Large outgoing transaction of %zu bytes, interface descriptor %s, code %" PRIu32 "",
                Parcel_dataSize(data),
                String_size(&this->mDescriptorCache) ? String_data(&this->mDescriptorCache) : "<uncached descriptor>",
                code);
            pthread_mutex_unlock(&this->mLock);
        }

        if (status == STATUS_DEAD_OBJECT) {
            this->mAlive = 0;
        }
        return status;
    }
    return STATUS_DEAD_OBJECT;
}

static uint32_t BpBinder_linkToDeath(BpBinder* this, DeathRecipient* recipient,
    void* cookie, uint32_t flags)
{
    IPCThreadState* self = IPCThreadState_self();
    RefBase_weakref* weakref;
    Obituary ob;
    ob.recipient = recipient;
    ob.cookie = cookie;
    ob.flags = flags;

    LOG_FATAL_IF(recipient == NULL,
        "linkToDeath(): recipient must be non-NULL");

    pthread_mutex_lock(&this->mLock);

    if (!this->mObitsSent) {
        if (!this->mObituaries) {
            this->mObituaries = VectorImpl_new();
            if (!this->mObituaries) {
                pthread_mutex_unlock(&this->mLock);
                return STATUS_NO_MEMORY;
            }
            BINDER_LOGV("Requesting death notification: %p handle %" PRIi32 "\n",
                this, this->binderHandle(this));
            weakref = this->getWeakRefs(this);
            weakref->incWeak(weakref, this);
            self->requestDeathNotification(self, this->binderHandle(this), this);
            self->flushCommands(self);
        }

        ssize_t res = this->mObituaries->add(this->mObituaries, &ob);
        pthread_mutex_unlock(&this->mLock);
        return res >= (ssize_t)STATUS_OK ? (uint32_t)STATUS_OK : res;
    }
    pthread_mutex_unlock(&this->mLock);

    return STATUS_DEAD_OBJECT;
}

static uint32_t BpBinder_unlinkToDeath(BpBinder* this, DeathRecipient* recipient,
    void* cookie, uint32_t flags,
    DeathRecipient** outRecipient)
{
    IPCThreadState* self = IPCThreadState_self();

    pthread_mutex_lock(&this->mLock);
    if (this->mObitsSent) {
        pthread_mutex_unlock(&this->mLock);
        return STATUS_DEAD_OBJECT;
    }

    const size_t N = this->mObituaries ? this->mObituaries->size(this->mObituaries) : 0;
    for (size_t i = 0; i < N; i++) {
        Obituary* obit = this->mObituaries->get(this->mObituaries, i);
        if ((obit->recipient == recipient || (recipient == NULL && obit->cookie == cookie))
            && obit->flags == flags) {
            if (outRecipient != NULL) {
                *outRecipient = obit->recipient;
            }
            obit = this->mObituaries->removeItemAt(this->mObituaries, i);
            free(obit);
            if (this->mObituaries->size(this->mObituaries) == 0) {
                BINDER_LOGV("Clearing death notification: %p handle %" PRIi32 "\n",
                    this, this->binderHandle(this));
                self->clearDeathNotification(self, this->binderHandle(this), this);
                self->flushCommands(self);
                VectorImpl_delete(this->mObituaries);
                this->mObituaries = NULL;
            }
            pthread_mutex_unlock(&this->mLock);
            return STATUS_OK;
        }
    }
    pthread_mutex_unlock(&this->mLock);

    return STATUS_NAME_NOT_FOUND;
}

static void* BpBinder_attachObject(BpBinder* this, const void* objectID,
    void* object, void* cleanupCookie,
    object_cleanup_func func)
{
    void* p_ret;

    pthread_mutex_lock(&this->mLock);
    BINDER_LOGV("Attaching object %p to binder %p (manager=%p)",
        object, this, &this->mObjects);
    p_ret = this->mObjects.attach(&this->mObjects, objectID, object,
        cleanupCookie, func);
    pthread_mutex_unlock(&this->mLock);

    return p_ret;
}

static void* BpBinder_findObject(BpBinder* this, const void* objectID)
{
    void* p_ret;

    pthread_mutex_lock(&this->mLock);
    p_ret = this->mObjects.find(&this->mObjects, objectID);
    pthread_mutex_unlock(&this->mLock);
    return p_ret;
}

static void* BpBinder_detachObject(BpBinder* this, const void* objectID)
{
    void* p_ret;

    pthread_mutex_lock(&this->mLock);
    p_ret = this->mObjects.detach(&this->mObjects, objectID);
    pthread_mutex_unlock(&this->mLock);
    return p_ret;
}

BpBinder* BpBinder_remoteBinder(BpBinder* this)
{
    return this;
}

static void BpBinder_sendObituary(BpBinder* this)
{
    BINDER_LOGV("Sending obituary for proxy %p handle %" PRIi32 ", mObitsSent=%s\n",
        this, this->binderHandle(this),
        this->mObitsSent ? "true" : "false");
    this->mAlive = 0;
    if (this->mObitsSent)
        return;

    pthread_mutex_lock(&this->mLock);
    VectorImpl* obits = this->mObituaries;
    if (obits != NULL) {
        BINDER_LOGV("Clearing sent death notification: %p handle %" PRIi32 "\n",
            this, this->binderHandle(this));
        IPCThreadState* self = IPCThreadState_self();
        self->clearDeathNotification(self, this->binderHandle(this), this);
        self->flushCommands(self);
        this->mObituaries = NULL;
    }
    this->mObitsSent = 1;
    pthread_mutex_unlock(&this->mLock);

    BINDER_LOGV("Reporting death of proxy %p for %zu recipients\n",
        this, obits ? obits->size(obits) : 0U);

    if (obits != NULL) {
        const size_t N = obits->size(obits);
        for (size_t i = 0; i < N; i++) {
            this->reportOneDeath(this, obits->get(obits, i));
        }
        VectorImpl_delete(obits);
    }
}

static void BpBinder_reportOneDeath(BpBinder* this, const struct Obituary* obit)
{
    RefBase* ref = &obit->recipient->m_refbase;
    RefBase_weakref* weakref = ref->getWeakRefs(ref);
    weakref->attemptIncStrong(weakref, NULL);

    DeathRecipient* recipient = obit->recipient;
    BINDER_LOGV("Reporting death to recipient: %p\n", recipient);
    if (recipient == NULL)
        return;

    ref = &this->m_IBinder.m_refbase;
    weakref = ref->getWeakRefs(ref);
    weakref->incWeak(weakref, (void*)this);

    recipient->binderDied(recipient, &this->m_IBinder);
}

static void BpBinder_withLock(BpBinder* this, withLockcallback doWithLock)
{
    pthread_mutex_lock(&this->mLock);
    doWithLock((IBinder*)this);
    pthread_mutex_unlock(&this->mLock);
}

static void BpBinder_onFirstRef(BpBinder* this)
{
    IPCThreadState* self = IPCThreadState_self();

    BINDER_LOGV("onFirstRef BpBinder %p handle %" PRIi32 "\n", this, this->binderHandle(this));
    if (self) {
        self->incStrongHandle(self, this->binderHandle(this), this);
    }
}

static void BpBinder_onLastStrongRef(BpBinder* this, const void* id)
{
    IPCThreadState* self = IPCThreadState_self();

    BINDER_LOGV("onLastStrongRef BpBinder %p handle %" PRIi32 "\n", this, this->binderHandle(this));

    if (self) {
        self->decStrongHandle(self, this->binderHandle(this));
    }

    pthread_mutex_lock(&this->mLock);
    VectorImpl* obits = this->mObituaries;
    if (obits != NULL) {
        if (!obits->isEmpty(obits)) {
            BINDER_LOGI("onLastStrongRef automatically unlinking death recipients: %s",
                String_size(&this->mDescriptorCache) ? String_data(&this->mDescriptorCache) : "<uncached descriptor>");
        }

        if (self) {
            self->clearDeathNotification(self, this->binderHandle(this), this);
        }
        this->mObituaries = NULL;
    }
    pthread_mutex_unlock(&this->mLock);

    if (obits != NULL) {
        /* XXX Should we tell any remaining DeathRecipient
         * objects that the last strong ref has gone away, so they
         * are no longer linked?
         */

        VectorImpl_delete(obits);
    }
}

static bool BpBinder_onIncStrongAttempted(BpBinder* this, uint32_t flags, const void* id)
{
    IPCThreadState* self = IPCThreadState_self();

    BINDER_LOGV("onIncStrongAttempted BpBinder %p handle %" PRIi32 "\n",
        this, this->binderHandle(this));
    return self ? self->attemptIncStrongHandle(self, this->binderHandle(this)) == STATUS_OK : false;
}

static PrivateAccessor* BpBinder_getPrivateAccessor(BpBinder* this)
{
    return PrivateAccessor_new(this);
}

static void RefBase_Vfun_onFirstRef(RefBase* v_this)
{
    BpBinder* this = (BpBinder*)v_this;
    this->onFirstRef(this);
}

static void RefBase_Vfun_onLastStrongRef(RefBase* v_this, const void* id)
{
    BpBinder* this = (BpBinder*)v_this;
    this->onLastStrongRef(this, id);
}

static bool RefBase_Vfun_onIncStrongAttempted(RefBase* v_this, uint32_t flags, const void* id)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->onIncStrongAttempted(this, flags, id);
}

static String* BpBinder_Vfun_getInterfaceDescriptor(IBinder* v_this)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->getInterfaceDescriptor(this);
}

static bool BpBinder_Vfun_isBinderAlive(IBinder* v_this)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->isBinderAlive(this);
}

static uint32_t BpBinder_Vfun_pingBinder(IBinder* v_this)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->pingBinder(this);
}

static uint32_t BpBinder_Vfun_dump(IBinder* v_this, int fd, const VectorString* args)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->dump(this, fd, args);
}

static uint32_t BpBinder_Vfun_transact(IBinder* v_this, uint32_t code,
    const Parcel* data, Parcel* reply,
    uint32_t flags)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->transact(this, code, data, reply, flags);
}

static uint32_t BpBinder_Vfun_linkToDeath(IBinder* v_this, DeathRecipient* recipient,
    void* cookie, uint32_t flags)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->linkToDeath(this, recipient, cookie, flags);
}

static uint32_t BpBinder_Vfun_unlinkToDeath(IBinder* v_this, DeathRecipient* recipient,
    void* cookie, uint32_t flags,
    DeathRecipient** outRecipient)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->unlinkToDeath(this, recipient, cookie, flags, outRecipient);
}

static void* BpBinder_Vfun_attachObject(IBinder* v_this, const void* objectID,
    void* object, void* cleanupCookie,
    object_cleanup_func func)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->attachObject(this, objectID, object, cleanupCookie, func);
}

static void* BpBinder_Vfun_findObject(IBinder* v_this, const void* objectID)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->findObject(this, objectID);
}

static void* BpBinder_Vfun_detachObject(IBinder* v_this, const void* objectID)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->detachObject(this, objectID);
}

BpBinder* BpBinder_Vfun_remoteBinder(IBinder* v_this)
{
    BpBinder* this = (BpBinder*)v_this;
    return this->remoteBinder(this);
}

static void BpBinder_dtor(BpBinder* this)
{
    IPCThreadState* self = IPCThreadState_self();
    BpBinder_global* global = BpBinder_global_get();
    uint32_t tempValue;

    BINDER_LOGV("Destroying BpBinder %p handle %" PRIi32 "\n",
        this, this->binderHandle(this));

    if (this->mTrackedUid >= 0) {
        pthread_mutex_lock(&global->sTrackingLock);
        uint32_t trackedValue = global->sTrackingMap.get(&global->sTrackingMap, this->mTrackedUid);

        if ((trackedValue & COUNTING_VALUE_MASK) == 0) {
            BINDER_LOGE("Unexpected Binder Proxy tracking decrement in %p handle %" PRIi32 "\n", this,
                this->binderHandle(this));
        } else {
            if ((trackedValue & LIMIT_REACHED_MASK) && ((trackedValue & COUNTING_VALUE_MASK) <= global->sBinderProxyCountLowWatermark)) {
                BINDER_LOGI("Limit reached bit reset for uid %d (fewer than %" PRIu32 " proxies from uid %" PRIu32 " held)",
                    getuid(), global->sBinderProxyCountLowWatermark, this->mTrackedUid);
                tempValue = global->sTrackingMap.get(&global->sTrackingMap, this->mTrackedUid);
                tempValue &= ~LIMIT_REACHED_MASK;
                global->sTrackingMap.put(&global->sTrackingMap, this->mTrackedUid, tempValue);
                global->sLastLimitCallbackMap.erase(&global->sLastLimitCallbackMap, this->mTrackedUid);
            }
            tempValue = global->sTrackingMap.get(&global->sTrackingMap, this->mTrackedUid);
            if (--tempValue == 0) {
                global->sTrackingMap.erase(&global->sTrackingMap, this->mTrackedUid);
            } else {
                global->sTrackingMap.put(&global->sTrackingMap, this->mTrackedUid, tempValue);
            }
        }
        pthread_mutex_unlock(&global->sTrackingLock);
    }

    if (self) {
        self->expungeHandle(self, this->binderHandle(this), (IBinder*)this);
        self->decWeakHandle(self, this->binderHandle(this));
    }
    this->mObjects.dtor(&this->mObjects);
    this->m_IBinder.dtor(&this->m_IBinder);
    pthread_mutex_destroy(&this->mLock);
}

static void BpBinder_ctor(BpBinder* this, int32_t handle, int32_t trackedUid)
{
    IBinder* ibinder = &this->m_IBinder;
    RefBase* refbase = &this->m_IBinder.m_refbase;
    IPCThreadState* self = IPCThreadState_self();

    IBinder_ctor(&this->m_IBinder);
    ObjectManager_ctor(&this->mObjects);

    /* Override IBinder virtual function */
    ibinder->isBinderAlive = BpBinder_Vfun_isBinderAlive;
    ibinder->pingBinder = BpBinder_Vfun_pingBinder;
    ibinder->getInterfaceDescriptor = BpBinder_Vfun_getInterfaceDescriptor;
    ibinder->transact = BpBinder_Vfun_transact;
    ibinder->linkToDeath = BpBinder_Vfun_linkToDeath;
    ibinder->unlinkToDeath = BpBinder_Vfun_unlinkToDeath;
    ibinder->dump = BpBinder_Vfun_dump;
    ibinder->remoteBinder = BpBinder_Vfun_remoteBinder;
    ibinder->attachObject = BpBinder_Vfun_attachObject;
    ibinder->findObject = BpBinder_Vfun_findObject;
    ibinder->detachObject = BpBinder_Vfun_detachObject;

    /* Override RefBase virtual function */
    refbase->onFirstRef = RefBase_Vfun_onFirstRef;
    refbase->onLastStrongRef = RefBase_Vfun_onLastStrongRef;
    refbase->onIncStrongAttempted = RefBase_Vfun_onIncStrongAttempted;

    /* Virtual function for IBinder */
    this->isBinderAlive = BpBinder_isBinderAlive;
    this->pingBinder = BpBinder_pingBinder;
    this->getInterfaceDescriptor = BpBinder_getInterfaceDescriptor;
    this->transact = BpBinder_transact;
    this->linkToDeath = BpBinder_linkToDeath;
    this->unlinkToDeath = BpBinder_unlinkToDeath;
    this->dump = BpBinder_dump;
    this->remoteBinder = BpBinder_remoteBinder;
    this->attachObject = BpBinder_attachObject;
    this->findObject = BpBinder_findObject;
    this->detachObject = BpBinder_detachObject;

    /* Virtual Function for RefBase */
    this->getWeakRefs = BpBinder_getWeakRefs;
    this->onFirstRef = BpBinder_onFirstRef;
    this->onLastStrongRef = BpBinder_onLastStrongRef;
    this->onIncStrongAttempted = BpBinder_onIncStrongAttempted;

    /* Member function */
    this->sendObituary = BpBinder_sendObituary;
    this->getDebugBinderHandle = BpBinder_getDebugBinderHandle;
    this->binderHandle = BpBinder_binderHandle;
    this->reportOneDeath = BpBinder_reportOneDeath;
    this->isDescriptorCached = BpBinder_isDescriptorCached;
    this->withLock = BpBinder_withLock;
    this->getPrivateAccessor = BpBinder_getPrivateAccessor;

    pthread_mutex_init(&this->mLock, NULL);
    this->binder_handle = handle;
    this->mStability = 0;
    this->mAlive = true;
    this->mObitsSent = false;
    this->mObituaries = NULL;
    this->mTrackedUid = trackedUid;
    refbase->extendObjectLifetime(refbase, OBJECT_LIFETIME_WEAK);

    BINDER_LOGD("Creating BpBinder %p handle %" PRIi32 "\n", this, this->binderHandle(this));
    self->incWeakHandle(self, this->binderHandle(this), this);
    this->dtor = BpBinder_dtor;
}

void BpBinder_delete(BpBinder* this)
{
    this->dtor(this);
    free(this);
}

BpBinder* BpBinder_new(int32_t handle, int32_t trackedUid)
{
    BpBinder* this;
    this = zalloc(sizeof(BpBinder));

    BpBinder_ctor(this, handle, trackedUid);
    return this;
}

BpBinder* BpBinder_create(int32_t handle)
{
    BpBinder_global* global = BpBinder_global_get();
    int32_t trackedUid = -1;
    uint32_t tempValue;

    if (global->sCountByUidEnabled) {
        IPCThreadState* ts = IPCThreadState_self();
        trackedUid = ts->getCallingUid(ts);
        pthread_mutex_lock(&global->sTrackingLock);
        uint32_t trackedValue = global->sTrackingMap.get(&global->sTrackingMap, trackedUid);
        if (trackedValue & LIMIT_REACHED_MASK) {
            if (global->sBinderProxyThrottleCreate) {
                pthread_mutex_unlock(&global->sTrackingLock);
                return NULL;
            }
            trackedValue = trackedValue & COUNTING_VALUE_MASK;
            uint32_t lastLimitCallbackAt = global->sLastLimitCallbackMap.get(&global->sLastLimitCallbackMap, trackedUid);

            if ((trackedValue > lastLimitCallbackAt) && (trackedValue - lastLimitCallbackAt > global->sBinderProxyCountHighWatermark)) {
                BINDER_LOGD("Still too many binder proxy objects sent "
                            "to uid %d from uid %" PRIi32 " (%" PRIu32 " proxies held)",
                    getuid(), trackedUid, trackedValue);
                if (global->sLimitCallback) {
                    global->sLimitCallback(trackedUid);
                }
                global->sLastLimitCallbackMap.put(&global->sLastLimitCallbackMap, trackedUid, trackedValue);
            }
        } else {
            if ((trackedValue & COUNTING_VALUE_MASK) >= global->sBinderProxyCountHighWatermark) {
                BINDER_LOGD("Too many binder proxy objects sent "
                            "to uid %d from uid %" PRIi32 " (%" PRIu32 " proxies held)",
                    getuid(), trackedUid, trackedValue);
                tempValue = global->sTrackingMap.get(&global->sTrackingMap, trackedUid);
                tempValue |= LIMIT_REACHED_MASK;
                global->sTrackingMap.put(&global->sTrackingMap, trackedUid, tempValue);
                if (global->sLimitCallback) {
                    global->sLimitCallback(trackedUid);
                }
                global->sLastLimitCallbackMap.put(&global->sLastLimitCallbackMap,
                    trackedUid, trackedValue & COUNTING_VALUE_MASK);
                if (global->sBinderProxyThrottleCreate) {
                    BINDER_LOGD("Throttling binder proxy creates from "
                                "uid %" PRIi32 " in uid %d until binder proxy count drops below %" PRIu32 "",
                        trackedUid, getuid(), global->sBinderProxyCountLowWatermark);
                    pthread_mutex_unlock(&global->sTrackingLock);
                    return NULL;
                }
            }
        }
        tempValue = global->sTrackingMap.get(&global->sTrackingMap, trackedUid);
        tempValue++;
        global->sTrackingMap.put(&global->sTrackingMap, trackedUid, tempValue);
        pthread_mutex_unlock(&global->sTrackingLock);
    }

    return BpBinder_new(handle, trackedUid);
}
