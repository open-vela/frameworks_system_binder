/*
 * Copyright (C) 2015 The Android Open Source Project
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

#define LOG_TAG "Status"

/****************************************************************************
 * Included Files
 ****************************************************************************/

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

#include <android/binder_status.h>

#include "IBinder.h"
#include "Parcel.h"
#include "Status.h"
#include "utils/Binderlog.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void Status_init(Status* this)
{
    this->mException = EX_NONE;
    this->mErrorCode = 0;
    String_init(&this->mMessage, NULL);
}

uint32_t Status_writeToParcel(Status* this, Parcel* parcel)
{
    if (this->mException == EX_TRANSACTION_FAILED) {
        return this->mErrorCode;
    }

    uint32_t status = Parcel_writeInt32(parcel, this->mException);
    if (status != STATUS_OK)
        return status;
    if (this->mException == EX_NONE) {
        return status;
    }

    status = Parcel_writeString16(parcel, &this->mMessage);
    if (status != STATUS_OK)
        return status;
    status = Parcel_writeInt32(parcel, 0);
    if (status != STATUS_OK)
        return status;
    if (this->mException == EX_SERVICE_SPECIFIC) {
        status = Parcel_writeInt32(parcel, this->mErrorCode);
    } else if (this->mException == EX_PARCELABLE) {
        status = Parcel_writeInt32(parcel, 0);
    }
    return status;
}

uint32_t Status_writeOverParcel(Status* this, Parcel* parcel)
{
    Parcel_setDataSize(parcel, 0);
    Parcel_setDataPosition(parcel, 0);
    return Status_writeToParcel(this, parcel);
}

void Status_fromExceptionCode(Status* this, int32_t exceptionCode, const char* string)
{
    this->mException = exceptionCode;
    String_init(&this->mMessage, string);

    if (exceptionCode == EX_TRANSACTION_FAILED) {
        this->mErrorCode = STATUS_FAILED_TRANSACTION;
    } else {
        this->mErrorCode = STATUS_OK;
    }
}

void Status_setFromStatusT(Status* this, int32_t status)
{
    this->mException = (status == STATUS_OK) ? EX_NONE : EX_TRANSACTION_FAILED;
    this->mErrorCode = status;
    String_clear(&this->mMessage);
}

uint32_t Status_readFromParcel(Status* this, Parcel* parcel_in)
{
    /*Parcel  parcel;

      Parcel_initState(&parcel);
      Parcel_dup(&parcel, parcel_in);*/

    uint32_t status = Parcel_readInt32(parcel_in, &this->mException);
    if (status != OK) {
        Status_setFromStatusT(this, status);
        return status;
    }

    /* Skip over fat response headers.  Not used (or propagated) in native code. */
    if (this->mException == EX_HAS_REPLY_HEADER) {
        /* Note that the header size includes the 4 byte size field. */
        const size_t header_start = Parcel_dataPosition(parcel_in);
        /* Get available size before reading more */
        const size_t header_avail = Parcel_dataAvail(parcel_in);
        int32_t header_size;
        status = Parcel_readInt32(parcel_in, &header_size);
        if (status != STATUS_OK) {
            Status_setFromStatusT(this, status);
            return status;
        }
        if (header_size < 0 || (size_t)(header_size) > header_avail) {
            Status_setFromStatusT(this, STATUS_UNKNOWN_ERROR);
            return STATUS_UNKNOWN_ERROR;
        }
        Parcel_setDataPosition(parcel_in, header_start + header_size);
        /* And fat response headers are currently only used when there are no
         * exceptions, so act like there was no error.
         */
        this->mException = EX_NONE;
    }
    if (this->mException == EX_NONE) {
        return status;
    }

    /* The remote threw an exception.  Get the message back.*/
    String message;
    status = Parcel_readString16_to(parcel_in, &message);
    if (status != OK) {
        Status_setFromStatusT(this, status);
        return status;
    }
    String_dup(&this->mMessage, &message);

    /* Skip over the remote stack trace data */
    const size_t remote_start = Parcel_dataPosition(parcel_in);
    /* Get available size before reading more */
    const size_t remote_avail = Parcel_dataAvail(parcel_in);
    int32_t remote_stack_trace_header_size;
    status = Parcel_readInt32(parcel_in, &remote_stack_trace_header_size);
    if (status != OK) {
        Status_setFromStatusT(this, status);
        return status;
    }
    if (remote_stack_trace_header_size < 0 || (size_t)(remote_stack_trace_header_size) > remote_avail) {
        Status_setFromStatusT(this, STATUS_UNKNOWN_ERROR);
        return STATUS_UNKNOWN_ERROR;
    }

    if (remote_stack_trace_header_size != 0) {
        Parcel_setDataPosition(parcel_in, remote_start + remote_stack_trace_header_size);
    }

    if (this->mException == EX_SERVICE_SPECIFIC) {
        status = Parcel_readInt32(parcel_in, &this->mErrorCode);
    } else if (this->mException == EX_PARCELABLE) {
        /* Skip over the blob of Parcelable data */
        const size_t header_start = Parcel_dataPosition(parcel_in);
        /* Get available size before reading more */
        const size_t header_avail = Parcel_dataAvail(parcel_in);

        int32_t header_size;
        status = Parcel_readInt32(parcel_in, &header_size);
        if (status != OK) {
            Status_setFromStatusT(this, status);
            return status;
        }

        if (header_size < 0 || (size_t)(header_size) > header_avail) {
            Status_setFromStatusT(this, STATUS_UNKNOWN_ERROR);
            return STATUS_UNKNOWN_ERROR;
        }

        Parcel_setDataPosition(parcel_in, header_start + header_size);
    }
    if (status != OK) {
        Status_setFromStatusT(this, status);
        return status;
    }

    return status;
}

