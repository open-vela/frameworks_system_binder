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

#ifndef __BINDER_INCLUDE_BINDER_STATUS_H__
#define __BINDER_INCLUDE_BINDER_STATUS_H__

#include "IBinder.h"
#include "Parcel.h"
#include "utils/BinderString.h"

/* This is special and Java specific; see Parcel.java. */

#define EX_HAS_REPLY_HEADER -128

struct Status {
    int32_t mException;
    int32_t mErrorCode;
    String mMessage;
};

typedef struct Status Status;

void Status_init(Status* this);
uint32_t Status_readFromParcel(Status* this, Parcel* parcel);
uint32_t Status_writeToParcel(Status* this, Parcel* parcel);
void Status_fromExceptionCode(Status* this, int32_t exceptionCode, const char* string);
void Status_setFromStatusT(Status* this, int32_t status);

#endif /* __BINDER_INCLUDE_BINDER_STATUS_H__ */
