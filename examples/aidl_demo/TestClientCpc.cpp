#define LOG_TAG "TestClient"

#include "ITestStuff.h"

#include <binder/RpcSession.h>
#include <utils/Log.h>

using namespace android;
using android::binder::Status;

extern "C" int main(int argc, char** argv)
{
    ALOGI("client RpcSession::make...");
    auto session = RpcSession::make();
    auto status = session->setupRpmsgSockClient("ap", "cpc");
    ALOGI("session->setupCpcSockClient:%" PRIi32, status);

    auto remoteBinder = session->getRootObject();
    // interface_cast restore ITest interface
    sp<ITestStuff> service = interface_cast<ITestStuff>(remoteBinder);
    if (service) {
        // callback server service interface
        service->write(123);
        service->read(456);
    } else {
        ALOGI("remoteBinder get is null error\n");
    }
    return 0;
}