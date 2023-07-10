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

#define LOG_TAG "RefBase"

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

#define INITIAL_STRONG_VALUE (1 << 28)
#define MAX_COUNT 0xfffff

/* Test whether the argument is a clearly invalid strong reference count.
 * Used only for error checking on the value before an atomic decrement.
 * Intended to be very cheap.
 * Note that we cannot just check for excess decrements by comparing to zero
 * since the object would be deallocated before that.
 */

#define BAD_STRONG(c) \
    ((c) == 0 || ((c) & (~(MAX_COUNT | INITIAL_STRONG_VALUE))) != 0)

/* Same for weak counts. */

#define BAD_WEAK(c) ((c) == 0 || ((c) & (~MAX_COUNT)) != 0)

static void RefBase_weakref_impl_addStrongRef(RefBase_weakref_impl* this, const void* id)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_removeStrongRef(RefBase_weakref_impl* this, const void* id)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_renameStrongRefId(RefBase_weakref_impl* this,
    const void* old_id, const void* new_id)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_addWeakRef(RefBase_weakref_impl* this, const void* id)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_removeWeakRef(RefBase_weakref_impl* this, const void* id)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_renameWeakRefId(RefBase_weakref_impl* this,
    const void* old_id, const void* new_id)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_printRefs(RefBase_weakref_impl* this)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_trackMe(RefBase_weakref_impl* this, bool, bool)
{
    BINDER_LOGV("calling\n");
}

static void RefBase_weakref_impl_incWeak(RefBase_weakref_impl* this, const void* id)
{
    this->m_RefBase_weakref.incWeak(&this->m_RefBase_weakref, id);
}

static void RefBase_weakref_impl_decWeak(RefBase_weakref_impl* this, const void* id)
{
    this->m_RefBase_weakref.decWeak(&this->m_RefBase_weakref, id);
}

static void RefBase_weakref_impl_dtor(RefBase_weakref_impl* this)
{
    this->m_RefBase_weakref.dtor(&this->m_RefBase_weakref);
}

static void RefBase_weakref_impl_ctor(RefBase_weakref_impl* this, RefBase* base)
{
    RefBase_weakref_ctor(&this->m_RefBase_weakref);

    this->mBase = base;
    atomic_init(&this->mStrong, INITIAL_STRONG_VALUE);
    atomic_init(&this->mWeak, 0);
    atomic_init(&this->mFlags, OBJECT_LIFETIME_STRONG);

    /* Null function */
    this->addStrongRef = RefBase_weakref_impl_addStrongRef;
    this->removeStrongRef = RefBase_weakref_impl_removeStrongRef;
    this->renameStrongRefId = RefBase_weakref_impl_renameStrongRefId;
    this->addWeakRef = RefBase_weakref_impl_addWeakRef;
    this->removeWeakRef = RefBase_weakref_impl_removeWeakRef;
    this->renameWeakRefId = RefBase_weakref_impl_renameWeakRefId;
    this->printRefs = RefBase_weakref_impl_printRefs;
    this->trackMe = RefBase_weakref_impl_trackMe;

    /* inherit RefBase_weakref */
    this->incWeak = RefBase_weakref_impl_incWeak;
    this->decWeak = RefBase_weakref_impl_decWeak;

    this->dtor = RefBase_weakref_impl_dtor;
}

RefBase_weakref_impl* RefBase_weakref_impl_new(RefBase* base)
{
    RefBase_weakref_impl* this;
    this = zalloc(sizeof(RefBase_weakref_impl));

    RefBase_weakref_impl_ctor(this, base);
    return this;
}

void RefBase_weakref_impl_delete(RefBase_weakref_impl* this)
{
    BINDER_LOGE("need to check");
    /*
    this->dtor(this);
    free(this);
    */
}

static RefBase* RefBase_weakref_refBase(RefBase_weakref* this)
{
    return ((RefBase_weakref_impl*)(this))->mBase;
}

static void RefBase_weakref_incWeak(RefBase_weakref* this, const void* id)
{
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);
    impl->addWeakRef(impl, id);
    const int32_t c = atomic_fetch_add_explicit(&impl->mWeak, 1, memory_order_relaxed);
    LOG_ASSERT(c >= 0, "incWeak called on %p after last weak ref", this);
}

