# Platform Interfaces (stock Win32)

Introduced 2026-10-09 on top of the playable stock Win32 build.

## Goal

Register narrow host interfaces while the original Windows backends still run. Gameplay call sites stay on `Sys_*` / `FS_*` / `IN_*` for now. Godot and Android replace the implementations later, one subsystem at a time.

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

1. Grow `IRenderBackend` / `IAudioBackend` as Godot bridges appear.
2. Continue Windows x64 work (`docs/port/x64_attempt.md`).
