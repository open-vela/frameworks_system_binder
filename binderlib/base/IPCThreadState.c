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

#define LOG_TAG "IPCThreadState"

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <inttypes.h>
#include <nuttx/android/binder.h>
#include <nuttx/clock.h>
#include <nuttx/tls.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "IPCThreadState.h"
#include "utils/Binderlog.h"
#include "utils/Timers.h"
#include <android/binder_status.h>

static const int64_t kWorkSourcePropagatedBitIndex = 32;
static const int32_t kUnsetWorkSource = -1;

#ifdef CONFIG_BINDER_LIB_DEBUG
static const char* statusToString(int32_t s)
{
    return strerror(-s);
}

/* Static const and functions will be optimized out if not used,
 * when LOG_NDEBUG and references in IF_LOG_COMMANDS() are optimized out.
 */

static const char* kReturnStrings[] = {
    "BR_ACQUIRE",
    "BR_ACQUIRE_RESULT",
    "BR_ATTEMPT_ACQUIRE",
    "BR_CLEAR_DEATH_NOTIFICATION_DONE",
    "BR_DECREFS",
    "BR_DEAD_BINDER",
    "BR_DEAD_REPLY",
    "BR_ERROR",
    "BR_FAILED_REPLY",
    "BR_FINISHED",
    "BR_FROZEN_REPLY",
    "BR_INCREFS",
    "BR_NOOP",
    "BR_OK",
    "BR_ONEWAY_SPAM_SUSPECT",
    "BR_RELEASE",
    "BR_REPLY",
    "BR_SPAWN_LOOPER",
    "BR_TRANSACTION",
    "BR_TRANSACTION_COMPLETE",
    "BR_TRANSACTION_SEC_CTX",
};

static const char* kCommandStrings[] = {
    "BC_TRANSACTION",
    "BC_REPLY",
    "BC_ACQUIRE_RESULT",
    "BC_FREE_BUFFER",
    "BC_INCREFS",
    "BC_ACQUIRE",
    "BC_RELEASE",
    "BC_DECREFS",
    "BC_INCREFS_DONE",
    "BC_ACQUIRE_DONE",
    "BC_ATTEMPT_ACQUIRE",
    "BC_REGISTER_LOOPER",
    "BC_ENTER_LOOPER",
    "BC_EXIT_LOOPER",
    "BC_REQUEST_DEATH_NOTIFICATION",
    "BC_CLEAR_DEATH_NOTIFICATION",
    "BC_DEAD_BINDER_DONE"
};

static const char* getReturnString(uint32_t cmd)
{
    size_t idx = _IOC_NR(cmd);

    if (idx < sizeof(kReturnStrings) / sizeof(kReturnStrings[0])) {
        return kReturnStrings[idx];
    } else {
        return "unknown";
    }
}

static const char* getCommandString(const void* _cmd)
{
    static const size_t N = sizeof(kCommandStrings) / sizeof(kCommandStrings[0]);
    const int32_t* cmd = (const int32_t*)_cmd;
    uint32_t code = (uint32_t)*cmd++;
    size_t cmdIndex = code & 0xff;
    if (cmdIndex >= N) {
        return "Unknown command";
    }
    return kCommandStrings[cmdIndex];
}
#endif

static void IPCThreadState_freeBuffer(Parcel* parcel, const uint8_t* data,
    size_t dataSize, const binder_size_t* objects,
    size_t objectsSize);

static bool IPCThreadState_backgroundSchedulingDisabled(IPCThreadState* this)
{
    ProcessState* proc = ProcessState_self();
    return atomic_load_explicit(&proc->mDisableBackgroundScheduling, memory_order_relaxed);
}

static pid_t IPCThreadState_getCallingPid(IPCThreadState* this)
{
    return this->mCallingPid;
}

static const char* IPCThreadState_getCallingSid(IPCThreadState* this)
{
    return this->mCallingSid;
}

static uid_t IPCThreadState_getCallingUid(IPCThreadState* this)
{
    return this->mCallingUid;
}

static void IPCThreadState_setStrictModePolicy(IPCThreadState* this, int32_t policy)
{
    this->mStrictModePolicy = policy;
}

static int32_t IPCThreadState_getStrictModePolicy(IPCThreadState* this)
{
    return this->mStrictModePolicy;
}

static int64_t IPCThreadState_setCallingWorkSourceUid(IPCThreadState* this, uid_t uid)
{
    int64_t token = this->setCallingWorkSourceUidWithoutPropagation(this, uid);
    this->mPropagateWorkSource = true;
    return token;
}

static int64_t IPCThreadState_setCallingWorkSourceUidWithoutPropagation(IPCThreadState* this, uid_t uid)
{
    const int64_t propagatedBit = ((int64_t)this->mPropagateWorkSource) << kWorkSourcePropagatedBitIndex;
    int64_t token = propagatedBit | this->mWorkSource;
    this->mWorkSource = uid;
    return token;
}

static void IPCThreadState_clearPropagateWorkSource(IPCThreadState* this)
{
    this->mPropagateWorkSource = false;
}

static bool IPCThreadState_shouldPropagateWorkSource(IPCThreadState* this)
{
    return this->mPropagateWorkSource;
}

static uid_t IPCThreadState_getCallingWorkSourceUid(IPCThreadState* this)
{
    return this->mWorkSource;
}

static int64_t IPCThreadState_clearCallingWorkSource(IPCThreadState* this)
{
    return this->setCallingWorkSourceUid(this, kUnsetWorkSource);
}

static void IPCThreadState_setLastTransactionBinderFlags(IPCThreadState* this, int32_t flags)
{
    this->mLastTransactionBinderFlags = flags;
}

