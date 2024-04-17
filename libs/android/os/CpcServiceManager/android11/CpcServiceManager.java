/*
** Copyright (C) 2024 Xiaomi Corporation
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

package android.os;

import android.os.IBinder;
import android.os.Binder;
import android.os.RemoteException;
import android.os.IServiceManager;
import android.os.IServiceCallback;
import android.os.IClientCallback;
import android.os.ServiceManagerNative;

/**
 * This class is a implementation for IServiceManager, which could be used
 * by java application for managing servicies.
 */

public class CpcServiceManager implements IServiceManager {
    private static native IBinder nativeCpcGetService(String name);
    private static native IBinder nativeCpcCheckService(String name);
    private static native void    nativeCpcAddService(String name, IBinder service,
                                                       boolean allowIsolated, int dumpPriority);
    private static native boolean nativeCpcIsDeclared(String name);
    private static native IBinder nativeGetCpcServiceManagerBinder();

    static {
        System.loadLibrary("cpcbinder");
    }

    public CpcServiceManager() {
        if (mServiceManager == null) {
            IBinder binder = nativeGetCpcServiceManagerBinder();
            mServiceManager = ServiceManagerNative.asInterface(Binder.allowBlocking(binder));
        }
    }

    public IBinder asBinder() {
        return nativeGetCpcServiceManagerBinder();
    }

    public IBinder getService(String name) throws RemoteException {
        // Same as checkService (old versions of servicemanager had both methods).
        return nativeCpcGetService(name);
    }

    public IBinder checkService(String name) throws RemoteException {
        return nativeCpcCheckService(name);
    }

    public void addService(String name, IBinder service, boolean allowIsolated, int dumpPriority)
            throws RemoteException {
        nativeCpcAddService(name, service, allowIsolated, dumpPriority);
    }

    public String[] listServices(int dumpPriority) throws RemoteException {
        return mServiceManager.listServices(dumpPriority);
    }

    public void registerForNotifications(String name, IServiceCallback cb)
            throws RemoteException {
        mServiceManager.registerForNotifications(name, cb);
    }

    public void unregisterForNotifications(String name, IServiceCallback cb)
            throws RemoteException {
        mServiceManager.unregisterForNotifications(name, cb);
    }

    public boolean isDeclared(String name) throws RemoteException {
        return nativeCpcIsDeclared(name);
    }

    public void registerClientCallback(String name, IBinder service, IClientCallback cb)
            throws RemoteException {
        mServiceManager.registerClientCallback(name, service, cb);
    }

    public void tryUnregisterService(String name, IBinder service) throws RemoteException {
        mServiceManager.tryUnregisterService(name, service);
    }

    private IServiceManager mServiceManager;
}
