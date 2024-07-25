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

#include <arpa/inet.h>
#include <binder/SocketDescriptor.h>
#include <fcntl.h>
#include <new>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>
#ifdef AF_RPMSG
#include <netpacket/rpmsg.h>
#endif
#include <pthread.h>

namespace android {

template <typename T>
struct SocketQueueBase {
    /**
     * @param SocketDescriptor socket describing the socket info.
     */
    SocketQueueBase(const ::binder::SocketDescriptor& desc, bool isServer, int timeout);

    ~SocketQueueBase();

    /**
     * @return Whether the FSQ is configured correctly.
     */
    bool isValid() const;

    /**
     * Blocking write to FSQ.
     *
     * @param data Pointer to the object of type T to be written into the FSQ.
     *
     * @return Whether the write was successful.
     */
    bool write(const T* data);

    /**
     * Blocking read from FSQ.
     *
     * @param data Pointer to the memory where the object read from the FSQ is
     * copied to.
     *
     * @return Whether the read was successful.
     */
    bool read(T* data);

    /**
     * Write some data into the FSQ.
     *
     * @param data Pointer to the array of items of type T.
     * @param count Number of items in array.
     *
     * @return Whether the write was successful.
     */
    bool write(const T* data, size_t count);

    /**
     * Read some data from the FSQ.
     *
     * @param data Pointer to the array to which read data is to be written.
     * @param count Number of items to be read.
     *
     * @return Whether the read was successful.
     */
    bool read(T* data, size_t count);

    /**
     * Setup FSQ
     *
     * @param sockaddr point to addr related structure.
     * @param socklen is the length of sockaddr.
     * @param isServer whether the socket is server or not.
     *
     */
    void setupSocket(struct sockaddr* sockaddr,
        socklen_t socklen, bool isServer);

    /**
     * Destroy FSQ
     *
     * no parameters.
     *
     */
    void destroySocket();

private:
    SocketQueueBase(const SocketQueueBase& other) = delete;
    SocketQueueBase& operator=(const SocketQueueBase& other) = delete;
    void createNetSocket(const char* addr, uint16_t port, bool isServer);
    void createLocalSocket(const char* sun_path, bool isServer);
    void createRpmsgSocket(const char* rp_cpu, const char* rp_name, bool isServer);

    int mSock;
    pthread_mutex_t mMutex;
    pthread_cond_t mCondition;
    bool mBusy;
};

template <typename T>
void SocketQueueBase<T>::setupSocket(struct sockaddr* sockaddr,
    socklen_t socklen, bool isServer)
{
    if (isServer) {
        int ret = bind(mSock, sockaddr, socklen);
        if (ret < 0) {
            ALOGE("SocketQueueBase: Failed to bind socket! err = %d\n", errno);
            close(mSock);
            mSock = -1;
            return;
        }

        ret = listen(mSock, 1);
        if (ret < 0) {
            ALOGE("SocketQueueBase: Failed to listen socket! err = %d\n", errno);
            close(mSock);
            mSock = -1;
            return;
        }

        int sock = accept(mSock, NULL, NULL);
        close(mSock);
        if (sock < 0) {
            ALOGE("SocketQueueBase: Failed to accept socket! err = %d\n", errno);
            mSock = -1;
            return;
        }
        mSock = sock;
    } else {
        int ret = connect(mSock, sockaddr, socklen);
        while (ret < 0) {
            ret = connect(mSock, sockaddr, socklen);
        }
    }
}

template <typename T>
void SocketQueueBase<T>::createNetSocket(const char* addr, uint16_t port, bool isServer)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        ALOGE("SocketQueueBase: Failed to create net socket! err = %d\n", errno);
        return;
    }

    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    if (!isServer) {
        sockaddr.sin_addr.s_addr = inet_addr(addr);
        mSock = sock;
    } else {
        sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        mSock = sock;
        int optval = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

    setupSocket((struct sockaddr*)&sockaddr,
        sizeof(struct sockaddr_in), isServer);
}

template <typename T>
void SocketQueueBase<T>::createLocalSocket(const char* sun_path, bool isServer)
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        ALOGE("SocketQueueBase: Failed to create local socket!, err = %d\n", errno);
        return;
    }

    struct sockaddr_un sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sun_family = AF_UNIX;
    sprintf(sockaddr.sun_path, "%s", sun_path);

    if (isServer) {
        unlink(sun_path);
        mSock = sock;
    } else {
        mSock = sock;
    }
    setupSocket((struct sockaddr*)&sockaddr,
        sizeof(struct sockaddr_un), isServer);
}

