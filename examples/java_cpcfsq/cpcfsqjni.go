package cpcfsqjni

import (
        "android/soong/android"
        "android/soong/cc"
)

func init() {
    android.RegisterModuleType("cpcfsqjni_defaults", cpcfsqjniDefaultsFactory)
}

func cpcfsqjniDefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, cpcfsqjniHook)
    return module
}

func cpcfsqjniHook(ctx android.LoadHookContext) {
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

