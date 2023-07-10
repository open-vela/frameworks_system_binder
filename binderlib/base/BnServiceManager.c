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

#define LOG_TAG "BnServiceManager"

#include <android/binder_status.h>
#include <inttypes.h>
#include <nuttx/android/binder.h>
#include <nuttx/tls.h>
#include <unistd.h>
#include <utils/HashMap.h>
#include <utils/Vector.h>

#include "AidlServiceManager.h"
#include "Binder.h"
#include "BnServiceManager.h"
#include "BpBinder.h"
#include "BpServiceManager.h"
#include "IBinder.h"
#include "IInterface.h"
#include "Parcel.h"
#include "utils/Binderlog.h"

static IInterface* BnInterface_IAIDLServiceManager_queryLocalInterface(
    BnInterface_IAIDLServiceManager* this,
    const String* _descriptor)
{
    IInterface* iface = (&this->m_IAIDLServiceManager.m_iface);
    if (String_cmp(_descriptor, this->getInterfaceDescriptor(this)) == 0) {
        iface->incStrongRequireStrong(iface, (void*)this);
        return iface;
    }
    return NULL;
}

static String* BnInterface_IAIDLServiceManager_getInterfaceDescriptor(BnInterface_IAIDLServiceManager* this)
{
    IAIDLServiceManager* iface = (&this->m_IAIDLServiceManager);
    return IAIDLServiceManager_getInterfaceDescriptor(iface);
}

static IBinder* BnInterface_IAIDLServiceManager_onAsBinder(BnInterface_IAIDLServiceManager* this)
{
    return (IBinder*)this;
}

static String* BnInterface_IAIDLServiceManager_Vfun_getInterfaceDescriptor(IBinder* v_this)
{
    BnInterface_IAIDLServiceManager* this = (BnInterface_IAIDLServiceManager*)v_this;
    return this->getInterfaceDescriptor(this);
}

static void BnInterface_IAIDLServiceManager_dtor(BnInterface_IAIDLServiceManager* this)
{
    this->m_IAIDLServiceManager.dtor(&this->m_IAIDLServiceManager);
    this->m_BBinder.dtor(&this->m_BBinder);
}

void BnInterface_IAIDLServiceManager_ctor(BnInterface_IAIDLServiceManager* this)
{
    IBinder* ibinder = &this->m_BBinder.m_IBinder;
    IAIDLServiceManager_ctor(&this->m_IAIDLServiceManager);
    BBinder_ctor(&this->m_BBinder);

    /* Override Pure Virtual function in IBinder */
    ibinder->getInterfaceDescriptor = BnInterface_IAIDLServiceManager_Vfun_getInterfaceDescriptor;

    this->queryLocalInterface = BnInterface_IAIDLServiceManager_queryLocalInterface;
    this->getInterfaceDescriptor = BnInterface_IAIDLServiceManager_getInterfaceDescriptor;
    this->onAsBinder = BnInterface_IAIDLServiceManager_onAsBinder;

    this->dtor = BnInterface_IAIDLServiceManager_dtor;
}