static void RefBase_weakref_incWeakRequireWeak(RefBase_weakref* this, const void* id)
{
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);
    impl->addWeakRef(impl, id);
    const int32_t c = atomic_fetch_add_explicit(&impl->mWeak, 1, memory_order_relaxed);
    LOG_FATAL_IF(c <= 0, "incWeakRequireWeak called on %p which has no weak refs", this);
}

static void RefBase_weakref_decWeak(RefBase_weakref* this, const void* id)
{
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);

    impl->removeWeakRef(impl, id);
    const int32_t c = atomic_fetch_sub_explicit(&impl->mWeak, 1, memory_order_release);
    LOG_FATAL_IF(BAD_WEAK(c), "decWeak called on %p too many times", this);
    if (c != 1)
        return;

    atomic_thread_fence(memory_order_acquire);
    int32_t flags = atomic_load_explicit(&impl->mFlags, memory_order_relaxed);
    if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
        if (atomic_load_explicit(&impl->mStrong, memory_order_relaxed)
            == INITIAL_STRONG_VALUE) {
            BINDER_LOGE("RefBase: Object at %p lost last weak reference "
                        "before it had a strong reference",
                impl->mBase);
        } else {
            RefBase_weakref_impl_delete(impl);
        }
    } else {
        impl->mBase->onLastWeakRef(impl->mBase, id);
        RefBase_delete(impl->mBase);
    }
}

static bool RefBase_weakref_attemptIncStrong(RefBase_weakref* this, const void* id)
{
    this->incWeak(this, id);
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);

    int32_t curCount = atomic_load_explicit(&impl->mStrong, memory_order_relaxed);
    LOG_ASSERT(curCount >= 0,
        "attemptIncStrong called on %p after underflow", this);

    while (curCount > 0 && curCount != INITIAL_STRONG_VALUE) {
        if (atomic_compare_exchange_weak(&impl->mStrong, &curCount, curCount + 1)) {
            break;
        }
    }
    if (curCount <= 0 || curCount == INITIAL_STRONG_VALUE) {
        int32_t flags = atomic_load_explicit(&impl->mFlags, memory_order_relaxed);
        if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
            if (curCount <= 0) {
                this->decWeak(this, id);
                return false;
            }
            while (curCount > 0) {
                if (atomic_compare_exchange_weak(&impl->mStrong, &curCount, curCount + 1)) {
                    break;
                }
            }
            if (curCount <= 0) {
                this->decWeak(this, id);
                return false;
            }
        } else {
            if (!impl->mBase->onIncStrongAttempted(impl->mBase, FIRST_INC_STRONG, id)) {
                this->decWeak(this, id);
                return false;
            }
            curCount = atomic_fetch_add_explicit(&impl->mStrong, 1, memory_order_relaxed);
            if (curCount != 0 && curCount != INITIAL_STRONG_VALUE) {
                impl->mBase->onLastStrongRef(impl->mBase, id);
            }
        }
    }
    impl->addStrongRef(impl, id);

    BINDER_LOGV("attemptIncStrong of %p from %p: cnt=%" PRIi32 "\n", this, id, curCount);
    if (curCount == INITIAL_STRONG_VALUE) {
        atomic_fetch_sub_explicit(&impl->mStrong, INITIAL_STRONG_VALUE,
            memory_order_relaxed);
    }
    return true;
}

static bool RefBase_weakref_attemptIncWeak(RefBase_weakref* this, const void* id)
{
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);

    int32_t curCount = atomic_load_explicit(&impl->mWeak, memory_order_relaxed);
    LOG_ASSERT(curCount >= 0, "attemptIncWeak called on %p after underflow",
        this);

    while (curCount > 0) {
        if (atomic_compare_exchange_weak(&impl->mWeak, &curCount, curCount + 1)) {
            break;
        }
        // curCount has been updated.
    }
    if (curCount > 0) {
        impl->addWeakRef(impl, id);
    }
    return curCount > 0;
}

static int32_t RefBase_weakref_getWeakCount(RefBase_weakref* this)
{
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);
    return atomic_load_explicit(&impl->mWeak, memory_order_relaxed);
}

