# T5 Portability Audit

Source: the repo root, pin `102f6992`  
Date: 2026-10-09  
Method: file/line inventory of `src/`, `DemonWare/`, and `tl/`, plus targeted review of Windows headers, inline asm, D3D9, audio, online services, and asset pointer fixups.

## Verdict

About **70% of the tree is gameplay or near-gameplay C++ worth keeping**. About **30% is platform, online, or D3D9 shell that Godot and stubs replace**.

The hard work is not rewriting Zombies. It is:

1. Replacing `gfx_d3d` with a Godot render bridge.
2. Making FastFile pointer fixups 64-bit safe (`DB_ConvertOffsetToPointer` stores pointers in `unsigned int`).
3. Wrapping Win32 filesystem, threads, timing, and input while the original Windows backends still run.
4. Replacing a handful of x86 `__asm` float/CPUID helpers with portable C++ (a portable SSE path already exists for `SnapFloat`).

## Size

| Scope | Files | Lines |
|---|---:|---:|
| Scanned (`src` + `DemonWare` + `tl`) | 2,251 | 1,213,668 |
| Gameplay keep / light port (`CORE_KEEP` + `CORE_PORT` + `ARCH_REWRITE`) | — | ~855k (70%) |
| Platform / Godot / offline remove | — | ~359k (30%) |

Build lock-in from `CMakeLists.txt`:

- `CMAKE_GENERATOR_PLATFORM "WIN32"` and `/machine:x86`
- `/arch:IA32` on `physics/phys_gjk.cpp`
- `/DYNAMICBASE:NO` (comment: ASLR crashes DirectX)
- Links: `d3d9`, `d3dx9`, `dsound`, `xinput`, `winmm`, `ws2_32`, `steam_api`, `binkw32`, `nvapi`

## Classification legend

| Class | Meaning |
|---|---|
| `CORE_KEEP` | Gameplay behavior. Prefer mechanical cleanup only. |
| `CORE_PORT` | Mostly portable. Needs Win32 header removals, thread/FS wrappers, or small arch fixes. |
| `PLATFORM_WRAP` | Keep the responsibility. Swap the Win32 implementation. |
| `GODOT_REPLACE` | Godot owns the backend (render, input, audio playback, lifecycle). |
| `OFFLINE_REMOVE` | Stub or delete for offline Zombies. |
| `ARCH_REWRITE` | 32-bit pointer / packing assumptions that break on x64/ARM64. |
| `UNKNOWN` | Needs a focused probe before a confident class. |

## Module table

