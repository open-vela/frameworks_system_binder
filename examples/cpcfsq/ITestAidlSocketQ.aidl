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

import binder.SocketDescriptor;

/**
 * Test interface for SocketDescriptor.
 */
interface ITestAidlSocketQ {
    /**
     * This method requests the service to create a synchronous read/write
     * wait-free SocketQ using the input descriptor with the client as reader.
     */
    oneway void createSocketQ(in SocketDescriptor desc);

    /**
     * This method requests the service to destroy a synchronous read/write
     * wait-free SocketQ using the input descriptor with the client as reader.
     */
    void destroySocketQ();

    /**
     * This method request the service to read from the async read/write
     * SocketQ.
     *
     * @param count Number to messages to read.
     *
     * @return True if the read operation was successful.
     */
    oneway void requestReadSocketQAsync(int count);

    /**
     * This method request the service to write into the async read/write
     * flavor of the SocketQ.
     *
     * @param count Number to messages to write.
     *
     * @return True if the write operation was successful.
     */
    oneway void requestWriteSocketQAsync(int count);

}
