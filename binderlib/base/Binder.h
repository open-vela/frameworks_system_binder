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

#ifndef __BINDER_INCLUDE_BINDER_BINDER_H__
#define __BINDER_INCLUDE_BINDER_BINDER_H__

#include "BpBinder.h"
#include "IBinder.h"
#include "Parcel.h"
#include "utils/RefBase.h"
#include "utils/Vector.h"

struct BBinder_Extras;
typedef struct BBinder_Extras BBinder_Extras;

struct BBinder_Extras {
    void (*dtor)(BBinder_Extras* this);

    IBinder* mExtension;
    int mPolicy;
    int mPriority;
    bool mRequestingSid;
    bool mInheritRt;
    pthread_mutex_t mLock;
    ObjectManager mObjects;
};

typedef _Atomic(BBinder_Extras*) Atomic_BBinder_Extras_ptr;

BBinder_Extras* BBinder_Extras_new(void);
void BBinder_Extras_delete(BBinder_Extras* this);

/*
uint32_t BBinder_Vfun_onTransact(void *v_this, uint32_t code,
                                 const struct Parcel *in_data,
                                 struct Parcel* reply, uint32_t flags);
*/

struct BBinder {
    struct IBinder m_IBinder;
    void (*dtor)(BBinder* this);

    /* Virtual Function for RefBase */

    void (*incStrong)(BBinder* this, const void* id);
    void (*decStrong)(BBinder* this, const void* id);
    RefBase_weakref* (*createWeak)(BBinder* this, const void* id);
    RefBase_weakref* (*getWeakRefs)(BBinder* this);
    void (*printRefs)(BBinder* this);

    /* Virtual Function for IBinder */

    BBinder* (*localBinder)(BBinder* this);
    uint32_t (*onTransact)(BBinder* this, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags);
    uint32_t (*transact)(BBinder* this, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags /* = 0 */);
    String* (*getInterfaceDescriptor)(BBinder* this);
    bool (*isBinderAlive)(BBinder* this);
    uint32_t (*pingBinder)(BBinder* this);
    uint32_t (*dump)(BBinder* this, int fd, const VectorString* args);
    uint32_t (*linkToDeath)(BBinder* this, DeathRecipient* recipient,
        void* cookie, uint32_t flags);
    uint32_t (*unlinkToDeath)(BBinder* this, DeathRecipient* recipient,
        void* cookie, uint32_t flags,
        DeathRecipient** outRecipient);
    void* (*attachObject)(BBinder* this, const void* objectID, void* object, void* cleanupCookie,
        object_cleanup_func func);
    void* (*findObject)(BBinder* this, const void* objectID);
    void* (*detachObject)(BBinder* this, const void* objectID);

    /* member function of BBinder */
    void (*withLock)(BBinder* this, withLockcallback doWithLock);
    bool (*isRequestingSid)(BBinder* this);
    void (*setRequestingSid)(BBinder* this, bool requestSid);
    IBinder* (*getExtension)(BBinder* this);
    void (*setExtension)(BBinder* this, IBinder* extension);
    void (*setMinSchedulerPolicy)(BBinder* this, int policy, int priority);
    int (*getMinSchedulerPolicy)(BBinder* this);
    int (*getMinSchedulerPriority)(BBinder* this);
    bool (*isInheritRt)(BBinder* this);
    void (*setInheritRt)(BBinder* this, bool inheritRt);
    pid_t (*getDebugPid)(BBinder* this);
    bool (*wasParceled)(BBinder* this);
    void (*setParceled)(BBinder* this);
    BBinder_Extras* (*getOrCreateExtras)(BBinder* this);

    Atomic_BBinder_Extras_ptr mExtras;
    int16_t mStability;
    bool mParceled;
};

void BBinder_ctor(BBinder* this);

struct BpRefBase;
typedef struct BpRefBase BpRefBase;

struct BpRefBase {
    struct RefBase m_refbase;
    void (*dtor)(BpRefBase* this);

    IBinder* (*remote)(BpRefBase* this);
    IBinder* (*remoteStrong)(BpRefBase* this);

    /* Virtual Function at m_refbase */
    void (*onFirstRef)(BpRefBase* this);
    void (*onLastStrongRef)(BpRefBase* this, const void* id);
    bool (*onIncStrongAttempted)(BpRefBase* this, uint32_t flags, const void* id);

    IBinder* mRemote;
    RefBase_weakref* mRefs;
    atomic_uint mState;
};

void BpRefBase_ctor(BpRefBase* this, IBinder* ibinder);

#endif /* __BINDER_INCLUDE_BINDER_BINDER_H__ */
