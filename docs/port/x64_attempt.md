# Windows x64 Attempt (2026-10-09)

## Goal of this attempt

Configure and build this repo as x64 after making FastFile pointer fixups pointer-width safe, while keeping the stock Win32 build playable.

## Commands

```text
cmake -S . -B build-x64 -G "Visual Studio 17 2022" -A x64 ^
  "-DDXSDK_DIR=C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)" ^
  -DBO1_PORT_X64=ON

cmake --build build-x64 --config Release --target BO1Zombies
```

Stock Win32 remains:

```text
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 ... -DBO1_PORT_X64=OFF
```

## What landed for x64

1. `DB_ConvertOffsetToPointer` / `DB_ConvertOffsetToAlias` write `uintptr_t` into pointer slots.
2. CMake option `BO1_PORT_X64` drops `/machine:x86`, `/arch:IA32`, `/DYNAMICBASE:NO`, uses DXSDK `Lib/x64`, links `nvapi64.lib`, and skips Win32-only Bink/Steam/libvpx import libs.
3. Profiler TEB reads use `GetCurrentThreadId()` on x64 (`profile.h`).
4. 32-bit `sizeof` / save layout `static_assert`s wrapped with `#if !defined(BO1_PORT_X64)`.
5. Portable SnapVector / MSG float rounding; MMX skin path stubbed (`r_model_skin_sse.cpp` + scalar force).
6. `jmp_buf` for `g_com_error` / `g_script_error`; CONTEXT via `BO1_CTX_*` (`bo1_arch.hpp`).
7. Affinity masks use `DWORD_PTR`.
8. Vendor stubs: `src/platform/x64_vendor_stubs.cpp` (Bink / Steam / libvpx) under `STEAM_API_NODLL` + `__RADINEXE__`.

## Results

| Stage | Result |
|---|---|
| x64 configure | Success |
| First x64 build | ~15,792 error lines (sizeof asserts + `__readfsdword`) |
| After TEB + sizeof wraps | **193** compile errors |
| After asm / jmp_buf / CONTEXT / skin / affinity / asserts | **0** compile errors |
| x64 link (with vendor stubs + nvapi64) | **Success** → `build-x64/Release/BO1Zombies.exe` (PE machine **8664 / x64**, ~14.5 MB) |
| Win32 Release after changes | Links; headless smoke exit 0; log line `platform: ready host=win32 ...` |

Full log: `docs/port/x64_build.log` (regenerated each attempt).

## Important limit

Pointer-width fixups and a linking x64 binary are **not** a playable x64 game. Retail FastFiles and many structs still assume 32-bit layouts. See `docs/port/fastfile_32_to_64.md`.

## FastFile expansion (started)

Stream-time expanders under `BO1_PORT_X64`: zone + content core + **Weapon/Menu** + remaining content leaves (SndDriverGlobals/PackIndex/XGlobals/DDL/Emblem/ImpactFx). See `docs/port/fastfile_32_to_64.md`.

## Headless smoke (in progress)

`tools/headless.ps1 -BuildDir build-x64\Release -RunDir build\run\x64smoke -Zombies -Commands "+loadzone zombie_pentagon"`.

Bring-up fixes landed this pass: `Win_RegisterClass` cbSize, dvar `.string` / enum tables, asset-entry freelist `.next`, localize GetName, FS path dvar overlays. Evidence: cdb shows `Loaded fastfile 'en_code_pre_gfx_mp'` / `code_pre_gfx_mp`. Current stop: heap corruption in `FS_Startup` (IWD lists still chain with 32-bit pointer slots). Win32 gate still `platform: ready`.

## Next

1. Finish x64 FS/IWD pointer-width; complete `+loadzone` smoke.
2. Godot host on Windows (still Win32 gameplay until expansion is ready).
