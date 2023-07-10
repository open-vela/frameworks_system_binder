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

#define LOG_TAG "Parcel"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <nuttx/android/binder.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/endian.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Binder.h"
#include "BpBinder.h"
#include "IPCThreadState.h"
#include "Parcel.h"
#include "ProcessGlobal.h"
#include "ProcessState.h"
#include "Stability.h"
#include "Status.h"
#include "utils/BinderString.h"
#include "utils/Binderlog.h"

static void release_object(ProcessState* proc, struct flat_binder_object* obj,
    const void* who);

#define gParcelGlobalAllocCount (Parcel_global_get()->gParcelGlobalAllocCount)
#define gParcelGlobalAllocSize (Parcel_global_get()->gParcelGlobalAllocSize)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define PAD_SIZE_UNSAFE(s) (((s) + 3) & ~3UL)

const int32_t kHeader = B_PACK_CHARS('S', 'Y', 'S', 'T');

#define STRICT_MODE_PENALTY_GATHER (1 << 31)

static inline size_t pad_size(size_t s)
{
    return PAD_SIZE_UNSAFE(s);
}

static uint8_t* reallocZeroFree(uint8_t* data, size_t oldCapacity,
    size_t newCapacity, bool zero)
{
    if (!zero) {
        return (uint8_t*)realloc(data, newCapacity);
    }

    uint8_t* newData = (uint8_t*)malloc(newCapacity);
    if (!newData) {
        return NULL;
    }

    memcpy(newData, data, min(oldCapacity, newCapacity));
    memset(data, 0, oldCapacity);
    free(data);
    return newData;
}

static int32_t continueWrite(Parcel* this, size_t desired)
{
    if (desired > INT32_MAX) {
        /* don't accept size_t values which may have come from an
         * inadvertent conversion from a negative int.
         */

        return STATUS_BAD_VALUE;
    }

    /* If shrinking, first adjust for any objects that appear
     * after the new data size.
     */

    size_t objectsSize = this->mObjectsSize;
    if (desired < this->mDataSize) {
        if (desired == 0) {
            objectsSize = 0;
        } else {
            while (objectsSize > 0) {
                if (this->mObjects[objectsSize - 1] < desired)
                    break;
                objectsSize--;
            }
        }
    }

    if (this->mOwner) {
        /* If the size is going to zero, just release the owner's data. */

        if (desired == 0) {
            Parcel_freeData(this);
            return STATUS_OK;
        }

        /* If there is a different owner, we need to take
         * posession.
         */

        uint8_t* data = (uint8_t*)malloc(desired);
        if (!data) {
            this->mError = STATUS_NO_MEMORY;
            return STATUS_NO_MEMORY;
        }
        binder_size_t* objects = NULL;

        if (objectsSize) {
            objects = (binder_size_t*)calloc(objectsSize, sizeof(binder_size_t));
            if (!objects) {
                free(data);

                this->mError = STATUS_NO_MEMORY;
                return STATUS_NO_MEMORY;
            }

            /* Little hack to only acquire references on objects
             * we will be keeping.
             */

            size_t oldObjectsSize = this->mObjectsSize;
            this->mObjectsSize = objectsSize;
            Parcel_acquireObjects(this);
            this->mObjectsSize = oldObjectsSize;
        }

        if (this->mData) {
            memcpy(data, this->mData, this->mDataSize < desired ? this->mDataSize : desired);
        }
        if (objects && this->mObjects) {
            memcpy(objects, this->mObjects, objectsSize * sizeof(binder_size_t));
        }

        this->mOwner(this, this->mData, this->mDataSize, this->mObjects, this->mObjectsSize);
        this->mOwner = NULL;

        BINDER_LOGD("Parcel %p: taking ownership of %zu capacity", this, desired);

        gParcelGlobalAllocSize += desired;
        gParcelGlobalAllocCount++;

        this->mData = data;
        this->mObjects = objects;
        this->mDataSize = (this->mDataSize < desired) ? this->mDataSize : desired;

        BINDER_LOGV("continueWrite Setting data size of %p to %zu", this, this->mDataSize);

        this->mDataCapacity = desired;
        this->mObjectsSize = this->mObjectsCapacity = objectsSize;
        this->mNextObjectHint = 0;
        this->mObjectsSorted = false;

    } else if (this->mData) {
        if (objectsSize < this->mObjectsSize) {
            /* Need to release refs on any objects we are dropping. */

            ProcessState* proc = ProcessState_self();
            for (size_t i = objectsSize; i < this->mObjectsSize; i++) {
                struct flat_binder_object* flat
                    = (struct flat_binder_object*)(this->mData + this->mObjects[i]);
                if (flat->hdr.type == BINDER_TYPE_FD) {
                    /* will need to rescan because we may have lopped off the only FDs */
                    this->mFdsKnown = false;
                }
                release_object(proc, flat, this);
            }

            if (objectsSize == 0) {
                free(this->mObjects);
                this->mObjects = NULL;
                this->mObjectsCapacity = 0;
            } else {
                binder_size_t* objects = (binder_size_t*)realloc(this->mObjects, objectsSize * sizeof(binder_size_t));
                if (objects) {
                    this->mObjects = objects;
                    this->mObjectsCapacity = objectsSize;
                }
            }
            this->mObjectsSize = objectsSize;
            this->mNextObjectHint = 0;
            this->mObjectsSorted = false;
        }

        /* We own the data, so we can just do a realloc(). */

        if (desired > this->mDataCapacity) {
            uint8_t* data = reallocZeroFree(this->mData, this->mDataCapacity,
                desired, this->mDeallocZero);

            if (data) {
                BINDER_LOGV("Parcel %p: continue from %zu to %zu capacity", this, this->mDataCapacity,
                    desired);
                gParcelGlobalAllocSize += desired;
                gParcelGlobalAllocSize -= this->mDataCapacity;
                this->mData = data;
                this->mDataCapacity = desired;
            } else {
                this->mError = STATUS_NO_MEMORY;
                return STATUS_NO_MEMORY;
            }
        } else {
            if (this->mDataSize > desired) {
                this->mDataSize = desired;
                BINDER_LOGV("continueWrite Setting data size of %p to %zu", this, this->mDataSize);
            }
            if (this->mDataPos > desired) {
                this->mDataPos = desired;
                BINDER_LOGV("continueWrite Setting data pos of %p to %zu", this, this->mDataPos);
            }
        }
    } else {
        // This is the first data.  Easy!
        uint8_t* data = (uint8_t*)malloc(desired);
        if (!data) {
            this->mError = STATUS_NO_MEMORY;
            return STATUS_NO_MEMORY;
        }

        if (!(this->mDataCapacity == 0 && this->mObjects == NULL
                && this->mObjectsCapacity == 0)) {
            BINDER_LOGE("continueWrite: %zu/%p/%zu/%zu", this->mDataCapacity,
                this->mObjects, this->mObjectsCapacity, desired);
        }

        BINDER_LOGV("Parcel %p: allocating with %zu capacity", this, desired);
        gParcelGlobalAllocSize += desired;
        gParcelGlobalAllocCount++;

        this->mData = data;
        this->mDataSize = this->mDataPos = 0;
        BINDER_LOGV("continueWrite Setting data size of %p to %zu", this, this->mDataSize);
        BINDER_LOGV("continueWrite Setting data pos of %p to %zu", this, this->mDataPos);
        this->mDataCapacity = desired;
    }

    return STATUS_OK;
}

