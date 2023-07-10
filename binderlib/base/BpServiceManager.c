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

#define LOG_TAG "BpServiceManager"

#include <inttypes.h>
#include <nuttx/tls.h>
#include <unistd.h>

#include <android/binder_status.h>

#include "AidlServiceManager.h"
#include "BnServiceManager.h"
#include "BpServiceManager.h"
#include "Parcel.h"
#include "utils/Binderlog.h"
#include "utils/HashMap.h"
#include "utils/Vector.h"

static IBinder* BpInterface_IAIDLServiceManager_onAsBinder(BpInterface_IAIDLServiceManager* this)
{
    BpRefBase* pRefBase = &this->m_BpRefBase;
    return pRefBase->remote(pRefBase);
}

static void BpInterface_IAIDLServiceManager_dtor(BpInterface_IAIDLServiceManager* this)
{
    this->m_IAIDLServiceManager.dtor(&this->m_IAIDLServiceManager);
    this->m_BpRefBase.dtor(&this->m_BpRefBase);
}

void BpInterface_IAIDLServiceManager_ctor(BpInterface_IAIDLServiceManager* this,
    IBinder* remote)
{
    BpRefBase_ctor(&this->m_BpRefBase, remote);
    IAIDLServiceManager_ctor(&this->m_IAIDLServiceManager);

    this->onAsBinder = BpInterface_IAIDLServiceManager_onAsBinder;

    this->dtor = BpInterface_IAIDLServiceManager_dtor;
}

static IBinder* BpAIDLServiceManager_remoteStrong(BpAIDLServiceManager* this)
{
    BpRefBase* pRefBase;

    pRefBase = &(this->m_BpIAidlServiceManager.m_BpRefBase);
    return pRefBase->remoteStrong(pRefBase);
}

static IBinder* BpAIDLServiceManager_remote(BpAIDLServiceManager* this)
{
    BpRefBase* pRefBase;

    pRefBase = &(this->m_BpIAidlServiceManager.m_BpRefBase);
    return pRefBase->remote(pRefBase);
}

static String* BpAIDLServiceManager_getInterfaceDescriptor(BpAIDLServiceManager* this)
{
    return IAIDLServiceManager_getInterfaceDescriptor((IAIDLServiceManager*)this);
}

static void BpAIDLServiceManager_getService(BpAIDLServiceManager* this, String* name,
    IBinder** aidl_return, Status* aidl_status)
{
    Parcel aidl_data;
    Parcel aidl_reply;
    IBinder* ibinder;
    int32_t aidl_ret_status = STATUS_OK;

    Parcel_initState(&aidl_data);
    Parcel_initState(&aidl_reply);

    Parcel_markForBinder(&aidl_data, this->remoteStrong(this));

    aidl_ret_status = Parcel_writeInterfaceToken(&aidl_data, this->getInterfaceDescriptor(this));
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    aidl_ret_status = Parcel_writeString16(&aidl_data, name);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    ibinder = this->remote(this);

    aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_getService,
        &aidl_data, &aidl_reply, 0);
    if (aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->getService(Impl, name, aidl_return, aidl_status);
        return;
    }
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }
    aidl_ret_status = Status_readFromParcel(aidl_status, &aidl_reply);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    aidl_ret_status = Parcel_readNullableStrongBinder(&aidl_reply, aidl_return);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

aidl_error:
    Status_setFromStatusT(aidl_status, aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_checkService(BpAIDLServiceManager* this, String* name,
    IBinder** aidl_return, Status* aidl_status)

{
    Parcel aidl_data;
    Parcel aidl_reply;
    IBinder* ibinder;
    int32_t aidl_ret_status = STATUS_OK;

    Parcel_initState(&aidl_data);
    Parcel_initState(&aidl_reply);

    Parcel_markForBinder(&aidl_data, this->remoteStrong(this));

    aidl_ret_status = Parcel_writeInterfaceToken(&aidl_data, this->getInterfaceDescriptor(this));
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    aidl_ret_status = Parcel_writeString16(&aidl_data, name);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    ibinder = this->remote(this);

    aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_checkService,
        &aidl_data, &aidl_reply, 0);
    if (aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->checkService(Impl, name, aidl_return, aidl_status);
        return;
    }

    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }
    aidl_ret_status = Status_readFromParcel(aidl_status, &aidl_reply);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    aidl_ret_status = Parcel_readNullableStrongBinder(&aidl_reply, aidl_return);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