static int32_t IPCThreadState_getLastTransactionBinderFlags(IPCThreadState* this)
{
    return this->mLastTransactionBinderFlags;
}

static void IPCThreadState_setCallRestriction(IPCThreadState* this, enum CallRestriction restriction)
{
    this->mCallRestriction = restriction;
}

static enum CallRestriction IPCThreadState_getCallRestriction(IPCThreadState* this)
{
    return this->mCallRestriction;
}

static void IPCThreadState_clearCaller(IPCThreadState* this)
{
    this->mCallingPid = getpid();
    this->mCallingSid = NULL; /* expensive to lookup */
    this->mCallingUid = getuid();
}

static void IPCThreadState_flushCommands(IPCThreadState* this)
{
    if (this->mProcess->mDriverFD < 0) {
        return;
    }
    this->talkWithDriver(this, false);

    /* The flush could have caused post-write refcount decrements to have
     * been executed, which in turn could result in BC_RELEASE/BC_DECREFS
     * being queued in mOut. So flush again, if we need to.
     */

    if (Parcel_dataSize(&this->mOut) > 0) {
        this->talkWithDriver(this, false);
    }
    if (Parcel_dataSize(&this->mOut) > 0) {
        BINDER_LOGE("mOut.dataSize() > 0 after flushCommands()");
    }
}

static bool IPCThreadState_flushIfNeeded(IPCThreadState* this)
{
    if (this->mIsLooper || this->mServingStackPointer != NULL || this->mIsFlushing) {
        return false;
    }
    this->mIsFlushing = true;

    /* In case this thread is not a looper and is not currently
     * serving a binder transaction, there's no guarantee that
     * this thread will call back into the kernel driver any time soon.
     * Therefore, flush pending commands such as BC_FREE_BUFFER,
     * to prevent them from getting stuck in this thread's out buffer.
     */

    this->flushCommands(this);
    this->mIsFlushing = false;
    return true;
}

static int32_t IPCThreadState_getAndExecuteCommand(IPCThreadState* this)
{
    int32_t result;
    int32_t cmd;

    result = this->talkWithDriver(this, true);
    if (result >= STATUS_OK) {
        size_t IN = Parcel_dataAvail(&this->mIn);
        if (IN < sizeof(int32_t)) {
            return result;
        }
        Parcel_readInt32(&this->mIn, &cmd);
        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("Processing top-level Command: %s", getReturnString(cmd));
        }
        pthread_mutex_lock(&this->mProcess->mThreadCountLock);
        this->mProcess->mExecutingThreadsCount++;
        if (this->mProcess->mExecutingThreadsCount >= this->mProcess->mMaxThreads && this->mProcess->mStarvationStartTimeMs == 0) {
            this->mProcess->mStarvationStartTimeMs = uptimeMillis();
        }
        pthread_mutex_unlock(&this->mProcess->mThreadCountLock);

        result = this->executeCommand(this, cmd);

        pthread_mutex_lock(&this->mProcess->mThreadCountLock);
        this->mProcess->mExecutingThreadsCount--;
        if (this->mProcess->mExecutingThreadsCount < this->mProcess->mMaxThreads && this->mProcess->mStarvationStartTimeMs != 0) {
            int64_t starvationTimeMs = uptimeMillis() - this->mProcess->mStarvationStartTimeMs;
            if (starvationTimeMs > 100) {
                BINDER_LOGE(
                    "binder thread pool (%zu threads) starved for %" PRId64 " ms",
                    this->mProcess->mMaxThreads, starvationTimeMs);
            }
            this->mProcess->mStarvationStartTimeMs = 0;
        }

        /* Cond broadcast can be expensive, so don't send it every time a binder
         * call is processed.
         */

        if (this->mProcess->mWaitingForThreads > 0) {
            pthread_cond_broadcast(&this->mProcess->mThreadCountDecrement);
        }
        pthread_mutex_unlock(&this->mProcess->mThreadCountLock);
    }
    return result;
}

static void IPCThreadState_processPendingDerefs(IPCThreadState* this)
{
    if (Parcel_dataPosition(&this->mIn) >= Parcel_dataPosition(&this->mIn)) {
        /* The decWeak()/decStrong() calls may cause a destructor to run,
         * which in turn could have initiated an outgoing transaction,
         * which in turn could cause us to add to the pending refs
         * vectors; so instead of simply iterating, loop until they're empty.
         *
         * We do this in an outer loop, because calling decStrong()
         * may result in something being added to mPendingWeakDerefs,
         * which could be delayed until the next incoming command
         * from the driver if we don't process it now.
         */

        while (this->mPendingWeakDerefs.size(&this->mPendingWeakDerefs) > 0 || this->mPendingStrongDerefs.size(&this->mPendingStrongDerefs) > 0) {
            while (this->mPendingWeakDerefs.size(&this->mPendingWeakDerefs) > 0) {
                RefBase_weakref* refs = this->mPendingWeakDerefs.get(&this->mPendingWeakDerefs, 0);
                this->mPendingWeakDerefs.removeAt(&this->mPendingWeakDerefs, 0);
                refs->decWeak(refs, (const void*)this->mProcess);
            }

            if (this->mPendingStrongDerefs.size(&this->mPendingStrongDerefs) > 0) {
                /* We don't use while() here because we don't want to re-order
                 * strong and weak decs at all; if this decStrong() causes both a
                 * decWeak() and a decStrong() to be queued, we want to process
                 * the decWeak() first.
                 */

                BBinder* obj = this->mPendingStrongDerefs.get(&this->mPendingStrongDerefs, 0);
                this->mPendingStrongDerefs.removeAt(&this->mPendingStrongDerefs, 0);
                obj->decStrong(obj, (const void*)this->mProcess);
            }
        }
    }
}

