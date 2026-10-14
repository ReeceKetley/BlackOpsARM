#include "system_initialization.h"
#include <qcommon/common.h>
#include <platform/platform.hpp>
#include <platform/android/headless_policy.h>
#include <platform/android/quit_services.h>
#include <platform/android/console_input.h>
#include <universal/com_files.h>
#include <win32/win_shared.h>
#include <sys/utsname.h>
#include <time.h>
#include <cstdlib>

void Platform_Print(char *message);
namespace {
struct NativePlatform final : IPlatform {
    const char *Name() const override { return "android_native"; }
    unsigned Milliseconds() override { return Sys_Milliseconds(); }
    unsigned MillisecondsRaw() override { return Sys_MillisecondsRaw(); }
    void RequestQuit() override { Sys_SetQuitRequested(); }
    bool IsQuitRequested() override { return Sys_IsQuitRequested(); }
    bool IsHeadless() override { return Sys_IsHeadless(); }
    void Print(const char *message) override { if (message) Platform_Print(const_cast<char *>(message)); }
};
struct NativeFileSystem final : IFileSystem {
    const char *Name() const override { return "fs_native"; }
    int ReadFile(const char *path,void **buffer) override { return FS_ReadFile(path,buffer); }
    void FreeFile(void *buffer) override { FS_FreeFile(buffer); }
    int WriteFile(const char *path,const void *buffer,unsigned size) override {
        return FS_WriteFile(const_cast<char *>(path),const_cast<char *>(static_cast<const char *>(buffer)),size);
    }
};
struct NativeInput final : IInputBackend {
    const char *Name() const override { return "native_headless_console"; }
    void Init() override { IN_Init(); }
    void Shutdown() override { IN_Shutdown(); }
    void Activate(bool active) override { IN_Activate(active); }
    void Frame() override { IN_Frame_Native(); }
};
NativePlatform host;
NativeFileSystem files;
NativeInput input;
}
void bo1::android::InitializeSystemOS() {
    utsname system{};
    timespec resolution{};
    if (uname(&system) || clock_getres(CLOCK_MONOTONIC,&resolution)) {
        Com_Error(ERR_FATAL,"Cannot query native Android kernel or monotonic clock");
        std::abort();
    }
    Com_Printf(16,"OS is %s %s %s; monotonic resolution %lld ns\n",system.sysname,
        system.release,system.machine,static_cast<long long>(resolution.tv_sec)*1000000000LL+resolution.tv_nsec);
}
void bo1::android::BootstrapNativePlatform() {
    if (!FS_Initialized()) {
        Com_Error(ERR_FATAL,"Native platform registration requires initialized engine filesystem");
        std::abort();
    }
    // Only implemented service descriptors are published. The original registry
    // remains incomplete until actual audio/render/online/video counterparts exist.
    Platform_SetBackends(&host,&files,&input,nullptr,nullptr,nullptr,nullptr);
    Platform_LogBootstrap();
    Com_Printf(16,"platform: native OS/filesystem/headless input bound; audio/render/online/video unbound\n");
}
