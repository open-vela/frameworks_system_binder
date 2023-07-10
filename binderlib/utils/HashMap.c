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

#define LOG_TAG "HashMap"

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

#include "utils/BinderString.h"
#include "utils/Binderlog.h"
#include "utils/HashMap.h"
#include <android/binder_status.h>

/* start with 4 buckets */
#define HASHMAP_MIN_CAP_BITS 2

static bool hashmap_needs_to_grow(HashMapBase* this)
{
    /* grow if empty or more than 75% filled */
    return (this->capacity == 0) || ((this->size + 1) * 4 / 3 > this->capacity);
}

static void hashmap_add_entry(HashMap_Entry** pprev,
    HashMap_Entry* entry)
{
    entry->next = *pprev;
    *pprev = entry;
}

static void hashmap_del_entry(HashMap_Entry** pprev,
    HashMap_Entry* entry)
{
    *pprev = entry->next;
    entry->next = NULL;
}

static bool hashmap_find_entry(HashMapBase* this,
    const long key, size_t hash,
    HashMap_Entry*** pprev,
    HashMap_Entry** entry)
{
    HashMap_Entry *cur, **prev_ptr;

    if (!this->buckets)
        return false;

    for (prev_ptr = &this->buckets[hash], cur = *prev_ptr;
         cur;
         prev_ptr = &cur->next, cur = cur->next) {
        if (this->equal((void*)this, cur->key, key)) {
            if (pprev)
                *pprev = prev_ptr;
            *entry = cur;
            return true;
        }
    }

    return false;
}

static int hashmap_grow(HashMapBase* this)
{
    HashMap_Entry** new_buckets;
    HashMap_Entry* cur;
    HashMap_Entry* tmp;
    size_t new_cap_bits, new_cap;
    size_t h, bkt;

    new_cap_bits = this->cap_bits + 1;
    if (new_cap_bits < HASHMAP_MIN_CAP_BITS)
        new_cap_bits = HASHMAP_MIN_CAP_BITS;

    new_cap = 1UL << new_cap_bits;
    new_buckets = calloc(new_cap, sizeof(new_buckets[0]));
    if (!new_buckets)
        return -ENOMEM;

    HashMap_for_each_entry_safe(this, cur, tmp, bkt)
    {
        h = hash_bits(this->hash(this, cur->key), new_cap_bits);
        hashmap_add_entry(&new_buckets[h], cur);
    }

    this->capacity = new_cap;
    this->cap_bits = new_cap_bits;
    free(this->buckets);
    this->buckets = new_buckets;

    return 0;
}

static size_t HashMapBase_hash(void* this, long key)
{
    return key;
}

static bool HashMapBase_equal(void* this, long key1, long key2)
{
    return key1 == key2;
}

static size_t HashMap_String_hash(void* this, long key)
{
    return str_hash(((String*)key)->mData);
}

static bool HashMap_String_equal(void* this, long key1, long key2)
{
    return strcmp(((String*)key1)->mData, ((String*)key2)->mData) == 0;
}

static int HashMapBase_insert(HashMapBase* this, long key, long value,
    enum hashmap_insert_strategy strategy,
    long* old_key, long* old_value)
{
    HashMap_Entry* entry;
    size_t h;
    int err;

    if (old_key)
        *old_key = 0;
    if (old_value)
        *old_value = 0;

    h = hash_bits(this->hash(this, key), this->cap_bits);
    if (strategy != HASHMAP_APPEND && hashmap_find_entry(this, key, h, NULL, &entry)) {
        if (old_key)
            *old_key = entry->key;
        if (old_value)
            *old_value = entry->value;

        if (strategy == HASHMAP_SET || strategy == HASHMAP_UPDATE) {
            entry->key = key;
            entry->value = value;
            return 0;
        } else if (strategy == HASHMAP_ADD) {
            return -EEXIST;
        }
    }

    if (strategy == HASHMAP_UPDATE)
        return -ENOENT;

    if (hashmap_needs_to_grow(this)) {
        err = hashmap_grow(this);
        if (err)
            return err;
        h = hash_bits(this->hash(this, key), this->cap_bits);
    }

    entry = malloc(sizeof(struct HashMap_Entry));
    if (!entry)
        return -ENOMEM;

    entry->key = key;
    entry->value = value;
    hashmap_add_entry(&this->buckets[h], entry);
    this->size++;

    return 0;
}

static bool HashMapBase_delete(HashMapBase* this, long key,
    long* old_key, long* old_value)
{
    HashMap_Entry **pprev, *entry;
    size_t h;

    h = hash_bits(this->hash(this, key), this->cap_bits);
    if (!hashmap_find_entry(this, key, h, &pprev, &entry))
        return false;

    if (old_key)
        *old_key = entry->key;
    if (old_value)
        *old_value = entry->value;

    hashmap_del_entry(pprev, entry);
    free(entry);
    this->size--;

    return true;
}

