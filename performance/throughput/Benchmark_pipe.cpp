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

#include <benchmark/benchmark.h>
#include <cstring>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

static int pipefd[2];
static int pipefds[2];

void BM_PipeReceive(benchmark::State& state)
{
    uint8_t send_buffer[state.range(0)];
    char response[state.range(0)];

    for (int i = 0; i < state.range(0); i++) {
        send_buffer[i] = i % 256;
    }

    close(pipefd[0]);

    for (auto _ : state) {
        ssize_t send_size = write(pipefd[1], send_buffer, sizeof(send_buffer));
        if (send_size < 0) {
            perror("Failed to send data through pipe");
            exit(EXIT_FAILURE);
        }

        ssize_t recv_size = read(pipefds[0], response, sizeof(response));
        if (recv_size < 0) {
            perror("Failed to recv data through pipe");
            exit(EXIT_FAILURE);
        }
    }

    close(pipefds[1]);
}

BENCHMARK(BM_PipeReceive)->RangeMultiplier(2)->Range(4, 1024);

int startServer()
{
    static char buffer[BUFFER_SIZE];

    close(pipefd[1]);

    while (1) {
        ssize_t recv_size = read(pipefd[0], buffer, sizeof(buffer));
        write(pipefds[1], buffer, recv_size);
    }

    close(pipefds[0]);
}

extern "C" int main(int argc, char** argv)
{
    if (pipe(pipefd) == -1 || pipe(pipefds) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    ::benchmark::Initialize(&argc, argv);
    pid_t pid = fork();
    if (pid == 0) {
        // Child, start benchmarks
        ::benchmark::RunSpecifiedBenchmarks();
    } else {
        startServer();
    };

    return 0;
}