static int32_t readAligned(Parcel* this, uint8_t* pArg, size_t size)
{
    if ((this->mDataPos + size) <= this->mDataSize) {
        memcpy(pArg, this->mData + this->mDataPos, size);
        this->mDataPos += size;
        return STATUS_OK;
    } else {
        return STATUS_NOT_ENOUGH_DATA;
    }
}

static int32_t finishWrite(Parcel* this, size_t len)
{
    if (len > INT32_MAX) {
        /* don't accept size_t values which may have come from an
         * inadvertent conversion from a negative int.
         */
        return STATUS_BAD_VALUE;
    }
    this->mDataPos += len;
    BINDER_LOGV("finishWrite Setting data pos of %p to %zu", this, this->mDataPos);
    if (this->mDataPos > this->mDataSize) {
        this->mDataSize = this->mDataPos;
        BINDER_LOGV("finishWrite Setting data size of %p to %zu", this, this->mDataSize);
    }
    return STATUS_OK;
}

static int32_t growData(Parcel* this, size_t len)
{
    if (len > INT32_MAX) {
        // don't accept size_t values which may have come from an
        // inadvertent conversion from a negative int.
        return STATUS_BAD_VALUE;
    }
    if (len > SIZE_MAX - this->mDataSize)
        return STATUS_NO_MEMORY; // overflow
    if (this->mDataSize + len > SIZE_MAX / 3)
        return STATUS_NO_MEMORY; // overflow
    size_t newSize = ((this->mDataSize + len) * 3) / 2;
    return (newSize <= this->mDataSize)
        ? STATUS_NO_MEMORY
        : continueWrite(this, max(newSize, (size_t)128));
}

static int32_t writeAligned(Parcel* this, uint8_t* pArg, size_t size)
{
    if ((this->mDataPos + size) <= this->mDataCapacity) {
    restart_write:
        memcpy(this->mData + this->mDataPos, pArg, size);
        return finishWrite(this, size);
    }

    int32_t err = growData(this, size);
    if (err == STATUS_OK)
        goto restart_write;
    return err;
}

static void* writeInplace(Parcel* this, size_t len)
{
    if (len > INT32_MAX) {
        return NULL;
    }
    const size_t padded = pad_size(len);

    /* check for integer overflow */
    if (this->mDataPos + padded < this->mDataPos) {
        return NULL;
    }

    if ((this->mDataPos + padded) <= this->mDataCapacity) {
    restart_write:
        uint8_t* const data = this->mData + this->mDataPos;
        if (padded != len) {
#if BYTE_ORDER == BIG_ENDIAN
            static const uint32_t mask[4] = {
                0x00000000, 0xffffff00, 0xffff0000, 0xff000000
            };
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
            static const uint32_t mask[4] = {
                0x00000000, 0x00ffffff, 0x0000ffff, 0x000000ff
            };
#endif
            *(uint32_t*)(data + padded - 4) &= mask[padded - len];
        }

        finishWrite(this, padded);
        return data;
    }

    int32_t err = growData(this, padded);
    if (err == STATUS_OK)
        goto restart_write;
    return NULL;
}

