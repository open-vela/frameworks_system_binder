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

#ifndef __BINDER_INCLUDE_BINDER_ICLIENTCALLBACK_H__
#define __BINDER_INCLUDE_BINDER_ICLIENTCALLBACK_H__

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

struct IClientCallback;
typedef struct IClientCallback IClientCallback;

struct IClientCallback {
    struct IInterface m_iface;
    void (*dtor)(IClientCallback* this);

    /* Virtual Function */
    String* (*getInterfaceDescriptor)(IClientCallback* v_this);

    /* Pure Virtual Function */
    int32_t (*onClients)(void* v_this, const IBinder* registered, bool hasClients);

    String descriptor;
};

void IClientCallback_ctor(IClientCallback* this);

IClientCallback* IClientCallback_asInterface(const IBinder* obj);
bool IClientCallback_setDefaultImpl(IClientCallback* impl);
IClientCallback* IClientCallback_getDefaultImpl(void);

/****************************************************************************
 * BnClientCallback
 ****************************************************************************/

struct BnInterface_IClientCallback;
typedef struct BnInterface_IClientCallback BnInterface_IClientCallback;

struct BnInterface_IClientCallback {
    IClientCallback m_IClientCallback;
    BBinder m_BBinder;

    void (*dtor)(BnInterface_IClientCallback* this);

    IInterface* (*queryLocalInterface)(BnInterface_IClientCallback* this,
        const String* _descriptor);
    String* (*getInterfaceDescriptor)(BnInterface_IClientCallback* this);
    IBinder* (*onAsBinder)(BnInterface_IClientCallback* this);
};

void BnInterface_IClientCallback_ctor(BnInterface_IClientCallback* this);

enum {
    BnClientCallback_TRANSACTION_onClients = FIRST_CALL_TRANSACTION + 0,
};

struct BnClientCallback;
typedef struct BnClientCallback BnClientCallback;

struct BnClientCallback {
    BnInterface_IClientCallback m_BnIClientCallback;

    void (*dtor)(BnClientCallback* this);

    /* Virtual function */
    uint32_t (*onTransact)(BnClientCallback* this, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags);
};

void BnClientCallback_ctor(BnClientCallback* this);

/****************************************************************************
 * BpClientCallback
 ****************************************************************************/

struct BpInterface_IClientCallback;
typedef struct BpInterface_IClientCallback BpInterface_IClientCallback;

struct BpInterface_IClientCallback {
    IClientCallback m_IClientCallback;
    BpRefBase m_BpRefBase;

    void (*dtor)(BpInterface_IClientCallback* this);

    IBinder* (*onAsBinder)(BnInterface_IClientCallback* this);
};

void BpInterface_IClientCallback_ctor(BpInterface_IClientCallback* this,
    IBinder* remote);

struct BpClientCallback;
typedef struct BpClientCallback BpClientCallback;

struct BpClientCallback {
    BpInterface_IClientCallback m_BpIClientCallback;

    void (*dtor)(BpClientCallback* this);

    /* Virtual function*/
    uint32_t (*onClients)(BpClientCallback* this, IBinder* registered,
        bool hasClients);
};

BpClientCallback* BpClientCallback_new(IBinder* impl);

#endif /* __BINDER_INCLUDE_BINDER_ICLIENTCALLBACK_H__ */
