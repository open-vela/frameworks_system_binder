#define LOG_TAG "TestServiceDumpsys"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "BnTestStuff.h"
#include "BpTestStuff.h"
#include "ITestStuff.h"

#include <utils/Log.h>
#include <utils/String8.h>

using namespace android;
using android::NO_ERROR;
using android::binder::Status;

namespace android {
class ITestDumpsysServer : public BnTestStuff {
public:
    virtual status_t dump(int fd, const Vector<String16>& args) override
    {
        dprintf(fd, "hello dumpsys!\n");
        return NO_ERROR;
    }
    virtual Status write(int32_t sample) override
    {
        return Status::ok();
    }

    virtual Status read(int32_t index) override
    {
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
    sp<ITestDumpsysServer> testServer = new ITestDumpsysServer;

    // add service
    sm->addService(String16("aidldemo.dumpservice"), testServer);
    ALOGI("add aidldemo.dumpservice to service manager");

    // start worker thread
    ProcessState::self()->startThreadPool();

    // join the main thread
    IPCThreadState::self()->joinThreadPool();
    return 0;
}