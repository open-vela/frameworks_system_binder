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

#define BUFFER_SIZE 16
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
    char response[BUFFER_SIZE];
    Results* results_fifo = (Results*)priv->result;
    int no_inherent = priv->no_inherent;
    int no_sync = priv->no_sync;
    int& read_fd = priv->read_fd;
    int& write_fd = priv->write_fd;
    int cpu, priority;
    int num1, num2;
    Tick sta, end;

    num1 = thread_pri();
    num2 = sched_getcpu();

    char* buffer = (char*)malloc(BUFFER_SIZE);
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &num1, sizeof(int32_t));
    memcpy(buffer + sizeof(int32_t), &num2, sizeof(int32_t));

    sta = tickNow();
    write(write_fd, buffer, sizeof(buffer));
    read(read_fd, response, sizeof(response));
    end = tickNow();
    results_fifo->add_time(tickNano(sta, end));

    memcpy(&priority, response, sizeof(int32_t));
    memcpy(&cpu, response + sizeof(int32_t), sizeof(int32_t));

    no_inherent += priority;
    no_sync += cpu;

    free(buffer);

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
    char recv_buffer[BUFFER_SIZE];
    int pipefd[2];
    int pipefds[2];
    int no_inherent = 0;
    int no_trans;
    int no_sync = 0;
    int priority, priority_caller = 0;
    int cpu, cpu_caller = 0;
    int h = 0, s = 0;
    int hh, ss;
    int num1, num2;
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
        close(pipefd[1]);

        while (1) {
            read(pipefd[0], buffer, sizeof(buffer));

            memcpy(&priority_caller, buffer, sizeof(int32_t));
            memcpy(&cpu_caller, buffer + sizeof(int32_t), sizeof(int32_t));

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

            char* response = (char*)malloc(BUFFER_SIZE);
            memset(response, 0, BUFFER_SIZE);
            memcpy(response, &h, sizeof(int32_t));
            memcpy(response + sizeof(int32_t), &s, sizeof(int32_t));

            write(pipefds[1], response, BUFFER_SIZE);

            free(response);
        }

        close(pipefds[1]);
    }

    close(pipefd[0]);

    for (int i = 0; i < iterations; i++) {
        thread_transaction(&results_fifo, no_inherent, no_sync, pipefd[1], pipefds[0]);

        num1 = thread_pri();
        num2 = sched_getcpu();

        char* send_buffer = (char*)malloc(BUFFER_SIZE);
        memset(send_buffer, 0, BUFFER_SIZE);
        memcpy(send_buffer, &num1, sizeof(int32_t));
        memcpy(send_buffer + sizeof(int32_t), &num2, sizeof(int32_t));

        sta = tickNow();
        write(pipefd[1], buffer, sizeof(buffer));
        read(pipefds[0], recv_buffer, sizeof(recv_buffer));
        end = tickNow();
        results_other.add_time(tickNano(sta, end));

        memcpy(&hh, recv_buffer, sizeof(int32_t));
        memcpy(&ss, recv_buffer + sizeof(int32_t), sizeof(int32_t));

        no_inherent += hh;
        no_sync += ss;

        free(send_buffer);
    }

    close(pipefds[0]);

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
