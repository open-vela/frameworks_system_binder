package ICpcServiceManager

import (
        "android/soong/android"
        "android/soong/cc"
        "strings"
)

func init() {
    android.RegisterModuleType("ICpcServiceManager_defaults", ICpcServiceManagerDefaultsFactory)
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
        Enabled *bool
    }

    p := &props{}
    var enabled bool = true

    if (strings.Compare(Version, "15") == 0 || strings.Compare(Version, "Baklava") == 0) {
        p.Srcs = append(p.Srcs, ":ICpcServiceManagerAndroid")
    } else if (strings.Compare(Version, "14") == 0 || strings.Compare(Version, "UpsideDownCake") == 0) {
        p.Srcs = append(p.Srcs, ":ICpcServiceManagerAndroid14")
    } else if (strings.Compare(Version, "13") == 0 || strings.Compare(Version, "Tiramisu") == 0) {
        p.Srcs = append(p.Srcs, ":ICpcServiceManagerAndroid13")
    } else {
        enabled = false
        p.Enabled = &enabled
    }

    if enabled {
        p.Srcs = append(p.Srcs, ":SocketDescriptor")
    }

    ctx.AppendProperties(p)
}