#if 0
Status Status::ok() {
    return Status();
}

Status Status::fromExceptionCode(int32_t exceptionCode) {
    if (exceptionCode == EX_TRANSACTION_FAILED) {
        return Status(exceptionCode, FAILED_TRANSACTION);
    }
    return Status(exceptionCode, OK);
}

Status Status::fromExceptionCode(int32_t exceptionCode,
                                 const String8& message) {
    if (exceptionCode == EX_TRANSACTION_FAILED) {
        return Status(exceptionCode, FAILED_TRANSACTION, message);
    }
    return Status(exceptionCode, OK, message);
}

Status Status::fromExceptionCode(int32_t exceptionCode,
                                 const char* message) {
    return fromExceptionCode(exceptionCode, String8(message));
}

Status Status::fromServiceSpecificError(int32_t serviceSpecificErrorCode) {
    return Status(EX_SERVICE_SPECIFIC, serviceSpecificErrorCode);
}

Status Status::fromServiceSpecificError(int32_t serviceSpecificErrorCode,
                                        const String8& message) {
    return Status(EX_SERVICE_SPECIFIC, serviceSpecificErrorCode, message);
}

Status Status::fromServiceSpecificError(int32_t serviceSpecificErrorCode,
                                        const char* message) {
    return fromServiceSpecificError(serviceSpecificErrorCode, String8(message));
}

Status Status::fromStatusT(status_t status) {
    Status ret;
    ret.setFromStatusT(status);
    return ret;
}

std::string Status::exceptionToString(int32_t exceptionCode) {
    switch (exceptionCode) {
#define EXCEPTION_TO_CASE(EXCEPTION) \
    case EXCEPTION:                  \
        return #EXCEPTION;
        EXCEPTION_TO_CASE(EX_NONE)
        EXCEPTION_TO_CASE(EX_SECURITY)
        EXCEPTION_TO_CASE(EX_BAD_PARCELABLE)
        EXCEPTION_TO_CASE(EX_ILLEGAL_ARGUMENT)
        EXCEPTION_TO_CASE(EX_NULL_POINTER)
        EXCEPTION_TO_CASE(EX_ILLEGAL_STATE)
        EXCEPTION_TO_CASE(EX_NETWORK_MAIN_THREAD)
        EXCEPTION_TO_CASE(EX_UNSUPPORTED_OPERATION)
        EXCEPTION_TO_CASE(EX_SERVICE_SPECIFIC)
        EXCEPTION_TO_CASE(EX_PARCELABLE)
        EXCEPTION_TO_CASE(EX_HAS_REPLY_HEADER)
        EXCEPTION_TO_CASE(EX_TRANSACTION_FAILED)
#undef EXCEPTION_TO_CASE
        default: return std::to_string(exceptionCode);
    }
}


Status::Status(int32_t exceptionCode, int32_t errorCode, const String8& message)
    : mException(exceptionCode),
      mErrorCode(errorCode),
      mMessage(message) {}



void Status::setException(int32_t ex, const String8& message) {
    mException = ex;
    mErrorCode = ex == EX_TRANSACTION_FAILED ? FAILED_TRANSACTION : NO_ERROR;
    mMessage.setTo(message);
}

void Status::setServiceSpecificError(int32_t errorCode, const String8& message) {
    setException(EX_SERVICE_SPECIFIC, message);
    mErrorCode = errorCode;
}


String8 Status::toString8() const {
    String8 ret;
    if (mException == EX_NONE) {
        ret.append("No error");
    } else {
        ret.appendFormat("Status(%d, %s): '", mException, exceptionToString(mException).c_str());
        if (mException == EX_SERVICE_SPECIFIC) {
            ret.appendFormat("%d: ", mErrorCode);
        } else if (mException == EX_TRANSACTION_FAILED) {
            ret.appendFormat("%s: ", statusToString(mErrorCode).c_str());
        }
        ret.append(String8(mMessage));
        ret.append("'");
    }
    return ret;
}

#endif
