package CpcServiceManager

import (
        "android/soong/android"
        "android/soong/cc"
        "strings"
)

func init() {
    android.RegisterModuleType("CpcServiceManager_defaults", CpcServiceManagerDefaultsFactory)
}

func ICpcServiceManagerDefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, ICpcServiceManagerHook)
    return module
}

func ICpcServiceManagerHook(ctx android.LoadHookContext) {
    //AConfig() function is at build/soong/android/config.go

    Version := ctx.AConfig().PlatformVersionName()

    type props struct {
        Srcs []string
    }

    p := &props{}

    if (strings.Compare(Version, "14") < 0) {
        p.Srcs = append(p.Srcs, ":ICpcServiceManagerAndroid13")
    } else {
        p.Srcs = append(p.Srcs, ":ICpcServiceManagerAndroid")
    }

    ctx.AppendProperties(p)
}

