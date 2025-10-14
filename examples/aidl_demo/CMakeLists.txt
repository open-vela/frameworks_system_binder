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
if(CONFIG_BINDER_EXAMPLES)

  nuttx_library(binder_examples)

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  # aidl sources
  nuttx_add_aidl(
    TARGET
    binder_examples
    AIDL_BASE_DIR
    ${CMAKE_CURRENT_LIST_DIR}
    AIDL_INCLUDE_DIR
    ${CMAKE_CURRENT_LIST_DIR}
    AIDL_HEADER_DIR
    ${CMAKE_CURRENT_BINARY_DIR}
    AIDL_OUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}
    AIDL_FLAGS
    --lang=cpp
    AIDLS
    ITestStuff.aidl
    ITestPromise.aidl
    ITestPromiseCallback.aidl)

  nuttx_include_directories(${INCDIR})

  macro(add_binder_example_app PROG MAIN)
    nuttx_add_application(
      MODULE
      ${CONFIG_BINDER_EXAMPLES}
      NAME
      ${PROG}
      STACKSIZE
      ${CONFIG_BINDER_EXAMPLES_STACKSIZE}
      SRCS
      ${MAIN}
      COMPILE_FLAGS
      ${FLAGS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      binder_examples)
  endmacro()

  add_binder_example_app(TestClient TestClient.cpp)
  add_binder_example_app(TestServer TestServer.cpp)
  add_binder_example_app(TestServerEpoll TestServerEpoll.cpp)
  add_binder_example_app(PromiseClient PromiseClient.cpp)
  add_binder_example_app(PromiseServer PromiseServer.cpp)
  add_binder_example_app(TestServerDumpsys TestServerDumpsys.cpp)

  if(CONFIG_ANDROID_BINDER_RPC)
    add_binder_example_app(PromiseClientCpc PromiseClientCpc.cpp)
    add_binder_example_app(PromiseServerCpc PromiseServerCpc.cpp)
    add_binder_example_app(TestClientCpc TestClientCpc.cpp)
    add_binder_example_app(TestServerCpc TestServerCpc.cpp)
    if(CONFIG_LIBUV)
      add_binder_example_app(TestServerCpcLibuv TestServerCpcLibuv.cpp)
    endif()
  endif()

  if(CONFIG_LIBUV)
    add_binder_example_app(TestServerLibuv TestServerLibuv.cpp)
  endif()

endif()