static void IPCThreadState_processPostWriteDerefs(IPCThreadState* this)
{
    for (size_t i = 0; i < this->mPostWriteWeakDerefs.size(&this->mPostWriteWeakDerefs); i++) {
        RefBase_weakref* refs = this->mPostWriteWeakDerefs.get(&this->mPostWriteWeakDerefs, i);
        refs->decWeak(refs, (const void*)this->mProcess);
    }

    this->mPostWriteWeakDerefs.clear(&this->mPostWriteWeakDerefs);
    for (size_t i = 0; i < this->mPostWriteStrongDerefs.size(&this->mPostWriteStrongDerefs); i++) {
        RefBase* obj = this->mPostWriteStrongDerefs.get(&this->mPostWriteStrongDerefs, i);
        obj->decStrong(obj, (const void*)this->mProcess);
    }
    this->mPostWriteStrongDerefs.clear(&this->mPostWriteStrongDerefs);
}

static void IPCThreadState_joinThreadPool(IPCThreadState* this, bool isMain)
{
    BINDER_LOGV("%s Thread %d is Joining the threadpool of Process %d\n",
        isMain ? "Main" : "Child", gettid(), getpid());

    pthread_mutex_lock(&this->mProcess->mThreadCountLock);
    this->mProcess->mCurrentThreads++;
    pthread_mutex_unlock(&this->mProcess->mThreadCountLock);
    Parcel_writeInt32(&this->mOut, isMain ? BC_ENTER_LOOPER : BC_REGISTER_LOOPER);

    this->mIsLooper = true;
    int32_t result;
    do {
        this->processPendingDerefs(this);
        /* now get the next command to be processed, waiting if necessary */
        result = this->getAndExecuteCommand(this);
        if (result < STATUS_OK && result != STATUS_TIMED_OUT && result != -ECONNREFUSED && result != -EBADF) {
            LOG_FATAL("getAndExecuteCommand(fd=%d) returned unexpected error %" PRIi32 ", aborting",
                this->mProcess->mDriverFD, result);
        }

        /* Let this thread exit the thread pool if it is no longer
         * needed and it is not the main process thread.
         */
        if (result == STATUS_TIMED_OUT && !isMain) {
            break;
        }
    } while (result != -ECONNREFUSED && result != -EBADF);

    BINDER_LOGV("%s Thread %d is leaving the threadpool of Process %d, err=%" PRIi32 "\n",
        isMain ? "Main" : "Child", gettid(), getpid(), result);

    Parcel_writeInt32(&this->mOut, BC_EXIT_LOOPER);
    this->mIsLooper = false;
    this->talkWithDriver(this, false);
    pthread_mutex_lock(&this->mProcess->mThreadCountLock);
    LOG_FATAL_IF(this->mProcess->mCurrentThreads == 0,
        "Threadpool thread count = 0. Thread cannot exist and exit in empty "
        "threadpool\n"
        "Misconfiguration. Increase threadpool max threads configuration\n");
    this->mProcess->mCurrentThreads--;
    pthread_mutex_unlock(&this->mProcess->mThreadCountLock);
}

static int32_t IPCThreadState_setupPolling(IPCThreadState* this, int* fd)
{
    if (this->mProcess->mDriverFD < 0) {
        return -EBADF;
    }

    Parcel_writeInt32(&this->mOut, BC_ENTER_LOOPER);
    this->flushCommands(this);
    *fd = this->mProcess->mDriverFD;
    return 0;
}

static int32_t IPCThreadState_handlePolledCommands(IPCThreadState* this)
{
    int32_t result;

    do {
        result = this->getAndExecuteCommand(this);
    } while (Parcel_dataPosition(&this->mIn) < Parcel_dataSize(&this->mIn));
    this->processPendingDerefs(this);
    this->flushCommands(this);

    return result;
}

static int32_t IPCThreadState_transact(IPCThreadState* this, int32_t handle, uint32_t code,
    const Parcel* data, Parcel* reply,
    uint32_t flags)
{
    int32_t err;

    flags |= TF_ACCEPT_FDS;

    err = this->writeTransactionData(this, BC_TRANSACTION, flags, handle, code, data, NULL);

    if (err != STATUS_OK) {
        if (reply)
            Parcel_setError(reply, err);
        return (this->mLastError = err);
    }

    if ((flags & TF_ONE_WAY) == 0) {
        if (this->mCallRestriction != CALL_RESTRICTION_NONE) {
            if (this->mCallRestriction == CALL_ERROR_IF_NOT_ONEWAY) {
                BINDER_LOGE("Process making non-oneway call (code: %" PRIu32 ") but is restricted.", code);
            } else {
                /* FATAL_IF_NOT_ONEWAY */
                LOG_FATAL("Process may not make non-oneway calls (code: %" PRIu32 ").", code);
            }
        }
        if (reply) {
            err = this->waitForResponse(this, reply, NULL);
        } else {
            Parcel fakeReply;
            Parcel_initState(&fakeReply);
            err = this->waitForResponse(this, &fakeReply, NULL);
        }
    } else {
        err = this->waitForResponse(this, NULL, NULL);
    }
    return err;
}

