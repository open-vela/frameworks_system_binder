/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "Utils.Timers"

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <nuttx/android/binder.h>
#include <nuttx/clock.h>
#include <nuttx/tls.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "utils/Binderlog.h"
#include "utils/Thread.h"
#include "utils/Timers.h"

#define CLOCK_ID_MAX 5

static void checkClockId(int clock)
{
    LOG_FATAL_IF(clock < 0 || clock >= CLOCK_ID_MAX, "invalid clock id");
}

nsecs_t systemTime(int clock)
{
    struct timespec t;

    checkClockId(clock);

    static clockid_t clocks[] = {
        CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID,
        CLOCK_THREAD_CPUTIME_ID, CLOCK_BOOTTIME
    };
    clock_gettime(clocks[clock], &t);

    return (nsecs_t)(t.tv_sec) * 1000000000LL + t.tv_nsec;
}

int toMillisecondTimeoutDelay(nsecs_t referenceTime, nsecs_t timeoutTime)
{
    uint64_t timeoutDelay;

    if (timeoutTime <= referenceTime) {
        return 0;
    }

    timeoutDelay = (uint64_t)(timeoutTime - referenceTime);
    if (timeoutDelay > (uint64_t)((INT_MAX - 1) * 1000000LL)) {
        return -1;
    }
    return (timeoutDelay + 999999LL) / 1000000LL;
}