| Module | Files | Lines | Class | Windows | x86 | External | Gameplay critical? | Action | Difficulty |
|---|---:|---:|---|---|---|---|---|---|---|
| `src/game` | 112 | 76,351 | CORE_KEEP | none | decomp int casts | none | yes | keep | low |
| `src/bgame` | 54 | 48,534 | CORE_KEEP | none | light | none | yes | keep (Pmove, weapons) | low |
| `src/game_mp` | 34 | 54,548 | CORE_KEEP | 1 header | light | none | yes | keep ZM path | low |
| `src/game_sp` | 62 | 23,125 | CORE_KEEP | few headers | light | none | partial | keep needed ZM helpers | low |
| `src/cgame` | 87 | 59,103 | CORE_PORT | 2 headers | entity bit casts | none | yes | wrap headers | medium |
| `src/cgame_mp` | 44 | 47,600 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/clientscript` | 35 | 50,720 | CORE_PORT | `__debugbreak` / headers | light | none | yes | portable asserts | medium |
| `src/xanim` | 22 | 19,925 | CORE_KEEP | none | packed structs | none | yes | keep; validate on x64 | medium |
| `src/physics` | 61 | 53,535 | CORE_PORT | Interlocked headers | `/arch:IA32` on GJK | none | yes | drop IA32 flag; wrap atomics | medium |
| `src/EffectsCore` | 40 | 23,972 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/DynEntity` | 12 | 8,462 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/glass` | 13 | 11,192 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/ik` | 10 | 8,552 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/ragdoll` | 10 | 3,499 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/vehicle` | 8 | 6,534 | CORE_KEEP | none | none | none | partial | keep if needed | low |
| `src/flame` | 20 | 6,221 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/aim_assist` | 4 | 3,478 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/turret` | 2 | 568 | CORE_KEEP | none | none | none | partial | keep | low |
| `src/client` | 33 | 14,725 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/client_mp` | 27 | 22,194 | CORE_KEEP | none | light | none | yes | keep | low |
| `src/server` | 9 | 3,486 | CORE_KEEP | none | none | none | yes | keep | low |
| `src/server_mp` | 21 | 15,046 | CORE_KEEP | light | light | none | yes | keep local listen | low |
| `src/demo` | 14 | 13,068 | CORE_KEEP | none | light | none | no | defer | low |
| `src/stringed` | 8 | 1,905 | CORE_KEEP | none | none | none | yes | keep | low |
| `src/ddl` | 10 | 1,672 | CORE_KEEP | none | none | none | yes | keep | low |
| `src/common` | 2 | 1,438 | CORE_KEEP | none | none | none | yes | keep | low |
| `src/ui` | 41 | 40,104 | CORE_PORT | none | light | renderer | menus | keep engine UI first; Godot menus later | medium |
| `src/ui_mp` | 8 | 12,372 | CORE_PORT | none | light | renderer | menus | same | medium |
| `src/qcommon` | 84 | 46,296 | CORE_PORT | threads/net/cmd | `__asm` in `msg.cpp`, `common.h` | Winsock | yes | wrap platform; replace x87 | medium |
| `src/universal` | 80 | 44,379 | CORE_PORT | memory/timing | light | Win32 | yes | portable FS/mem/time | medium |
| `src/database` | 19 | 17,623 | ARCH_REWRITE | OVERLAPPED I/O | **pointer fixups in `unsigned int`** | zlib | yes | rewrite offset→pointer for 64-bit | high |
| `src/gfx_d3d` | 261 | 140,984 | GODOT_REPLACE | D3D9 device | SIMD helpers | d3d9/d3dx9/nvapi/bink | visual state | keep scene/material meaning; replace API | **critical** |
| `src/sound` | 36 | 20,018 | GODOT_REPLACE | headers | light | XAudio2 | aliases yes | keep alias/bank logic; replace driver | high |
| `src/win32` | 41 | 10,072 | PLATFORM_WRAP | entire module | x87/CPUID asm | XInput/Steam | no | Godot lifecycle/input + portable stubs | high |
| `tl/` | 17 | 7,891 | PLATFORM_WRAP | threads/jobs | none | Win32 | no | portable job queue | medium |
| `src/devgui` | 6 | 4,032 | PLATFORM_WRAP | input | light | none | no | optional debug host | low |
| `src/live` | 62 | 39,893 | OFFLINE_REMOVE | storage/presence | light | Steam | no | stub offline profile/storage | low |
| `src/steam` | 46 | 17,846 | OFFLINE_REMOVE | Steamworks | none | steam_api | no | remove | low |
| `src/DW` | 28 | 9,880 | OFFLINE_REMOVE | light | light | DemonWare | no | remove | low |
| `DemonWare/` | 32 | 3,689 | OFFLINE_REMOVE | platform mutex | none | DemonWare | no | remove | low |
| `src/nvapi` | 1 | 25,420 | OFFLINE_REMOVE | none | none | NVAPI | no | remove | low |
| `src/binklib` | 6 | 4,533 | OFFLINE_REMOVE | D3D9 textures | none | Bink | no | skip/convert later | low |
| `src/groupvoice` | 76 | 20,085 | OFFLINE_REMOVE | DirectSound | ARM speex asm | dsound | no | remove for offline | low |
| `src/speex` | 9 | 1,305 | OFFLINE_REMOVE | none | none | speex | no | remove with voice | low |
| `src/mjpeg` | 6 | 2,516 | OFFLINE_REMOVE | AVI/Win32 | light | none | no | defer | low |
| `src/monkey` | 4 | 417 | OFFLINE_REMOVE | Win32 | none | none | no | remove | low |
| `src/tracy` | 82 | 50,135 | OFFLINE_REMOVE | Tracy Win32 | asm | Tracy | no | optional debug only | low |
| `src/libs` | 454 | 65,803 | CORE_KEEP | 1 PRNG file | none | tomcrypt/tommath/vpx | crypto/video | keep vendored libs | low |
| `src/zlib` | 21 | 10,159 | CORE_KEEP | zconf | none | zlib | yes | keep | low |
| `src/jpeg` | 59 | 24,349 | CORE_KEEP | light | none | jpeg | yes | keep | low |
| `src/minilzo` | 4 | 9,924 | CORE_KEEP | none | optional asm paths | minilzo | yes | keep C path | low |
| `src/CubeMapGenLib` | 12 | 4,269 | CORE_PORT | ErrorMsg | none | none | tools | portable later | low |
| `src/vpx` | 2 | 221 | CORE_PORT | none | none | libvpx | video | keep if needed | low |

## Highest-confidence evidence

### Asset database is 32-bit by construction

`DB_ConvertOffsetToPointer` writes a host pointer into an `unsigned int`:

```82:85:src/database/db_stream_load.cpp
void __cdecl DB_ConvertOffsetToPointer(unsigned int *data)
{
    *data = (unsigned int)&g_streamBlocks[(unsigned int)(*data - 1) >> 29].data[(*data - 1) & 0x1FFFFFFF];
}
```

`db_load.cpp` calls this about 100 times while loading assets. `XModel` is tagged `sizeof=0xFC` with many 4-byte pointer fields. This is the first architecture gate after “does it still build as Win32”.

### Float rounding already has a portable escape hatch

`SnapFloat` / `SnapFloatToInt` keep x87 `fistp` under `BO1_PURE && _WIN32`, and otherwise use SSE `_mm_cvtss_si32`. `MSG_RoundFloatToInt` in `msg.cpp` is still hard-coded `__asm` and must be converted the same way.

### Renderer is the largest single replacement

`gfx_d3d` is ~141k lines across 261 files (`r_*` scene/material code + `rb_*` backend). It is the Godot bridge target. Do not rewrite gameplay to match Godot meshes; translate render/game state into Godot resources.

### Offline Zombies can drop a large online surface

`live` + `steam` + `DW` + `DemonWare` + `nvapi` + `binklib` + `groupvoice` + Tracy are removable or deferrable for the first playable path. That is roughly 175k lines that do not need a faithful port.

### Windows headers leak outside `win32/`

About 50 non-`win32` files include `<Windows.h>` or `<windows.h>`, including `database`, `sound`, `physics`, `clientscript`, `qcommon/threads`, and `universal` memory/timing. Most are Interlocked, OVERLAPPED, or `__debugbreak` — wrap those first.

## How much is “mechanical portable C++”?

Rough answer to the design-doc question:

| Bucket | Share | Reality |
|---|---:|---|
| Keep with little or no rewrite | ~43% | `game`, `bgame`, `xanim`, effects, AI-adjacent, most scripted gameplay |
| Keep after wrapping platform calls | ~26% | `qcommon`, `universal`, `cgame`, `clientscript`, physics, UI |
| Must replace with Godot backends | ~13% | D3D9 renderer + audio driver |
| Stub/remove for offline | ~15% | Steam, DemonWare, live services, NVAPI, Bink, voice, Tracy |
| Architecture rewrite now | ~1.5% by size, outsized risk | FastFile pointer fixups and packed asset structs |

So most of the engine can compile as normal C++ after mechanical changes. The subsystems that genuinely need replacement are the D3D9 backend, the Win32 shell, the audio driver, and the 32-bit asset pointer model.

## Recommended order

1. Get the original Win32 x86 build running with owned game data.
2. Introduce narrow interfaces (`IPlatform`, `IFileSystem`, `IAudioBackend`, `IRenderBackend`, `IInputBackend`, `IPlatformServices`) while old backends still work.
3. Replace x87/`cpuid` helpers and make a Windows x64 compile attempt focused on `database` pointer fixups.
4. Put Godot in charge of window/lifecycle/input.
5. Bridge sound aliases to Godot AudioServer.
6. Bridge the first world/model draw through Godot.
7. Prove a BO1 Zombies round loop on Windows.
8. Only then target Android ARM64.

## Open probes (do not block the audit)

- Exact count of packed structs whose `sizeof=` comments assume 32-bit pointers.
- Whether zombie solo can boot with `live`/`DW` fully stubbed on the stock Win32 build.
- How much of `ui`/`ui_mp` is required before a Godot debug HUD is enough.
- SIMD usage in `gfx_d3d` that must become portable before ARM64, separate from the Godot bridge itself.
