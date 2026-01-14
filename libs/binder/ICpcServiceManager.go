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
    CpcVal := ctx.Config().VendorConfig("vela").String("servicemanager")
    ForceVer := ctx.Config().VendorConfig("vela").String("forcever")

    type props struct {
        Srcs []string
        Cppflags []string
        Enabled *bool
    }

    p := &props{}
    var enabled bool = true

    if (CpcVal == "") {
        CpcVal = "ap"
    }

    p.Cppflags = append(p.Cppflags, "-DCONFIG_CPC_SERVICEMANAGER_CPUNAME=\"" + CpcVal + "\"")

    if (ForceVer != "") {
        p.Cppflags = append(p.Cppflags, "-DCONFIG_ANDROID_BINDER_VERSION=" + ForceVer)
    } else if (strings.Compare(Version, "15") == 0 || strings.Compare(Version, "Baklava") == 0) {
        p.Cppflags = append(p.Cppflags, "-DCONFIG_ANDROID_BINDER_VERSION=15")
    } else if (strings.Compare(Version, "14") == 0 || strings.Compare(Version, "UpsideDownCake") == 0) {
        p.Cppflags = append(p.Cppflags, "-DCONFIG_ANDROID_BINDER_VERSION=14")
    } else if (strings.Compare(Version, "13") == 0 || strings.Compare(Version, "Tiramisu") == 0) {
        p.Cppflags = append(p.Cppflags, "-DCONFIG_ANDROID_BINDER_VERSION=13")
    } else {
        enabled = false
        p.Enabled = &enabled
    }

    if enabled {
        p.Srcs = append(p.Srcs, ":SocketDescriptor")
    }

    ctx.AppendProperties(p)
}

