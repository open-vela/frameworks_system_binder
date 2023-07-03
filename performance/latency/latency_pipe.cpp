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

#include <cstdint>
#include <cstring>
#include <inttypes.h>
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "latency_time.h"

#define BUFFER_SIZE 64
struct pipe_priv {
    void* result;
    int no_inherent;
    int no_sync;
    int& read_fd;
    int& write_fd;
};

static void* thread_start(void* p)
{
    pipe_priv* priv = (pipe_priv*)p;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    Results* results_fifo = (Results*)priv->result;
    int no_inherent = priv->no_inherent;
    int no_sync = priv->no_sync;
    int& read_fd = priv->read_fd;
    int& write_fd = priv->write_fd;
    int cpu, priority;
    int recv_pri, recv_cpu;
    int h = 0, s = 0;
    Tick sta, end;

    sendBuffer(buffer, thread_pri(), sched_getcpu());

    sta = tickNow();
    write(write_fd, buffer, sizeof(buffer));
    read(read_fd, response, sizeof(response));
    recvBuffer(response, &recv_pri, &recv_cpu);
    priority = thread_pri();
    if (recv_pri != priority) {
        h++;
    }
    if (priority == sched_get_priority_max(SCHED_FIFO)) {
        cpu = sched_getcpu();
        if (cpu != recv_cpu) {
            s++;
        }
    }
    end = tickNow();
    results_fifo->add_time(tickNano(sta, end));

    no_inherent += h;
    no_sync += s;

    return nullptr;
}

// create a fifo thread to transact and wait it to finished
static void thread_transaction(Results* results_fifo, int no_inherent,
    int no_sync, int& read_fd, int& write_fd)
{
    pipe_priv thread_priv = {
        results_fifo, no_inherent, no_sync, read_fd, write_fd
    };
    void* dummy;
    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&thread, &attr, &thread_start, &thread_priv);
    pthread_join(thread, &dummy);
}

void pipe_transaction(int iterations)
{
    double sync_ratio;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];
    int pipefd[2];
    int pipefds[2];
    int no_inherent = 0;
    int no_trans;
    int no_sync = 0;
    int cpu, cpu_caller = 0;
    int priority, priority_caller = 0;
    int h = 0, s = 0;
    pid_t pid;
    Results results_other(false);
    Results results_fifo(false);
    Tick sta;
    Tick end;

    if (pipe(pipefd) == -1 || pipe(pipefds) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        while (1) {
            ssize_t read_size = read(pipefd[0], response, sizeof(response));
            write(pipefds[1], response, read_size);
        }
    }

    for (int i = 0; i < iterations; i++) {
        thread_transaction(&results_fifo, no_inherent, no_sync, pipefd[1], pipefds[0]);
        sendBuffer(buffer, thread_pri(), sched_getcpu());
        sta = tickNow();
        write(pipefd[1], buffer, sizeof(buffer));
        read(pipefds[0], recv_buffer, sizeof(recv_buffer));
        recvBuffer(recv_buffer, &priority_caller, &cpu_caller);
        priority = thread_pri();
        if (priority_caller != priority) {
            h++;
        }
        if (priority == sched_get_priority_max(SCHED_FIFO)) {
            cpu = sched_getcpu();
            if (cpu_caller != cpu) {
                s++;
            }
        }
        end = tickNow();
        results_other.add_time(tickNano(sta, end));

        no_inherent += h;
        no_sync += s;
    }

    no_trans = iterations * 2;
    sync_ratio = (1.0 - (double)no_sync / no_trans);

    printf("\"P%d\":{\"SYNC\":\"%s\",\"S\":%d,\"I\":%d,\"R\":%f,\n",
        0, ((sync_ratio > GOOD_SYNC_MIN) ? "GOOD" : "POOR"),
        (no_trans - no_sync), no_trans, sync_ratio);

    printf("  \"other_ms\":");
    results_other.dump();
    printf("  \"fifo_ms\":");
    results_fifo.dump();
    printf("},\n");
}

extern "C" int main(int argc, char** argv)
{
    int iterations = 100;

    for (int i = 1; i < argc; i++) {
        if (!(strcmp((argv[i]), "-i"))) {
            iterations = atoi(argv[i + 1]);
            i++;
            continue;
        }
    }

    printf("{\n");
    printf("\"cfg\":{\"iterations\":%d,\"deadline_us\":%d},\n", iterations, deadline_us);

    pipe_transaction(iterations);

    printf("PASS\n}\n");

    return 0;
}
