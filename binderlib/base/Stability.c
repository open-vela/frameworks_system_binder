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

#define LOG_TAG "Stability"

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

#include "Binder.h"
#include "BpBinder.h"
#include "IPCThreadState.h"
#include "Parcel.h"
#include "ProcessState.h"
#include "Stability.h"
#include "Status.h"
#include "utils/Binderlog.h"
#include <android/binder_status.h>

enum Level getLocalLevel(void)
{
    return STABILITY_SYSTEM;
}

static bool isDeclaredLevel(int32_t stability)
{
    return stability == STABILITY_VENDOR || stability == STABILITY_SYSTEM || stability == STABILITY_VINTF;
}

#ifdef CONFIG_BINDER_LIB_DEBUG
static const char* levelString(int32_t level)
{
    switch (level) {
    case STABILITY_UNDECLARED:
        return "undeclared stability";
    case STABILITY_VENDOR:
        return "vendor stability";
    case STABILITY_SYSTEM:
        return "system stability";
    case STABILITY_VINTF:
        return "vintf stability";
    }
    return "unknown stability";
}
#endif

static bool check(int16_t provided, enum Level required)
{
    bool stable = (provided & required) == required;
    if (provided != STABILITY_UNDECLARED && !isDeclaredLevel(provided)) {
        BINDER_LOGE("Unknown stability when checking interface stability %d.", provided);
        stable = false;
    }
    return stable;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int16_t Stability_getRepr(IBinder* binder)
{
    if (binder == NULL) {
        return STABILITY_UNDECLARED;
    }

    BBinder* local = binder->localBinder(binder);
    if (local != NULL) {
        return local->mStability;
    }
    return binder->remoteBinder(binder)->mStability;
}

int32_t Stability_setRepr(IBinder* binder, int32_t setting, uint32_t flags)
{
    bool log = flags & REPR_LOG;
    bool allowDowngrade = flags & REPR_ALLOW_DOWNGRADE;
    int16_t current = Stability_getRepr(binder);

    /* null binder is always written w/ 'UNDECLARED' stability */

    if (binder == NULL) {
        if (setting == STABILITY_UNDECLARED) {
            return STATUS_OK;
        } else {
            if (log) {
                BINDER_LOGE("Null binder written with stability %s.",
                    levelString(setting));
            }
            return STATUS_BAD_TYPE;
        }
    }

    if (!isDeclaredLevel(setting)) {
        if (log) {
            BINDER_LOGE("Can only set known stability, not %" PRId32 ".", setting);
        }
        return STATUS_BAD_TYPE;
    }
    enum Level levelSetting = setting;

    if (current == setting)
        return STATUS_OK;

    bool hasAlreadyBeenSet = current != STABILITY_UNDECLARED;
    bool isAllowedDowngrade = allowDowngrade && check(current, levelSetting);
    if (hasAlreadyBeenSet && !isAllowedDowngrade) {
        if (log) {
            BINDER_LOGE("Interface being set with %s but it is already marked as %s",
                levelString(setting), levelString(current));
        }
        return STATUS_BAD_TYPE;
    }

    if (isAllowedDowngrade) {
        BINDER_LOGI("Interface set with %s downgraded to %s stability",
            levelString(current), levelString(setting));
    }

    BBinder* local = binder->localBinder(binder);
    if (local != NULL) {
        local->mStability = setting;
    } else {
        binder->remoteBinder(binder)->mStability = setting;
    }

    return STATUS_OK;
}

void Stability_markCompilationUnit(IBinder* binder)
{
    int32_t result = Stability_setRepr(binder, getLocalLevel(), REPR_LOG);

    LOG_FATAL_IF(result != STATUS_OK, "Should only mark known object.");
}

void Stability_tryMarkCompilationUnit(IBinder* binder)
{
    int32_t result = Stability_setRepr(binder, getLocalLevel(), REPR_NONE);
    (void)result;
}
