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

if(CONFIG_ANDROID_BINDER_RPC)
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_CXX_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/binder/include
             ${CMAKE_CURRENT_BINARY_DIR}/binder/include)

  nuttx_add_library(SocketDescriptor STATIC)
  target_compile_options(SocketDescriptor PRIVATE -Wno-return-type)
  nuttx_add_aidl(
    TARGET
    SocketDescriptor
    AIDL_BASE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/binder/include
    AIDL_INCLUDE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/binder/include
    AIDL_HEADER_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/binder/include
    AIDL_OUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/binder/include/
    AIDL_FLAGS
    --lang=cpp
    AIDLS
    binder/include/binder/SocketDescriptor.aidl)

  include_directories_for_all_aidl(${CMAKE_CURRENT_LIST_DIR}/binder/include
                                   ${CMAKE_CURRENT_BINARY_DIR}/binder/include)

  target_sources(android PRIVATE binder/ICpcServiceManager.cpp)
endif()
