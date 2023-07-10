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

#ifndef __BINDER_INCLUDE_UTILS_HASHMAP_H__
#define __BINDER_INCLUDE_UTILS_HASHMAP_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

enum hashmap_insert_strategy {
    HASHMAP_ADD,
    HASHMAP_SET,
    HASHMAP_UPDATE,
    HASHMAP_APPEND,
};

/*
 * HashMap_for_each_entry - iterate over all entries in hashmap
 * @this: hashmap object to iterate
 * @cur:  HashMap_Entry * used as a loop cursor
 * @bkt:  integer used as a bucket loop cursor
 */
#define HashMap_for_each_entry(this, cur, bkt) \
    for (bkt = 0; bkt < this->capacity; bkt++) \
        for (cur = this->buckets[bkt]; cur; cur = cur->next)

/*
 * HashMap_for_each_entry_safe - iterate over all entries in hashmap,
 * safe against removals
 * @this: hashmap object to iterate
 * @cur:  HashMap_Entry * used as a loop cursor
 * @tmp:  HashMap_Entry * used as a temporary next cursor storage
 * @bkt:  integer used as a bucket loop cursor
 */
#define HashMap_for_each_entry_safe(this, cur, tmp, bkt) \
    for (bkt = 0; bkt < this->capacity; bkt++)           \
        for (cur = this->buckets[bkt];                   \
             cur && ({ tmp = cur->next;true; });                               \
             cur = tmp)

/*
 * HashMap_for_each_key_entry - iterate over entries associated with given key
 * @map: hashmap to iterate
 * @cur: HashMap_Entry* used as a loop cursor
 * @key: key to iterate entries for
 */
#define HashMap_for_each_key_entry(this, cur, _key)                               \
    for (cur = this->buckets                                                      \
             ? this->buckets[hash_bits(this->hash(this, (_key)), this->cap_bits)] \
             : NULL;                                                              \
         cur;                                                                     \
         cur = cur->next)                                                         \
        if (this->equal(this, cur->key, (_key)))

#define HashMap_for_each_key_entrysafe(this, cur, tmp, _key)                      \
    for (cur = this->buckets                                                      \
             ? this->buckets[hash_bits(this->hash(this, (_key)), this->cap_bits)] \
             : NULL;                                                              \
         cur && ({ tmp = cur->next;true; });                                                            \
         cur = tmp)                                                               \
        if (this->equal(cur->key, (_key), this->ctx))

/****************************************************************************
 * Private Function
 ****************************************************************************/

static inline size_t hash_bits(size_t h, int bits)
{
    /* shuffle bits and return requested number of upper bits */
    if (bits == 0) {
        return 0;
    }

#if (__SIZEOF_SIZE_T__ == __SIZEOF_LONG_LONG__)

    /* LP64 case */
    return (h * 11400714819323198485llu) >> (__SIZEOF_LONG_LONG__ * 8 - bits);
#elif (__SIZEOF_SIZE_T__ <= __SIZEOF_LONG__)
    return (h * 2654435769lu) >> (__SIZEOF_LONG__ * 8 - bits);
#else
#error "Unsupported size_t size"
#endif
}

/* generic C-string hashing function */
static inline size_t str_hash(const char* s)
{
    size_t h = 0;

    while (*s) {
        h = h * 31 + *s;
        s++;
    }
    return h;
}

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct HashMap_Entry;
typedef struct HashMap_Entry HashMap_Entry;

struct HashMap_Entry {
    union {
        long key;
        const void* pkey;
    };

    union {
        long value;
        void* pvalue;
    };

    struct HashMap_Entry* next;
};

typedef void (*HashMap_Entry_Callback)(const void* key, void* value);

/* Base object for HashMap */
struct HashMapBase;
typedef struct HashMapBase HashMapBase;

struct HashMapBase {
    void (*dtor)(HashMapBase* this);

    int (*insert)(HashMapBase* this, long key, long value,
        enum hashmap_insert_strategy strategy, long* old_key,
        long* old_value);
    bool (*delete)(HashMapBase* this, long key, long* old_key, long* old_value);
    bool (*find)(HashMapBase* this, long key, long* value);
    void (*iterator)(HashMapBase* this, HashMap_Entry_Callback cb);
    void (*clear)(HashMapBase* this);

    size_t (*hash)(void* this, long key);
    bool (*equal)(void* this, long key1, long key2);

    struct HashMap_Entry** buckets;
    size_t capacity;
    size_t cap_bits;
    size_t size;
};

void HashMapBase_ctor(HashMapBase* this);

struct HashMap;
typedef struct HashMap HashMap;

struct HashMap {
    HashMapBase m_HashMap;

    void (*dtor)(HashMap* this);

    long (*get)(HashMap* this, long key);
    uint32_t (*put)(HashMap* this, long key, long value);
    uint32_t (*erase)(HashMap* this, long key);
    uint32_t (*find)(HashMap* this, long key, long* value);
    uint32_t (*insert)(HashMap* this, long key, long value);
    void (*clear)(HashMap* this);
    void (*iterator)(HashMap* this, HashMap_Entry_Callback cb);
    uint32_t (*size)(HashMap* this);
};

void HashMap_String_ctor(HashMap* this);
void HashMap_ctor(HashMap* this);

#endif //__BINDER_INCLUDE_UTILS_HASHMAP_H__
