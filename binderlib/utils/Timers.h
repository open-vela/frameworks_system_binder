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

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef __BINDER_INCLUDE_UTILS_TIMERS_H__
#define __BINDER_INCLUDE_UTILS_TIMERS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t nsecs_t;

static inline nsecs_t seconds_to_nanoseconds(nsecs_t sec)
{
    return sec * 1000000000;
}

static inline nsecs_t milliseconds_to_nanoseconds(nsecs_t sec)
{
    return sec * 1000000;
}

static inline nsecs_t microseconds_to_nanoseconds(nsecs_t sec)
{
    return sec * 1000;
}

static inline nsecs_t nanoseconds_to_seconds(nsecs_t sec)
{
    return sec / 1000000000;
}

static inline nsecs_t nanoseconds_to_milliseconds(nsecs_t sec)
{
    return sec / 1000000;
}

static inline nsecs_t nanoseconds_to_microseconds(nsecs_t sec)
{
    return sec / 1000;
}

/* return the system-time according to the specified clock
 */

nsecs_t systemTime(int clock);

static inline int64_t uptimeNanos(void)
{
    return systemTime(CLOCK_MONOTONIC);
}

static inline int64_t uptimeMillis(void)
{
    return nanoseconds_to_milliseconds(uptimeNanos());
}

/**
 * Returns the number of milliseconds to wait between the reference time and the timeout time.
 * If the timeout is in the past relative to the reference time, returns 0.
 * If the timeout is more than INT_MAX milliseconds in the future relative to the reference time,
 * such as when timeoutTime == LLONG_MAX, returns -1 to indicate an infinite timeout delay.
 * Otherwise, returns the difference between the reference time and timeout time
 * rounded up to the next millisecond.
 */
int toMillisecondTimeoutDelay(nsecs_t referenceTime, nsecs_t timeoutTime);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__BINDER_INCLUDE_UTILS_TIMERS_H__
