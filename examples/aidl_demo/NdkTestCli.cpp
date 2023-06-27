#include <stdio.h>

#include <aidl/INdkTest.h>
#include <android/binder_manager.h>

extern "C" int main(int argc, char** argv)
{
    printf("demo client start argc: %d, argv[0]: %s\n", argc, argv[0]);

    ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_checkService("ndktest.service"));
    if (binder.get() == nullptr) {
        printf("binder is null\n");
        return 1;
    }

    std::shared_ptr<aidl::INdkTest> proxy = aidl::INdkTest::fromBinder(binder);
    if (proxy.get() == nullptr) {
        printf("proxy is null\n");
        return 1;
    }

    // // Test C type string usage
    char* out;
    ndk::ScopedAStatus status = proxy->repeatString("def", "abc", &out);
    if (AStatus_getStatus(status.get()) != STATUS_OK) {
        printf("repeartString error\n");
        return 1;
    } else {
        printf("out is %s\n", out);
    }
    free(out);

    // Test STL string usage
    std::string str_out;
    ndk::ScopedAStatus status1 = proxy->repeatString("def1", "abc1", &str_out);
    if (AStatus_getStatus(status1.get()) != STATUS_OK) {
        printf("string repeartString error\n");
        return 1;
    } else {
        printf("string out is %s\n", str_out.c_str());
    }

    // Test nullable C type string usage
    char* out_1;
    ndk::ScopedAStatus status2 = proxy->repeatNullableString(nullptr, "abc", &out_1);
    if (AStatus_getStatus(status2.get()) != STATUS_OK) {
        printf("repeartString_1 error\n");
        return 1;
    } else {
        printf("out is %s\n", out_1);
    }
    free(out_1);

    // Test nullable STL string usage
    std::optional<std::string> str_out_1;
    std::optional<std::string> str_in_1;
    std::optional<std::string> str_in_2("abc1");
    ndk::ScopedAStatus status3 = proxy->repeatNullableString(str_in_1, str_in_2, &str_out_1);
    if (AStatus_getStatus(status3.get()) != STATUS_OK) {
        printf("string repeartString_1 error\n");
        return 1;
    } else {
        printf("string out is %s\n", str_out_1->c_str());
    }

    return 0;
}
