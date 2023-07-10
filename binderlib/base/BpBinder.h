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

#ifndef __BINDER_INCLUDE_BINDER_BPBINDER_H__
#define __BINDER_INCLUDE_BINDER_BPBINDER_H__

#include <pthread.h>
#include <stdbool.h>

#include "IBinder.h"
#include "utils/HashMap.h"
#include "utils/RefBase.h"
#include "utils/Vector.h"

struct ObjectEntry {
    void* object;
    void* cleanupCookie;
    object_cleanup_func func;
};

typedef struct ObjectEntry ObjectEntry;

struct ObjectManager;
typedef struct ObjectManager ObjectManager;

struct ObjectManager {
    void (*dtor)(ObjectManager* this);

    void* (*attach)(ObjectManager* this, const void* objectID, void* object,
        void* cleanupCookie, object_cleanup_func func);
    void* (*find)(ObjectManager* this, const void* objectID);
    void* (*detach)(ObjectManager* this, const void* objectID);
    void (*kill)(ObjectManager* this);

    struct HashMap mObjects;
};

void ObjectManager_ctor(ObjectManager* this);

struct PrivateAccessor;
typedef struct PrivateAccessor PrivateAccessor;

struct PrivateAccessor {
    void (*dtor)(PrivateAccessor* this);
    int32_t (*binderHandle)(PrivateAccessor* this);

    BpBinder* mBinder;
};

PrivateAccessor* PrivateAccessor_new(BpBinder* binder);
void PrivateAccessor_delete(PrivateAccessor* this);
BpBinder* PrivateAccessor_create(int32_t handle);

struct Obituary {
    DeathRecipient* recipient;
    void* cookie;
    uint32_t flags;
};
typedef struct Obituary Obituary;

struct BpBinder {
    struct IBinder m_IBinder;

    void (*dtor)(BpBinder* this);

    /* Virtual Function for RefBase */
    RefBase_weakref* (*getWeakRefs)(BpBinder* this);
    void (*onFirstRef)(BpBinder* this);
    void (*onLastStrongRef)(BpBinder* this, const void* id);
    bool (*onIncStrongAttempted)(BpBinder* this, uint32_t flags, const void* id);
    void (*incStrong)(BpBinder* this, const void* id);
    void (*incStrongRequireStrong)(BpBinder* this, const void* id);
    void (*decStrong)(BpBinder* this, const void* id);
    void (*forceIncStrong)(BpBinder* this, const void* id);

    /* Virtual function for IBinder */
    BpBinder* (*remoteBinder)(BpBinder* this);
    String* (*getInterfaceDescriptor)(BpBinder* this);
    bool (*isBinderAlive)(BpBinder* this);
    uint32_t (*pingBinder)(BpBinder* this);
    uint32_t (*dump)(BpBinder* this, int fd, const VectorString* args);
    uint32_t (*transact)(BpBinder* this, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags /* = 0 */);
    uint32_t (*linkToDeath)(BpBinder* this, DeathRecipient* recipient,
        void* cookie, uint32_t flags);
    uint32_t (*unlinkToDeath)(BpBinder* this, DeathRecipient* recipient,
        void* cookie, uint32_t flags,
        DeathRecipient** outRecipient);
    void* (*attachObject)(BpBinder* this, const void* objectID, void* object, void* cleanupCookie,
        object_cleanup_func func);
    void* (*findObject)(BpBinder* this, const void* objectID);
    void* (*detachObject)(BpBinder* this, const void* objectID);

    /* Member Function */
    void (*sendObituary)(BpBinder* this);
    int32_t (*getDebugBinderHandle)(BpBinder* this);
    int32_t (*binderHandle)(BpBinder* this);
    void (*reportOneDeath)(BpBinder* this, const struct Obituary* obit);
    bool (*isDescriptorCached)(BpBinder* this);
    void (*withLock)(BpBinder* this, withLockcallback doWithLock);
    PrivateAccessor* (*getPrivateAccessor)(BpBinder* this);

    int32_t binder_handle;
    int32_t mStability;
    pthread_mutex_t mLock;
    volatile int32_t mAlive;
    volatile int32_t mObitsSent;
    VectorImpl* mObituaries;
    ObjectManager mObjects;
    String mDescriptorCache;
    int32_t mTrackedUid;
};

BpBinder* BpBinder_new(int32_t handle, int32_t trackedUid);
void BpBinder_delete(BpBinder* this);

BpBinder* BpBinder_create(int32_t handle);

#endif /* __BINDER_INCLUDE_BINDER_BPBINDER_H__ */
