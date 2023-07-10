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

#ifndef __BINDER_INCLUDE_UTILS_VECTOR_H__
#define __BINDER_INCLUDE_UTILS_VECTOR_H__

#include "utils/BinderString.h"

struct Vector_Entry;
typedef struct Vector_Entry Vector_Entry;

struct Vector_Entry {
    union {
        long value;
        void* pvalue;
    };
};

struct VectorBase;
typedef struct VectorBase VectorBase;

#define VECTOR_INIT_CAPACITY 16

typedef void (*Vector_free_func)(void* arg);

struct VectorBase {
    void (*dtor)(VectorBase* this);

    int (*count)(const VectorBase* this);
    int (*cap)(const VectorBase* this);
    int (*resize)(VectorBase* this, int capacity);
    size_t (*add)(VectorBase* this, long value);
    void (*set)(VectorBase* this, int index, long value);
    long (*get)(const VectorBase* this, int index);
    long (*delete)(VectorBase* this, int index);
    void (*clear)(VectorBase* this, Vector_free_func freecb);

    Vector_Entry* m_items;
    int capacity;
    int total;
};

void VectorBase_ctor(VectorBase* this);

struct VectorImpl;
typedef struct VectorImpl VectorImpl;

struct VectorImpl {
    VectorBase m_VectorBase;
    void (*dtor)(VectorImpl* this);

    size_t (*size)(const VectorImpl* this);
    void* (*get)(const VectorImpl* this, int index);
    int32_t (*removeAt)(VectorImpl* this, int index);
    void (*push)(VectorImpl* this, void* value);
    int32_t (*clear)(VectorImpl* this);
    int32_t (*append)(VectorImpl* this, void* value, size_t numItems);
    void* (*editItemAt)(VectorImpl* this, size_t index);
    size_t (*add)(VectorImpl* this, void* value);
    void* (*removeItemAt)(VectorImpl* this, int index);
    bool (*isEmpty)(const VectorImpl* this);
};

void VectorImpl_ctor(VectorImpl* this);
VectorImpl* VectorImpl_new(void);
void VectorImpl_delete(VectorImpl* this);

struct VectorString;
typedef struct VectorString VectorString;

struct VectorString {
    VectorBase m_VectorBase;
    void (*dtor)(VectorString* this);

    void (*add)(VectorString* this, String* string);
    size_t (*size)(const VectorString* this);
    String* (*get)(const VectorString* this, int index);
};

void VectorString_ctor(VectorString* this);

#endif //__BINDER_INCLUDE_UTILS_CVECTOR_H__
