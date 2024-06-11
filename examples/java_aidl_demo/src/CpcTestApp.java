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

package src;

import android.content.Context;
import android.os.IBinder;
import android.os.Binder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.ServiceManagerNative;
import android.os.IServiceManager;
import android.os.IServiceCallback;
import android.os.IClientCallback;
import android.os.CpcServiceManager;

import src.IJavaTestStuff;

public class CpcTestApp {

    private static IServiceManager sCpcServiceManager;

    private static final IBinder mBinder = new IJavaTestStuff.Stub() {

        @Override
        public void read(int sample) throws RemoteException {
            System.out.println("read sample " + sample);
        }

        @Override
        public void write(int index) throws RemoteException {
            System.out.println("write index " + index);
        }
    };

    public static void main(String[] args) {
        if (args.length != 1) {
            System.out.println("[usage]: cpctest server or cpctest client\n");
            for (int i = 0; i < args.length; ++i) {
                System.out.println("arg " + i + " val: " + args[i] + "\n");
            }
            return;
        }

        if (args[0].equals("server")) {
            System.out.println("CPC Server Test\n");
            CpcServiceManager.addService("asvc_cpctest", mBinder);
            Binder.joinThreadPool();
        } else if (args[0].equals("client")) {
            System.out.println("CPC Client Test\n");
            IBinder binder = CpcServiceManager.getService("vsac_cpctest");
            if (binder == null) {
                System.out.println("Get binder failed!\n");
                return;
            }

            IJavaTestStuff testStuff= IJavaTestStuff.Stub.asInterface(binder);
            try {
                testStuff.write(123);
            } catch (RemoteException re) {
                re.printStackTrace();
            }

            try {
                testStuff.read(456);
            } catch (RemoteException re) {
                re.printStackTrace();
            }
        }
    }
}
