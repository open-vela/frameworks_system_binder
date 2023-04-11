#define LOG_TAG "TestClient"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include "ITestStuff.h"
#include "BnTestStuff.h"
#include "BpTestStuff.h"

#include <utils/Log.h>
#include <utils/String8.h>
#include <binder/RpcServer.h>
#include <binder/RpcSession.h>
#include <binder/RpcCertificateFormat.h>
#include <binder/RpcTransportRaw.h>

using namespace android;
using android::binder::Status;

extern "C" int main(int count, char **argv)
{
    ALOGI("client RpcSession::make...");
    auto session = RpcSession::make();
    auto status = session->setupRpmsgSockClient("ap", "cpc");
    ALOGI("session->setupCpcSockClient:%d", status);

    auto remoteBinder = session->getRootObject();
    // interface_cast restore ITest interface
    sp<ITestStuff> service = interface_cast<ITestStuff>(remoteBinder);
    if (service) {
        ALOGI("service:%p", service);
        // callback server service interface
        service->write(123);
        service->read(456);
    } else {
        ALOGI("remoteBinder get is null error\n");
    }
    return 0;
}