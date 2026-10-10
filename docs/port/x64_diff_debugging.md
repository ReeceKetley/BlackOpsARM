# x64 bring-up: differential debugging against the Win32 build

The stock Win32 build is the oracle. The x64 build is made correct by diffing what it does against what Win32 does,
instead of debugging one crash at a time.

## Tools

| Tool | Purpose |
|---|---|
| `BO1_STREAM_TRACE=<file>` | Both builds log the Win32-layout stream position of every FastFile asset (`S idx type w1..w6 f<filebytes>`). `src/database/db_stream.cpp` (`DB_Trace_*`). |
| `BO1_TRACE_ZONE=<zone> BO1_TRACE_ASSET=<n>` | Additionally log every stream alloc/inc (`A`/`I block pos f<filebytes>`) inside one asset. |
| `BO1_TRACE_STACK_OP=<n>` | Dump the call stack (module RVAs) at the n-th non-block-0 op of that asset. Symbolize with `llvm-symbolizer --relative-address` against the exe + PDB (the PDB must also exist as `%%%BO1Zombies.pdb%%%`, the name embedded in the exe). |
| `tools/trace_diff.py w32.txt x64.txt` | First divergence per zone. Ignores block 0 (its Win32 position is not tracked in `g_streamPosArray[0]`). |
| `Bo1_Diag(fmt, ...)` / `Bo1_DiagStack(tag)` | Append to the file named by `BO1_DIAG_LOG` (default `x64_diag.log` in the working directory), prefixed with the zone being loaded. Asserts and the fatal-exception handler log here too. |
| Guard-page heap (`-DBO1_X64_GUARDHEAP=ON`, `build-x64-guard`) | `src/platform/bo1_guardheap.cpp`: every malloc/new block ends flush against a PAGE_NOACCESS page and freed blocks are quarantined, so an overflow / use-after-free faults at the offending instruction instead of corrupting the heap (root-caused the intermittent 0xC0000374). Hunk/pool memory is *not* covered; those overflows show up as unrelated later corruption, so audit literal sizes there. |
| cdb | `cdb -g -G` + `.sympath <Release dir>`; `k`, `u @rip-24`, `r` at the fault. Fastest way to see which field/offset a crash dereferences. |
| `/d1reportSingleClassLayout<T>` | Compile one TU for x86 and x64 (`cl /Zs ...`) and compare member offsets: turns every Win32 numeric offset in a table into an `offsetof` expression (used for the entity/client/sentient field tables). |
| ASan build (`build-x64-asan`, `-DBO1_X64_LOW_VA=OFF`, `/fsanitize=address`) | Finds real heap overflows. Needs `clang_rt.asan_dynamic-x86_64.dll` on `PATH` and `ASAN_OPTIONS=windows_hook_rtl_allocators=true`. Page heap (`gflags`) needs elevation, ASan does not. |

Trace **file cursor** (`f...`) matters as much as stream position: x64 once consumed 16 extra file bytes per block-1 array.

## Bugs found with it (all fixed)

1. **Heap overflow**, `FS_AddIwdPureCheckReference`: `Z_Malloc(520)` for `iwd_pure_check_s` (528 bytes on x64). This was the old "FS_Startup heap corruption".
2. **XAnim**: `XAnimPartTrans` / `XAnimDeltaPartQuat` have a 4-byte Win32 header but `u` is at offset 8 natively. Added expanders.
3. **Hunk allocator** (`com_memory.cpp`): page math through `(unsigned int)` pointer casts, `Hunk_AllocateTempMemoryHigh` returned `unsigned int`, `Hunk_AddAsset` used an int array.
4. **Zone blocks**: allocated at the Win32 size, but expanded data is larger -> overran the block. Now 2x + 64 KB; `PMem` arena doubled.
5. **`DB_AllocStreamPos` alignment**: `alignment == 7` was assumed to be "pointer slot" and remapped to 3; real 8-byte alignments (menuDef) then drifted by 4. Win32 shadow cursor now uses the caller's literal alignment; native cursor aligns to >= 8.
6. **Offset -> pointer mapping**: replaced drift arithmetic with exact per-commit segments and a registry of every pointer *field* (alias targets can be any previously expanded pointer field, e.g. `XAsset[].header`). `DB_ExpandPtrStore` records the field's Win32 position from its address inside the Win32 image being expanded.
7. **Block 1 (runtime)** is zero-filled by `Load_Stream`, never read from the file. Expanders read it anyway (`DB_Expand_ReadFile`).
8. Decompiler artefacts: `*(_BYTE *)dvar->current.integer` (string dvar read as int, 67 sites), `((int (__cdecl *)(...))fn)(fn)` calls that truncate pointer returns (18 sites), `qsort(..., 4u, ...)` hard-coded element sizes (90 sites), `nodeCmp` reading node weights through `unsigned int` addresses.
9. **Win32 `sizeof` baked in as literals**: per-client memory budgets (`AllocatePerLocalClientMemory`, `CG_/FX_/SV_AllocateClientMemory`), `DB_SizeofXAsset_*` (asset singleton copy/clone sizes; the clipmap copy kept only 332 of 568 bytes, `box_brush` was lost). Pattern: `Bo1_MaxSize(sizeof(T) * N, win32Literal)` keeps Win32 identical and gives x64 the real size.

