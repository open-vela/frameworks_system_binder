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

#include <debug.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/init.h>

#include "utils/Binderlog.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_BINDER_LIB_DEBUG

char binderlib_log[BINDER_LIB_LOG_BUFSIZE];

const uint32_t binderlib_debug_mask = BINDERLIB_DEBUG_ERROR | BINDERLIB_DEBUG_WARNING;

void binderlib_log_write(struct binder_log_message* log)
{
    syslog(LOG_INFO, "[%s (%d)][%d:%d]:%s",
        log->tag, log->line, log->pid,
        log->tid, log->message);
}
#endif

void BinderProcess_assert(void)
{
    /* For binder main thread, it's a NuttX task (process)
     * For binder thread pool threads, it's a pthread task
     * so need different method to exit thread
     */

    if (gettid() == getpid()) {
        exit(EXIT_FAILURE);
    } else {
        abort();
    }
}
