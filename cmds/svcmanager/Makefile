#
# Copyright (C) 2022 Xiaomi Corporation
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

PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE    = $(CONFIG_BINDER_CMD_SVCMANAGER)

CFLAGS += -Werror
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/binder/binderlib
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/android/frameworks/native/libs/binder/ndk/include_ndk

CSRCS    += ServiceManager.c
MAINSRC  += svc_main.c
PROGNAME += svcmanager

include $(APPDIR)/Application.mk
