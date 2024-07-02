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

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.IServiceManager;
import android.os.IServiceCallback;
import android.os.ServiceDebugInfo;
import android.os.ServiceManagerNative;
import android.util.Log;

public class CpcServiceManager {
    private static final String TAG = "CpcServiceManager";

    private static IServiceManager sServiceManager;

    private static IServiceManager getIServiceManager() {
        if (sServiceManager != null) {
            return sServiceManager;
        }

        // Find the cpc service manager
        sServiceManager = ServiceManagerNative
                .asInterface(Binder.allowBlocking(nativeGetCpcServiceManagerBinder()));
        return sServiceManager;
    }

    /**
     * Returns a reference to a service with the given name.
     *
     * @param name the name of the service to get
     * @return a reference to the service, or <code>null</code> if the service doesn't exist
     * @hide
     */
    @SystemApi
    public static IBinder getService(String name) {
        return nativeCpcGetService(name);
    }

    /**
     * Returns a reference to a service with the given name, or throws
     * {@link ServiceNotFoundException} if none is found.
     *
     * @param name the name of the service to get
     * @hide
     */
    @SystemApi
    public static IBinder getServiceOrThrow(String name) throws ServiceNotFoundException {
        final IBinder binder = getService(name);
        if (binder != null) {
            return binder;
        } else {
            throw new ServiceNotFoundException(name);
        }
    }

    /**
     * Place a new @a service called @a name into the service
     * manager.
     *
     * @param name the name of the new service
     * @param service the service object
     * @hide
     */
    @SystemApi
    public static void addService(String name, IBinder service) {
        addService(name, service, false, IServiceManager.DUMP_FLAG_PRIORITY_DEFAULT);
    }

    /**
     * Place a new @a service called @a name into the service
     * manager.
     *
     * @param name the name of the new service
     * @param service the service object
     * @param allowIsolated set to true to allow isolated sandboxed processes
     * to access this service
     * @hide
     */
    @SystemApi
    public static void addService(String name, IBinder service, boolean allowIsolated) {
        addService(name, service, allowIsolated, IServiceManager.DUMP_FLAG_PRIORITY_DEFAULT);
    }

    /**
     * Place a new @a service called @a name into the service
     * manager.
     *
     * @param name the name of the new service
     * @param service the service object
     * @param allowIsolated set to true to allow isolated sandboxed processes
     * @param dumpPriority supported dump priority levels as a bitmask
     * to access this service
     * @hide
     */
    @SystemApi
    public static void addService(String name, IBinder service, boolean allowIsolated,
            int dumpPriority) {
        nativeCpcAddService(name, service, allowIsolated, dumpPriority);
    }

    /**
     * Retrieve an existing service called @a name from the
     * service manager.  Non-blocking.
     *
     * @param name the name of the new service
     * @hide
     */
    @SystemApi
    public static IBinder checkService(String name) {
        return nativeCpcCheckService(name);
    }

    /**
     * Returns whether the specified service is declared.
     *
     * @param name the name of the new service
     * @return true if the service is declared somewhere (eg. VINTF manifest) and
     * waitForService should always be able to return the service.
     * @hide
     */
    @SystemApi
    public static boolean isDeclared(@NonNull String name)
            throws MethodNotImplementedException {
        throw new MethodNotImplementedException("isDeclared");
    }

    /**
     * Returns an array of all declared instances for a particular interface.
     *
     * For instance, if 'android.foo.IFoo/foo' is declared (e.g. in VINTF
     * manifest), and 'android.foo.IFoo' is passed here, then ["foo"] would be
     * returned.
     *
     * @hide
     */
    @SystemApi
    public static String[] getDeclaredInstances(@NonNull String iface)
            throws MethodNotImplementedException {
        throw new MethodNotImplementedException("getDeclaredInstances");
    }


    /**
     * Register callback for service registration notifications.
     *
     * @param name the name of the new service
     * @param callback for service registration notifications.
     * @throws RemoteException for underlying error.
     * @hide
     */
    public static void registerForNotifications(
            @NonNull String name, @NonNull IServiceCallback callback) throws RemoteException {
        getIServiceManager().registerForNotifications(name, callback);
    }

    /**
     * Return a list of all currently running services.
     *
     * @return an array of all currently running services, or <code>null</code> in
     * case of an exception
     * @hide
     */
    @SystemApi
    public static String[] listServices() {
        try {
            return getIServiceManager().listServices(IServiceManager.DUMP_FLAG_PRIORITY_ALL);
        } catch (RemoteException e) {
            Log.e(TAG, "error in listServices", e);
            return null;
        }
    }

    /**
     * Get service debug info.
     * @return an array of information for each service (like listServices, but with PIDs)
     * @hide
     */
    public static ServiceDebugInfo[] getServiceDebugInfo() {
        try {
            return getIServiceManager().getServiceDebugInfo();
        } catch (RemoteException e) {
            Log.e(TAG, "error in getServiceDebugInfo", e);
            return null;
        }
    }

    /**
     * @hide
     */
    @SystemApi
    public static class MethodNotImplementedException extends Exception {
        public MethodNotImplementedException(String name) {
            super("method (" + name + ") not implemented");
        }
    }

    /**
     * Exception thrown when no service published for given name. This might be
     * thrown early during boot before certain services have published
     * themselves.
     *
     * @hide
     */
    @SystemApi
    public static class ServiceNotFoundException extends Exception {
        public ServiceNotFoundException(String name) {
            super("No service published for: " + name);
        }
    }

    private static native IBinder nativeGetCpcServiceManagerBinder();
    private static native IBinder nativeCpcGetService(String name);
    private static native IBinder nativeCpcCheckService(String name);
    private static native void    nativeCpcAddService(String name, IBinder service,
                                                      boolean allowIsolated, int dumpPriority);
    private static native boolean nativeCpcIsDeclared(String name);

    static {
        System.loadLibrary("cpcbinder_jni.xiaomi");
    }
}
