#define LOG_TAG "TestService"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include "BnTestStuff.h"
#include "BpTestStuff.h"
#include "ITestStuff.h"

#include <binder/RpcCertificateFormat.h>
#include <binder/RpcServer.h>
#include <binder/RpcSession.h>
#include <binder/RpcTransportRaw.h>
#include <utils/Log.h>
#include <utils/String8.h>

using namespace android;
using android::binder::Status;

namespace android {
class ITestServer : public BnTestStuff {
public:
    Status read(int32_t sample)
    {
        ALOGI("ITest::read() called %" PRIi32 ", server start do hard work", sample);
        return Status::ok();
    }
    Status write(int32_t index)
    {
        ALOGI("ITest::write() called %" PRIi32 ", server start do hard work", index);
        return Status::ok();
    }
};
}

extern "C" int main(int argc, char** argv)
{
    ALOGI("sample service start count: %d, argv[0]: %s", argc, argv[0]);
    sp<ITestServer> testServer = new ITestServer;
    sp<RpcServer> server = RpcServer::make();

    server->setRootObject(testServer);
    auto status = server->setupRpmsgSockServer("cpc");
    ALOGI("setupCpcSockServer %" PRIi32 "\n", status);

    server->join();

    return 0;
}