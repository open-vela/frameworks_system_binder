#
# Copyright (C) 2023 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
if(CONFIG_BINDER_PERFORMANCE_THROUGHPUT)

  if(CONFIG_BINDER_PERFORMANCE_THROUGHPUT_BINDER)
    nuttx_add_application(
      MODULE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT}
      NAME
      Benchmark_binder
      STACKSIZE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT_STACKSIZE}
      SRCS
      Benchmark_binder.cpp)
  endif()

  if(CONFIG_BINDER_PERFORMANCE_THROUGHPUT_SOCKET)
    nuttx_add_application(
      MODULE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT}
      NAME
      throughput_server
      STACKSIZE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT_STACKSIZE}
      SRCS
      throughput_server.cpp)
    nuttx_add_application(
      MODULE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT}
      NAME
      Benchmark_socket
      STACKSIZE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT_STACKSIZE}
      SRCS
      Benchmark_socket.cpp)
  endif()

  if(CONFIG_BINDER_PERFORMANCE_THROUGHPUT_PIPE)
    nuttx_add_application(
      MODULE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT}
      NAME
      Benchmark_pipe
      STACKSIZE
      ${CONFIG_BINDER_PERFORMANCE_THROUGHPUT_STACKSIZE}
      SRCS
      Benchmark_pipe.cpp)
  endif()

endif()
