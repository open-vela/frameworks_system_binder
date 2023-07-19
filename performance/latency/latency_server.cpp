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
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "latency_time.h"

#define BUFFER_SIZE 16
#define SOCKET_PATH "/dev/local_socket"

extern "C" int main(int argc, char** argv)
{
    char buffer[BUFFER_SIZE];
    int client_socket;
    int server_socket;
    int cpu, cpu_caller = 0;
    int priority, priority_caller = 0;
    int h = 0, s = 0;
    struct sockaddr_un server_address, client_address;
    socklen_t client_address_len;

    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    server_address.sun_family = AF_LOCAL;
    strncpy(server_address.sun_path, SOCKET_PATH, sizeof(server_address.sun_path) - 1);
    unlink(server_address.sun_path);

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to bind socket.");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1) == -1) {
        perror("Failed to listen for connections.");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    client_address_len = sizeof(client_address);
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Failed to accept client connection.");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        while (1) {
            ssize_t recv_size = recv(client_socket, buffer, sizeof(buffer), 0);
            if (recv_size < 0) {
                perror("Failed to receive data from client.");
                break;
            }

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

            ssize_t response_size = send(client_socket, response, BUFFER_SIZE, 0);
            if (response_size < 0) {
                perror("Failed to send data to client.");
                break;
            }

            free(response);
        }

        close(client_socket);
    }

    close(server_socket);
    unlink(server_address.sun_path);

    return 0;
}
