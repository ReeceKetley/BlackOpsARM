# Handoff: making the x64 build genuinely 64-bit clean (drop BO1_X64_LOW_VA)

Written 2026-10-10 mid-task. Everything below is the state at the moment work stopped. Nothing from this phase is committed (HEAD is `5dd11a4`, already pushed).

## Why this work exists

The x64 build links with `BO1_X64_LOW_VA=ON`: `/LARGEADDRESSAWARE:NO /DYNAMICBASE:NO /FIXED /BASE:0x400000`. That keeps every pointer below 2 GB so the decompiled `(int)ptr` / `(unsigned)ptr` idioms still round-trip. Costs:

- A hard 2 GB address-space cap. With sound enabled the windowed game dies with `CreateTexture ... Ran out of memory` (`r_image.cpp:679`). Workaround today: always pass `+set nosnd 1`.
- It cannot work on Android ARM64 (no equivalent guarantee), so it blocks the port.

Decision DEC-0011 (ledger): remove the bridge properly, with Win32 as the correctness oracle.

## Design (implemented)

`src/universal/narrow32.h` / `narrow32.cpp`:

- `Narrow32<T>` is a 4-byte holder used where the decompile builds layouts out of 4-byte slots (GSC parse tree `sval_u`, script VM `VariableUnion`, compiled code stream).
- With `BO1_NARROW_HANDLES 1` (now the default in the header) a narrow pointer is a **region-tagged handle**: bits 31..24 = chunk index (1..255) over 16 MB chunks of the address space, bits 23..0 = offset in the chunk, 0 = null. Chunks register on first encode (`Bo1_NarrowEncodeSlow`); decode is one table load. Works at any address, no allocator changes. Fails fast (`Bo1_NarrowFail` + stack) if more than 255 distinct chunks are used.
- Raw 32-bit slots that hold pointers (code stream, `VariableValueInternal::next`, stack buffers) use `BO1_NARROW_ENCODE(p)` / `BO1_NARROW_DECODE(T, bits)`. Raw bits of a `Narrow32` member: `Bo1_NarrowBits` / `Bo1_NarrowSetBits`. All of these also compile on Win32 (plain addresses) so Win32 verifies the rewrites.
- `BO1_NARROW_HANDLES 0` gives the old raw-address behaviour (valid only with LOW_VA on).

## What was done in this phase

1. **Expression evaluator** (`universal/com_expressions_eval.cpp`, 304 sites): string pointers now live in the operand union's `.string`, not `.intVal` (mechanical, tool: `tools/port/fixwarn.py expr`).
2. **Script parse tree / compiler / evaluator** (`clientscript/cscr_compiler.cpp`, `cscr_evaluate.cpp`, `cscr_parsetree.cpp`): raw `*(sval_u *)(x.stringValue + N)` arithmetic replaced by typed `x.node[N/4]`, node allocation uses `result.node = Scr_AllocNode(...)`, `scr_block_s` allocation typed, `SVAL_PTR_AT` forms rewritten (tool: `tools/port/svalfix.py`).
3. **Code stream**: `EmitCodepos` stores an encoded handle; new `EmitUInt32` for plain integers that used to be cast to `const char *`. Builtin function pointers: `gScrCompilePub.func_table` is now `uintptr_t[]`; compile-time values carry a small registry slot (`Scr_FuncPtrToSlot` / `Scr_SlotToFuncPtr` in `cscr_compiler.cpp`).
4. **VM** (`cscr_vm.cpp`): stack buffers accessed through `Bo1_StackBuf(bits)`, codepos reads through `Bo1_LoadCodePos`, vectors through `u.vectorValue`, `VM_ArchiveStack` result through `u.stackValue`, two bogus `localFs.top` pointer-in-`.type` sequences replaced.
5. **Variables / animtree** (`cscr_variable.cpp`, `cscr_animtree.*`, `cscr_main.cpp`): vector arithmetic via `u.vectorValue`, link lists via encoded handles, `scr_anim_s(int)` sets raw bits.
6. **Unrelated fixes that came out of this**: audio reverb (`snd_radverb.cpp`) used `(unsigned int)` offsets, which turned negative pointer differences into +4 GB addresses and crashed the XAudio2 thread; fixed with `ptrdiff_t`.

