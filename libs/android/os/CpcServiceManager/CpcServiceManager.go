package CpcServiceManager

import (
        "android/soong/android"
        "android/soong/java"
)

func init() {
    android.RegisterModuleType("cpc_service_manager_defaults", cpcServiceManagerDefaultsFactory)
}

func cpcServiceManagerDefaultsFactory() (android.Module) {
    module := java.DefaultsFactory()
    android.AddLoadHook(module, cpcServiceManagerHook)
    return module
}

func cpcServiceManagerHook(ctx android.LoadHookContext) {
    //AConfig() function is at build/soong/android/config.go

    Version := ctx.AConfig().PlatformVersionName()

    type props struct {
        Srcs []string
    }

    p := &props{}

    if (Version == "11") {
        p.Srcs = append(p.Srcs, ":CpcServiceManagerAndroid11")
    } else if (Version == "13") {
        p.Srcs = append(p.Srcs, ":CpcServiceManagerAndroid13")
    } else {
        p.Srcs = append(p.Srcs, ":CpcServiceManagerAndroid")
    }

    ctx.AppendProperties(p)
}

