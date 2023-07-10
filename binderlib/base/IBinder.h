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

#ifndef __BINDER_INCLUDE_BINDER_IBINDER_H__
#define __BINDER_INCLUDE_BINDER_IBINDER_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/android/binder.h>

#include "utils/BinderString.h"
#include <utils/RefBase.h>
#include <utils/Vector.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct Parcel;
typedef struct Parcel Parcel;

struct IBinder;
typedef struct IBinder IBinder;

struct BBinder;
typedef struct BBinder BBinder;

struct BpBinder;
typedef struct BpBinder BpBinder;

struct IInterface;
typedef struct IInterface IInterface;

typedef void (*object_cleanup_func)(const void* id, void* obj, void* cleanupCookie);
typedef void (*binder_proxy_limit_callback)(int val);
typedef void (*withLockcallback)(IBinder* this);

enum {
    FIRST_CALL_TRANSACTION = 0x00000001,

    LAST_CALL_TRANSACTION = 0x00ffffff,
    PING_TRANSACTION = B_PACK_CHARS('_', 'P', 'N', 'G'),
    DUMP_TRANSACTION = B_PACK_CHARS('_', 'D', 'M', 'P'),
    SHELL_COMMAND_TRANSACTION = B_PACK_CHARS('_', 'C', 'M', 'D'),
    INTERFACE_TRANSACTION = B_PACK_CHARS('_', 'N', 'T', 'F'),
    SYSPROPS_TRANSACTION = B_PACK_CHARS('_', 'S', 'P', 'R'),
    EXTENSION_TRANSACTION = B_PACK_CHARS('_', 'E', 'X', 'T'),
    DEBUG_PID_TRANSACTION = B_PACK_CHARS('_', 'P', 'I', 'D'),
    SET_RPC_CLIENT_TRANSACTION = B_PACK_CHARS('_', 'R', 'P', 'C'),
    TWEET_TRANSACTION = B_PACK_CHARS('_', 'T', 'W', 'T'),
    LIKE_TRANSACTION = B_PACK_CHARS('_', 'L', 'I', 'K'),

    FLAG_ONEWAY = 0x00000001,
    FLAG_CLEAR_BUF = 0x00000020,
    FLAG_PRIVATE_VENDOR = 0x10000000,
};

struct DeathRecipient;
typedef struct DeathRecipient DeathRecipient;

struct DeathRecipient {

    struct RefBase m_refbase;

    void (*dtor)(DeathRecipient* this);

    /* Pure Virtual Function */
    void (*binderDied)(DeathRecipient* v_this, IBinder* who);
};

void DeathRecipient_ctor(DeathRecipient* this);

/**
 * Base class and low-level protocol for a remotable object.
 * You can derive from this class to create an object for which other
 * processes can hold references to it.  Communication between processes
 * (method calls, property get and set) is down through a low-level
 * protocol implemented on top of the transact() API.
 */

/* Abstract Class (Interface Binder) */

struct IBinder {
    struct RefBase m_refbase;

    void (*dtor)(IBinder* this);

    /* Virtual Function for RefBase */
    void (*incStrong)(IBinder* this, const void* id);
    void (*incStrongRequireStrong)(IBinder* this, const void* id);
    void (*decStrong)(IBinder* this, const void* id);
    void (*forceIncStrong)(IBinder* this, const void* id);

    /* Virtual Function */
    IInterface* (*queryLocalInterface)(IBinder* this, String* descriptor);
    bool (*checkSubclass)(IBinder* this, const void* subclassID);
    BBinder* (*localBinder)(IBinder* this);
    BpBinder* (*remoteBinder)(IBinder* this);

    /* Pure Virtual Function */

    String* (*getInterfaceDescriptor)(IBinder* this);
    bool (*isBinderAlive)(IBinder* this);
    uint32_t (*pingBinder)(IBinder* this);
    uint32_t (*dump)(IBinder* this, int fd, const VectorString* args);
    uint32_t (*transact)(IBinder* this, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags /* = 0 */);
    uint32_t (*linkToDeath)(IBinder* this, DeathRecipient* recipient,
        void* cookie, uint32_t flags);
    uint32_t (*unlinkToDeath)(IBinder* this, DeathRecipient* recipient,
        void* cookie, uint32_t flags,
        DeathRecipient** outRecipient);
    void* (*attachObject)(IBinder* this, const void* objectID, void* object, void* cleanupCookie,
        object_cleanup_func func);
    void* (*findObject)(IBinder* this, const void* objectID);
    void* (*detachObject)(IBinder* this, const void* objectID);

    /* member function */
    uint32_t (*getExtension)(IBinder* this, IBinder** out);
    uint32_t (*getDebugPid)(IBinder* this, pid_t* outPid);
    void (*withLock)(IBinder* this, withLockcallback doWithLock);
};

void IBinder_ctor(IBinder* this);

#endif /* __BINDER_INCLUDE_BINDER_IBINDER_H__ */