Warning count (C4311/C4312/C4302, unique sites): **2507 at the start -> 854** at the last full rebuild (before the final few edits). By directory then: gfx_d3d 202, physics 166, ui 89, database 64, game 39, client_mp 31, EffectsCore 29, cgame_mp 29, live 26, qcommon 20, ragdoll 19, cgame 19. The whole `clientscript` directory is down to the script debugger (11 sites, inert unless `BO1_SCRDEBUGGER=1`).

## Verification state (important)

- `build-x64-hi` (LOW_VA OFF, handles ON) and `build` (Win32) both **compile** with the current tree.
- **Not yet re-run since the script rewrite**: the Win32 24-step headless route, plain `build-x64` (LOW_VA ON but now also handles ON), and the guard build. The script system touches Win32 too, so run the Win32 regression FIRST (see below). If Win32 regresses, fix that before anything else.
- High-address headless smoke (`tools/port/hi.ps1`): script parse and compile now **complete**; first crash is now in the engine proper (below).

## Immediate next crash (start here)

`DObjCalcAnim` writes through `obj->skel.mat`, which is a truncated pointer (`0xacd42d00`). Source: the per-client skeleton memory start is aligned with a 32-bit cast:

- `src/client_mp/cl_main_mp.cpp:1266` and `:1271`: `skelMemoryStart = (char *)((unsigned int)&...skelMemory[15] & 0xFFFFFFF0)`
- check the server twin in `src/server/sv_game.cpp` (`sv.skelMemoryStart` / `SV_DObjCreateSkelForBone` paths around lines 441 and 502)

Pattern fix: `(char *)(((uintptr_t)&buf[15]) & ~(uintptr_t)15)`.

## How to work (all scripts are in `tools/port/`)

Build dirs (all gitignored): `build` Win32, `build-x64` plain x64 (LOW_VA on), `build-x64-guard`, **`build-x64-hi`** (LOW_VA off, the oracle for this work; configure with `cmake -S . -B build-x64-hi -G "Visual Studio 17 2022" -A x64 -DBO1_PORT_X64=ON -DBO1_X64_LOW_VA=OFF`).

- `bld.ps1 -Dir build-x64-hi [-Rebuild] [-Log file]` builds and prints error count and truncation-warning count. NOTE: the `-Rebuild` splat bug was just fixed in the file; `-Rebuild` (full recompile, about 3 minutes) is how you get a fresh warning log.
- `warnsum.py <log> [N]` counts unique C4311/C4312/C4302 sites by directory and file; `listwarn.py <log> <path-substring>` prints the flagged source lines; `warnidioms.py <log> <substring>` groups them into idiom classes.
- `fixwarn.py` (rule sets `expr`, `vec`) and `svalfix.py` apply mechanical rewrites to flagged lines. They never change line counts. Review the diff after every use (one rule once turned `== 8` into `= = 8`).
- `hi.ps1` = build + headless smoke + symbolized crash stack on `build-x64-hi`. `hrun.ps1 -Name <dir> -Build <build dir> -God -Plan "<steps>" -Wait` runs a headless route; `symz.ps1 -Run <dir> -Build <build dir>` symbolizes a crash (llvm-symbolizer; cdb names are unreliable). `cdbh.ps1` runs the headless game under cdb for registers and disassembly. `winrun.ps1`, `cdbwin.ps1`, `cycle.ps1`, `gameshot.ps1` are the windowed equivalents (they launch a window on your desktop; `gameshot.ps1` captures only the game window). `vmmap.ps1 -ProcId <pid>` prints address-space usage of a running process.
- Hard-coded paths: the scripts assume the repo at `D:\Desktop\Black Ops 2 Android\BlackOps64` and copy sidecar DLLs from `build\run\x64_thief_escape`. Fresh run dirs need `localization.txt` and the DLL sidecars.
- Writing scripts: do not use bash heredocs for Python with backslashes (they get mangled); write files instead.

