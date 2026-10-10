#include "win_platform.hpp"

#include <platform/platform.hpp>

#include <win32/win_shared.h>
#include <win32/win_input.h>
#include <win32/win_main.h>
#include <universal/com_files.h>

namespace
{

struct WinPlatform final : IPlatform
{
    const char *Name() const override { return "win32"; }

    unsigned int Milliseconds() override { return Sys_Milliseconds_Win32(); }

    unsigned int MillisecondsRaw() override { return Sys_MillisecondsRaw_Win32(); }

    void RequestQuit() override { Sys_SetQuitRequested(); }

    bool IsQuitRequested() override { return Sys_IsQuitRequested(); }

    bool IsHeadless() override { return Sys_IsHeadless(); }

    void Print(const char *msg) override
    {
        if (msg)
            Sys_Print_Win32(const_cast<char *>(msg));
    }
};

struct WinFileSystem final : IFileSystem
{
    const char *Name() const override { return "fs_stock"; }

    int ReadFile(const char *qpath, void **outBuffer) override
    {
        return FS_ReadFile(qpath, outBuffer);
    }

    void FreeFile(void *buffer) override
    {
        FS_FreeFile(buffer);
    }

    int WriteFile(const char *filename, const void *buffer, unsigned int size) override
    {
        // FS_WriteFile takes non-const char* in this decomp.
        return FS_WriteFile(const_cast<char *>(filename), const_cast<char *>(static_cast<const char *>(buffer)), size);
    }
};

struct WinInputBackend final : IInputBackend
{
    const char *Name() const override { return "win_input"; }

    void Init() override { IN_Init(); }

    void Shutdown() override { IN_Shutdown(); }

    void Frame() override { IN_Frame_Win32(); }

    void Activate(bool active) override { IN_Activate(active ? 1 : 0); }
};

struct WinAudioBackend final : IAudioBackend
{
    const char *Name() const override { return "xaudio2_stock"; }

    bool ShouldInit() override
    {
        // Existing engine gate: nosnd / headless mute paths.
        return !PC_StartWithNoSounds();
    }
};

struct WinRenderBackend final : IRenderBackend
{
    const char *Name() const override { return "gfx_d3d"; }

    const char *ApiName() const override { return "d3d9"; }
};

struct WinPlatformServices final : IPlatformServices
{
    const char *Name() const override { return "offline_stub"; }

    bool IsOnline() const override { return false; }

    bool HasSteam() const override { return false; }

    bool HasDemonWare() const override { return false; }
};

struct WinVideoBackend final : IVideoBackend
{
    const char *Name() const override { return "bink_deferred"; }

    bool RequiredForGameplay() const override { return false; }
};

WinPlatform g_winPlatform;
WinFileSystem g_winFileSystem;
WinInputBackend g_winInput;
WinAudioBackend g_winAudio;
WinRenderBackend g_winRender;
WinPlatformServices g_winServices;
WinVideoBackend g_winVideo;

} // namespace

void Platform_BootstrapWindows()
{
    if (Platform_IsReady())
        return;

    Platform_SetBackends(
        &g_winPlatform,
        &g_winFileSystem,
        &g_winInput,
        &g_winAudio,
        &g_winRender,
        &g_winServices,
        &g_winVideo);
    Platform_LogBootstrap();
}