static void IPCThreadState_incStrongHandle(IPCThreadState* this, int32_t handle, BpBinder* proxy)
{
    Parcel_writeInt32(&this->mOut, BC_ACQUIRE);
    Parcel_writeInt32(&this->mOut, handle);
    if (!this->flushIfNeeded(this)) {
        /* Create a temp reference until the driver has handled this command.*/
        proxy->m_IBinder.m_refbase.incStrong(&proxy->m_IBinder.m_refbase, (const void*)this->mProcess);
        this->mPostWriteStrongDerefs.push(&this->mPostWriteStrongDerefs, &proxy->m_IBinder.m_refbase);
    }
}

static void IPCThreadState_decStrongHandle(IPCThreadState* this, int32_t handle)
{
    Parcel_writeInt32(&this->mOut, BC_RELEASE);
    Parcel_writeInt32(&this->mOut, handle);
    this->flushIfNeeded(this);
}

static void IPCThreadState_incWeakHandle(IPCThreadState* this, int32_t handle, BpBinder* proxy)
{
    Parcel_writeInt32(&this->mOut, BC_INCREFS);
    Parcel_writeInt32(&this->mOut, handle);
    if (!this->flushIfNeeded(this)) {
        RefBase_weakref* weakref = proxy->m_IBinder.m_refbase.getWeakRefs(&proxy->m_IBinder.m_refbase);
        /* Create a temp reference until the driver has handled this command. */
        weakref->incWeak(weakref, (const void*)this->mProcess);
        this->mPostWriteWeakDerefs.push(&this->mPostWriteWeakDerefs, weakref);
    }
}

static void IPCThreadState_decWeakHandle(IPCThreadState* this, int32_t handle)
{
    Parcel_writeInt32(&this->mOut, BC_DECREFS);
    Parcel_writeInt32(&this->mOut, handle);
    this->flushIfNeeded(this);
}

static int32_t IPCThreadState_attemptIncStrongHandle(IPCThreadState* this, int32_t handle)
{
    BINDER_LOGE("(%" PRIi32 "): Not supported\n", handle);
    return STATUS_INVALID_OPERATION;
}

static void IPCThreadState_expungeHandle(IPCThreadState* this, int32_t handle, IBinder* binder)
{
    IPCThreadState* self = IPCThreadState_self();
    self->mProcess->expungeHandle(self->mProcess, handle, binder);
}

static int32_t IPCThreadState_requestDeathNotification(IPCThreadState* this, int32_t handle, BpBinder* proxy)
{
    Parcel_writeInt32(&this->mOut, BC_REQUEST_DEATH_NOTIFICATION);
    Parcel_writeInt32(&this->mOut, (int32_t)handle);
    Parcel_writePointer(&this->mOut, (uintptr_t)proxy);
    return STATUS_OK;
}

static int32_t IPCThreadState_clearDeathNotification(IPCThreadState* this, int32_t handle, BpBinder* proxy)
{
    Parcel_writeInt32(&this->mOut, BC_CLEAR_DEATH_NOTIFICATION);
    Parcel_writeInt32(&this->mOut, (int32_t)handle);
    Parcel_writePointer(&this->mOut, (uintptr_t)proxy);
    return STATUS_OK;
}

static int32_t IPCThreadState_sendReply(IPCThreadState* this, const Parcel* reply, uint32_t flags)
{
    int32_t err;
    int32_t statusBuffer;
    err = this->writeTransactionData(this, BC_REPLY, flags, -1, 0, reply, &statusBuffer);

    if (err < STATUS_OK) {
        return err;
    }

    return this->waitForResponse(this, NULL, NULL);
}

static int32_t IPCThreadState_waitForResponse(IPCThreadState* this, Parcel* reply,
    int32_t* acquireResult)
{
    int32_t cmd;
    int32_t err;

    while (1) {
        if ((err = this->talkWithDriver(this, true)) < STATUS_OK)
            break;

        err = Parcel_errorCheck(&this->mIn);
        if (err < STATUS_OK)
            break;
        if (Parcel_dataAvail(&this->mIn) == 0)
            continue;

        Parcel_readInt32(&this->mIn, &cmd);

        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("Processing waitForResponse Command: %s",
                getReturnString(cmd));
        }

        switch (cmd) {
        case BR_ONEWAY_SPAM_SUSPECT:
            BINDER_LOGE("Process seems to be sending too many oneway calls.");
        case BR_TRANSACTION_COMPLETE: {
            if (!reply && !acquireResult)
                goto finish;
            break;
        }
        case BR_DEAD_REPLY: {
            err = STATUS_DEAD_OBJECT;
            goto finish;
        }
        case BR_FAILED_REPLY: {
            err = STATUS_FAILED_TRANSACTION;
            goto finish;
        }
        case BR_FROZEN_REPLY: {
            err = STATUS_FAILED_TRANSACTION;
            goto finish;
        }
        case BR_ACQUIRE_RESULT: {
            LOG_ASSERT(acquireResult != NULL, "Unexpected brACQUIRE_RESULT");
            int32_t result;
            Parcel_readInt32(&this->mIn, &result);
            if (!acquireResult)
                continue;
            *acquireResult = result ? STATUS_OK : STATUS_INVALID_OPERATION;
            goto finish;
        }
        case BR_REPLY: {
            struct binder_transaction_data tr;

            err = Parcel_read(&this->mIn, &tr, sizeof(tr));
            LOG_ASSERT(err == STATUS_OK, "Not enough command data for brREPLY");
            if (err != STATUS_OK) {
                goto finish;
            }

            if (reply) {
                if ((tr.flags & TF_STATUS_CODE) == 0) {
                    Parcel_ipcSetDataReference(reply, (uint8_t*)(tr.data.ptr.buffer),
                        tr.data_size, (binder_size_t*)(tr.data.ptr.offsets),
                        tr.offsets_size / sizeof(binder_size_t),
                        IPCThreadState_freeBuffer);
                } else {
                    err = *(const int32_t*)(tr.data.ptr.buffer);
                    IPCThreadState_freeBuffer(NULL, (const uint8_t*)(tr.data.ptr.buffer),
                        tr.data_size, (const binder_size_t*)(tr.data.ptr.offsets),
                        tr.offsets_size / sizeof(binder_size_t));
                }
            } else {
                IPCThreadState_freeBuffer(NULL, (const uint8_t*)(tr.data.ptr.buffer),
                    tr.data_size, (const binder_size_t*)(tr.data.ptr.offsets),
                    tr.offsets_size / sizeof(binder_size_t));
                continue;
            }

            goto finish;
        }
        default:
            err = this->executeCommand(this, cmd);
            if (err != STATUS_OK)
                goto finish;
            break;
        }
    }