static bool HashMapBase_find(HashMapBase* this, long key, long* value)
{
    HashMap_Entry* entry;
    size_t h;

    h = hash_bits(this->hash(this, key), this->cap_bits);
    if (!hashmap_find_entry(this, key, h, NULL, &entry))
        return false;

    if (value)
        *value = entry->value;
    return true;
}

static void HashMapBase_iterator(HashMapBase* this, HashMap_Entry_Callback callback)
{
    HashMap_Entry* cur;
    HashMap_Entry* tmp;
    size_t bkt;

    HashMap_for_each_entry_safe(this, cur, tmp, bkt)
    {
        callback(cur->pkey, cur->pvalue);
    }
}

static void HashMapBase_clear(HashMapBase* this)
{
    HashMap_Entry* cur;
    HashMap_Entry* tmp;
    size_t bkt;

    HashMap_for_each_entry_safe(this, cur, tmp, bkt)
    {
        this->delete (this, cur->key, NULL, NULL);
    }
}

static void HashMapBase_dtor(HashMapBase* this)
{
    HashMap_Entry* cur;
    HashMap_Entry* tmp;
    size_t bkt;

    HashMap_for_each_entry_safe(this, cur, tmp, bkt)
    {
        free(cur);
    }
    if (this->buckets) {
        free(this->buckets);
    }
}

void HashMapBase_ctor(HashMapBase* this)
{
    this->buckets = NULL;
    this->capacity = 0;
    this->cap_bits = 0;
    this->size = 0;

    this->insert = HashMapBase_insert;
    this->delete = HashMapBase_delete;
    this->find = HashMapBase_find;
    this->iterator = HashMapBase_iterator;
    this->clear = HashMapBase_clear;

    /* default hash/equal function */

    this->hash = HashMapBase_hash;
    this->equal = HashMapBase_equal;

    this->dtor = HashMapBase_dtor;
}

static long HashMap_get(HashMap* this, long key)
{
    long value;
    HashMapBase* base = &this->m_HashMap;

    if (base->find(base, (long)key, (long*)(&value))) {
        return value;
    } else {
        return 0;
    }
}

static uint32_t HashMap_put(HashMap* this, long key, long value)
{
    HashMapBase* base = &this->m_HashMap;

    if (base->insert(base, key, value, HASHMAP_ADD, NULL, NULL) < 0) {
        return STATUS_INVALID_OPERATION;
    }
    return STATUS_OK;
}

static uint32_t HashMap_erase(HashMap* this, long key)
{
    HashMapBase* base = &this->m_HashMap;
    long value;

    value = this->get(this, key);
    if (value != STATUS_NAME_NOT_FOUND) {
        base->delete (base, (long)key, (long*)(&key), (long*)(&value));
        return STATUS_OK;
    } else {
        return STATUS_NAME_NOT_FOUND;
    }
}

uint32_t HashMap_find(HashMap* this, long key, long* value)
{
    HashMapBase* base = &this->m_HashMap;

    if (base->find(base, key, value)) {
        return STATUS_OK;
    } else {
        return STATUS_NAME_NOT_FOUND;
    }
}

static uint32_t HashMap_insert(HashMap* this, long key, long value)
{
    HashMapBase* base = &this->m_HashMap;

    if (base->insert(base, (long)key, (long)value, HASHMAP_ADD, NULL, NULL) < 0) {
        return STATUS_INVALID_OPERATION;
    }
    return STATUS_OK;
}

static void HashMap_clear(HashMap* this)
{
    HashMapBase* base = &this->m_HashMap;

    base->clear(base);
}

static void HashMap_iterator(HashMap* this, HashMap_Entry_Callback cb)
{
    HashMapBase* base = &this->m_HashMap;
    base->iterator(base, cb);
}

static uint32_t HashMap_size(HashMap* this)
{
    return this->m_HashMap.size;
}

static void HashMap_dtor(HashMap* this)
{
    this->m_HashMap.dtor(&this->m_HashMap);
}

void HashMap_ctor(HashMap* this)
{
    HashMapBase_ctor(&this->m_HashMap);

    this->get = HashMap_get;
    this->find = HashMap_find;
    this->put = HashMap_put;
    this->erase = HashMap_erase;
    this->insert = HashMap_insert;
    this->size = HashMap_size;
    this->clear = HashMap_clear;
    this->iterator = HashMap_iterator;

    this->dtor = HashMap_dtor;
}

void HashMap_String_ctor(HashMap* this)
{
    HashMap_ctor(this);

    /* Override function */

    this->m_HashMap.hash = HashMap_String_hash;
    this->m_HashMap.equal = HashMap_String_equal;
}
