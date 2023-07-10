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

#ifndef __BINDER_INCLUDE_BINDER_IINTERFACE_H__
#define __BINDER_INCLUDE_BINDER_IINTERFACE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "Binder.h"
#include "IBinder.h"
#include "utils/RefBase.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct IInterface;
typedef struct IInterface IInterface;

struct IInterface {
    struct RefBase m_refbase;

    void (*dtor)(IInterface* this);

    /* Virtual Function for RefBase */
    void (*incStrongRequireStrong)(IInterface* this, const void* id);

    /* Pure Virtual Function */
    IBinder* (*onAsBinder)(IInterface* this);
};

IBinder* IInterface_asBinder(IInterface* iface);

void IInterface_ctor(IInterface* this);

/**
 * If this is a local object and the descriptor matches, this will return the
 * actual local object which is implementing the interface. Otherwise, this will
 * return a proxy to the interface without checking the interface descriptor.
 * This means that subsequent calls may fail with BAD_TYPE.
 *
 * C++ template define
 * template<typename INTERFACE>
 * inline sp<INTERFACE> interface_cast(const sp<IBinder>& obj)
 * {
 *    return INTERFACE::asInterface(obj);
 * }
 */

#define interface_cast(INTERFACE, obj)            \
    ({                                            \
#INTERFACE* __cast_val;                   \
        __cast_val = #INTERFACE##asInterface(obj) \
            __cast_val;                           \
    })

/**
 * This is the same as interface_cast, except that it always checks to make sure
 * the descriptor matches, and if it doesn't match, it will return nullptr.
 *
 * C++ template define
 *
 * template<typename INTERFACE>
 * inline sp<INTERFACE> checked_interface_cast(const sp<IBinder>& obj)
 * {
 *     if (obj->getInterfaceDescriptor() != INTERFACE::descriptor) {
 *         return nullptr;
 *     }
 *
 *     return interface_cast<INTERFACE>(obj);
 * }
 */

#define checked_interface_cast(INTERFACE, obj)             \
    ({                                                     \
#INTERFACE* __checked_val;                         \
        String* __str1 = obj->getInterfaceDescriptor(obj); \
        String* __str2 = &(I##INTERFACE##__descriptor);    \
        if (String_cmp(__str1, __str2) != 0) {             \
            return NULL;                                   \
        }                                                  \
        __checked_val = interface_cast(INTERFACE, obj)     \
            __checked_val;                                 \
    })

/* template<typename INTERFACE>
 * class BnInterface : public INTERFACE, public BBinder
 * {
 * public:
 *     virtual sp<IInterface>      queryLocalInterface(const String16& _descriptor);
 *     virtual const String16&     getInterfaceDescriptor() const;
 *
 * protected:
 *     typedef INTERFACE           BaseInterface;
 *     virtual IBinder*            onAsBinder();
 * };
 */

/*
#define BEGIN_DECLARE_BN_INTERFACE(INTERFACE)                              \
  struct BnInterface_##INTERFACE;                                          \
  typedef struct BnInterface_##INTERFACE BnInterface_##INTERFACE;          \
                                                                           \
  struct BnInterface_##INTERFACE                                           \
  {                                                                        \
    BBinder     m_BBinder;                                                 \
    #INTERFACE  m_##INTERFACE;                                             \
    void   (*dtor)(BnInterface_##INTERFACE *this);                         \
                                                                           \
    IInterface* (*queryLocalInterface)(BnInterface_##INTERFACE *this,      \
                 const String *_descriptor);                               \
    String *    (*getInterfaceDescriptor)(BnInterface_##INTERFACE *this);  \
    IBinder*    (*onAsBinder)(BnInterface_##INTERFACE * this);             \

#define END_DECLARE_BN_INTERFACE(INTERFACE)                                \
  };                                                                       \
  void BnInterface_##INTERFACE##_ctor(BnInterface_##INTERFACE *this);
*/

/* template<typename INTERFACE>
 * class BpInterface : public INTERFACE, public BpRefBase
 * {
 * public:
 *     explicit                    BpInterface(const sp<IBinder>& remote);
 *
 * protected:
 *     typedef INTERFACE           BaseInterface;
 *     virtual IBinder*            onAsBinder();
 * };
 */

/*
#define BEGIN_DECLARE_BP_INTERFACE(INTERFACE)                        \
  struct BpInterface_##INTERFACE;                                    \
  typedef struct BpInterface_##INTERFACE BpInterface_##INTERFACE;    \
                                                                     \
  struct BpInterface_##INTERFACE                                     \
  {                                                                  \
    BpRefBase   m_BpRefBase;                                         \
    #INTERFACE  m_##INTERFACE;                                       \
                                                                     \
    void   (*dtor)(BpInterface_##INTERFACE *this);                   \
    IBinder*    (*onAsBinder)(BnInterface_##INTERFACE * this);

#define END_DECLARE_BP_INTERFACE(INTERFACE)                          \
  };                                                                 \
  void BpInterface_##INTERFACE##_ctor(BpInterface_##INTERFACE *this, \
                                      IBinder *remote);
*/

/*
struct BnInterface;
typedef struct BnInterface BnInterface;

struct BnInterface
{
  BBinder     m_BBinder;
  void   (*dtor)(BnInterface *this);
};

void   BnInterface_ctor(BnInterface *this);

struct BpInterface;
typedef struct BpInterface BpInterface;

struct BpInterface
{
  BpRefBase   m_BpRefBase;

  void   (*dtor)(BpInterface *this);
};

void  BpInterface_ctor(BpInterface *this, IBinder *remote);
*/

#endif /* __BINDER_INCLUDE_BINDER_IINTERFACE_H__ */