finish:
    if (err != STATUS_OK) {
        if (acquireResult)
            *acquireResult = err;
        if (reply)
            Parcel_setError(reply, err);
        this->mLastError = err;
    }
    return err;
}

static int32_t IPCThreadState_talkWithDriver(IPCThreadState* this,
    bool doReceive)
{
    if (this->mProcess->mDriverFD < 0) {
        return -EBADF;
    }
    struct binder_write_read bwr;

    /* Is the read buffer empty? */

    const bool needRead = Parcel_dataPosition(&this->mIn) >= Parcel_dataSize(&this->mIn);

    /* We don't want to write anything if we are still reading
     * from data left in the input buffer and the caller
     * has requested to read the next data.
     */

    const size_t outAvail = (!doReceive || needRead) ? Parcel_dataSize(&this->mOut) : 0;
    bwr.write_size = outAvail;
    bwr.write_buffer = (uintptr_t)Parcel_data(&this->mOut);

    /* This is what we'll read. */

    if (doReceive && needRead) {
        bwr.read_size = Parcel_dataCapacity(&this->mIn);
        bwr.read_buffer = (uintptr_t)Parcel_data(&this->mIn);
    } else {
        bwr.read_size = 0;
        bwr.read_buffer = 0;
    }

    IF_LOG_VERBOSE()
    {
        if (outAvail != 0) {
            const void* cmds = (const void*)bwr.write_buffer;
            UNUSED(cmds);
            BINDER_LOGD("Sending commands to driver: %s", getCommandString(cmds));
        }
        BINDER_LOGD("Size of receive buffer: %ld, needRead: %s, doReceive: %s",
            bwr.read_size, needRead ? "true" : "false",
            doReceive ? "true" : "false");
    }

    /* Return immediately if there is nothing to do. */

    if ((bwr.write_size == 0) && (bwr.read_size == 0)) {
        return STATUS_OK;
    }

    bwr.write_consumed = 0;
    bwr.read_consumed = 0;
    int32_t err;
    do {
        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("About to read/write, write size = %zu",
                Parcel_dataSize(&this->mOut));
        }
        if (ioctl(this->mProcess->mDriverFD, BINDER_WRITE_READ, &bwr) >= 0) {
            err = STATUS_OK;
        } else {
            err = -errno;
        }

        if (this->mProcess->mDriverFD < 0) {
            err = -EBADF;
        }

        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("Finished read/write, write size = %zu",
                Parcel_dataSize(&this->mOut));
        }
    } while (err == -EINTR);

    IF_LOG_VERBOSE()
    {
        BINDER_LOGD("Our err: %" PRIi32 ", write consumed: %zu (of %zu), read consumed: %zu",
            err, (size_t)bwr.write_consumed, Parcel_dataSize(&this->mOut),
            (size_t)bwr.read_consumed);
    }

    if (err >= STATUS_OK) {
        if (bwr.write_consumed > 0) {
            if (bwr.write_consumed < Parcel_dataSize(&this->mOut)) {
                LOG_FATAL("Driver did not consume write buffer. "
                          "err: %s consumed: %zu of %zu",
                    statusToString(err), (size_t)bwr.write_consumed,
                    Parcel_dataSize(&this->mOut));
            } else {
                Parcel_setDataSize(&this->mOut, 0);
                this->processPostWriteDerefs(this);
            }
        }
        if (bwr.read_consumed > 0) {
            Parcel_setDataSize(&this->mIn, bwr.read_consumed);
            Parcel_setDataPosition(&this->mIn, 0);
        }

        return STATUS_OK;
    }
    return err;
}

static int32_t IPCThreadState_writeTransactionData(IPCThreadState* this, int32_t cmd, uint32_t binderFlags,
    int32_t handle, uint32_t code, const Parcel* data,
    int32_t* statusBuffer)
{
    struct binder_transaction_data tr;
    tr.target.ptr = 0; /* Don't pass uninitialized stack data to a remote process */
    tr.target.handle = handle;
    tr.code = code;
    tr.flags = binderFlags;
    tr.cookie = 0;
    tr.sender_pid = 0;
    tr.sender_euid = 0;
    const uint32_t err = Parcel_errorCheck(data);
    if (err == STATUS_OK) {
        tr.data_size = Parcel_ipcDataSize(data);
        tr.data.ptr.buffer = Parcel_ipcData(data);
        ;
        tr.offsets_size = Parcel_ipcObjectsCount(data) * sizeof(binder_size_t);
        tr.data.ptr.offsets = Parcel_ipcObjects(data);
    } else if (statusBuffer) {
        tr.flags |= TF_STATUS_CODE;
        *statusBuffer = err;
        tr.data_size = sizeof(uint32_t);
        tr.data.ptr.buffer = (uintptr_t)(statusBuffer);
        tr.offsets_size = 0;
        tr.data.ptr.offsets = 0;
    } else {
        return (this->mLastError = err);
    }
    Parcel_writeInt32(&this->mOut, cmd);
    Parcel_write(&this->mOut, &tr, sizeof(tr));
    return STATUS_OK;
}