static const void* readInplace(Parcel* this, size_t len)
{
    if (len > INT32_MAX) {
        return NULL;
    }

    if ((this->mDataPos + pad_size(len)) >= this->mDataPos && (this->mDataPos + pad_size(len)) <= this->mDataSize && len <= pad_size(len)) {
        const void* data = this->mData + this->mDataPos;
        this->mDataPos += pad_size(len);
        BINDER_LOGV("readInplace Setting data pos of %p to %zu",
            this, this->mDataPos);
        return data;
    }
    return NULL;
}

static const char* readString8Inplace(Parcel* this, size_t* outLen)
{
    int32_t size;

    Parcel_readInt32(this, &size);

    /* watch for potential int overflow from size+1*/

    if (size >= 0 && size < INT32_MAX) {
        *outLen = size;
        const char* str = (const char*)readInplace(this, size + 1);
        if (str != NULL) {
            if (str[size] == '\0') {
                return str;
            }
            LOG_FATAL("172655291");
        }
    }
    *outLen = 0;
    return NULL;
}

static void acquire_object(ProcessState* proc, struct flat_binder_object* obj,
    const void* who)
{
    IBinder* ibinder;

    switch (obj->hdr.type) {
    case BINDER_TYPE_BINDER:
        if (obj->binder) {
            ibinder = (IBinder*)(obj->cookie);
            BINDER_LOGV("Parcel %p acquiring reference on local %lx", who, obj->cookie);
            ibinder->incStrong(ibinder, who);
        }
        return;
    case BINDER_TYPE_HANDLE: {
        ibinder = proc->getStrongProxyForHandle(proc, obj->handle);
        if (ibinder != NULL) {
            BINDER_LOGV("Parcel %p acquiring reference on remote %p", who, ibinder);
            ibinder->incStrong(ibinder, who);
        }
        return;
    }
    case BINDER_TYPE_FD: {
        return;
    }
    }
    BINDER_LOGE("Invalid object type 0x%08" PRIx32, obj->hdr.type);
}

static void release_object(ProcessState* proc, struct flat_binder_object* obj,
    const void* who)
{
    IBinder* ibinder;

    switch (obj->hdr.type) {
    case BINDER_TYPE_BINDER:
        if (obj->binder) {
            ibinder = (IBinder*)(obj->cookie);
            BINDER_LOGV("Parcel %p releasing reference on local %lx", who, obj->cookie);
            ibinder->decStrong(ibinder, who);
        }
        return;
    case BINDER_TYPE_HANDLE: {
        ibinder = proc->getStrongProxyForHandle(proc, obj->handle);
        if (ibinder != NULL) {
            BINDER_LOGV("Parcel %p releasing reference on remote %p", who, ibinder);
            ibinder->decStrong(ibinder, who);
        }
        return;
    }
    case BINDER_TYPE_FD: {
        if (obj->cookie != 0) {
            close(obj->handle);
        }
        return;
    }
    }
    BINDER_LOGE("Invalid object type 0x%08" PRIx32, obj->hdr.type);
}

static int32_t Parcel_hasFileDescriptorsInRange(Parcel* this, size_t offset, size_t len, bool* result)
{
    if (len > INT32_MAX || offset > INT32_MAX) {
        /* Don't accept size_t values which may have come from
         * an inadvertent conversion from a negative int.
         */
        return STATUS_BAD_VALUE;
    }
    size_t limit;
    if (__builtin_add_overflow(offset, len, &limit) || limit > this->mDataSize) {
        return STATUS_BAD_VALUE;
    }
    *result = false;
    for (size_t i = 0; i < this->mObjectsSize; i++) {
        size_t pos = this->mObjects[i];
        if (pos < offset)
            continue;
        if (pos + sizeof(struct flat_binder_object) > offset + len) {
            if (this->mObjectsSorted)
                break;
            else
                continue;
        }
        struct flat_binder_object* flat = (struct flat_binder_object*)(this->mData + pos);
        if (flat->hdr.type == BINDER_TYPE_FD) {
            *result = true;
            break;
        }
    }
    return STATUS_OK;
}

static void Parcel_scanForFds(Parcel* this)
{
    int32_t status = Parcel_hasFileDescriptorsInRange(this, 0, Parcel_dataSize(this),
        &this->mHasFds);
    if (status != STATUS_OK) {
        BINDER_LOGE("Error %" PRId32 " calling hasFileDescriptorsInRange()", status);
    }
    this->mFdsKnown = true;
}

static void Parcel_updateWorkSourceRequestHeaderPosition(Parcel* this)
{
    /* Only update the request headers once. We only want to point
     * to the first headers read/written.
     */

    if (!this->mRequestHeaderPresent) {
        this->mWorkSourceRequestHeaderPosition = Parcel_dataPosition(this);
        this->mRequestHeaderPresent = true;
    }
}

