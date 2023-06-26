#define LOG_TAG "TestClient"

#include <binder/IServiceManager.h>

#include "ITestStuff.h"

#include <utils/Log.h>
#include <utils/String8.h>

using namespace android;
using android::binder::Status;

extern "C" int main(int argc, char** argv)
{
    ALOGI("demo client start count: %d, argv[0]: %s", argc, argv[0]);

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());

    // obtain demo.service
    sp<IBinder> binder = sm->getService(String16("aidldemo.service"));
    if (binder == NULL) {
        ALOGE("aidldemo service binder is null, abort...");
        return -1;
    }
    ALOGI("aidldemo service binder is %p", binder.get());

    // by interface_cast restore ITest
    sp<ITestStuff> service = interface_cast<ITestStuff>(binder);
    ALOGI("aidldemo service is %p", service.get());

    // use service provided by demo.service
    service->write(123);
    service->read(456);
    return 0;
}