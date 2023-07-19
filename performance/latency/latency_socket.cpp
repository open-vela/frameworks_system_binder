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

#include <arpa/inet.h>
#include <cstdint>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#include "latency_time.h"

#define BUFFER_SIZE 16
#define SOCKET_PATH "/dev/local_socket"

struct socket_priv_t {
    void* result;
    int no_inherent;
    int no_sync;
    int client_socket;
};

static void* thread_start(void* p)
{
    socket_priv_t* priv = (socket_priv_t*)p;
    Results* results_fifo = (Results*)priv->result;
    int no_inherent = priv->no_inherent;
    int no_sync = priv->no_sync;
    int client_socket = priv->client_socket;
    int cpu;
    int priority;
    Tick sta, end;
    char recv_buffer[BUFFER_SIZE];
    int num1, num2;

    num1 = thread_pri();
    num2 = sched_getcpu();

    char* send_buffer = (char*)malloc(BUFFER_SIZE);
    memset(send_buffer, 0, BUFFER_SIZE);
    memcpy(send_buffer, &num1, sizeof(int32_t));
    memcpy(send_buffer + sizeof(int32_t), &num2, sizeof(int32_t));

    sta = tickNow();
    send(client_socket, send_buffer, sizeof(send_buffer), 0);
    recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
    end = tickNow();
    results_fifo->add_time(tickNano(sta, end));

    memcpy(&priority, recv_buffer, sizeof(int32_t));
    memcpy(&cpu, recv_buffer + sizeof(int32_t), sizeof(int32_t));

    no_inherent += priority;
    no_sync += cpu;

    free(send_buffer);

    return nullptr;
}

// create a fifo thread to transact and wait it to finished
static void thread_transaction(Results* results_fifo, int no_inherent, int no_sync, int client_socket)
{
    socket_priv_t thread_priv = {
        results_fifo, no_inherent, no_sync, client_socket
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

int client(int iterations)
{
    char recv_buffer[BUFFER_SIZE];
    double sync_ratio;
    int client_socket;
    int no_inherent = 0;
    int no_sync = 0;
    int no_trans;
    int priority = 0;
    int cpu = 0;
    int num1, num2;
    Results results_other(false);
    Results results_fifo(false);
    struct sockaddr_un serverAddr;
    Tick sta;
    Tick end;

    if ((client_socket = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.sun_family = AF_LOCAL;
    strncpy(serverAddr.sun_path, SOCKET_PATH, sizeof(serverAddr.sun_path) - 1);
    unlink(serverAddr.sun_path);

    if (connect(client_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < iterations; i++) {
        thread_transaction(&results_fifo, no_inherent, no_sync, client_socket);

        num1 = thread_pri();
        num2 = sched_getcpu();

        char* send_buffer = (char*)malloc(BUFFER_SIZE);
        memset(send_buffer, 0, BUFFER_SIZE);
        memcpy(send_buffer, &num1, sizeof(int32_t));
        memcpy(send_buffer + sizeof(int32_t), &num2, sizeof(int32_t));

        sta = tickNow();
        send(client_socket, send_buffer, sizeof(send_buffer), 0);
        recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
        end = tickNow();
        results_other.add_time(tickNano(sta, end));

        memcpy(&priority, recv_buffer, sizeof(int32_t));
        memcpy(&cpu, recv_buffer + sizeof(int32_t), sizeof(int32_t));

        no_inherent += priority;
        no_sync += cpu;

        free(send_buffer);
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

    return client_socket;
}

extern "C" int main(int argc, char** argv)
{
    int client_socket;
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

    client_socket = client(iterations);
    printf("PASS\n}\n");
    close(client_socket);
    return 0;
}