int32_t Parcel_writeObject(Parcel* this, struct flat_binder_object* val, bool nullMetaData)
{
    const bool enoughData = (this->mDataPos + sizeof(struct flat_binder_object))
        <= this->mDataCapacity;
    const bool enoughObjects = this->mObjectsSize < this->mObjectsCapacity;

    if (enoughData && enoughObjects) {
    restart_write:
        struct flat_binder_object* obj;
        obj = (struct flat_binder_object*)((this->mData + this->mDataPos));
        memcpy(obj, val, sizeof(struct flat_binder_object));

        /* remember if it's a file descriptor */

        if (val->hdr.type == BINDER_TYPE_FD) {
            if (!this->mAllowFds) {
                /* fail before modifying our object index */
                return STATUS_FDS_NOT_ALLOWED;
            }
            this->mHasFds = this->mFdsKnown = true;
        }
        /* Need to write meta-data? */
        if (nullMetaData || val->binder != 0) {
            this->mObjects[this->mObjectsSize] = this->mDataPos;
            acquire_object(ProcessState_self(), val, this);
            this->mObjectsSize++;
        }
        return finishWrite(this, sizeof(struct flat_binder_object));
    }
    if (!enoughData) {
        const int32_t err = growData(this, sizeof(val));
        if (err != STATUS_OK)
            return err;
    }
    if (!enoughObjects) {
        if (this->mObjectsSize > SIZE_MAX - 2)
            return STATUS_NO_MEMORY;

        if ((this->mObjectsSize + 2) > SIZE_MAX / 3)
            return STATUS_NO_MEMORY;

        size_t newSize = ((this->mObjectsSize + 2) * 3) / 2;

        if (newSize > SIZE_MAX / sizeof(binder_size_t))
            return STATUS_NO_MEMORY; // overflow

        binder_size_t* objects = (binder_size_t*)realloc(this->mObjects, newSize * sizeof(binder_size_t));
        if (objects == NULL)
            return STATUS_NO_MEMORY;
        this->mObjects = objects;
        this->mObjectsCapacity = newSize;
    }
    goto restart_write;
}

struct flat_binder_object* Parcel_readObject(Parcel* this, bool nullMetaData)
{
    const size_t DPOS = this->mDataPos;
    if ((DPOS + sizeof(struct flat_binder_object)) <= this->mDataSize) {
        struct flat_binder_object* obj
            = (struct flat_binder_object*)(this->mData + DPOS);
        this->mDataPos = DPOS + sizeof(struct flat_binder_object);
        if (!nullMetaData && (obj->cookie == 0 && obj->binder == 0)) {
            /* When transferring a NULL object, we don't write it into
             * the object list, so we don't want to check for it when
             * reading.
             */
            BINDER_LOGV("readObject Setting data pos of %p to %zu", this, this->mDataPos);
            return obj;
        }

        /* Ensure that this object is valid... */

        binder_size_t* const OBJS = this->mObjects;
        const size_t N = this->mObjectsSize;
        size_t opos = this->mNextObjectHint;
        if (N > 0) {
            BINDER_LOGV("Parcel %p looking for obj at %zu, hint=%zu",
                this, DPOS, opos);
            /* Start at the current hint position, looking for an object at
             * the current data position.
             */

            if (opos < N) {
                while (opos < (N - 1) && OBJS[opos] < DPOS) {
                    opos++;
                }
            } else {
                opos = N - 1;
            }
            if (OBJS[opos] == DPOS) {
                /* Found it! */
                BINDER_LOGV("Parcel %p found obj %zu at index %zu with forward search",
                    this, DPOS, opos);
                this->mNextObjectHint = opos + 1;
                BINDER_LOGV("readObject Setting data pos of %p to %zu", this, this->mDataPos);
                return obj;
            }
            /* Look backwards for it...*/
            while (opos > 0 && OBJS[opos] > DPOS) {
                opos--;
            }
            if (OBJS[opos] == DPOS) {
                /* Found it! */
                BINDER_LOGV("Parcel %p found obj %zu at index %zu with backward search",
                    this, DPOS, opos);
                this->mNextObjectHint = opos + 1;
                BINDER_LOGV("readObject Setting data pos of %p to %zu", this, this->mDataPos);
                return obj;
            }
        }
        BINDER_LOGW("Attempt to read object from Parcel %p at "
                    "offset %zu that is not in the object list",
            this, DPOS);
    }
    return NULL;
}

static int32_t Parcel_finishFlattenBinder(Parcel* this, IBinder* binder)
{
    int16_t rep;

    Stability_tryMarkCompilationUnit(binder);
    rep = Stability_getRepr(binder);
    return Parcel_writeInt32(this, rep);
}

static int32_t Parcel_finishUnflattenBinder(Parcel* this,
    IBinder* binder, IBinder** out)
{
    int32_t stability;
    int32_t status = Parcel_readInt32(this, &stability);
    if (status != STATUS_OK)
        return status;

    status = Stability_setRepr(binder, (int16_t)stability, true);
    if (status != STATUS_OK)
        return status;
    *out = binder;
    return STATUS_OK;
}

