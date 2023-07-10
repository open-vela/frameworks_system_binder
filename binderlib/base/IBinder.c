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

#define LOG_TAG "IBinder"

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <inttypes.h>
#include <nuttx/sched.h>
#include <stdio.h>

#include "Binder.h"
#include "BpBinder.h"
#include "IBinder.h"
#include "IInterface.h"
#include "IPCThreadState.h"
#include "Parcel.h"
#include "utils/Binderlog.h"
#include <android/binder_status.h>

static IInterface* IBinder_queryLocalInterface(IBinder* this, String* descriptor)
{
    return NULL;
}

static bool IBinder_checkSubclass(IBinder* this, const void* subclassID)
{
    return false;
}

static BBinder* IBinder_localBinder(IBinder* this)
{
    return NULL;
}

static BpBinder* IBinder_remoteBinder(IBinder* this)
{
    return NULL;
}

static uint32_t IBinder_getExtension(IBinder* this, IBinder** out)
{
    BBinder* local = this->localBinder(this);
    if (local != NULL) {
        *out = local->getExtension(local);
        return STATUS_OK;
    }

    BpBinder* proxy = this->remoteBinder(this);
    LOG_FATAL_IF(proxy == NULL, "binder object must be either local or remote");

    Parcel data;
    Parcel reply;

    Parcel_initState(&data);
    Parcel_initState(&reply);

    uint32_t status = this->transact(this, EXTENSION_TRANSACTION,
        &data, &reply, 0);
    if (status != STATUS_OK) {
        return status;
    }
    return Parcel_readNullableStrongBinder(&reply, out);
}

static uint32_t IBinder_getDebugPid(IBinder* this, pid_t* out)
{
    BBinder* local = this->localBinder(this);
    if (local != NULL) {
        *out = local->getDebugPid(local);
        return 0;
    }
    BpBinder* proxy = this->remoteBinder(this);

    CHECK_FAIL(proxy == NULL)

    Parcel data;
    Parcel reply;

    Parcel_initState(&data);
    Parcel_initState(&reply);

    uint32_t status = this->transact(this, DEBUG_PID_TRANSACTION,
        &data, &reply, 0);
    if (status != STATUS_OK) {
        return status;
    }

    int32_t pid;
    status = Parcel_readInt32(&reply, &pid);

    if (status != STATUS_OK) {
        return status;
    }

    if (pid < 0) {
        return STATUS_BAD_VALUE;
    }
    *out = pid;
    return STATUS_OK;
}

static void IBinder_withLock(IBinder* this, withLockcallback doWithLock)
{
    BBinder* local = this->localBinder(this);
    if (local) {
        local->withLock(local, doWithLock);
        return;
    }

    BpBinder* proxy = this->remoteBinder(this);
    LOG_FATAL_IF(proxy == NULL, "binder object must be either local or remote");
    proxy->withLock(proxy, doWithLock);
}

void IBinder_incStrong(IBinder* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->incStrong(refbase, id);
}

void IBinder_incStrongRequireStrong(IBinder* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->incStrongRequireStrong(refbase, id);
}

void IBinder_decStrong(IBinder* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->decStrong(refbase, id);
}

void IBinder_forceIncStrong(IBinder* this, const void* id)
{
    RefBase* refbase = &this->m_refbase;
    return refbase->forceIncStrong(refbase, id);
}

static void IBinder_dtor(IBinder* this)
{
    this->m_refbase.dtor(&this->m_refbase);
}

void IBinder_ctor(IBinder* this)
{
    RefBase_ctor(&this->m_refbase);

    /* Virtual Function for RefBase */
    this->incStrong = IBinder_incStrong;
    this->incStrongRequireStrong = IBinder_incStrongRequireStrong;
    this->decStrong = IBinder_decStrong;
    this->forceIncStrong = IBinder_forceIncStrong;

    /* Virtual Function */
    this->queryLocalInterface = IBinder_queryLocalInterface;
    this->localBinder = IBinder_localBinder;
    this->remoteBinder = IBinder_remoteBinder;
    this->checkSubclass = IBinder_checkSubclass;

    /* Member function */
    this->getExtension = IBinder_getExtension;
    this->getDebugPid = IBinder_getDebugPid;
    this->withLock = IBinder_withLock;

    this->dtor = IBinder_dtor;
}

static void DeathRecipient_dtor(DeathRecipient* this)
{
    this->m_refbase.dtor(&this->m_refbase);
}

void DeathRecipient_ctor(DeathRecipient* this)
{
    RefBase_ctor(&this->m_refbase);

    this->binderDied = NULL;
    this->dtor = DeathRecipient_dtor;
}