static void IPCThreadState_setTheContextObject(IPCThreadState* this,
    BBinder* obj)
{
    ProcessState* proc = ProcessState_self();

    proc->mContextObject = obj;
}

static int32_t IPCThreadState_executeCommand(IPCThreadState* this,
    int32_t cmd)
{
    BBinder* obj;
    RefBase_weakref* refs;
    int32_t result = STATUS_OK;

    switch ((uint32_t)cmd) {
    case BR_ERROR: {
        Parcel_readInt32(&this->mIn, &result);
        break;
    }

    case BR_OK: {
        break;
    }

    case BR_ACQUIRE: {
        refs = (RefBase_weakref*)Parcel_readPointer(&this->mIn);
        obj = (BBinder*)Parcel_readPointer(&this->mIn);

        /* refs->mBase should be equal &obj->m_refbase */

        LOG_ASSERT((void*)refs->refBase(refs) == (void*)obj,
            "BR_ACQUIRE: object %p does not match cookie %p (expected %p)",
            refs, obj, refs->refBase(refs));
        obj->incStrong(obj, (const void*)this->mProcess);

        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("BR_ACQUIRE from driver on %p", obj);
            obj->printRefs(obj);
        }
        Parcel_writeInt32(&this->mOut, BC_ACQUIRE_DONE);
        Parcel_writePointer(&this->mOut, (uintptr_t)refs);
        Parcel_writePointer(&this->mOut, (uintptr_t)obj);
        break;
    }

    case BR_RELEASE: {
        refs = (RefBase_weakref*)Parcel_readPointer(&this->mIn);
        obj = (BBinder*)Parcel_readPointer(&this->mIn);
        LOG_ASSERT((void*)refs->refBase(refs) == (void*)obj,
            "BR_RELEASE: object %p does not match cookie %p (expected %p)",
            refs, obj, refs->refBase(refs));
        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("BR_RELEASE from driver on %p", obj);
            obj->printRefs(obj);
        }
        this->mPendingStrongDerefs.push(&this->mPendingStrongDerefs, obj);
        break;
    }

    case BR_INCREFS: {
        refs = (RefBase_weakref*)Parcel_readPointer(&this->mIn);
        obj = (BBinder*)Parcel_readPointer(&this->mIn);
        refs->incWeak(refs, (const void*)this->mProcess);
        Parcel_writeInt32(&this->mOut, BC_INCREFS_DONE);
        Parcel_writePointer(&this->mOut, (uintptr_t)refs);
        Parcel_writePointer(&this->mOut, (uintptr_t)obj);
        break;
    }

    case BR_DECREFS: {
        refs = (RefBase_weakref*)Parcel_readPointer(&this->mIn);
        obj = (BBinder*)Parcel_readPointer(&this->mIn);
        this->mPendingWeakDerefs.push(&this->mPendingWeakDerefs, refs);
        break;
    }
    case BR_ATTEMPT_ACQUIRE: {
        bool success;

        refs = (RefBase_weakref*)Parcel_readPointer(&this->mIn);
        obj = (BBinder*)Parcel_readPointer(&this->mIn);
        success = refs->attemptIncStrong(refs, (const void*)this->mProcess);
        LOG_ASSERT(success && (void*)refs->refBase(refs) == (void*)obj,
            "BR_ATTEMPT_ACQUIRE: object %p does not match cookie %p (expected %p)",
            refs, obj, refs->refBase(refs));
        Parcel_writeInt32(&this->mOut, BC_ACQUIRE_RESULT);
        Parcel_writeInt32(&this->mOut, (int32_t)success);
        break;
    }

    case BR_TRANSACTION_SEC_CTX:
    case BR_TRANSACTION: {
        struct binder_transaction_data tr;
        result = Parcel_read(&this->mIn, &tr, sizeof(tr));
        LOG_ASSERT(result == STATUS_OK,
            "Not enough command data for brTRANSACTION");
        if (result != STATUS_OK) {
            break;
        }

        Parcel buffer;
        Parcel_initState(&buffer);
        Parcel_ipcSetDataReference(&buffer, (uint8_t*)(tr.data.ptr.buffer),
            tr.data_size, (binder_size_t*)(tr.data.ptr.offsets),
            tr.offsets_size / sizeof(binder_size_t),
            IPCThreadState_freeBuffer);
        const void* origServingStackPointer = this->mServingStackPointer;
        this->mServingStackPointer = __builtin_frame_address(0);
        const pid_t origPid = this->mCallingPid;
        const char* origSid = this->mCallingSid;
        const uid_t origUid = this->mCallingUid;
        const int32_t origStrictModePolicy = this->mStrictModePolicy;
        const int32_t origTransactionBinderFlags = this->mLastTransactionBinderFlags;
        const int32_t origWorkSource = this->mWorkSource;
        const bool origPropagateWorkSet = this->mPropagateWorkSource;

        /* Calling work source will be set by Parcel#enforceInterface.
         * Parcel#enforceInterface
         * is only guaranteed to be called for AIDL-generated stubs so we reset
         * the work source
         * here to never propagate it.
         */

        this->clearCallingWorkSource(this);
        this->clearPropagateWorkSource(this);
        this->mCallingPid = tr.sender_pid;
        this->mCallingSid = "SECCTX_TMP";
        this->mCallingUid = tr.sender_euid;
        this->mLastTransactionBinderFlags = tr.flags;
        Parcel reply;
        Parcel_initState(&reply);
        int32_t error;
        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("BR_TRANSACTION thr %p  / obj %p /code %" PRIu32 ""
                        "Data addr = %p",
                (void*)(uintptr_t)pthread_self(), (void*)tr.target.ptr,
                tr.code, (void*)tr.data.ptr.buffer);
        }
        if (tr.target.ptr) {
            /* We only have a weak reference on the target object, so we must
             * first try to
             * safely acquire a strong reference before doing anything else with
             * it.
             */

            RefBase_weakref* weakref = (RefBase_weakref*)(tr.target.ptr);
            BBinder* bbinder;

            if (weakref->attemptIncStrong(weakref, (const void*)this)) {
                bbinder = (BBinder*)(tr.cookie);
                error = bbinder->transact(bbinder, tr.code, &buffer,
                    &reply, tr.flags);
                bbinder->decStrong(bbinder, (const void*)this);
            } else {
                error = STATUS_UNKNOWN_TRANSACTION;
            }
        } else {
            BBinder* bbinder = this->mProcess->mContextObject;
            error = bbinder->transact(bbinder, tr.code, &buffer, &reply, tr.flags);
        }
        if ((tr.flags & TF_ONE_WAY) == 0) {
            BINDER_LOGI("Sending reply to %d!", this->mCallingPid);
            if (error < STATUS_OK) {
                Parcel_setError(&reply, error);
            }
            uint32_t kForwardReplyFlags = TF_CLEAR_BUF;
            this->sendReply(this, &reply, (tr.flags & kForwardReplyFlags));
        } else {
            if (error != STATUS_OK) {
                BINDER_LOGW("oneway function results for code %" PRIu32 " on binder at %p"
                            "will be dropped but finished with status %s",
                    tr.code, (void*)tr.target.ptr, statusToString(error));

                /* ideally we could log this even when error == OK, but it
                 * causes too much logspam because some manually-written
                 * interfaces have clients that call methods which always
                 * write results, sometimes as oneway methods.
                 */
                if (Parcel_dataSize(&reply) != 0) {
                    BINDER_LOGD(" and reply parcel size %zu\n",
                        Parcel_dataSize(&reply));
                }
            }
            BINDER_LOGI("NOT sending reply to %d!", this->mCallingPid);
        }
        this->mServingStackPointer = origServingStackPointer;
        this->mCallingPid = origPid;
        this->mCallingSid = origSid;
        this->mCallingUid = origUid;
        this->mStrictModePolicy = origStrictModePolicy;
        this->mLastTransactionBinderFlags = origTransactionBinderFlags;
        this->mWorkSource = origWorkSource;
        this->mPropagateWorkSource = origPropagateWorkSet;
        IF_LOG_VERBOSE()
        {
            BINDER_LOGD("BC_REPLY thr %p / obj %p",
                (void*)(uintptr_t)pthread_self(),
                (void*)tr.target.ptr);
        }
    } break;

    case BR_DEAD_BINDER: {
        BpBinder* proxy = (BpBinder*)Parcel_readPointer(&this->mIn);
        proxy->sendObituary(proxy);
        Parcel_writeInt32(&this->mOut, BC_DEAD_BINDER_DONE);
        Parcel_writePointer(&this->mOut, (uintptr_t)proxy);
    } break;

    case BR_CLEAR_DEATH_NOTIFICATION_DONE: {
        RefBase_weakref* weakref;
        BpBinder* proxy = (BpBinder*)Parcel_readPointer(&this->mIn);
        weakref = proxy->getWeakRefs(proxy);
        weakref->decWeak(weakref, (const void*)proxy);
    } break;

    case BR_FINISHED: {
        result = STATUS_TIMED_OUT;
        break;
    }

    case BR_NOOP: {
        break;
    }

    case BR_SPAWN_LOOPER: {
        this->mProcess->spawnPooledThread(this->mProcess, false);
        break;
    }

    default: {
        BINDER_LOGE("*** BAD COMMAND %" PRIi32 " received from Binder driver\n", cmd);
        result = STATUS_UNKNOWN_TRANSACTION;
        break;
    }
    }

    if (result != STATUS_OK) {
        this->mLastError = result;
    }

    return result;
}

