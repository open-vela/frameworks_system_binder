package CpcServiceManagerNative

import (
        "android/soong/android"
        "android/soong/cc"
        "strings"
)

func init() {
    android.RegisterModuleType("CpcServiceManagerNative_defaults",
                                CpcServiceManagerNativeDefaultsFactory)
}

func CpcServiceManagerNativeDefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, CpcServiceManagerNativeHook)
    return module
}

func CpcServiceManagerNativeHook(ctx android.LoadHookContext) {
    //AConfig() function is at build/soong/android/config.go

    Version := ctx.AConfig().PlatformVersionName()

    type props struct {
        Srcs []string
        Enabled *bool
    }

    p := &props{}

    var enabled bool = true

    if (strings.Compare(Version, "12") != 0) {
        if (strings.Compare(Version, "14") < 0) {
            p.Srcs = append(p.Srcs, ":CpcServiceManagerNativeAndroid13")
        } else {
            p.Srcs = append(p.Srcs, ":CpcServiceManagerNativeAndroid")
        }
    } else {
        enabled = false
        p.Enabled = &enabled
    }

    ctx.AppendProperties(p)
}

