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

#ifndef __BINDER_INCLUDE_BINDER_PARCEL_H__
#define __BINDER_INCLUDE_BINDER_PARCEL_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IBinder.h"
#include "utils/BinderString.h"
#include "utils/Vector.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* TODO: Make Parcel data become growable */

#define PARCEL_INIT_CAPACITY 64

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct Parcel;
typedef struct Parcel Parcel;

typedef void (*release_func)(Parcel* parcel, const uint8_t* data, size_t dataSize,
    const binder_size_t* objects, size_t objectsSize);

struct Parcel {
    uint8_t* mData;
    size_t mDataSize;
    size_t mDataCapacity;
    size_t mDataPos;
    int32_t mError;
    binder_size_t* mObjects;
    size_t mObjectsSize;
    size_t mObjectsCapacity;
    size_t mNextObjectHint;
    bool mObjectsSorted;
    bool mRequestHeaderPresent;
    size_t mWorkSourceRequestHeaderPosition;
    bool mFdsKnown;
    bool mHasFds;
    bool mAllowFds;
    bool mDeallocZero;
    release_func mOwner;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Misc function */

void Parcel_initState(Parcel* this);
int32_t Parcel_dup(Parcel* new, const Parcel* old);
void Parcel_freeData(Parcel* this);

/* Read functions */

int32_t Parcel_readInt32(Parcel* this, int32_t* pArg);
const char* Parcel_readString16(Parcel* this);
int32_t Parcel_readString16_to(Parcel* this, String* pArg);
int Parcel_readFileDescriptor(Parcel* this);
uintptr_t Parcel_readPointer(Parcel* this);
int32_t Parcel_read(Parcel* this, void* outData, size_t len);
int32_t Parcel_readNullableStrongBinder(Parcel* this, IBinder** val);
int32_t Parcel_readStrongBinder(Parcel* this, IBinder** val);
int32_t Parcel_readBool(Parcel* this, bool* pArg);
int32_t Parcel_readStringVector(Parcel* this, VectorString* str);
int32_t Parcel_readUtf8FromUtf16(Parcel* this, String* str);
int32_t Parcel_readUtf8VectorFromUtf16Vector(Parcel* this, VectorString* strVtor);

struct flat_binder_object* Parcel_readObject(Parcel* this, bool nullMetaData);
void Parcel_acquireObjects(Parcel* this);

/* Write functions */

int32_t Parcel_writeInt32(Parcel* this, int32_t val);
int32_t Parcel_writeString16(Parcel* this, String* str);
int32_t Parcel_writeStringVector(Parcel* this, VectorString* str);
int32_t Parcel_writeFileDescriptor(Parcel* this, int fd, bool takeOwnership /* = false */);
int32_t Parcel_writePointer(Parcel* this, uintptr_t val);
int32_t Parcel_writeStrongBinder(Parcel* this, IBinder* val);
int32_t Parcel_write(Parcel* this, const void* data, size_t len);
int32_t Parcel_writeBool(Parcel* this, bool val);
int32_t Parcel_writeUtf8VectorAsUtf16Vector(Parcel* this, VectorString* strVtor);

/* IPC functions */

uintptr_t Parcel_ipcObjects(const Parcel* this);
size_t Parcel_ipcObjectsCount(const Parcel* this);
uintptr_t Parcel_ipcData(const Parcel* this);
size_t Parcel_ipcDataSize(const Parcel* this);
void Parcel_ipcSetDataReference(Parcel* this, uint8_t* data, size_t dataSize,
    binder_size_t* objects, size_t objectsCount,
    release_func relFunc);

/* Writes the IPC header. */
int32_t Parcel_writeInterfaceToken(Parcel* this, String* interface);

/* IPC functions */
void Parcel_markForBinder(Parcel* this, const IBinder* binder);
void Parcel_setDataPosition(Parcel* this, size_t pos);
void Parcel_markSensitive(Parcel* this);
size_t Parcel_dataPosition(const Parcel* this);
size_t Parcel_dataCapacity(const Parcel* this);
void Parcel_setError(Parcel* this, int32_t err);
int32_t Parcel_setDataSize(Parcel* this, size_t size);
int32_t Parcel_errorCheck(const Parcel* this);
const uint8_t* Parcel_data(const Parcel* this);
size_t Parcel_dataSize(const Parcel* this);
size_t Parcel_dataAvail(const Parcel* this);
int32_t Parcel_setDataCapacity(Parcel* this, size_t size);
void Parcel_closeFileDescriptors(Parcel* this);
bool Parcel_checkInterface(Parcel* this, IBinder* binder);

#endif //__BINDER_INCLUDE_BINDER_PARCEL_H__