void IPCThreadState_threadDestructor(void* st)
{
    IPCThreadState* self = (IPCThreadState*)st;

    if (self) {
        self->flushCommands(self);
        if (self->mProcess->mDriverFD >= 0) {
            ioctl(self->mProcess->mDriverFD, BINDER_THREAD_EXIT, 0);
        }
        IPCThreadState_delete(self);
    }
}

static void IPCThreadState_freeBuffer(Parcel* parcel, const uint8_t* data,
    size_t dataSize, const binder_size_t* objects,
    size_t objectsSize)
{
    IF_LOG_VERBOSE()
    {
        BINDER_LOGD("Writing BC_FREE_BUFFER for %p", data);
    }

    LOG_ASSERT(data != NULL, "Called with NULL data");

    if (parcel != NULL) {
        Parcel_closeFileDescriptors(parcel);
    }

    IPCThreadState* state = IPCThreadState_self();
    Parcel_writeInt32(&state->mOut, BC_FREE_BUFFER);
    Parcel_writePointer(&state->mOut, (uintptr_t)data);
    state->flushIfNeeded(state);
}

static void IPCThreadState_dtor(IPCThreadState* this)
{
    this->mPendingStrongDerefs.dtor(&this->mPendingStrongDerefs);
    this->mPendingWeakDerefs.dtor(&this->mPendingWeakDerefs);
    this->mPostWriteStrongDerefs.dtor(&this->mPostWriteStrongDerefs);
    this->mPostWriteWeakDerefs.dtor(&this->mPostWriteWeakDerefs);
}

