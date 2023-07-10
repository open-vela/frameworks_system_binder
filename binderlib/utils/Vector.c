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

#define LOG_TAG "Vector"

#include <debug.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <nuttx/android/binder.h>
#include <nuttx/clock.h>
#include <nuttx/tls.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "utils/Binderlog.h"
#include "utils/RefBase.h"
#include "utils/Vector.h"
#include <android/binder_status.h>

static int VectorBase_count(const VectorBase* this)
{
    return this->total;
}
static int VectorBase_capacity(const VectorBase* this)
{
    return this->capacity;
}

static int VectorBase_resize(VectorBase* this, int capacity)
{
    Vector_Entry* items = realloc(this->m_items, sizeof(Vector_Entry) * capacity);
    if (items) {
        this->m_items = items;
        this->capacity = capacity;
        return -ENOMEM;
    }
    return 0;
}

static size_t VectorBase_add(VectorBase* this, long value)
{
    if (this->capacity == this->total) {
        this->resize(this, this->capacity * 2);
    }
    this->m_items[this->total++].value = value;
    return this->total;
}

static void VectorBase_set(VectorBase* this, int index, long value)
{
    if (index >= 0 && index < this->total) {
        this->m_items[index].value = value;
    }
}

static long VectorBase_get(const VectorBase* this, int index)
{
    if (index >= 0 && index < this->total)
        return this->m_items[index].value;
    return -ENOENT;
}

static long VectorBase_delete(VectorBase* this, int index)
{
    long value;

    if (index < 0 || index >= this->total)
        return -ENOENT;
    value = this->m_items[index].value;
    for (int i = index; i < this->total - 1; i++) {
        this->m_items[i].value = this->m_items[i + 1].value;
    }

    this->total--;
    if (this->total > 0 && this->total == this->capacity / 4) {
        this->resize(this, this->capacity / 2);
    }
    return value;
}

static void VectorBase_clear(VectorBase* this, Vector_free_func freecb)
{
    for (int i = 0; i < this->total; i++) {
        if (freecb != NULL) {
            freecb((void*)this->m_items[i].value);
        }
    }
    this->total = 0;
}

static void VectorBase_dtor(VectorBase* this)
{
    free(this->m_items);
}

void VectorBase_ctor(VectorBase* this)
{
    this->capacity = VECTOR_INIT_CAPACITY;

    this->total = 0;
    this->m_items = malloc(sizeof(Vector_Entry) * this->capacity);

    this->count = VectorBase_count;
    this->cap = VectorBase_capacity;
    this->resize = VectorBase_resize;
    this->add = VectorBase_add;
    this->set = VectorBase_set;
    this->get = VectorBase_get;
    this->delete = VectorBase_delete;
    this->clear = VectorBase_clear;

    this->dtor = VectorBase_dtor;
}

static void VectorString_add(VectorString* this, String* string)
{
    VectorBase* base = &this->m_VectorBase;
    String* pstr = zalloc(sizeof(String));
    String_dup(pstr, string);
    base->add(base, (long)pstr);
}

static size_t VectorString_size(const VectorString* this)
{
    const VectorBase* base = &this->m_VectorBase;
    return base->count(base);
}

static String* VectorString_get(const VectorString* this, int index)
{
    const VectorBase* base = &this->m_VectorBase;
    return (String*)base->get(base, index);
}

static void VectorString_entry_free(void* arg)
{
    free(arg);
}

static void VectorString_dtor(VectorString* this)
{
    this->m_VectorBase.clear(&this->m_VectorBase, VectorString_entry_free);
    this->m_VectorBase.dtor(&this->m_VectorBase);
}

void VectorString_ctor(VectorString* this)
{
    VectorBase_ctor(&this->m_VectorBase);

    this->add = VectorString_add;
    this->size = VectorString_size;
    this->get = VectorString_get;

    this->dtor = VectorString_dtor;
}

static void VectorImpl_push(VectorImpl* this, void* value)
{
    VectorBase* base = &this->m_VectorBase;
    base->add(base, (long)value);
}

static int32_t VectorImpl_removeAt(VectorImpl* this, int index)
{
    VectorBase* base = &this->m_VectorBase;
    base->delete (base, index);
    return STATUS_OK;
}

static size_t VectorImpl_size(const VectorImpl* this)
{
    const VectorBase* base = &this->m_VectorBase;
    return base->count(base);
}

static void* VectorImpl_get(const VectorImpl* this, int index)
{
    const VectorBase* base = &this->m_VectorBase;
    return (void*)base->get(base, index);
}

static int32_t VectorImpl_clear(VectorImpl* this)
{
    VectorBase* base = &this->m_VectorBase;
    base->clear(base, NULL);
    return STATUS_OK;
}

static int32_t VectorImpl_append(VectorImpl* this, void* value, size_t numItems)
{
    VectorBase* base = &this->m_VectorBase;

    if (base->cap(base) < base->count(base) + numItems) {
        if (base->resize(base, base->cap(base) * 2 + numItems) < 0) {
            return STATUS_NO_MEMORY;
        }
    }
    for (int i = 0; i < numItems; i++) {
        base->add(base, (long)value);
    }
    return STATUS_OK;
}

static void* VectorImpl_editItemAt(VectorImpl* this, size_t index)
{
    const VectorBase* base = &this->m_VectorBase;
    return (void*)base->get(base, index);
}

static size_t VectorImpl_add(VectorImpl* this, void* value)
{
    VectorBase* base = &this->m_VectorBase;
    return base->add(base, (long)value);
}

static void* VectorImpl_removeItemAt(VectorImpl* this, int index)
{
    void* ret;
    VectorBase* base = &this->m_VectorBase;

    ret = (void*)base->delete (base, index);
    if (ret < 0) {
        return NULL;
    }
    return ret;
}

static bool VectorImpl_isEmpty(const VectorImpl* this)
{
    const VectorBase* base = &this->m_VectorBase;

    return (base->count(base) == 0);
}

static void VectorImpl_dtor(VectorImpl* this)
{
    this->m_VectorBase.dtor(&this->m_VectorBase);
}

void VectorImpl_ctor(VectorImpl* this)
{
    VectorBase_ctor(&this->m_VectorBase);

    this->size = VectorImpl_size;
    this->get = VectorImpl_get;
    this->removeAt = VectorImpl_removeAt;
    this->push = VectorImpl_push;
    this->clear = VectorImpl_clear;
    this->append = VectorImpl_append;
    this->add = VectorImpl_add;
    this->isEmpty = VectorImpl_isEmpty;

    this->removeItemAt = VectorImpl_removeItemAt;
    this->editItemAt = VectorImpl_editItemAt;

    this->dtor = VectorImpl_dtor;
}

VectorImpl* VectorImpl_new(void)
{
    VectorImpl* this;
    this = zalloc(sizeof(VectorImpl));

    VectorImpl_ctor(this);
    return this;
}

void VectorImpl_delete(VectorImpl* this)
{
    this->dtor(this);
    free(this);
}