template <typename T>
void SocketQueueBase<T>::createRpmsgSocket(const char* rp_cpu,
    const char* rp_name, bool isServer)
{
    int sock = socket(PF_RPMSG, SOCK_STREAM, 0);
    if (sock == -1) {
        ALOGE("SocketQueueBase: Failed to create rpmsg socket! err = %d\n", errno);
        return;
    }

    struct sockaddr_rpmsg sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.rp_family = AF_RPMSG;
    strlcpy(sockaddr.rp_name, rp_name, RPMSG_SOCKET_NAME_SIZE);
    mSock = sock;
    if (isServer) {
        strlcpy(sockaddr.rp_cpu, "", RPMSG_SOCKET_CPU_SIZE);
    } else {
        strlcpy(sockaddr.rp_cpu, rp_cpu, RPMSG_SOCKET_CPU_SIZE);
    }

    setupSocket((struct sockaddr*)&sockaddr,
        sizeof(struct sockaddr_rpmsg), isServer);
}

template <typename T>
SocketQueueBase<T>::SocketQueueBase(const ::binder::SocketDescriptor& desc,
    bool isServer, int timeout)
{
    switch (desc.sock_addr.getTag()) {
    case ::binder::SocketDescriptor::SockAddr::Tag::local_sock_addr: {
        const ::binder::SocketDescriptor::LocalSockAddr& addr = desc.sock_addr.get<::binder::SocketDescriptor::SockAddr::Tag::local_sock_addr>();
        ALOGV("SocketQueueBase: Create local socket!\n");
        createLocalSocket(String8(addr.sun_path).c_str(), isServer);
        break;
    }
    case ::binder::SocketDescriptor::SockAddr::Tag::rpmsg_sock_addr: {
        const ::binder::SocketDescriptor::RpmsgSockAddr& addr = desc.sock_addr.get<::binder::SocketDescriptor::SockAddr::Tag::rpmsg_sock_addr>();
        ALOGV("SocketQueueBase: Create rpmsg socket!\n");
        createRpmsgSocket(String8(addr.rp_cpu).c_str(),
            String8(addr.rp_name).c_str(), isServer);
        break;
    }
    case ::binder::SocketDescriptor::SockAddr::Tag::net_sock_addr: {
        ALOGV("SocketQueueBase: Create net socket!\n");
        const ::binder::SocketDescriptor::NetSockAddr& addr = desc.sock_addr.get<::binder::SocketDescriptor::SockAddr::Tag::net_sock_addr>();
        createNetSocket(String8(addr.net_addr).c_str(), addr.net_port, isServer);
        break;
    }
    default:
        ALOGV("SocketQueueBase: Unknown socket type!\n");
        break;
    }

    if (mSock != -1 && timeout >= 0) {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout % 1000 * 1000;
        setsockopt(mSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(mSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCondition, NULL);
    mBusy = false;
}

template <typename T>
void SocketQueueBase<T>::destroySocket()
{
    if (mSock != -1) {
        close(mSock);
    }
}

template <typename T>
SocketQueueBase<T>::~SocketQueueBase()
{
    pthread_mutex_lock(&mMutex);

    while (mBusy != false) {
        pthread_cond_wait(&mCondition, &mMutex);
    }

    pthread_mutex_unlock(&mMutex);

    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCondition);
    destroySocket();
}

template <typename T>
bool SocketQueueBase<T>::write(const T* data)
{
    return write(data, 1);
}

template <typename T>
bool SocketQueueBase<T>::read(T* data)
{
    return read(data, 1);
}

template <typename T>
bool SocketQueueBase<T>::write(const T* data, size_t nMessages)
{
    auto remain = nMessages * sizeof(T);
    ssize_t offset = 0;
    bool result = true;

    pthread_mutex_lock(&mMutex);
    mBusy = true;

    errno = 0;

    while (remain != 0) {
        auto ret = ::write(mSock, (const uint8_t*)data + offset, remain);
        if (errno == EWOULDBLOCK) {
            ALOGE("SocketQueueBase: Write Timeout, exit!\n");
            result = false;
            break;
        } else if (ret <= 0) {
            ALOGE("SocketQueueBase: Failed to write to remote err: %d!\n", errno);
            result = false;
            break;
        }
        offset += ret;
        remain -= ret;
    }

    mBusy = false;
    pthread_cond_broadcast(&mCondition);
    pthread_mutex_unlock(&mMutex);

    return result;
}

template <typename T>
bool SocketQueueBase<T>::read(T* data, size_t nMessages)
{
    auto remain = nMessages * sizeof(T);
    ssize_t offset = 0;
    bool result = true;

    pthread_mutex_lock(&mMutex);
    mBusy = true;

    errno = 0;

    while (remain != 0) {
        auto ret = ::read(mSock, (uint8_t*)data + offset, remain);
        if (errno == EWOULDBLOCK) {
            ALOGE("SocketQueueBase: Read Timeout, exit!\n");
            result = false;
            break;
        } else if (ret <= 0) {
            ALOGE("SocketQueueBase: Failed to read from remote err: %d!\n", errno);
            result = false;
            break;
        }
        offset += ret;
        remain -= ret;
    }

    mBusy = false;
    pthread_cond_broadcast(&mCondition);
    pthread_mutex_unlock(&mMutex);

    return result;
}

template <typename T>
bool SocketQueueBase<T>::isValid() const
{
    return mSock != -1;
}

} // namespace hardware