static void RefBase_weakref_printRefs(RefBase_weakref* this)
{
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);
    impl->printRefs(impl);
}

static void RefBase_weakref_trackMe(RefBase_weakref* this, bool enable, bool retain)
{
    RefBase_weakref_impl* const impl = (RefBase_weakref_impl*)(this);
    impl->trackMe(impl, enable, retain);
}

static void RefBase_weakref_dtor(RefBase_weakref* this)
{
}

void RefBase_weakref_ctor(RefBase_weakref* this)
{
    this->refBase = RefBase_weakref_refBase;
    this->incWeak = RefBase_weakref_incWeak;
    this->incWeakRequireWeak = RefBase_weakref_incWeakRequireWeak;
    this->decWeak = RefBase_weakref_decWeak;
    this->attemptIncStrong = RefBase_weakref_attemptIncStrong;
    this->attemptIncWeak = RefBase_weakref_attemptIncWeak;
    this->getWeakCount = RefBase_weakref_getWeakCount;
    this->printRefs = RefBase_weakref_printRefs;
    this->trackMe = RefBase_weakref_trackMe;

    this->dtor = RefBase_weakref_dtor;
}

static void RefBase_onFirstRef(RefBase* this)
{
}

static void RefBase_onLastStrongRef(RefBase* this, const void* id)
{
}
static bool RefBase_onIncStrongAttempted(RefBase* this, uint32_t flags, const void* id)
{
    return (flags & FIRST_INC_STRONG) ? true : false;
}

static void RefBase_onLastWeakRef(RefBase* this, const void* id)
{
}

static void RefBase_extendObjectLifetime(RefBase* this, int32_t mode)
{
    atomic_fetch_or_explicit(&this->mRefs->mFlags, mode, memory_order_relaxed);
}

static void RefBase_incStrong(RefBase* this, const void* id)
{
    RefBase_weakref_impl* const refs = this->mRefs;

    refs->incWeak(refs, id);
    refs->addStrongRef(refs, id);

    const int32_t c = atomic_fetch_add_explicit(&refs->mStrong, 1, memory_order_relaxed);
    LOG_ASSERT(c > 0, "incStrong() called on %p after last strong ref", refs);
    BINDER_LOGV("incStrong of %p from %p: cnt=%" PRIi32 "\n", this, id, c);

    if (c != INITIAL_STRONG_VALUE) {
        return;
    }
    int32_t old = atomic_fetch_sub_explicit(&refs->mStrong, INITIAL_STRONG_VALUE,
        memory_order_relaxed);

    LOG_ASSERT(old > INITIAL_STRONG_VALUE, "0x%" PRIi32 " too small", old);
    refs->mBase->onFirstRef(refs->mBase);
}

static void RefBase_incStrongRequireStrong(RefBase* this, const void* id)
{
    RefBase_weakref_impl* const refs = this->mRefs;

    refs->incWeak(refs, id);
    refs->addStrongRef(refs, id);

    const int32_t c = atomic_fetch_add_explicit(&refs->mStrong, 1, memory_order_relaxed);
    LOG_FATAL_IF(c <= 0 || c == INITIAL_STRONG_VALUE,
        "incStrongRequireStrong() called on %p which isn't already owned", refs);
    BINDER_LOGV("incStrong (requiring strong) of %p from %p: cnt=%" PRIi32 "\n", this, id, c);
}

static void RefBase_decStrong(RefBase* this, const void* id)
{
    RefBase_weakref_impl* const refs = this->mRefs;

    refs->removeStrongRef(refs, id);

    const int32_t c = atomic_fetch_sub_explicit(&refs->mStrong, 1, memory_order_release);
    BINDER_LOGV("decStrong of %p from %p: cnt=%" PRIi32 "\n", this, id, c);
    LOG_FATAL_IF(BAD_STRONG(c), "decStrong() called on %p too many times", refs);

    if (c == 1) {
        atomic_thread_fence(memory_order_acquire);
        refs->mBase->onLastStrongRef(refs->mBase, id);
        int32_t flags = atomic_load_explicit(&refs->mFlags, memory_order_relaxed);
        if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
            RefBase_delete(this);
        }
    }
    refs->decWeak(refs, id);
}

