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
#include <benchmark/benchmark.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/dev/local_socket"

static int client_socket;

void BM_Socket(benchmark::State& state)
{
    uint8_t buffer[state.range(0)];
    uint8_t response[state.range(0)];

    for (int i = 0; i < state.range(0); i++) {
        buffer[i] = i % 256;
    }

    for (auto _ : state) {
        ssize_t send_size = send(client_socket, buffer, sizeof(buffer), 0);
        if (send_size < 0) {
            perror(" Failed to send data from server.");
            break;
        }

        memset(response, 0, sizeof(response));

        ssize_t recv_size = recv(client_socket, response, sizeof(response), 0);
        if (recv_size < 0) {
            perror(" Failed to receive data to server.");
            break;
        }
    }
}

BENCHMARK(BM_Socket)->RangeMultiplier(2)->Range(4, 1024);

extern "C" int main(int argc, char** argv)
{
    client_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror(" Failed to create socket from client.");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un serverAddr;
    serverAddr.sun_family = AF_LOCAL;
    strncpy(serverAddr.sun_path, SOCKET_PATH, sizeof(serverAddr.sun_path) - 1);
    unlink(serverAddr.sun_path);

    if (connect(client_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror(" Failed to connect socket from client.");
        exit(EXIT_FAILURE);
    }

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();

    close(client_socket);

    return 0;
}
