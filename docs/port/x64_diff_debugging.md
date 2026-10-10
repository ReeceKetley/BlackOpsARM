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

## Round-loop verification (2026-10-10)

The level-start save exposed a Win32-only `tagInfo_s` field table in `g_sp_save_entity.inl`: the table read the x64 `next` pointer at byte 4 rather than byte 8, producing `WriteField1: reference type 2 out of range` at level time 500. The later leaked-string assert was shutdown fallout. The table now uses `offsetof` and native field sizes; the reference delta uses `uintptr_t`, and field-size validation accepts native pointer widths.

A headless x64 `zombie_pentagon` run with the fighting test client and `bo1_autoquit 180000` exited 0. Round one ended at level time 86450 (six zombies dead by 91050); round two began at 96450, ended at 170950 (14 total zombies dead by 176050); round three began at 180950. No `Com_ERROR` occurred. A rebuild of x64 and Win32 Release succeeded; a fresh 12-second x64 smoke passed the save and entered round one.

Still open / follow-ups: ~2,970 pointer-truncation warnings (audit by runtime reach), `print_smodel`, the remaining `vehicle_info_t` tail offsets in `cent_fields` (>7268), `tools/headless.ps1` private-desktop crash, re-enable video detection, the debugger UI, and the Godot host.

## Physics feature route (2026-10-10)

The longer Pentagon route activated previously untested physics after the conference door opened. Native fixes now cover rigid-body iteration, broadphase and SAP allocation sizes, pointer arrays and atomic queues, environment broadphase info, contact manifold pointer strides/alignment, contact-point headers/pulse caches, partition assembly, contact priority sorting, and contact teardown. The collision/contact math and island grouping rules are preserved; raw Win32 offsets are replaced by typed fields and native sizes.

The guard build passed all 12 plan steps: `qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power`. Power activated at level time 82900; plan completed at 82950 and quit at 87950 with exit 0 and no headless assertions/crashes. The broken-board cleanup failure was `phys_sys::destroy_all_constraint` validating/freeing a native contact node using Win32 size 0x38; typed list removal and native pool keys fix it.

Evidence: `build/run/x64guard_route/main/console_mp.log` and archived `replay_*.log`; debugger stack in `build/run/x64guard_route/attach_pool.log`. Test flags include headless, testclient fight/god/score=10000, quitdone=5000, autoquit=240000. God mode and injected points make this a feature-path check, not a survival/balance test. Regular x64 and Win32 regression outcomes are recorded below when complete.

Final builds of Win32, plain x64, and guard x64 Release succeeded. After the final native GJK-cache and solver-buffer size fixes, guard x64 repeated all 12 PASS steps (complete 83350, quit 88350, exit 0). Win32 completed the same route at 83850, quit at 88850, exit 0, all 12 steps PASS. The comparison logs are `build/run/w32_physics_route/main/console_mp.log` and `build/run/x64_physics_route/main/console_mp.log`. Fresh run directories need the existing `localization.txt` sidecar; its absence causes an early launcher assertion before game startup.

The extended plain x64 run also exposed a separate script lifecycle bug: `cleanup_unoccupied_floor` kept executing after `self Delete()` sent the death notification, then called `RandomIntRange(0, 0)` on its initial empty destination array. All three hidden elevator struct tables contain six entries on both architectures. `Scr_TerminateRunningThread` delegates to `Scr_DebugTerminateThread` in developer mode; the x64 debugger guard returned immediately when the UI was not initialized, skipping termination. The fallback now sets the frame's native `fs.pos` to `g_EndPos`, matching normal VM termination. Temporary struct-table diagnostics were removed. Final replay results follow.

Final termination-fix verification: guard x64 passed all 12 steps, completed at 83350, quit at 88350, exit 0. Plain x64 passed all 12 steps, completed at 84200 and continued fighting for 120 seconds: round one ended at 165400, round two began at 177900, quit at 204200, exit 0. Both logs have zero headless assertions, crashes, failed plan steps, or script runtime errors. Win32 also passed a 120-second post-route soak with exit 0 before the x64-only termination fix; final Win32 Release rebuild and a 15-second startup smoke passed. Preserved full-route logs are `build/run/{x64guard_route,x64_physics_route,w32_physics_route}/feature_route_verified.log`. No renderer or Android changes were made.