Gate after every batch of changes:

1. `build` (Win32) and `build-x64-hi` compile.
2. Win32 headless route green. Plan: `qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power jugg speedcola doubletap pap` (expect 24 PASS, 0 FAIL, exit 0, identical `SaveGame check` lines).
3. `build-x64-hi` headless route green with the same plan (this is the real goal).
4. Then plain x64 and guard x64 routes (regression).

## Remaining work, in order

1. Fix the skeleton memory alignment above, rerun `hi.ps1`, repeat: each crash symbolizes straight to the next unfixed `(unsigned int)ptr` site. Expect many from: client/server skeleton memory, DB loader (`database/db_load.cpp` 60, `db_memory.cpp`, `mem_fixed.cpp`, `mem_userhunk.cpp`), xanim loader (`xanim_load_obj.cpp` 36 sites; offsets into `dest[...]` buffers), `universal/assertive.cpp` (19), UI (`ui/ui_shared.cpp` 61).
2. Sweep the remaining warnings by directory (physics 166, gfx_d3d 202, ui 89, database 64, ...). `listwarn.py` per directory, fix, rebuild. Explicit `(uintptr_t)` casts do not warn; audit them (`grep -rn "(unsigned int)(uintptr_t)\|(DWORD)(uintptr_t)"`).
3. Run the Win32 + hi + low + guard gates, then a 30 minute hi soak (`hrun.ps1 ... -AutoquitMs 1800000`).
4. Windowed hi build with **sound enabled** (this is the actual goal: no more out-of-memory). Check with `vmmap.ps1` that usage can exceed 2 GB.
5. Make the 64-bit-clean build the default: flip `BO1_X64_LOW_VA` to OFF in `CMakeLists.txt`, delete the low-VA comments, drop `BO1_NARROW_HANDLES 0` if no longer needed, update `STATE.md` and `docs/port/x64_diff_debugging.md`, commit and push.
6. Only then: remove the TEMP diagnostics (`db_file_load.cpp` block-use log, `r_dpvs_static.cpp` null visdata log, `rb_resource.cpp` shader failure log), re-enable the client glass renderer (`glass_client.cpp`, skipped on x64 with a skip reader), and port the script debugger UI (11 sites in `cscr_debugger.cpp` plus `Scr_DumpScriptVariables` helpers in `cscr_variable.cpp` that still use 16-byte record strides).

## Deferred / known leftovers

- Script debugger and the dev-only variable dump (`cscr_variable.cpp` ~lines 195-400, 230-330): `VariableDebugInfo` has host pointers but the code uses `16 * n` strides and `char *infoArray`; also `qsort` with `sizeof(char)`.
- `DemonWare/bdCore/bdContainers/bdArray.h` has two pointer-to-int casts (online code, unused offline).
- `groupvoice/speex/*.c` warnings (about 80) are third-party; check whether they matter before spending time.
- `cscr_evaluate.cpp` lines ~717-791 still use `SVAL_PTR_AT(sval_u, exprlist->stringValue)` (debug expression evaluator): convert to `exprlist->node[0].node`.
- Windowed x64 is playable but only with `+set nosnd 1` until step 4.

## Pitfalls learned

- Warnings undercount: explicit `(uintptr_t)` casts and `(int)` pointer differences compile silently.
- A pointer difference stored in `unsigned int` and later added back (`(char *)p + (unsigned int)diff`) turns negative offsets into +4 GB; use `ptrdiff_t`.
- Hard-coded Win32 struct sizes/strides (`24 *`, `56`, `0x38`, `20 * n + 32`) are the other big class; replace with `sizeof` (see `docs/port/x64_diff_debugging.md` for the bug catalogue).
- Never kill other `BO1Zombies.exe` processes blindly if another run might be in progress; the scripts do (`Stop-Process`).
