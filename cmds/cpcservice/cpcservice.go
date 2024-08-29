package cpcservice

import (
        "android/soong/android"
        "android/soong/cc"
)

func init() {
    android.RegisterModuleType("cpcservice_defaults", cpcserviceDefaultsFactory)
}

func cpcserviceDefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, cpcserviceHook)
    return module
}

func cpcserviceHook(ctx android.LoadHookContext) {
    //AConfig() function is at build/soong/android/config.go

    Version := ctx.AConfig().PlatformVersionName()

    type props struct {
        Enabled *bool
    }

    p := &props{}

    var enabled bool = true

    if (Version == "12") {
        enabled = false
    }

    p.Enabled = &enabled

    ctx.AppendProperties(p)
}