aidl_error:
    Status_setFromStatusT(aidl_status, aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_addService(BpAIDLServiceManager* this, String* name, IBinder* service,
    bool allowIsolated, int32_t dumpPriority, Status* aidl_status)
{
    Parcel aidl_data;
    Parcel aidl_reply;
    IBinder* ibinder;
    int32_t aidl_ret_status = STATUS_OK;

    Parcel_initState(&aidl_data);
    Parcel_initState(&aidl_reply);

    Parcel_markForBinder(&aidl_data, this->remoteStrong(this));

    aidl_ret_status = Parcel_writeInterfaceToken(&aidl_data, this->getInterfaceDescriptor(this));
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    aidl_ret_status = Parcel_writeString16(&aidl_data, name);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    aidl_ret_status = Parcel_writeStrongBinder(&aidl_data, service);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }
    aidl_ret_status = Parcel_writeBool(&aidl_data, allowIsolated);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }
    aidl_ret_status = Parcel_writeInt32(&aidl_data, dumpPriority);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }

    ibinder = this->remote(this);

    aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_addService,
        &aidl_data, &aidl_reply, 0);
    if (aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->addService(Impl, name, service, allowIsolated, dumpPriority, aidl_status);
        return;
    }

    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }
    aidl_ret_status = Status_readFromParcel(aidl_status, &aidl_reply);
    if (aidl_ret_status != STATUS_OK) {
        goto aidl_error;
    }
    if (aidl_status->mException != EX_NONE) {
        return;
    }

aidl_error:
    Status_setFromStatusT(aidl_status, aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_listServices(BpAIDLServiceManager* this, int32_t dumpPriority,
    VectorString* _aidl_return, Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeInt32(&_aidl_data, dumpPriority);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_listServices,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->listServices(Impl, dumpPriority, _aidl_return, _aidl_status);
        return;
    }
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }
    _aidl_ret_status = Parcel_readUtf8VectorFromUtf16Vector(&_aidl_reply, _aidl_return);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_registerForNotifications(BpAIDLServiceManager* this, String* name,
    const IServiceCallback* callback, Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeString16(&_aidl_data, name);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeStrongBinder(&_aidl_data, (IBinder*)callback);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_registerForNotifications,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->registerForNotifications(Impl, name, callback, _aidl_status);
        return;
    }
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }

_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_unregisterForNotifications(BpAIDLServiceManager* this, String* name,
    const IServiceCallback* callback, Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeString16(&_aidl_data, name);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeStrongBinder(&_aidl_data, (IBinder*)callback);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_unregisterForNotifications,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->unregisterForNotifications(Impl, name, callback, _aidl_status);
        return;
    }
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }

