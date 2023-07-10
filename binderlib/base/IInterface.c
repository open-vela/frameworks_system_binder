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

#define LOG_TAG "IInterface"

#include "IInterface.h"
#include "utils/Binderlog.h"

IBinder* IInterface_onAsBinder(IInterface* iface)
{
    IBinder* ibinder;

    if (iface == NULL)
        return NULL;

    ibinder = iface->onAsBinder(iface);

    iface->incStrongRequireStrong(iface, (const void*)ibinder);

    return ibinder;
}

void IInterface_incStrongRequireStrong(IInterface* this, const void* id)
{
    this->m_refbase.incStrongRequireStrong(&this->m_refbase, id);
}

static void IInterface_dtor(IInterface* this)
{
    this->m_refbase.dtor(&this->m_refbase);
}

void IInterface_ctor(IInterface* this)
{
    RefBase_ctor(&this->m_refbase);

    this->incStrongRequireStrong = IInterface_incStrongRequireStrong;
    this->onAsBinder = IInterface_onAsBinder;

    this->dtor = IInterface_dtor;
}
