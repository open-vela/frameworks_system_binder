/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#pragma once

#include <binder/SocketDescriptor.h>
#include <binder/SocketQueueBase.h>
#include <utils/Log.h>

namespace android {

template <typename T>
struct AidlSocketQueue final : public SocketQueueBase<T> {
    /**
     * This constructor uses the external descriptor used with AIDL interfaces.
     * It will create an FSQ based on the descriptor that was obtained from
     * another FSQ instance for communication.
     *
     * @param desc Descriptor from another FSQ that contains all of the
     * information required to create a new instance of that queue.
     * @param resetPointers Boolean indicating whether the read/write pointers
     * should be reset or not.
     */
    AidlSocketQueue(const ::binder::SocketDescriptor& desc, bool isServer, int timeout = -1);
    ~AidlSocketQueue() = default;

private:
    AidlSocketQueue(const AidlSocketQueue& other) = delete;
    AidlSocketQueue& operator=(const AidlSocketQueue& other) = delete;
    AidlSocketQueue() = delete;
};

template <typename T>
AidlSocketQueue<T>::AidlSocketQueue(const ::binder::SocketDescriptor& desc,
    bool isServer, int timeout)
    : SocketQueueBase<T>(desc, isServer, timeout)
{
}

} // namespace android
