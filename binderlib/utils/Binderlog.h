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

#ifndef __BINDER_INCLUDE_UTILS_BINDERLOG_H__
#define __BINDER_INCLUDE_UTILS_BINDERLOG_H__

#include <debug.h>
#include <errno.h>
#include <inttypes.h>
#include <nuttx/config.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include <nuttx/android/binder.h>
#include <nuttx/list.h>
#include <nuttx/mutex.h>

#ifdef CONFIG_BINDER_LIB_DEBUG

enum {
    BINDERLIB_DEBUG_ERROR = 1U << 0,
    BINDERLIB_DEBUG_WARNING = 1U << 1,
    BINDERLIB_DEBUG_INFO = 1U << 2,
    BINDERLIB_DEBUG_VERBOSE = 1U << 3
};

/**
 * Logger data struct used for writing log messages
 */

struct binder_log_message {
    /* The tag for the log message. */
    const char* tag;

    /* Optional line number, ignore if file is nullptr. */
    int line;

    /* The function for the log message. */
    const char* func;

    /* The Process ID and Thread ID for the log message. */
    pid_t pid;
    pid_t tid;

    /* The log message itself. */
    const char* message;
};

/**
 * Writes the log message specified by log_message.  log_message includes
 * additional file name and
 * line number information that a logger may use.
 * @param log_message the log message itself, see binderlib_log_message.
 */
void binderlib_log_write(struct binder_log_message* log_message);

#define BINDER_LIB_LOG_BUFSIZE 512
extern char binderlib_log[BINDER_LIB_LOG_BUFSIZE];
extern const uint32_t binderlib_debug_mask;

#define BINDER_LOGD(x...)                \
    do {                                 \
        struct binder_log_message __log; \
        __log.tag = LOG_TAG;             \
        __log.line = __LINE__;           \
        __log.func = __func__;           \
        __log.pid = getpid();            \
        __log.tid = gettid();            \
        snprintf(binderlib_log,          \
            BINDER_LIB_LOG_BUFSIZE, x);  \
        __log.message = binderlib_log;   \
        binderlib_log_write(&__log);     \
    } while (0)

#define BINDER_LOGW(x...)                                     \
    do {                                                      \
        if (binderlib_debug_mask & BINDERLIB_DEBUG_WARNING) { \
            BINDER_LOGD(x);                                   \
        }                                                     \
    } while (0)

#define BINDER_LOGE(x...)                                   \
    do {                                                    \
        if (binderlib_debug_mask & BINDERLIB_DEBUG_ERROR) { \
            BINDER_LOGD(x);                                 \
        }                                                   \
    } while (0)

#define BINDER_LOGI(x...)                                  \
    do {                                                   \
        if (binderlib_debug_mask & BINDERLIB_DEBUG_INFO) { \
            BINDER_LOGD(x);                                \
        }                                                  \
    } while (0)

#define BINDER_LOGV(x...)                                     \
    do {                                                      \
        if (binderlib_debug_mask & BINDERLIB_DEBUG_VERBOSE) { \
            BINDER_LOGD(x);                                   \
        }                                                     \
    } while (0)

#define IF_LOG_VERBOSE() \
    if (binderlib_debug_mask & BINDERLIB_DEBUG_VERBOSE)

#else
#define BINDER_LOGD(x...)
#define BINDER_LOGV(x...)
#define BINDER_LOGW(x...)
#define BINDER_LOGE(x...)
#define BINDER_LOGI(x...)
#define IF_LOG_VERBOSE() if (false)
#endif
#define CHECK(x)                          \
    if (x) {                              \
        BINDER_LOGD("Check failed: " #x); \
    }

#define CHECK_FAIL(x)                           \
    if (x) {                                    \
        BINDER_LOGD("Check FATAL_failed: " #x); \
        BinderProcess_assert();                 \
    }

#define LOG_FATAL_IF(condition, x...)                       \
    do {                                                    \
        if (condition) {                                    \
            BINDER_LOGD("Check FATAL_failed: " #condition); \
            BINDER_LOGD(x);                                 \
            BinderProcess_assert();                         \
        }                                                   \
    } while (0)

#define LOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ##__VA_ARGS__)

#define LOG_FATAL(x...)         \
    do {                        \
        BINDER_LOGD(x);         \
        BinderProcess_assert(); \
    } while (0)

void BinderProcess_assert(void);

/* Log any transactions for which the data exceeds this size */

#define LOG_TRANSACTIONS_OVER_SIZE (2 * 1024)

/* Log any reply transactions for which the data exceeds this size */

#define LOG_REPLIES_OVER_SIZE (2 * 1024)
#endif //__BINDER_INCLUDE_UTILS_LOOPER_H__