static void IPCThreadState_ctor(IPCThreadState* this)
{
    VectorImpl_ctor(&this->mPendingStrongDerefs);
    VectorImpl_ctor(&this->mPendingWeakDerefs);
    VectorImpl_ctor(&this->mPostWriteStrongDerefs);
    VectorImpl_ctor(&this->mPostWriteWeakDerefs);

    Parcel_initState(&this->mIn);
    Parcel_initState(&this->mOut);

    this->mProcess = ProcessState_self();
    this->mServingStackPointer = NULL;
    this->mWorkSource = kUnsetWorkSource;
    this->mPropagateWorkSource = false;
    this->mIsLooper = false;
    this->mIsFlushing = false;
    this->mStrictModePolicy = 0;
    this->mLastTransactionBinderFlags = 0;
    this->mCallRestriction = this->mProcess->mCallRestriction;

    this->backgroundSchedulingDisabled = IPCThreadState_backgroundSchedulingDisabled;
    this->getCallingPid = IPCThreadState_getCallingPid;
    this->getCallingSid = IPCThreadState_getCallingSid;
    this->getCallingUid = IPCThreadState_getCallingUid;
    this->setStrictModePolicy = IPCThreadState_setStrictModePolicy;
    this->getStrictModePolicy = IPCThreadState_getStrictModePolicy;
    this->setCallingWorkSourceUid = IPCThreadState_setCallingWorkSourceUid;
    this->clearPropagateWorkSource = IPCThreadState_clearPropagateWorkSource;
    this->shouldPropagateWorkSource = IPCThreadState_shouldPropagateWorkSource;
    this->getCallingWorkSourceUid = IPCThreadState_getCallingWorkSourceUid;
    this->clearCallingWorkSource = IPCThreadState_clearCallingWorkSource;
    this->setLastTransactionBinderFlags = IPCThreadState_setLastTransactionBinderFlags;
    this->getLastTransactionBinderFlags = IPCThreadState_getLastTransactionBinderFlags;

    this->setCallRestriction = IPCThreadState_setCallRestriction;
    this->getCallRestriction = IPCThreadState_getCallRestriction;
    this->clearCaller = IPCThreadState_clearCaller;
    this->flushCommands = IPCThreadState_flushCommands;
    this->flushIfNeeded = IPCThreadState_flushIfNeeded;
    this->getAndExecuteCommand = IPCThreadState_getAndExecuteCommand;
    this->processPendingDerefs = IPCThreadState_processPendingDerefs;
    this->processPostWriteDerefs = IPCThreadState_processPostWriteDerefs;
    this->joinThreadPool = IPCThreadState_joinThreadPool;
    this->setupPolling = IPCThreadState_setupPolling;
    this->handlePolledCommands = IPCThreadState_handlePolledCommands;
    this->transact = IPCThreadState_transact;
    this->sendReply = IPCThreadState_sendReply;
    this->clearDeathNotification = IPCThreadState_clearDeathNotification;
    this->requestDeathNotification = IPCThreadState_requestDeathNotification;
    this->expungeHandle = IPCThreadState_expungeHandle;
    this->attemptIncStrongHandle = IPCThreadState_attemptIncStrongHandle;
    this->decWeakHandle = IPCThreadState_decWeakHandle;
    this->incWeakHandle = IPCThreadState_incWeakHandle;
    this->decStrongHandle = IPCThreadState_decStrongHandle;
    this->incStrongHandle = IPCThreadState_incStrongHandle;
    this->waitForResponse = IPCThreadState_waitForResponse;
    this->talkWithDriver = IPCThreadState_talkWithDriver;
    this->writeTransactionData = IPCThreadState_writeTransactionData;
    this->setTheContextObject = IPCThreadState_setTheContextObject;
    this->executeCommand = IPCThreadState_executeCommand;

    this->setCallingWorkSourceUidWithoutPropagation = IPCThreadState_setCallingWorkSourceUidWithoutPropagation;

    pthread_setspecific(this->mProcess->mTLS, this);
    this->clearCaller(this);
    Parcel_setDataCapacity(&this->mIn, 256);
    Parcel_setDataCapacity(&this->mOut, 256);

    this->dtor = IPCThreadState_dtor;
}

IPCThreadState* IPCThreadState_new(void)
{
    IPCThreadState* this;

    this = zalloc(sizeof(IPCThreadState));

    IPCThreadState_ctor(this);
    return this;
}

void IPCThreadState_delete(IPCThreadState* this)
{
    this->dtor(this);
    free(this);
}

IPCThreadState* IPCThreadState_self(void)
{
    ProcessState* proc = ProcessState_self();

    /* Racey, heuristic test for simultaneous shutdown. */
    if (atomic_load_explicit(&proc->mShutdown, memory_order_relaxed)) {
        BINDER_LOGE(
            "Calling IPCThreadState::self() during shutdown is dangerous, expect a crash.\n");
        return NULL;
    }
    IPCThreadState* st = (IPCThreadState*)pthread_getspecific(proc->mTLS);

    if (st) {
        return st;
    }
    return IPCThreadState_new();
}

IPCThreadState* IPCThreadState_selfOrNull(void)
{
    ProcessState* proc = ProcessState_self();

    return (IPCThreadState*)pthread_getspecific(proc->mTLS);
}
