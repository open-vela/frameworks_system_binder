#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "BnTestPromiseCallback.h"
#include "ITestPromise.h"
#include "ITestPromiseCallback.h"

#include <utils/Log.h>

#include <future>

using namespace android;

class CallbackPromise : public BnTestPromiseCallback, public std::promise<int32_t> {
public:
    binder::Status result(int32_t res)
    {
        this->set_value(res);
        return binder::Status::ok();
    }
};

extern "C" int main(int argc, char** argv)
{
    ALOGD("promise client start argc: %d, argv[0]: %s", argc, argv[0]);

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());

    // obtain service
    sp<IBinder> binder = sm->getService(String16("testpromise.service"));
    if (binder == NULL) {
        ALOGE("service binder is null, abort...");
        return -1;
    }

    ProcessState::self()->startThreadPool();

    sp<ITestPromise> service = interface_cast<ITestPromise>(binder);

    sp<CallbackPromise> callback = sp<CallbackPromise>::make();
    service->add(102, 302, callback);

    std::future<int32_t> f = callback->get_future();
    ALOGI("result %d", f.get());

    return 0;
}