_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_isDeclared(BpAIDLServiceManager* this, String* name,
    bool* _aidl_return, Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeString16(&_aidl_data, name);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_isDeclared,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->isDeclared(Impl, name, _aidl_return, _aidl_status);
        return;
    }

    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }
    _aidl_ret_status = Parcel_readBool(&_aidl_reply, _aidl_return);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_getDeclaredInstances(BpAIDLServiceManager* this, String* iface,
    VectorString* _aidl_return, Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeString16(&_aidl_data, iface);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_getDeclaredInstances,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->getDeclaredInstances(Impl, iface, _aidl_return, _aidl_status);
        return;
    }
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }
    _aidl_ret_status = Parcel_readUtf8VectorFromUtf16Vector(&_aidl_reply, _aidl_return);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_registerClientCallback(BpAIDLServiceManager* this, String* name, const IBinder* service,
    const IClientCallback* callback, Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeString16(&_aidl_data, name);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeStrongBinder(&_aidl_data, (IBinder*)service);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeStrongBinder(&_aidl_data, (IBinder*)callback);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_registerClientCallback,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->registerClientCallback(Impl, name, service, callback, _aidl_status);
        return;
    }
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }
_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_tryUnregisterService(BpAIDLServiceManager* this, String* name,
    IBinder* service, Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeString16(&_aidl_data, name);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Parcel_writeStrongBinder(&_aidl_data, service);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_tryUnregisterService,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->tryUnregisterService(Impl, name, service, _aidl_status);
        return;
    }
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }
_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static void BpAIDLServiceManager_getServiceDebugInfo(BpAIDLServiceManager* this, VectorString* _aidl_return,
    Status* _aidl_status)
{
    Parcel _aidl_data;
    Parcel _aidl_reply;
    IBinder* ibinder;
    int32_t _aidl_ret_status = STATUS_OK;

    Parcel_initState(&_aidl_data);
    Parcel_initState(&_aidl_reply);

    Parcel_markForBinder(&_aidl_data, this->remoteStrong(this));

    _aidl_ret_status = Parcel_writeInterfaceToken(&_aidl_data,
        this->getInterfaceDescriptor(this));
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    ibinder = this->remote(this);
    _aidl_ret_status = ibinder->transact(ibinder, BnServiceManager_TRANSACTION_getServiceDebugInfo,
        &_aidl_data, &_aidl_reply, 0);

    if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && IAIDLServiceManager_getDefaultImpl()) {
        IAIDLServiceManager* Impl = IAIDLServiceManager_getDefaultImpl();
        Impl->getServiceDebugInfo(Impl, _aidl_return, _aidl_status);
        return;
    }
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    _aidl_ret_status = Status_readFromParcel(_aidl_status, &_aidl_reply);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }
    if (_aidl_status->mException != EX_NONE) {
        return;
    }
    //_aidl_ret_status = _aidl_reply.readParcelableVector(_aidl_return);
    _aidl_ret_status = Parcel_readUtf8VectorFromUtf16Vector(&_aidl_reply, _aidl_return);
    if (_aidl_ret_status != STATUS_OK) {
        goto _aidl_error;
    }

_aidl_error:
    Status_setFromStatusT(_aidl_status, _aidl_ret_status);
    return;
}

static String* AIDLServiceManager_Vfun_getInterfaceDescriptor(IAIDLServiceManager* v_this)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    return this->getInterfaceDescriptor(this);
}

static void AIDLServiceManager_Vfun_getService(IAIDLServiceManager* v_this, String* name,
    IBinder** aidl_return, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->getService(this, name, aidl_return, retStatus);
}

static void AIDLServiceManager_Vfun_checkService(IAIDLServiceManager* v_this, String* name,
    IBinder** aidl_return, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->checkService(this, name, aidl_return, retStatus);
}

static void AIDLServiceManager_Vfun_addService(IAIDLServiceManager* v_this, String* name, IBinder* service,
    bool allowIsolated, int32_t dumpPriority, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->addService(this, name, service, allowIsolated, dumpPriority, retStatus);
}

static void AIDLServiceManager_Vfun_listServices(IAIDLServiceManager* v_this, int32_t dumpPriority,
    VectorString* aidl_return, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->listServices(this, dumpPriority, aidl_return, retStatus);
}

static void AIDLServiceManager_Vfun_registerForNotifications(IAIDLServiceManager* v_this, String* name,
    const IServiceCallback* callback, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->registerForNotifications(this, name, callback, retStatus);
}

static void AIDLServiceManager_Vfun_unregisterForNotifications(IAIDLServiceManager* v_this, String* name,
    const IServiceCallback* callback, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->unregisterForNotifications(this, name, callback, retStatus);
}

static void AIDLServiceManager_Vfun_isDeclared(IAIDLServiceManager* v_this, String* name,
    bool* _aidl_return, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->isDeclared(this, name, _aidl_return, retStatus);
}

static void AIDLServiceManager_Vfun_getDeclaredInstances(IAIDLServiceManager* v_this, String* iface,
    VectorString* aidl_return, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->getDeclaredInstances(this, iface, aidl_return, retStatus);
}

