/*
 * Copyright (C) 2023 Xiaomi Corporation
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

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>

#define DUMP_PRESICION 2
#define deadline_us 2500
#define GOOD_SYNC_MIN 0.6

typedef std::chrono::time_point<std::chrono::high_resolution_clock> Ticks;

static inline Ticks tickNow()
{
    return std::chrono::high_resolution_clock::now();
}

static inline uint64_t tickNano(Ticks& sta, Ticks& end)
{
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(end - sta).count());
}

struct Results {
    uint64_t best = 0xffffffffffffffffULL;
    uint64_t worst = 0;
    uint64_t trans = 0;
    uint64_t total_time = 0;
    uint64_t m_miss = 0;
    bool tracing;
    explicit Results(bool _tracing)
        : tracing(_tracing)
    {
    }
    inline bool miss_deadline(uint64_t nanos)
    {
        return nanos > deadline_us * 1000;
    }
    void add_time(uint64_t nanos)
    {
        best = std::min(nanos, best);
        worst = std::max(nanos, worst);
        trans += 1;
        total_time += nanos;
        if (miss_deadline(nanos))
            m_miss++;
    }
    void dump()
    {
        double best = (double)best / 1.0E6;
        double worst = (double)worst / 1.0E6;
        double average = (double)total_time / trans / 1.0E6;
        int W = DUMP_PRESICION + 2;

        printf("{ \"avg\":%*.*f,\"wst\":%*.*f,\"bst\":%*.*f,\"miss\":%" PRIu64 ",\"meetR\":%.3f}\n",
            W, DUMP_PRESICION, average, W, DUMP_PRESICION, worst, W, DUMP_PRESICION,
            best, m_miss, 1.0 - (double)m_miss / trans);
    }
};

static int thread_pri()
{
    int policy;
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &policy, &param);
    return param.sched_priority;
}
