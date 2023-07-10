/****************************************************************************
 * frameworks/binder/binderlib/base/AidlServiceManager.c
 *
 *  Copyright (C) 2023 Xiaomi Corperation
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#define LOG_TAG "IServiceManager"

#include <inttypes.h>
#include <nuttx/tls.h>
#include <unistd.h>

#include "utils/BinderString.h"
#include "utils/Binderlog.h"
#include "utils/HashMap.h"
#include "utils/Timers.h"
#include "utils/Vector.h"
#include <android/binder_status.h>

#include "AidlServiceManager.h"
#include "BnServiceManager.h"
#include "BpServiceManager.h"
#include "IPCThreadState.h"
#include "IServiceCallback.h"
#include "IServiceManager.h"
#include "Parcel.h"
#include "ProcessGlobal.h"
#include "Status.h"

String* IAIDLServiceManager_getInterfaceDescriptor(IAIDLServiceManager* this)
{
    IAIDLServiceManager_global* global = IAIDLServiceManager_global_get();
    return &global->descriptor;
}

IAIDLServiceManager* IAIDLServiceManager_asInterface(IBinder* obj)
{
    IAIDLServiceManager* intr = NULL;
    IAIDLServiceManager_global* global = IAIDLServiceManager_global_get();

    if (obj != NULL) {
        intr = (IAIDLServiceManager*)obj->queryLocalInterface(obj, &global->descriptor);

        if (intr == NULL) {
            intr = (IAIDLServiceManager*)BpAIDLServiceManager_new(obj);
        }
    }
    return intr;
}

bool IAIDLServiceManager_setDefaultImpl(IAIDLServiceManager* impl)
{
    IAIDLServiceManager_global* global = IAIDLServiceManager_global_get();

    /* Only one user of this interface can use this function
     * at a time. This is a heuristic to detect if two different
     * users in the same process use this function.
     */

    assert(!global->default_impl);
    if (impl) {
        global->default_impl = impl;
        return true;
    }
    return false;
}

IAIDLServiceManager* IAIDLServiceManager_getDefaultImpl()
{
    IAIDLServiceManager_global* global = IAIDLServiceManager_global_get();
    return global->default_impl;
}

void IAIDLServiceManager_dtor(IAIDLServiceManager* this)
{
    this->m_iface.dtor(&this->m_iface);
}

void IAIDLServiceManager_ctor(IAIDLServiceManager* this)
{
    IAIDLServiceManager_global* global = IAIDLServiceManager_global_get();

    IInterface_ctor(&this->m_iface);
    String_init(&global->descriptor, "Vela.os.IServiceManager");

    this->getInterfaceDescriptor = IAIDLServiceManager_getInterfaceDescriptor;

    this->dtor = IAIDLServiceManager_dtor;
}