static inline int schedPolicyMask(int policy, int priority)
{
    return (priority & FLAT_BINDER_FLAG_PRIORITY_MASK) | ((policy & 3) << FLAT_BINDER_FLAG_SCHED_POLICY_SHIFT);
}

static int32_t Parcel_flattenBinder(Parcel* this, IBinder* binder)
{
    BBinder* local = NULL;
    if (binder) {
        local = binder->localBinder(binder);
    }
    if (local) {
        local->setParceled(local);
    }

    struct flat_binder_object obj;
    int schedBits = 0;
    IPCThreadState* self = IPCThreadState_self();

    if (!self->backgroundSchedulingDisabled(self)) {
        schedBits = schedPolicyMask(SCHED_NORMAL, 19);
    }
    if (binder != NULL) {
        if (!local) {
            BpBinder* proxy = (binder)->remoteBinder(binder);
            if (proxy == NULL) {
                BINDER_LOGE("null proxy");
            }
            int32_t handle;
            if (proxy) {
                PrivateAccessor* pAccessor = proxy->getPrivateAccessor(proxy);
                handle = pAccessor->binderHandle(pAccessor);
            } else {
                handle = 0;
            }
            obj.hdr.type = BINDER_TYPE_HANDLE;
            obj.binder = 0; /* Don't pass uninitialized stack data to a remote process */
            obj.flags = 0;
            obj.handle = handle;
            obj.cookie = 0;
        } else {
            int policy = local->getMinSchedulerPolicy(local);
            int priority = local->getMinSchedulerPriority(local);
            if (policy != 0 || priority != 0) {
                /* override value, since it is set explicitly */
                schedBits = schedPolicyMask(policy, priority);
            }
            obj.flags = FLAT_BINDER_FLAG_ACCEPTS_FDS;
            if (local->isRequestingSid(local)) {
                obj.flags |= FLAT_BINDER_FLAG_TXN_SECURITY_CTX;
            }
            if (local->isInheritRt(local)) {
                obj.flags |= FLAT_BINDER_FLAG_INHERIT_RT;
            }
            obj.hdr.type = BINDER_TYPE_BINDER;
            obj.binder = (uintptr_t)(local->getWeakRefs(local));
            obj.cookie = (uintptr_t)(local);
        }
    } else {
        obj.hdr.type = BINDER_TYPE_BINDER;
        obj.flags = 0;
        obj.binder = 0;
        obj.cookie = 0;
    }
    obj.flags |= schedBits;
    int32_t status = Parcel_writeObject(this, &obj, false);
    if (status != STATUS_OK)
        return status;
    return Parcel_finishFlattenBinder(this, binder);
}