static void AIDLServiceManager_Vfun_registerClientCallback(IAIDLServiceManager* v_this, String* name,
    const IBinder* service, const IClientCallback* callback,
    Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->registerClientCallback(this, name, service, callback, retStatus);
}

static void AIDLServiceManager_Vfun_tryUnregisterService(IAIDLServiceManager* v_this, String* name,
    IBinder* service, Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->tryUnregisterService(this, name, service, retStatus);
}

static void AIDLServiceManager_Vfun_getServiceDebugInfo(IAIDLServiceManager* v_this, VectorString* aidl_return,
    Status* retStatus)
{
    BpAIDLServiceManager* this = (BpAIDLServiceManager*)v_this;
    this->getServiceDebugInfo(this, aidl_return, retStatus);
}

static void BpAIDLServiceManager_dtor(BpAIDLServiceManager* this)
{
    this->m_BpIAidlServiceManager.dtor(&this->m_BpIAidlServiceManager);
}

static void BpAIDLServiceManager_ctor(BpAIDLServiceManager* this, IBinder* impl)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BpIAidlServiceManager.m_IAIDLServiceManager);

    BpInterface_IAIDLServiceManager_ctor(&this->m_BpIAidlServiceManager, impl);

    /* Override Virtual function for AIDLServiceManager */
    aidl->getInterfaceDescriptor = AIDLServiceManager_Vfun_getInterfaceDescriptor;
    aidl->getService = AIDLServiceManager_Vfun_getService;
    aidl->checkService = AIDLServiceManager_Vfun_checkService;
    aidl->addService = AIDLServiceManager_Vfun_addService;
    aidl->listServices = AIDLServiceManager_Vfun_listServices;
    aidl->registerForNotifications = AIDLServiceManager_Vfun_registerForNotifications;
    aidl->unregisterForNotifications = AIDLServiceManager_Vfun_unregisterForNotifications;
    aidl->isDeclared = AIDLServiceManager_Vfun_isDeclared;
    aidl->getDeclaredInstances = AIDLServiceManager_Vfun_getDeclaredInstances;
    aidl->registerClientCallback = AIDLServiceManager_Vfun_registerClientCallback;
    aidl->tryUnregisterService = AIDLServiceManager_Vfun_tryUnregisterService;
    aidl->getServiceDebugInfo = AIDLServiceManager_Vfun_getServiceDebugInfo;

    /* Virtual Function for BpRefBase */
    this->remoteStrong = BpAIDLServiceManager_remoteStrong;
    this->remote = BpAIDLServiceManager_remote;

    /* Virtual Function for IAIDLServiceManager */
    this->getInterfaceDescriptor = BpAIDLServiceManager_getInterfaceDescriptor;
    this->getService = BpAIDLServiceManager_getService;
    this->checkService = BpAIDLServiceManager_checkService;
    this->addService = BpAIDLServiceManager_addService;
    this->listServices = BpAIDLServiceManager_listServices;
    this->registerForNotifications = BpAIDLServiceManager_registerForNotifications;
    this->unregisterForNotifications = BpAIDLServiceManager_unregisterForNotifications;
    this->isDeclared = BpAIDLServiceManager_isDeclared;
    this->getDeclaredInstances = BpAIDLServiceManager_getDeclaredInstances;
    this->registerClientCallback = BpAIDLServiceManager_registerClientCallback;
    this->tryUnregisterService = BpAIDLServiceManager_tryUnregisterService;
    this->getServiceDebugInfo = BpAIDLServiceManager_getServiceDebugInfo;

    this->dtor = BpAIDLServiceManager_dtor;
}

BpAIDLServiceManager* BpAIDLServiceManager_new(IBinder* impl)
{
    BpAIDLServiceManager* this;
    this = zalloc(sizeof(BpAIDLServiceManager));

    BpAIDLServiceManager_ctor(this, impl);
    return this;
}

void BpAIDLServiceManager_delete(BpAIDLServiceManager* this)
{
    this->dtor(this);
    free(this);
}