static uint32_t BnAIDLServiceManager_onTransact(BnAIDLServiceManager* this, uint32_t aidl_code,
    const Parcel* aidl_data, Parcel* aidl_reply,
    uint32_t aidl_flags)
{
    int32_t aidl_ret_status = STATUS_OK;
    Parcel data;

    Parcel_initState(&data);

    Parcel_dup(&data, aidl_data);

    switch (aidl_code) {
    case BnServiceManager_TRANSACTION_getService: {
        String in_name;
        String_init(&in_name, NULL);
        IBinder* aidl_return;
        if (!(Parcel_checkInterface(&data, (IBinder*)this))) {
            aidl_ret_status = STATUS_BAD_TYPE;
            break;
        }
        aidl_ret_status = Parcel_readString16_to(&data, &in_name);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        Status aidl_status;
        Status_init(&aidl_status);
        this->getService(this, &in_name, &aidl_return, &aidl_status);
        aidl_ret_status = Status_writeToParcel(&aidl_status, aidl_reply);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        if (!(aidl_status.mException == EX_NONE)) {
            break;
        }
        aidl_ret_status = Parcel_writeStrongBinder(aidl_reply, aidl_return);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
    } break;
    case BnServiceManager_TRANSACTION_checkService: {
        String in_name;
        String_init(&in_name, NULL);
        IBinder* aidl_return = NULL;
        if (!(Parcel_checkInterface(&data, (IBinder*)this))) {
            aidl_ret_status = STATUS_BAD_TYPE;
            break;
        }
        aidl_ret_status = Parcel_readString16_to(&data, &in_name);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        Status aidl_status;
        Status_init(&aidl_status);
        this->checkService(this, &in_name, &aidl_return, &aidl_status);
        aidl_ret_status = Status_writeToParcel(&aidl_status, aidl_reply);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        if (!(aidl_status.mException == EX_NONE)) {
            break;
        }
        aidl_ret_status = Parcel_writeStrongBinder(aidl_reply, aidl_return);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
    } break;
    case BnServiceManager_TRANSACTION_addService: {
        String in_name;
        String_init(&in_name, NULL);
        IBinder* in_service;
        bool in_allowIsolated;
        int32_t in_dumpPriority;
        if (!(Parcel_checkInterface(&data, (IBinder*)this))) {
            aidl_ret_status = STATUS_BAD_TYPE;
            break;
        }
        aidl_ret_status = Parcel_readString16_to(&data, &in_name);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        aidl_ret_status = Parcel_readStrongBinder(&data, &in_service);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        aidl_ret_status = Parcel_readBool(&data, &in_allowIsolated);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        aidl_ret_status = Parcel_readInt32(&data, &in_dumpPriority);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        Status aidl_status;
        Status_init(&aidl_status);
        this->addService(this, &in_name, in_service, in_allowIsolated, in_dumpPriority, &aidl_status);
        aidl_ret_status = Status_writeToParcel(&aidl_status, aidl_reply);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        if (!(aidl_status.mException == EX_NONE)) {
            break;
        }
    } break;
    case BnServiceManager_TRANSACTION_listServices: {
        int32_t in_dumpPriority;
        VectorString aidl_return;
        if (!(Parcel_checkInterface(&data, (IBinder*)this))) {
            aidl_ret_status = STATUS_BAD_TYPE;
            break;
        }
        aidl_ret_status = Parcel_readInt32(&data, &in_dumpPriority);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        Status aidl_status;
        Status_init(&aidl_status);
        this->listServices(this, in_dumpPriority, &aidl_return, &aidl_status);
        aidl_ret_status = Status_writeToParcel(&aidl_status, aidl_reply);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        if (!(aidl_status.mException == EX_NONE)) {
            break;
        }
        aidl_ret_status = Parcel_writeUtf8VectorAsUtf16Vector(aidl_reply, &aidl_return);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
    } break;
    case BnServiceManager_TRANSACTION_isDeclared: {
        String in_name;
        String_init(&in_name, NULL);
        bool aidl_return;
        if (!(Parcel_checkInterface(&data, (IBinder*)this))) {
            aidl_ret_status = STATUS_BAD_TYPE;
            break;
        }
        aidl_ret_status = Parcel_readUtf8FromUtf16(&data, &in_name);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        Status aidl_status;
        Status_init(&aidl_status);
        this->isDeclared(this, &in_name, &aidl_return, &aidl_status);
        aidl_ret_status = Status_writeToParcel(&aidl_status, aidl_reply);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        if (!(aidl_status.mException == EX_NONE)) {
            break;
        }
        aidl_ret_status = Parcel_writeBool(aidl_reply, aidl_return);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
    } break;
    case BnServiceManager_TRANSACTION_getDeclaredInstances: {
        String in_iface;
        String_init(&in_iface, NULL);
        VectorString aidl_return;
        if (!(Parcel_checkInterface(&data, (IBinder*)this))) {
            aidl_ret_status = STATUS_BAD_TYPE;
            break;
        }
        aidl_ret_status = Parcel_readUtf8FromUtf16(&data, &in_iface);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        Status aidl_status;
        Status_init(&aidl_status);
        this->getDeclaredInstances(this, &in_iface, &aidl_return, &aidl_status);
        aidl_ret_status = Status_writeToParcel(&aidl_status, aidl_reply);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
        if (!(aidl_status.mException == EX_NONE)) {
            break;
        }
        aidl_ret_status = Parcel_writeUtf8VectorAsUtf16Vector(aidl_reply, &aidl_return);
        if (aidl_ret_status != STATUS_OK) {
            break;
        }
    } break;
    case BnServiceManager_TRANSACTION_registerForNotifications:
    case BnServiceManager_TRANSACTION_unregisterForNotifications:
    case BnServiceManager_TRANSACTION_getConnectionInfo:
    case BnServiceManager_TRANSACTION_registerClientCallback:
    case BnServiceManager_TRANSACTION_tryUnregisterService:
    case BnServiceManager_TRANSACTION_getServiceDebugInfo:
        BINDER_LOGE("Unsupport at persent, aidl_code=%" PRIu32 "\n", aidl_code - FIRST_CALL_TRANSACTION);
        break;
    default: {
        BBinder* pBBinder = &this->m_BnIAidlServiceManager.m_BBinder;
        aidl_ret_status = pBBinder->onTransact(pBBinder, aidl_code, aidl_data, aidl_reply, aidl_flags);
    } break;
    }
    if (aidl_ret_status == STATUS_UNEXPECTED_NULL) {
        Status aidl_status;
        Status_init(&aidl_status);
        Status_fromExceptionCode(&aidl_status, EX_NULL_POINTER, "Null Pointer");
        aidl_ret_status = Status_writeToParcel(&aidl_status, aidl_reply);
    }
    return aidl_ret_status;
}

