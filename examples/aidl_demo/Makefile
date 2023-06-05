#
# Copyright (C) 2023 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include $(APPDIR)/Make.defs

CXXEXT = .cpp

MODULE    = $(CONFIG_BINDER_EXAMPLES)
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_BINDER_EXAMPLES_STACKSIZE)

AIDLFLAGS = --lang=cpp -h. -o.
AIDLFLAGS += -I.
AIDLSRCS += ITestStuff.aidl
AIDLSRCS += INdkTest.aidl
INdkTest.aidl_AIDLFLAGS = --lang=ndk --ndk=ctype
AIDLSRCS += ITestPromise.aidl ITestPromiseCallback.aidl

CXXFLAGS += -I.

CXXSRCS  += ITestStuff.cpp
CXXSRCS  += INdkTest.cpp

MAINSRC  += NdkTestSrv.cpp
PROGNAME += NdkTestSrv
MAINSRC  += NdkTestCli.cpp
PROGNAME += NdkTestCli

CXXSRCS  += ITestPromise.cpp ITestPromiseCallback.cpp

MAINSRC  += TestClient.cpp
PROGNAME += TestClient
MAINSRC  += TestServer.cpp
PROGNAME += TestServer

MAINSRC  += PromiseClient.cpp
PROGNAME += PromiseClient
MAINSRC  += PromiseServer.cpp
PROGNAME += PromiseServer

MAINSRC  += TestClientCpc.cpp
PROGNAME += TestClientCpc
MAINSRC  += TestServerCpc.cpp
PROGNAME += TestServerCpc

MAINSRC  += TestServerDumpsys.cpp
PROGNAME += TestServerDumpsys

include $(APPDIR)/Application.mk
