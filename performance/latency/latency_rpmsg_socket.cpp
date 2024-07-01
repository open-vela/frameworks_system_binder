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
#include <netpacket/rpmsg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#include "latency_time.h"

#define BUFFER_SIZE 1024

static void latency_test(int iterations, const char* rp_name,
    const char* rp_cpu, bool oneway)
{
    int client_socket;
    Results results(false);
    struct sockaddr_rpmsg serverAddr;
    Tick begin;
    Tick end;

    printf("thread sched_priority: %d\n", thread_pri());

    if ((client_socket = socket(PF_RPMSG, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.rp_family = AF_RPMSG;
    strlcpy(serverAddr.rp_name, rp_name, RPMSG_SOCKET_NAME_SIZE);
    strlcpy(serverAddr.rp_cpu, rp_cpu, RPMSG_SOCKET_CPU_SIZE);

    if (connect(client_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    char* send_buffer = (char*)malloc(BUFFER_SIZE);
    char* recv_buffer = (char*)malloc(BUFFER_SIZE);

    if (send_buffer != NULL && recv_buffer != NULL) {
        memset(send_buffer, 0, BUFFER_SIZE);

        for (int i = 0; i < iterations; i++) {
            begin = tickNow();
            send(client_socket, send_buffer, BUFFER_SIZE, 0);
            if (!oneway) {
                recv(client_socket, recv_buffer, BUFFER_SIZE, 0);
            }
            end = tickNow();
            results.add_time(tickNano(begin, end));
        }

        printf("  \"Result(ms)\":");
        results.dump();
    }

    free(send_buffer);
    free(recv_buffer);
}

extern "C" int main(int argc, char** argv)
{
    if (argc != 5) {
        printf("[usage]: latency_rpmsg_socket [test_count] [rp_name] [rp_cpu] [oneway]\n");
        return 0;
    }

    int iterations = atoi(argv[1]);
    latency_test(iterations, argv[2], argv[3], !strcmp(argv[4], "oneway"));

    return 0;
}