static void BnAIDLServiceManager_getService(BnAIDLServiceManager* this, String* name,
    IBinder** aidl_return, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->getService(aidl, name, aidl_return, retStatus);
}

static void BnAIDLServiceManager_checkService(BnAIDLServiceManager* this, String* name,
    IBinder** aidl_return, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->checkService(aidl, name, aidl_return, retStatus);
}

static void BnAIDLServiceManager_addService(BnAIDLServiceManager* this, String* name, IBinder* service,
    bool allowIsolated, int32_t dumpPriority, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->addService(aidl, name, service, allowIsolated, dumpPriority, retStatus);
}

static void BnAIDLServiceManager_listServices(BnAIDLServiceManager* this, int32_t dumpPriority,
    VectorString* aidl_return, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->listServices(aidl, dumpPriority, aidl_return, retStatus);
}

static void BnAIDLServiceManager_registerForNotifications(BnAIDLServiceManager* this, String* name,
    const IServiceCallback* callback, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->registerForNotifications(aidl, name, callback, retStatus);
}

static void BnAIDLServiceManager_unregisterForNotifications(BnAIDLServiceManager* this, String* name,
    const IServiceCallback* callback, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->unregisterForNotifications(aidl, name, callback, retStatus);
}

static void BnAIDLServiceManager_isDeclared(BnAIDLServiceManager* this, String* name,
    bool* aidl_return, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->isDeclared(aidl, name, aidl_return, retStatus);
}

static void BnAIDLServiceManager_getDeclaredInstances(BnAIDLServiceManager* this, String* iface,
    VectorString* aidl_return, Status* retStatus)
{
    IAIDLServiceManager* aidl;
    aidl = &(this->m_BnIAidlServiceManager.m_IAIDLServiceManager);
    aidl->getDeclaredInstances(aidl, iface, aidl_return, retStatus);
}

static uint32_t BnAIDLServiceManager_Vfun_onTransact(BBinder* v_this, uint32_t code, const Parcel* data,
    Parcel* reply, uint32_t flags)
{
    BnAIDLServiceManager* this = (BnAIDLServiceManager*)v_this;
    return this->onTransact(this, code, data, reply, flags);
}

static void BnAIDLServiceManager_dtor(BnAIDLServiceManager* this)
{
    this->m_BnIAidlServiceManager.dtor(&this->m_BnIAidlServiceManager);
}

void BnAIDLServiceManager_ctor(BnAIDLServiceManager* this)
{
    BBinder* bbinder = &(this->m_BnIAidlServiceManager.m_BBinder);
    BnInterface_IAIDLServiceManager_ctor(&this->m_BnIAidlServiceManager);

    /* Virtual function override at BBinder */
    bbinder->onTransact = BnAIDLServiceManager_Vfun_onTransact;

    this->getService = BnAIDLServiceManager_getService;
    this->checkService = BnAIDLServiceManager_checkService;
    this->addService = BnAIDLServiceManager_addService;
    this->listServices = BnAIDLServiceManager_listServices;
    this->registerForNotifications = BnAIDLServiceManager_registerForNotifications;
    this->unregisterForNotifications = BnAIDLServiceManager_unregisterForNotifications;
    this->isDeclared = BnAIDLServiceManager_isDeclared;
    this->getDeclaredInstances = BnAIDLServiceManager_getDeclaredInstances;

    this->onTransact = BnAIDLServiceManager_onTransact;

    this->dtor = BnAIDLServiceManager_dtor;
}
