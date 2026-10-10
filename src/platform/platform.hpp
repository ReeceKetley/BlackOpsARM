#pragma once

#include "IPlatform.hpp"
#include "IFileSystem.hpp"
#include "IInputBackend.hpp"
#include "IAudioBackend.hpp"
#include "IRenderBackend.hpp"
#include "IPlatformServices.hpp"
#include "IVideoBackend.hpp"

// Global accessors. Null until Platform_Bootstrap() runs.
IPlatform *Platform();
IFileSystem *FileSystem();
IInputBackend *InputBackend();
IAudioBackend *AudioBackend();
IRenderBackend *RenderBackend();
IPlatformServices *PlatformServices();
IVideoBackend *VideoBackend();

// Install the Windows backends that forward to existing Sys_/FS_/IN_* code.
// Safe to call once after Com_Printf works (Sys_Init).
void Platform_BootstrapWindows();

// Used by platform backends to publish themselves.
void Platform_SetBackends(
    IPlatform *platform,
    IFileSystem *fileSystem,
    IInputBackend *input,
    IAudioBackend *audio,
    IRenderBackend *render,
    IPlatformServices *services,
    IVideoBackend *video);
void Platform_LogBootstrap();

// True after a successful bootstrap.
bool Platform_IsReady();

// Preferred call-site helpers. Use the registered backends when ready; otherwise
// fall back to the stock Win32 entry points so early boot still works.
unsigned int Platform_Milliseconds();
unsigned int Platform_MillisecondsRaw();
void Platform_Print(const char *msg);
void Platform_InputFrame();
