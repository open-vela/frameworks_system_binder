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

if(CONFIG_BINDER_LIB)

  # ############################################################################
  # binder base lib
  # ############################################################################

  nuttx_add_library(binderlib STATIC)

  target_sources(
    binderlib
    PRIVATE base/BBinder.c
            base/BpRefBase.c
            base/BnServiceManager.c
            base/BpBinder.c
            base/BpServiceManager.c
            base/IBinder.c
            base/IInterface.c
            base/IPCThreadState.c
            base/IServiceManager.c
            base/AidlServiceManager.c
            base/Parcel.c
            base/ProcessState.c
            base/ProcessGlobal.c
            base/Status.c
            base/Stability.c
            utils/HashMap.c
            utils/logger_write.c
            utils/RefBase.c
            utils/BinderString.c
            utils/Threads.c
            utils/Timers.c
            utils/Vector.c)

  target_include_directories(
    binderlib
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}
      ${NUTTX_APPS_DIR}/external/android/frameworks/native/libs/binder/ndk/include_ndk
  )

endif()
