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

#ifndef __BINDER_INCLUDE_BINDER_IPCTHREADSTATE_H__
#define __BINDER_INCLUDE_BINDER_IPCTHREADSTATE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/clock.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>

#include "Binder.h"
#include "IBinder.h"
#include "Parcel.h"
#include "ProcessState.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct IPCThreadState;
typedef struct IPCThreadState IPCThreadState;

struct IPCThreadState {
    void (*dtor)(IPCThreadState* this);

    /* public function */

    pid_t (*getCallingPid)(IPCThreadState* this);
    const char* (*getCallingSid)(IPCThreadState* this);
    uid_t (*getCallingUid)(IPCThreadState* this);
    void (*setStrictModePolicy)(IPCThreadState* this, int32_t policy);
    int32_t (*getStrictModePolicy)(IPCThreadState* this);
    int64_t (*setCallingWorkSourceUid)(IPCThreadState* this, uid_t uid);
    int64_t (*setCallingWorkSourceUidWithoutPropagation)(IPCThreadState* this, uid_t uid);
    uid_t (*getCallingWorkSourceUid)(IPCThreadState* this);
    int64_t (*clearCallingWorkSource)(IPCThreadState* this);
    void (*clearPropagateWorkSource)(IPCThreadState* this);
    bool (*shouldPropagateWorkSource)(IPCThreadState* this);
    void (*setLastTransactionBinderFlags)(IPCThreadState* this, int32_t flags);
    int32_t (*getLastTransactionBinderFlags)(IPCThreadState* this);
    void (*setCallRestriction)(IPCThreadState* this, enum CallRestriction restriction);
    int32_t (*setupPolling)(IPCThreadState* this, int* fd);
    int32_t (*handlePolledCommands)(IPCThreadState* this);
    void (*flushCommands)(IPCThreadState* this);
    bool (*flushIfNeeded)(IPCThreadState* this);
    void (*joinThreadPool)(IPCThreadState* this, bool isMain);
    int32_t (*transact)(IPCThreadState* this, int32_t handle, uint32_t code,
        const Parcel* data, Parcel* reply,
        uint32_t flags);
    void (*incStrongHandle)(IPCThreadState* this, int32_t handle, BpBinder* proxy);
    void (*decStrongHandle)(IPCThreadState* this, int32_t handle);
    void (*incWeakHandle)(IPCThreadState* this, int32_t handle, BpBinder* proxy);
    void (*decWeakHandle)(IPCThreadState* this, int32_t handle);
    int32_t (*attemptIncStrongHandle)(IPCThreadState* this, int32_t handle);
    void (*expungeHandle)(IPCThreadState* this, int32_t handle, IBinder* binder);
    int32_t (*requestDeathNotification)(IPCThreadState* this, int32_t handle,
        BpBinder* proxy);
    int32_t (*clearDeathNotification)(IPCThreadState* this, int32_t handle,
        BpBinder* proxy);
    bool (*backgroundSchedulingDisabled)(IPCThreadState* this);
    void (*setTheContextObject)(IPCThreadState* this, BBinder* obj);

    enum CallRestriction (*getCallRestriction)(IPCThreadState* this);

    /* private function */
    int32_t (*sendReply)(IPCThreadState* this, const Parcel* reply, uint32_t flags);
    int32_t (*waitForResponse)(IPCThreadState* this, Parcel* reply,
        int32_t* acquireResult /* =NULL */);
    int32_t (*talkWithDriver)(IPCThreadState* this, bool doReceive /* =true */);
    int32_t (*writeTransactionData)(IPCThreadState* this, int32_t cmd, uint32_t binderFlags,
        int32_t handle, uint32_t code,
        const Parcel* data,
        int32_t* statusBuffer);
    int32_t (*getAndExecuteCommand)(IPCThreadState* this);
    int32_t (*executeCommand)(IPCThreadState* this, int32_t command);
    void (*processPendingDerefs)(IPCThreadState* this);
    void (*processPostWriteDerefs)(IPCThreadState* this);
    void (*clearCaller)(IPCThreadState* this);

    /* private data */
    ProcessState* mProcess;

    VectorImpl mPendingStrongDerefs;
    VectorImpl mPendingWeakDerefs;
    VectorImpl mPostWriteStrongDerefs;
    VectorImpl mPostWriteWeakDerefs;

    Parcel mIn;
    Parcel mOut;
    int32_t mLastError;
    const void* mServingStackPointer;
    pid_t mCallingPid;
    const char* mCallingSid;
    uid_t mCallingUid;
    int32_t mWorkSource;
    bool mPropagateWorkSource;
    bool mIsLooper;
    bool mIsFlushing;
    int32_t mStrictModePolicy;
    int32_t mLastTransactionBinderFlags;
    enum CallRestriction mCallRestriction;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: IPCThreadState_delete
 *
 * Description:
 *
 ****************************************************************************/

void IPCThreadState_delete(IPCThreadState* this);

/****************************************************************************
 * Name: IPCThreadState_self
 *
 * Description:
 *
 ****************************************************************************/

IPCThreadState* IPCThreadState_self(void);

/****************************************************************************
 * Name: IPCThreadState_selfOrNull
 *
 * Description:
 *
 ****************************************************************************/

IPCThreadState* IPCThreadState_selfOrNull(void);

/****************************************************************************
 * Name: IPCThreadState_threadDestructor
 *
 * Description:
 *
 ****************************************************************************/

void IPCThreadState_threadDestructor(void* st);

#endif /* __BINDER_INCLUDE_BINDER_IPCTHREADSTATE_H__ */