## Known non-fixes / notes

* `Sys_DetectVideoCard` (Direct3DCreate9 -> Release) trips an NVIDIA driver heap check on a plain heap; probe is opt-in on x64 (`BO1_VIDEODETECT=1`). The renderer is being replaced by Godot anyway.
* `tools/headless.ps1` (private desktop launch) still crashes the x64 exe at ~2 s (0xC0000374) while direct launches pass. Verify with direct launches until this is understood.
* `BO1_X64_LOW_VA` (default ON) links the exe `/LARGEADDRESSAWARE:NO /DYNAMICBASE:NO /FIXED /BASE:0x400000` so pointers fit 32 bits. It is a bridge, not a fix: ~2,970 pointer-truncation warnings remain (`docs/port/x64_fullbuild_warnings.log`). The script compiler (`sval_u`, 4-byte nodes with `+4` strides) needs a real 64-bit rework.
* `smodel` developer-console statistics (`phys_main.cpp: print_smodel`) still stores pointers in `unsigned int base[]`.

## Phase 2: script/game bring-up (2026-10-10)

Result: `+devmap zombie_pentagon` runs on x64 (plain heap and guard heap): zones load with a stream trace identical to Win32, GSC
compiles and runs, entities spawn, the server runs frames, 60 s soak exits 0 via `bo1_autoquit`, and the console log matches the
Win32 log line for line apart from thread ordering / environment.

Bug classes found (all fixed), beyond the FastFile list above:

10. **Win32 sizes in allocations from non-system pools** (not caught by the guard heap): `g_DebugHunkUser` (SourceBufferInfo 24, ArchivedCanonicalStringInfo 8, `debugAnimNames` 4*, `Scr_OpcodeList_s` 8, variable breakpoints 4*), `XAnim_s` header 24 vs 40 and `XAnimTreeSize()` 8, `fileData_s` `strlen+10` (hunk overflow dropped the AITypeScript entry), `XAnimParts` clone 104, DObj model arrays `5*n`, `tagInfo_s` 112, IK state arrays, debug brush/patch/line buffers.
11. **Raw byte-offset struct access** (`*((_DWORD *)p + N)`, `buf[112]`): `DObj` (objBuf rows were 31 uints; `DObjCreate`, all IK imports), `centity_s` flag bit-field (223 sites -> `CENT_FLAGS`, found from `updateDelayedNext`), `tagInfo_s`, `SV_LocateGameData(.., 760, .., 10720)`.
12. **Field-name tables holding Win32 numeric offsets** (`fields_1` in g_spawn_mp, g_client_fields, sentient/actor fields, `cent_fields`): a `light` entity's `classname` write landed in `pTurretInfo`. Now `offsetof`.
13. **4-byte values inside the code stream / archived stacks**: `scr_anim_s::linkPointer` is `Narrow32` (it lives inline in opcodes), VM code-position reads/writes use `Bo1_LoadCodePos`, `VM_UnarchiveStack`/`VM_TerminateStack` entries are 5 bytes.
14. **Expanded arrays referenced by interior pointers** (brush `sides` into the `brushsides` array): stream segments now record the Win32/native element stride so element starts map exactly.
15. **Script debugger UI** is Win32-layout heavy: inert on x64 (`BO1_SCRDEBUGGER=1` opts in). Breakpoints are only ever planted by it.

16. **IDA word macros are size-dependent**: `HIWORD(x)` of a value that was 4 bytes on Win32 but is a pointer now picked bits 48..63 (= 0). `LAST_IND` in `q_shared.h` treats pointer-typed operands as 4 bytes on x64 (LOW_VA), which fixed `animhasnotetrack` (zombie death scripts).
17. **Physics transient allocator** (`phys_transient_allocator`): block header written as 12 bytes with a 4-byte `next`, then read as a 16-byte header; CAS on a 64-bit pointer through a 32-bit interlocked op. Rewritten with the real struct and `_InterlockedCompareExchangePointer`.
18. **Steam stubs**: `SteamInternal_ContextInit` must return a zeroed context (not null) or `SteamFriends()` faults (test-client name lookup).
19. A `sess`-relative conversion off by 4 (`gclient_s::sess` is at 9896 on Win32, not 9892) briefly broke `self.pers`: derive offsets with the layout dump, never by hand.

Gameplay verification (headless dedicated + fighting test client, `+set bo1_testclient 1 +set bo1_testclient_fight 1 +devmap zombie_pentagon`): x64 plain and guard-heap builds run 100-240 s, zombies spawn and path, the bot kills them (death scripts run), scores update, exit 0. Win32 in the same setup behaves the same (including its own "potential infinite loop" script warnings in `zombie_move.gsc`).

Still open / follow-ups: ~2,970 pointer-truncation warnings (audit by runtime reach), `print_smodel`, the remaining `vehicle_info_t` tail offsets in `cent_fields` (>7268), `tools/headless.ps1` private-desktop crash, re-enable video detection, the debugger UI, and the Godot host.
