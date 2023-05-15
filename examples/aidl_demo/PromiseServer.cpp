#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "BnTestPromise.h"
#include "ITestPromise.h"
#include "ITestPromiseCallback.h"

#include <utils/Log.h>

using namespace android;

class MyTestServer : public BnTestPromise {
public:
    binder::Status add(int32_t a, int32_t b, const sp<ITestPromiseCallback>& callback)
    {
        usleep(500000);
        callback->result(a + b);
        return binder::Status::ok();
    }
};

extern "C" int main(int argc, char** argv)
{
    ALOGD("promise service start argc: %d, argv[0]: %s", argc, argv[0]);

    sp<MyTestServer> server = sp<MyTestServer>::make();
    defaultServiceManager()->addService(String16("testpromise.service"), server);

    IPCThreadState::self()->joinThreadPool();
    return 0;
}
