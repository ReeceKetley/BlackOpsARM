#include "platform.hpp"

#include <qcommon/common.h>
#include <win32/win_shared.h>
#include <win32/win_input.h>
#include <win32/win_main.h>

static IPlatform *s_platform = nullptr;
static IFileSystem *s_fileSystem = nullptr;
static IInputBackend *s_input = nullptr;
static IAudioBackend *s_audio = nullptr;
static IRenderBackend *s_render = nullptr;
static IPlatformServices *s_services = nullptr;
static IVideoBackend *s_video = nullptr;
static bool s_ready = false;

IPlatform *Platform() { return s_platform; }
IFileSystem *FileSystem() { return s_fileSystem; }
IInputBackend *InputBackend() { return s_input; }
IAudioBackend *AudioBackend() { return s_audio; }
IRenderBackend *RenderBackend() { return s_render; }
IPlatformServices *PlatformServices() { return s_services; }
IVideoBackend *VideoBackend() { return s_video; }

bool Platform_IsReady()
{
    return s_ready;
}

void Platform_SetBackends(
    IPlatform *platform,
    IFileSystem *fileSystem,
    IInputBackend *input,
    IAudioBackend *audio,
    IRenderBackend *render,
    IPlatformServices *services,
    IVideoBackend *video)
{
    s_platform = platform;
    s_fileSystem = fileSystem;
    s_input = input;
    s_audio = audio;
    s_render = render;
    s_services = services;
    s_video = video;
    s_ready = platform && fileSystem && input && audio && render && services && video;
}

void Platform_LogBootstrap()
{
    if (!s_ready)
    {
        Com_Printf(16, "platform: bootstrap incomplete\n");
        return;
    }

    Com_Printf(
        16,
        "platform: ready host=%s fs=%s input=%s audio=%s render=%s/%s services=%s video=%s online=%d\n",
        s_platform->Name(),
        s_fileSystem->Name(),
        s_input->Name(),
        s_audio->Name(),
        s_render->Name(),
        s_render->ApiName(),
        s_services->Name(),
        s_video->Name(),
        s_services->IsOnline() ? 1 : 0);
}

unsigned int Platform_Milliseconds()
{
    if (s_ready && s_platform)
        return s_platform->Milliseconds();
    return Sys_Milliseconds_Win32();
}

unsigned int Platform_MillisecondsRaw()
{
    if (s_ready && s_platform)
        return s_platform->MillisecondsRaw();
    return Sys_MillisecondsRaw_Win32();
}

void Platform_Print(const char *msg)
{
    if (s_ready && s_platform)
        s_platform->Print(msg);
    else if (msg)
        Sys_Print_Win32(const_cast<char *>(msg));
}

void Platform_InputFrame()
{
    if (s_ready && s_input)
        s_input->Frame();
    else
        IN_Frame_Win32();
}
