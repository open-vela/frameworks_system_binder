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

#include <inttypes.h>
#include <string>

#include "android_util_Binder.h"
#include <android/os/IServiceManager.h>
#include <binder/ICpcServiceManager.h>
#include <binder/IServiceManager.h>
#include <jni.h>
#include <nativehelper/JNIHelp.h>
#include <nativehelper/ScopedUtfChars.h>

namespace android {

static jobject nativeCpcGetService(JNIEnv* env, jobject /* clazz */, jstring jName)
{
    ScopedUtfChars utfName(env, jName);
    std::string name(utfName.c_str());
    sp<IServiceManager> sm(defaultCpcServiceManager());

    if (sm == nullptr) {
        return nullptr;
    }

    sp<IBinder> binder = sm->getService(String16(name.c_str()));

    return javaObjectForIBinder(env, binder);
}

static jobject nativeCpcCheckService(JNIEnv* env, jobject /* clazz */, jstring jName)
{
    ScopedUtfChars utfName(env, jName);
    std::string name(utfName.c_str());
    sp<IServiceManager> sm(defaultCpcServiceManager());

    if (sm == nullptr) {
        return nullptr;
    }

    sp<IBinder> binder = sm->checkService(String16(name.c_str()));

    return javaObjectForIBinder(env, binder);
}

static void nativeCpcAddService(JNIEnv* env, jobject /* clazz */, jstring jName,
    jobject jCpcServiceBinder, bool allowIsolated, int dumpPriority)
{
    sp<IBinder> cpcServiceBinder = ibinderForJavaObject(env, jCpcServiceBinder);
    ScopedUtfChars utfName(env, jName);
    std::string name(utfName.c_str());
    sp<IServiceManager> sm(defaultCpcServiceManager());

    if (sm == nullptr) {
        return;
    }

    sm->addService(String16(name.c_str()), cpcServiceBinder, allowIsolated, dumpPriority);
}

static jboolean nativeCpcIsDeclared(JNIEnv* env, jobject /* clazz */, jstring jName)
{
    ScopedUtfChars utfName(env, jName);
    std::string name(utfName.c_str());
    sp<IServiceManager> sm(defaultCpcServiceManager());

    if (sm == nullptr) {
        return false;
    }

    return sm->isDeclared(String16(name.c_str()));
}

static jobject nativeGetCpcServiceManagerBinder(JNIEnv* env, jobject /* clazz */)
{
    sp<IBinder> binder = IInterface::asBinder(defaultCpcServiceManager());

    if (binder == nullptr) {
        return nullptr;
    }

    return javaObjectForIBinder(env, binder);
}

static JNINativeMethod sMethods[] = {
    { "nativeCpcGetService", "(Ljava/lang/String;)Landroid/os/IBinder;",
        (void*)nativeCpcGetService },

    { "nativeCpcCheckService", "(Ljava/lang/String;)Landroid/os/IBinder;",
        (void*)nativeCpcCheckService },

    { "nativeCpcAddService", "(Ljava/lang/String;Landroid/os/IBinder;ZI)V",
        (void*)nativeCpcAddService },

    { "nativeCpcIsDeclared", "(Ljava/lang/String;)Z", (void*)nativeCpcIsDeclared },

    { "nativeGetCpcServiceManagerBinder", "()Landroid/os/IBinder;",
        (void*)nativeGetCpcServiceManagerBinder },
};

} // namespace android

jint JNI_OnLoad(JavaVM* jvm, void*)
{
    JNIEnv* env = nullptr;
    if (jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6)) {
        return JNI_ERR;
    }

    if (jniRegisterNativeMethods(env, "android/os/CpcServiceManager",
            android::sMethods, NELEM(android::sMethods))
        < 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
