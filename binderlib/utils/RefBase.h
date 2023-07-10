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

#ifndef __BINDER_INCLUDE_UTILS_REFBASE_H__
#define __BINDER_INCLUDE_UTILS_REFBASE_H__

#include <stdatomic.h>

struct RefBase;
typedef struct RefBase RefBase;

enum {
    OBJECT_LIFETIME_STRONG = 0x0000,
    OBJECT_LIFETIME_WEAK = 0x0001,
    OBJECT_LIFETIME_MASK = 0x0001
};

enum {
    FIRST_INC_STRONG = 0x0001
};

struct RefBase_weakref;
typedef struct RefBase_weakref RefBase_weakref;

struct RefBase_weakref {
    void (*dtor)(RefBase_weakref* this);

    RefBase* (*refBase)(RefBase_weakref* this);
    void (*incWeak)(RefBase_weakref* this, const void* id);
    void (*incWeakRequireWeak)(RefBase_weakref* this, const void* id);
    void (*decWeak)(RefBase_weakref* this, const void* id);
    bool (*attemptIncStrong)(RefBase_weakref* this, const void* id);
    bool (*attemptIncWeak)(RefBase_weakref* this, const void* id);
    int32_t (*getWeakCount)(RefBase_weakref* this);
    void (*printRefs)(RefBase_weakref* this);
    void (*trackMe)(RefBase_weakref* this, bool enable, bool retain);
};

void RefBase_weakref_ctor(RefBase_weakref* this);

struct RefBase_weakref_impl;
typedef struct RefBase_weakref_impl RefBase_weakref_impl;

struct RefBase_weakref_impl {
    RefBase_weakref m_RefBase_weakref;

    void (*dtor)(RefBase_weakref_impl* this);

    void (*addStrongRef)(RefBase_weakref_impl* this, const void* /*id*/);
    void (*removeStrongRef)(RefBase_weakref_impl* this, const void* /*id*/);
    void (*renameStrongRefId)(RefBase_weakref_impl* this, const void* /*old_id*/, const void* /*new_id*/);
    void (*addWeakRef)(RefBase_weakref_impl* this, const void* /*id*/);
    void (*removeWeakRef)(RefBase_weakref_impl* this, const void* /*id*/);
    void (*renameWeakRefId)(RefBase_weakref_impl* this, const void* /*old_id*/, const void* /*new_id*/);
    void (*printRefs)(RefBase_weakref_impl* this);
    void (*trackMe)(RefBase_weakref_impl* this, bool, bool);

    /* inherit RefBase_weakref */
    void (*incWeak)(RefBase_weakref_impl* this, const void* id);
    void (*decWeak)(RefBase_weakref_impl* this, const void* id);

    atomic_uint mStrong;
    atomic_uint mWeak;
    atomic_uint mFlags;
    RefBase* mBase;
};

RefBase_weakref_impl* RefBase_weakref_impl_new(RefBase* base);
void RefBase_weakref_impl_delete(RefBase_weakref_impl* this);

struct RefBase {
    void (*dtor)(RefBase* this);

    /* public */
    void (*incStrong)(RefBase* this, const void* id);
    void (*incStrongRequireStrong)(RefBase* this, const void* id);
    void (*decStrong)(RefBase* this, const void* id);
    void (*forceIncStrong)(RefBase* this, const void* id);
    int32_t (*getStrongCount)(RefBase* this);
    void (*printRefs)(RefBase* this);
    void (*trackMe)(RefBase* this, bool enable, bool retain);

    RefBase_weakref* (*createWeak)(RefBase* this, const void* id);
    RefBase_weakref* (*getWeakRefs)(RefBase* this);

    /* protected */
    void (*extendObjectLifetime)(RefBase* this, int32_t mode);

    /* virtual function */
    void (*onFirstRef)(RefBase* v_this);
    void (*onLastStrongRef)(RefBase* v_this, const void* id);
    bool (*onIncStrongAttempted)(RefBase* v_this, uint32_t flags, const void* id);
    void (*onLastWeakRef)(RefBase* v_this, const void* id);

    RefBase_weakref_impl* mRefs;
};

void RefBase_ctor(RefBase* this);
RefBase* RefBase_new(void);
void RefBase_delete(RefBase* this);

#endif //__BINDER_INCLUDE_UTILS_REFBASE_H__
