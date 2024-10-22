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

############################################################################
# binder base lib
############################################################################

CFLAGS   += -Werror
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/frameworks/system/binder/binderlib
CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/external/android/frameworks/native/libs/binder/ndk/include_ndk

CSRCS += base/BBinder.c
CSRCS += base/BpRefBase.c
CSRCS += base/BnServiceManager.c
CSRCS += base/BpBinder.c
CSRCS += base/BpServiceManager.c
CSRCS += base/IBinder.c
CSRCS += base/IInterface.c
CSRCS += base/IPCThreadState.c
CSRCS += base/IServiceManager.c
CSRCS += base/AidlServiceManager.c
CSRCS += base/Parcel.c
CSRCS += base/ProcessState.c
CSRCS += base/ProcessGlobal.c
CSRCS += base/Status.c
CSRCS += base/Stability.c

CSRCS += utils/HashMap.c
CSRCS += utils/logger_write.c
CSRCS += utils/RefBase.c
CSRCS += utils/BinderString.c
CSRCS += utils/Threads.c
CSRCS += utils/Timers.c
CSRCS += utils/Vector.c

include $(APPDIR)/Application.mk
