/*
 * Copyright (C) 2024 Xiaomi Corporation
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
#include <netpacket/rpmsg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

extern "C" int main(int argc, char** argv)
{
    char *buffer, *response;
    int client_socket;
    int server_socket;
    struct sockaddr_rpmsg server_address, client_address;
    socklen_t client_address_len;

    if (argc != 3) {
        printf("[usage]: latency_rpmsg_server [rp_name] [oneway]\n");
        return 0;
    }

    response = (char*)malloc(BUFFER_SIZE);
    buffer = (char*)malloc(BUFFER_SIZE);

    server_socket = socket(PF_RPMSG, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    server_address.rp_family = AF_RPMSG;
    strlcpy(server_address.rp_name, argv[1], RPMSG_SOCKET_NAME_SIZE);
    server_address.rp_cpu[0] = '\0';

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to bind socket.");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 16) == -1) {
        perror("Failed to listen for connections.");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    client_address_len = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr*)&client_address,
        &client_address_len);
    if (client_socket == -1) {
        perror("Failed to accept client connection.");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        ssize_t recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (recv_size <= 0) {
            perror("Failed to receive data from client.");
            break;
        }

        if (strcmp(argv[2], "oneway")) {
            ssize_t response_size = send(client_socket, response, BUFFER_SIZE, 0);
            if (response_size < 0) {
                perror("Failed to send data to client.");
                break;
            }
        }
    }

    close(client_socket);
    close(server_socket);

    free(response);
    free(buffer);

    return 0;
}
