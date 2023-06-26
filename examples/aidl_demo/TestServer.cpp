#define LOG_TAG "TestService"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "BnTestStuff.h"

#include <utils/Log.h>
#include <utils/String8.h>

using namespace android;
using android::binder::Status;

namespace android {
class ITestServer : public BnTestStuff {
public:
    // ITestServer::read()
    Status read(int32_t sample)
    {
        ALOGD("ITest::read() called %" PRIi32 ", start do hard work", sample);
        return Status::ok();
    }

    // ITestServer::write()
    Status write(int32_t index)
    {
        ALOGD("ITest::write() called %" PRIi32 ", start do hard work", index);
        return Status::ok();
    }
};
} // namespace android

extern "C" int main(int argc, char** argv)
{
    ALOGI("sample service start count: %d, argv[0]: %s", argc, argv[0]);

    // create ProcessState
    sp<ProcessState> proc(ProcessState::self());

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());
    sp<ITestServer> testServer = new ITestServer;

    // add service
    sm->addService(String16("aidldemo.service"), testServer);
    ALOGI("add aidldemo.service to service manager");

    // start worker thread
    ProcessState::self()->startThreadPool();

    // join the main thread
    IPCThreadState::self()->joinThreadPool();
    return 0;
}