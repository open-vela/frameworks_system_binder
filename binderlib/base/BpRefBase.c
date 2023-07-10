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

#define LOG_TAG "BpRefBase"

#include <android/binder_status.h>
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

enum {
    // This is used to transfer ownership of the remote binder from
    // the BpRefBase object holding it (when it is constructed), to the
    // owner of the BpRefBase object when it first acquires that BpRefBase.
    kRemoteAcquired = 0x00000001
};

static void BpRefBase_onFirstRef(BpRefBase* this)
{
    atomic_fetch_or_explicit(&this->mState, kRemoteAcquired, memory_order_relaxed);
}

static void BpRefBase_onLastStrongRef(BpRefBase* this, const void* id)
{
    if (this->mRemote) {
        this->mRemote->decStrong(this->mRemote, this);
    }
}

static bool BpRefBase_onIncStrongAttempted(BpRefBase* this, uint32_t flags,
    const void* id)
{
    return this->mRemote ? this->mRefs->attemptIncStrong(this->mRefs, (const void*)this) : false;
}

static IBinder* BpRefBase_remote(BpRefBase* this)
{
    return this->mRemote;
}

IBinder* BpRefBase_remoteStrong(BpRefBase* this)
{
    if (this->mRemote) {
        this->mRemote->m_refbase.incStrongRequireStrong(&this->mRemote->m_refbase, (const void*)this->mRemote);
    }
    return this->mRemote;
}

static void RefBase_Vfun_onFirstRef(RefBase* v_this)
{
    BpRefBase* this = (BpRefBase*)v_this;
    this->onFirstRef(this);
}

static void RefBase_Vfun_onLastStrongRef(RefBase* v_this, const void* id)
{
    BpRefBase* this = (BpRefBase*)v_this;
    this->onLastStrongRef(this, id);
}

static bool RefBase_Vfun_onIncStrongAttempted(RefBase* v_this, uint32_t flags,
    const void* id)
{
    BpRefBase* this = (BpRefBase*)v_this;
    return this->onIncStrongAttempted(this, flags, id);
}

void BpRefBase_dtor(BpRefBase* this)
{
    if (this->mRemote) {
        if (!(atomic_load_explicit(&this->mState, memory_order_relaxed) & kRemoteAcquired)) {
            this->mRemote->decStrong(this->mRemote, (const void*)this);
        }
        this->mRefs->decWeak(this->mRefs, (const void*)this);
    }

    this->m_refbase.dtor(&this->m_refbase);
}

void BpRefBase_ctor(BpRefBase* this, IBinder* ibinder)
{
    RefBase* refbase = &this->m_refbase;

    RefBase_ctor(refbase);

    this->mRemote = ibinder;
    this->mRefs = NULL;
    this->mState = 0;

    /* Override Pure Virtual function in IBinder */
    refbase->onFirstRef = RefBase_Vfun_onFirstRef;
    refbase->onLastStrongRef = RefBase_Vfun_onLastStrongRef;
    refbase->onIncStrongAttempted = RefBase_Vfun_onIncStrongAttempted;

    /* Override Pure Virtual function in IBinder */
    this->onFirstRef = BpRefBase_onFirstRef;
    this->onLastStrongRef = BpRefBase_onLastStrongRef;
    this->onIncStrongAttempted = BpRefBase_onIncStrongAttempted;

    this->remote = BpRefBase_remote;
    this->remoteStrong = BpRefBase_remoteStrong;

    refbase->extendObjectLifetime(refbase, OBJECT_LIFETIME_WEAK);

    if (this->mRemote) {
        refbase = &this->mRemote->m_refbase;
        refbase->incStrong(refbase, (const void*)this);
        this->mRefs = refbase->createWeak(refbase, (const void*)this);
    }

    this->dtor = BpRefBase_dtor;
}
