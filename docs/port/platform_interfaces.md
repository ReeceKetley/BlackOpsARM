# Platform Interfaces (stock Win32)

Introduced 2026-10-09 on top of the playable stock Win32 build.

## Goal

Register host interfaces while the original Windows backends still run. Gameplay call sites stay on `Sys_*` / `FS_*` / `IN_*` for now. Native Android adapters replace Windows implementations later, one subsystem at a time. Current rendering preserves the D3D9 renderer through DXVK/Vulkan (DEC0013).

## Interfaces

| Interface | Windows backend | Forwards to |
|---|---|---|
| `IPlatform` | `win32` | `Sys_Milliseconds`, quit flags, `Sys_Print`, headless |
| `IFileSystem` | `fs_stock` | `FS_ReadFile`, `FS_FreeFile`, `FS_WriteFile` |
| `IInputBackend` | `win_input` | `IN_Init` / `IN_Shutdown` / `IN_Frame` / `IN_Activate` |
| `IAudioBackend` | `xaudio2_stock` | `PC_StartWithNoSounds` |
| `IRenderBackend` | `gfx_d3d` / `d3d9` | identity only for now |
| `IPlatformServices` | `offline_stub` | always offline; no Steam/DemonWare |
| `IVideoBackend` | `bink_deferred` | not required for gameplay |

## Boot

`Sys_Init` calls `Platform_BootstrapWindows()` after `IN_Init()`. Console should print:

```text
platform: ready host=win32 fs=fs_stock input=win_input audio=xaudio2_stock render=gfx_d3d/d3d9 services=offline_stub video=bink_deferred online=0
```

## Source

- Headers: `src/platform/*.hpp`
- Registry: `src/platform/platform.cpp`
- Windows adapters: `src/platform/windows/win_platform.cpp`

## Call-site migration (done)

Helpers in `platform.hpp`:

- `Platform_Milliseconds()` / `Platform_MillisecondsRaw()`
- `Platform_Print()`
- `Platform_InputFrame()`

Stock callers now using them:

- `Com_Printf` → `Platform_Print`
- `Com_Init` menu path, `CL_RunOncePerClientFrame`, `DB` remote screen update → `Platform_InputFrame`

Windows backends call `*_Win32` raw implementations to avoid recursion.

## Next

1. Continue the Windows Vulkan stability and native layout checks in `vulkan_windows.md` and `64bit_clean_progress.md`.
2. Later implement native Android host/window/WSI, audio, input, files and lifecycle adapters. `IRenderBackend` currently identifies the renderer; it does not abstract its D3D9 device calls. The Windows DXVK DLL supplies that translation transparently.
3. Keep packaged shader bytecode and native material records portable. The repaired Windows D3DX compiler wrapper is still a Windows dependency; Android needs precompiled bytecode or a native compiler replacement. See `material_native.md` and `android_dxvk_notes.md`.