static int32_t Parcel_unflattenBinder(Parcel* this, IBinder** out)
{
    struct flat_binder_object* flat = Parcel_readObject(this, false);

    if (flat) {
        switch (flat->hdr.type) {
        case BINDER_TYPE_BINDER: {
            IBinder* binder = (IBinder*)(flat->cookie);
            binder->incStrongRequireStrong(binder, (void*)binder);
            return Parcel_finishUnflattenBinder(this, binder, out);
        }
        case BINDER_TYPE_HANDLE: {
            ProcessState* ps = ProcessState_self();
            IBinder* binder = ps->getStrongProxyForHandle(ps, flat->handle);
            return Parcel_finishUnflattenBinder(this, binder, out);
        }
        }
    }
    return STATUS_BAD_TYPE;
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int32_t Parcel_readInt32(Parcel* this, int32_t* pArg)
{
    return readAligned(this, (uint8_t*)pArg, sizeof(int32_t));
}

int32_t Parcel_writeInt32(Parcel* this, int32_t val)
{
    return writeAligned(this, (uint8_t*)&val, sizeof(int32_t));
}

int32_t Parcel_writePointer(Parcel* this, uintptr_t val)
{
    return writeAligned(this, (uint8_t*)&val, sizeof(binder_uintptr_t));
}

uintptr_t Parcel_readPointer(Parcel* this)
{
    uintptr_t result;
    if (readAligned(this, (uint8_t*)&result, sizeof(uintptr_t)) != STATUS_OK) {
        result = 0;
    }
    return result;
}

int32_t Parcel_read(Parcel* this, void* outData, size_t len)
{
    if (len > INT32_MAX) {
        /* don't accept size_t values which may have come from an
         * inadvertent conversion from a negative int.
         */
        return STATUS_BAD_VALUE;
    }

    if ((this->mDataPos + pad_size(len)) >= this->mDataPos && (this->mDataPos + pad_size(len)) <= this->mDataSize && (len <= pad_size(len))) {
        memcpy(outData, this->mData + this->mDataPos, len);
        this->mDataPos += pad_size(len);
        BINDER_LOGV("read Setting data pos of %p to %zu", this, this->mDataPos);
        return STATUS_OK;
    }
    return STATUS_NOT_ENOUGH_DATA;
}

int32_t Parcel_write(Parcel* this, const void* data, size_t len)
{
    if (len > INT32_MAX) {
        return STATUS_BAD_VALUE;
    }
    void* const d = writeInplace(this, len);
    if (d) {
        memcpy(d, data, len);
        return STATUS_OK;
    }
    return this->mError;
}

int32_t Parcel_writeString16(Parcel* this, String* str)
{
    size_t len;

    if (str == NULL)
        return Parcel_writeInt32(this, -1);

    len = String_size(str);
    int32_t err = Parcel_writeInt32(this, len);
    if (err == STATUS_OK) {
        uint8_t* data = (uint8_t*)writeInplace(this, len + sizeof(char));
        if (data) {
            String_cpyto(str, data, len);
            *(char*)(data + len) = 0;
            return STATUS_OK;
        }
        err = this->mError;
    }
    return err;
}

const char* Parcel_readString16(Parcel* this)
{
    size_t len;
    const char* str = readString8Inplace(this, &len);
    if (str == NULL) {
        BINDER_LOGE("Reading a NULL string not supported here.\n");
        return NULL;
    }
    return str;
}

int32_t Parcel_readString16_to(Parcel* this, String* pArg)
{
    const char* str = Parcel_readString16(this);
    if (str != NULL) {
        String_init(pArg, str);
        return STATUS_OK;
    } else {
        return STATUS_BAD_VALUE;
    }
}

const uint8_t* Parcel_data(const Parcel* this)
{
    return this->mData;
}

size_t Parcel_dataSize(const Parcel* this)
{
    return (this->mDataSize > this->mDataPos ? this->mDataSize : this->mDataPos);
}

size_t Parcel_dataAvail(const Parcel* this)
{
    size_t result = Parcel_dataSize(this) - Parcel_dataPosition(this);
    if (result > INT32_MAX) {
        LOG_FATAL("result too big: %zu", result);
    }
    return result;
}

size_t Parcel_dataPosition(const Parcel* this)
{
    return this->mDataPos;
}

size_t Parcel_dataCapacity(const Parcel* this)
{
    return this->mDataCapacity;
}

void Parcel_setError(Parcel* this, int32_t err)
{
    this->mError = err;
}

int32_t Parcel_errorCheck(const Parcel* this)
{
    return this->mError;
}

int32_t Parcel_setDataSize(Parcel* this, size_t size)
{
    if (size > INT32_MAX) {
        return STATUS_BAD_VALUE;
    }
    int32_t err;
    err = continueWrite(this, size);
    if (err == STATUS_OK) {
        this->mDataSize = size;
        BINDER_LOGV("setDataSize Setting data size of %p to %zu",
            this, this->mDataSize);
    }
    return err;
}

int32_t Parcel_setDataCapacity(Parcel* this, size_t size)
{
    if (size > INT32_MAX) {
        return STATUS_BAD_VALUE;
    }

    if (size > this->mDataCapacity)
        return continueWrite(this, size);
    return STATUS_OK;
}

uintptr_t Parcel_ipcData(const Parcel* this)
{
    return (uintptr_t)(this->mData);
}

size_t Parcel_ipcDataSize(const Parcel* this)
{
    return Parcel_dataSize(this);
}

void Parcel_markForBinder(Parcel* this, const IBinder* binder)
{
    /* At present, nothing need to be done for IPC */
}

void Parcel_initState(Parcel* this)
{
    this->mError = STATUS_OK;
    this->mData = NULL;
    this->mDataSize = 0;
    this->mDataCapacity = 0;
    this->mDataPos = 0;
    this->mObjects = NULL;
    this->mObjectsSize = 0;
    this->mObjectsCapacity = 0;
    this->mNextObjectHint = 0;
    this->mObjectsSorted = false;
    this->mHasFds = false;
    this->mFdsKnown = true;
    this->mAllowFds = true;
    this->mDeallocZero = false;
    this->mOwner = NULL;
    this->mWorkSourceRequestHeaderPosition = 0;
    this->mRequestHeaderPresent = false;
}

int32_t Parcel_dup(Parcel* new, const Parcel* old)
{
    // Parcel_setDataCapacity(new, old->mDataCapacity);
    // memcpy(new->mData, old->mData, old->mDataSize);
    new->mData = old->mData;
    new->mDataSize = old->mDataSize;
    new->mDataCapacity = old->mDataCapacity;
    new->mDataPos = old->mDataPos;
    new->mError = old->mError;

    new->mObjects = old->mObjects;
    new->mObjectsSize = old->mObjectsSize;
    new->mObjectsCapacity = old->mObjectsCapacity;
    new->mNextObjectHint = old->mNextObjectHint;
    new->mObjectsSorted = old->mObjectsSorted;
    new->mHasFds = old->mHasFds;
    new->mFdsKnown = old->mFdsKnown;
    new->mAllowFds = old->mAllowFds;
    new->mDeallocZero = old->mDeallocZero;
    new->mOwner = old->mOwner;

    new->mWorkSourceRequestHeaderPosition = old->mWorkSourceRequestHeaderPosition;
    new->mRequestHeaderPresent = old->mRequestHeaderPresent;

    return STATUS_OK;
}

void Parcel_releaseObjects(Parcel* this)
{
    size_t i = this->mObjectsSize;
    if (i == 0) {
        return;
    }

    ProcessState* proc = ProcessState_self();
    uint8_t* const data = this->mData;
    binder_size_t* const objects = this->mObjects;

    while (i > 0) {
        i--;
        struct flat_binder_object* flat
            = (struct flat_binder_object*)(data + objects[i]);
        release_object(proc, flat, this);
    }
}

void Parcel_acquireObjects(Parcel* this)
{
    size_t i = this->mObjectsSize;
    if (i == 0) {
        return;
    }

    ProcessState* proc = ProcessState_self();
    uint8_t* const data = this->mData;
    binder_size_t* const objects = this->mObjects;

    while (i > 0) {
        i--;
        struct flat_binder_object* flat
            = (struct flat_binder_object*)(data + objects[i]);
        acquire_object(proc, flat, this);
    }
}

void Parcel_freeDataNoInit(Parcel* this)
{
    if (this->mOwner) {
        BINDER_LOGV("Parcel %p: freeing other owner data", this);
        BINDER_LOGV("Freeing data ref of %p (pid=%d)", this, getpid());
        this->mOwner(this, this->mData, this->mDataSize, this->mObjects,
            this->mObjectsSize);
    } else {
        BINDER_LOGV("Parcel %p: freeing allocated data", this);
        Parcel_releaseObjects(this);
        if (this->mData) {
            BINDER_LOGV("Parcel %p: freeing with %zu capacity", this, this->mDataCapacity);
            gParcelGlobalAllocSize -= this->mDataCapacity;
            gParcelGlobalAllocCount--;
            if (this->mDeallocZero) {
                memset(this->mData, 0x0, this->mDataSize);
            }
            free(this->mData);
        }
        if (this->mObjects)
            free(this->mObjects);
    }
}

void Parcel_freeData(Parcel* this)
{
    Parcel_freeDataNoInit(this);
    Parcel_initState(this);
    ;
}

int Parcel_readFileDescriptor(Parcel* this)
{
    const struct flat_binder_object* flat = Parcel_readObject(this, true);

    if (flat && flat->hdr.type == BINDER_TYPE_FD) {
        return flat->handle;
    }
    return STATUS_BAD_TYPE;
}

int32_t Parcel_readBool(Parcel* this, bool* pArg)
{
    int32_t tmp = 0;
    int32_t ret = Parcel_readInt32(this, &tmp);
    *pArg = (tmp != 0);
    return ret;
}

int32_t Parcel_writeBool(Parcel* this, bool val)
{
    return Parcel_writeInt32(this, (int32_t)val);
}

void Parcel_ipcSetDataReference(Parcel* this, uint8_t* data, size_t dataSize,
    binder_size_t* objects, size_t objectsCount,
    release_func relFunc)
{
    /* this code uses 'mOwner == NULL' to understand whether it owns memory */

    LOG_FATAL_IF(relFunc == NULL, "must provide cleanup function");

    Parcel_freeData(this);

    this->mData = data;
    this->mDataSize = this->mDataCapacity = dataSize;
    this->mObjects = objects;
    this->mObjectsSize = this->mObjectsCapacity = objectsCount;
    this->mOwner = relFunc;

    binder_size_t minOffset = 0;
    for (size_t i = 0; i < this->mObjectsSize; i++) {
        binder_size_t offset = this->mObjects[i];
        if (offset < minOffset) {
            BINDER_LOGE("%s: bad object offset %" PRIu64 " < %" PRIu64 "\n",
                __func__, (uint64_t)offset, (uint64_t)minOffset);
            this->mObjectsSize = 0;
            break;
        }
        struct flat_binder_object* flat
            = (struct flat_binder_object*)(this->mData + offset);
        uint32_t type = flat->hdr.type;
        if (!(type == BINDER_TYPE_BINDER || type == BINDER_TYPE_HANDLE || type == BINDER_TYPE_FD)) {
            /* We should never receive other types (eg BINDER_TYPE_FDA)
             * as long as we don't support them in libbinder.
             * If we do receive them, it probably means a kernel bug;
             * try to recover gracefully by clearing out the objects.
             */

            BINDER_LOGE("%s: unsupported type object (%" PRIu32 ") at offset %" PRIu64 "\n",
                __func__, type, (uint64_t)offset);

            /* WARNING: callers of ipcSetDataReference need to make sure they
             * don't rely on mObjectsSize in their release_func.
             */

            this->mObjectsSize = 0;
            break;
        }
        minOffset = offset + sizeof(struct flat_binder_object);
    }
    Parcel_scanForFds(this);
}

static bool Parcel_enforceInterface(Parcel* this, String* interface,
    size_t len, IPCThreadState* threadState)
{
    /* StrictModePolicy. */

    int32_t strictPolicy;
    Parcel_readInt32(this, &strictPolicy);

    if (threadState == NULL) {
        threadState = IPCThreadState_self();
    }
    if ((threadState->getLastTransactionBinderFlags(threadState) & FLAG_ONEWAY) != 0) {
        /* For one-way calls, the callee is running entirely
         * disconnected from the caller, so disable StrictMode entirely.
         * Not only does disk/network usage not impact the caller, but
         * there's no way to communicate back violations anyway.
         */

        threadState->setStrictModePolicy(threadState, 0);
    } else {
        threadState->setStrictModePolicy(threadState, strictPolicy);
    }

    /* WorkSource. */

    Parcel_updateWorkSourceRequestHeaderPosition(this);
    int32_t workSource;
    Parcel_readInt32(this, &workSource);
    threadState->setCallingWorkSourceUidWithoutPropagation(threadState, workSource);

    /* vendor header */

    int32_t header;
    Parcel_readInt32(this, &header);
    if (header != kHeader) {
        BINDER_LOGE("Expecting header 0x%" PRIx32 " but found 0x%" PRIx32 ". Mixing copies of libbinder?", kHeader,
            header);
        return false;
    }

    /* Interface descriptor. */

    size_t parcel_interface_len;
    const char* parcel_interface = readString8Inplace(this, &parcel_interface_len);
    if (len == parcel_interface_len && (!len || !memcmp(parcel_interface, String_data(interface), len))) {
        return true;
    } else {
        BINDER_LOGW("**** enforceInterface() expected '%s' but read '%s'",
            String_data(interface), parcel_interface);
        return false;
    }
}

uintptr_t Parcel_ipcObjects(const Parcel* this)
{
    return (uintptr_t)(this->mObjects);
}

size_t Parcel_ipcObjectsCount(const Parcel* this)
{
    return this->mObjectsSize;
}

void Parcel_closeFileDescriptors(Parcel* this)
{
    size_t i = this->mObjectsSize;

    BINDER_LOGV("Closing file descriptors for %zu objects...", i);

    while (i > 0) {
        i--;
        struct flat_binder_object* flat
            = (struct flat_binder_object*)(this->mData + this->mObjects[i]);
        if (flat->hdr.type == BINDER_TYPE_FD) {
            BINDER_LOGV("Closing fd: %" PRIu32 "", flat->handle);
            close(flat->handle);
        }
    }
}

int32_t Parcel_readUtf8FromUtf16(Parcel* this, String* str)
{
    PANIC();
    return STATUS_BAD_VALUE;
}

bool Parcel_checkInterface(Parcel* this, IBinder* binder)
{
    String* descriptor = binder->getInterfaceDescriptor(binder);
    return Parcel_enforceInterface(this, descriptor, String_size(descriptor), NULL);
}

int32_t Parcel_writeStrongBinder(Parcel* this, IBinder* val)
{
    return Parcel_flattenBinder(this, val);
}

int32_t Parcel_readNullableStrongBinder(Parcel* this, IBinder** val)
{
    return Parcel_unflattenBinder(this, val);
}

int32_t Parcel_readStrongBinder(Parcel* this, IBinder** val)
{
    int32_t status = Parcel_readNullableStrongBinder(this, val);
    if (status == STATUS_OK && !(*val)) {
        BINDER_LOGW("Expecting binder but got null!");
        status = STATUS_UNEXPECTED_NULL;
    }
    return status;
}

int32_t Parcel_writeStringVector(Parcel* this, VectorString* strVtor)
{
    PANIC();
    return 0;
}

int32_t Parcel_readUtf8VectorFromUtf16Vector(Parcel* this, VectorString* strVtor)
{
    PANIC();
    return 0;
}

int32_t Parcel_writeUtf8VectorAsUtf16Vector(Parcel* this, VectorString* strVtor)
{
    return Parcel_writeStringVector(this, strVtor);
}

int32_t Parcel_writeInterfaceToken(Parcel* this, String* interface)
{
    IPCThreadState* threadState = IPCThreadState_self();

    Parcel_writeInt32(this, threadState->getStrictModePolicy(threadState) | STRICT_MODE_PENALTY_GATHER);
    Parcel_updateWorkSourceRequestHeaderPosition(this);
    Parcel_writeInt32(this, threadState->shouldPropagateWorkSource(threadState) ? threadState->getCallingWorkSourceUid(threadState) : -1);
    Parcel_writeInt32(this, kHeader);

    /* currently the interface identification token is just its name as a string */

    return Parcel_writeString16(this, interface);
}

void Parcel_setDataPosition(Parcel* this, size_t pos)
{
    if (pos > INT32_MAX) {
        /* don't accept size_t values which may have come from an
         * inadvertent conversion from a negative int.
         */
        LOG_FATAL("pos too big: %zu", pos);
    }

    this->mDataPos = pos;
    this->mNextObjectHint = 0;
    this->mObjectsSorted = false;
}

void Parcel_markSensitive(Parcel* this)
{
    this->mDeallocZero = true;
}

int32_t Parcel_writeFileDescriptor(Parcel* this, int fd, bool takeOwnership)
{
    struct flat_binder_object obj;
    obj.hdr.type = BINDER_TYPE_FD;
    obj.flags = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
    obj.binder = 0; /* Don't pass uninitialized stack data to a remote process */
    obj.handle = fd;
    obj.cookie = takeOwnership ? 1 : 0;
    return Parcel_writeObject(this, &obj, true);
}