void RefBase_forceIncStrong(RefBase* this, const void* id)
{
    RefBase_weakref_impl* const refs = this->mRefs;
    refs->incWeak(refs, id);
    refs->addStrongRef(refs, id);

    const int32_t c = atomic_fetch_add_explicit(&refs->mStrong, 1, memory_order_relaxed);
    LOG_ASSERT(c >= 0, "forceIncStrong called on %p after ref count underflow", refs);
    BINDER_LOGV("forceIncStrong of %p from %p: cnt=%" PRIi32 "\n", this, id, c);

    switch (c) {
    case INITIAL_STRONG_VALUE:
        atomic_fetch_sub_explicit(&refs->mStrong, INITIAL_STRONG_VALUE,
            memory_order_relaxed);
    case 0:
        refs->mBase->onFirstRef(refs->mBase);
    }
}

static int32_t RefBase_getStrongCount(RefBase* this)
{
    return atomic_load_explicit(&this->mRefs->mStrong, memory_order_relaxed);
}

static RefBase_weakref* RefBase_createWeak(RefBase* this, const void* id)
{
    this->mRefs->incWeak(this->mRefs, id);
    return (RefBase_weakref*)this->mRefs;
}

static RefBase_weakref* RefBase_getWeakRefs(RefBase* this)
{
    return (RefBase_weakref*)this->mRefs;
}

static void RefBase_printRefs(RefBase* this)
{
    RefBase_weakref* refs = this->getWeakRefs(this);
    refs->printRefs(refs);
}

static void RefBase_trackMe(RefBase* this, bool enable, bool retain)
{
    RefBase_weakref* const refs = this->getWeakRefs(this);
    refs->trackMe(refs, enable, retain);
}

static void RefBase_dtor(RefBase* this)
{
    int32_t flags = atomic_load_explicit(&this->mRefs->mFlags, memory_order_relaxed);
    if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_WEAK) {
        int32_t Weak = atomic_load_explicit(&this->mRefs->mWeak, memory_order_relaxed);
        if (Weak == 0) {
            RefBase_weakref_impl_delete(this->mRefs);
        }
    } else {
        int32_t strongs = atomic_load_explicit(&this->mRefs->mStrong, memory_order_relaxed);
        if (strongs == INITIAL_STRONG_VALUE) {
            BINDER_LOGE("RefBase: Explicit destruction, weak count = %d (in %p). "
                        "Use sp<> to manage this object.",
                this->mRefs->mWeak, this);
        } else if (strongs != 0) {
            LOG_FATAL("RefBase: object %p with strong count %" PRIi32 " deleted. Double owned?",
                this, strongs);
        }
    }
    this->mRefs = NULL;
}

void RefBase_ctor(RefBase* this)
{
    this->mRefs = RefBase_weakref_impl_new(this);

    /* public */
    this->incStrong = RefBase_incStrong;
    this->incStrongRequireStrong = RefBase_incStrongRequireStrong;
    this->decStrong = RefBase_decStrong;
    this->forceIncStrong = RefBase_forceIncStrong;
    this->getStrongCount = RefBase_getStrongCount;
    this->printRefs = RefBase_printRefs;
    this->trackMe = RefBase_trackMe;
    this->createWeak = RefBase_createWeak;
    this->getWeakRefs = RefBase_getWeakRefs;

    /* protected */
    this->extendObjectLifetime = RefBase_extendObjectLifetime;

    /* virtual function */
    this->onFirstRef = RefBase_onFirstRef;
    this->onLastStrongRef = RefBase_onLastStrongRef;
    this->onIncStrongAttempted = RefBase_onIncStrongAttempted;
    this->onLastWeakRef = RefBase_onLastWeakRef;

    this->dtor = RefBase_dtor;
}

RefBase* RefBase_new(void)
{
    RefBase* this;
    this = zalloc(sizeof(RefBase));

    RefBase_ctor(this);
    return this;
}

void RefBase_delete(RefBase* this)
{
    BINDER_LOGE("need to check");
    /*
    this->dtor(this);
    free(this);
    */
}
