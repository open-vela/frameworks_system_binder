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

#ifndef __BINDER_INCLUDE_BINDER_ISERVICECALLBACK_H__
#define __BINDER_INCLUDE_BINDER_ISERVICECALLBACK_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "IBinder.h"
#include "IInterface.h"
#include "Status.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * IServiceCallback
 ****************************************************************************/

struct IServiceCallback;
typedef struct IServiceCallback IServiceCallback;

struct IServiceCallback {
    struct IInterface m_iface;

    void (*dtor)(IServiceCallback* this);

    /* Virtual Function */
    String* (*getInterfaceDescriptor)(IServiceCallback* v_this);

    /* Pure Virtual Function */
    int32_t (*onRegistration)(void* v_this, const String* name, const IBinder* binder);

    String descriptor;
};

void IServiceCallback_ctor(IServiceCallback* this);

IServiceCallback* IServiceCallback_asInterface(const IBinder* obj);
bool IServiceCallback_setDefaultImpl(IServiceCallback* impl);
IServiceCallback* IServiceCallback_getDefaultImpl(void);

/****************************************************************************
 * BnServiceCallback
 ****************************************************************************/

struct BnInterface_IServiceCallback;
typedef struct BnInterface_IServiceCallback BnInterface_IServiceCallback;

struct BnInterface_IServiceCallback {
    IServiceCallback m_IServiceCallback;
    BBinder m_BBinder;

    void (*dtor)(BnInterface_IServiceCallback* this);

    IInterface* (*queryLocalInterface)(BnInterface_IServiceCallback* this,
        const String* _descriptor);
    String* (*getInterfaceDescriptor)(BnInterface_IServiceCallback* this);
    IBinder* (*onAsBinder)(BnInterface_IServiceCallback* this);
};

void BnInterface_IServiceCallback_ctor(BnInterface_IServiceCallback* this);

enum {
    BnServiceCallback_TRANSACTION_onRegistration = FIRST_CALL_TRANSACTION + 0,
};

struct BnServiceCallback;
typedef struct BnServiceCallback BnServiceCallback;

struct BnServiceCallback {
    BnInterface_IServiceCallback m_BnIServiceCallback;

    void (*dtor)(BnServiceCallback* this);

    /* Virtual function */
    uint32_t (*onTransact)(BnServiceCallback* v_this, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags);
};

void BnServiceCallback_ctor(BnServiceCallback* this);

/****************************************************************************
 * BpServiceCallback
 ****************************************************************************/

struct BpInterface_IServiceCallback;
typedef struct BpInterface_IServiceCallback BpInterface_IServiceCallback;

struct BpInterface_IServiceCallback {
    IServiceCallback m_IServiceCallback;
    BpRefBase m_BpRefBase;

    void (*dtor)(BpInterface_IServiceCallback* this);

    IBinder* (*onAsBinder)(BpInterface_IServiceCallback* this);
};

void BpInterface_IServiceCallback_ctor(BpInterface_IServiceCallback* this,
    IBinder* remote);

struct BpServiceCallback;
typedef struct BpServiceCallback BpServiceCallback;

struct BpServiceCallback {
    BpInterface_IServiceCallback m_BpIClientCallback;

    void (*dtor)(BpServiceCallback* this);

    /* Virtual function*/
    uint32_t (*onRegistration)(BpServiceCallback* this, const String* name,
        IBinder* binder);
};

BpServiceCallback* BpServiceCallback_new(IBinder* impl);

#endif /* __BINDER_INCLUDE_BINDER_ISERVICECALLBACK_H__ */
