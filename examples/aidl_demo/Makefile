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
AIDLSRCS += ITestPromise.aidl ITestPromiseCallback.aidl
CXXFLAGS += -I.

CXXSRCS  += ITestStuff.cpp
CXXSRCS  += ITestPromise.cpp ITestPromiseCallback.cpp

MAINSRC  += TestClient.cpp
PROGNAME += TestClient
MAINSRC  += TestServer.cpp
PROGNAME += TestServer
MAINSRC  += TestServerEpoll.cpp
PROGNAME += TestServerEpoll

ifdef CONFIG_LIBUV
MAINSRC  += TestServerLibuv.cpp
PROGNAME += TestServerLibuv
endif

MAINSRC  += PromiseClient.cpp
PROGNAME += PromiseClient
MAINSRC  += PromiseServer.cpp
PROGNAME += PromiseServer

MAINSRC  += TestServerDumpsys.cpp
PROGNAME += TestServerDumpsys

ifdef CONFIG_ANDROID_BINDER_RPC
MAINSRC  += PromiseClientCpc.cpp
PROGNAME += PromiseClientCpc
MAINSRC  += PromiseServerCpc.cpp
PROGNAME += PromiseServerCpc

MAINSRC  += TestClientCpc.cpp
PROGNAME += TestClientCpc
MAINSRC  += TestServerCpc.cpp
PROGNAME += TestServerCpc
ifdef CONFIG_LIBUV
MAINSRC  += TestServerCpcLibuv.cpp
PROGNAME += TestServerCpcLibuv
endif
endif

include $(APPDIR)/Application.mk
