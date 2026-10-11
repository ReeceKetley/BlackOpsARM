# Native pointer cleanup progress â€” 2026-10-10

HEAD is `9693e39`. Follow-up fixes below are uncommitted. Read this alongside `handoff_64bit_clean.md`; its original crash and verification status are superseded here.

## Verified

- Baseline Win32 after the script rewrite: `w32_reg3`, 24 PASS, 0 FAIL, exit 0, all save readbacks pass.
- True high-address x64: `x64hi_route3`, 26 PASS including thief recovery, 0 FAIL, exit 0, save readbacks pass.
- Win32 after initial native engine fixes: `w32_reg4`, 26 PASS, 0 FAIL, exit 0, save readbacks pass.
- High-address physics batch: `x64hi_physics4`, 25 PASS, 0 FAIL, exit 0, save readbacks pass.
- Win32 physics and database batches: `w32_physics5` and `w32_database7`, each 24 PASS, 0 FAIL, exit 0, all four save checks pass.
- UI batch 8 builds on x64 without truncation warnings in `src/ui/`. Its runtime gate is pending.

## Fixed so far

- Client/server skeleton and collision scratch alignment preserve complete native pointers.
- AVL, convex hull, broadphase and GJK cache keys use `uintptr_t`.
- Script archive cleanup reads raw union bits without incorrectly decoding numeric values as pointers. Archived code positions advance decoded pointers before encoding them again.
- Physics object and constraint IDs stored in integer fields use narrow handles; callers decode them before native access.
- Physics island sorting, static-model lookup, vehicle wheel access, ballistic body traversal and destructible damage use typed native structures.
- Database in-use cache stores full pointer values, and initializes every entry. Database byte counts have integer types, while asset pointers and sentinel handling are native width.
- Loose destructible, physics preset and constraint loaders use native sizes and member offsets. These compile on both platforms, but the fastfile route does not exercise loose-file loading.
- Speex stack alignment uses `uintptr_t`, replacing Windows' 32-bit `long` assumption. Online voice is not runtime-tested.
- UI menu scripts, local-variable lookup, feeder tables, animation allocation, precompiler structures and weapon viewer use typed access.
- Batch 9 adds native expression parser allocation/string access, fixed-hunk allocator structures/free-list traversal, and typed post-processing matrix access. Builds and tests are in progress.

## Running / pending

- `x64hi_database6` completed the 30-minute high-address fighting soak, 26 PASS / 0 FAIL, all four save checks pass, clean autoquit at 1,800,003 ms and exit 0. It reached round 15. FACT-0058. This is the batch 6 snapshot; later client/audio fixes still require gates.
- Batch 9 Win32 and high-address routes both passed 24 steps with clean exit 0. Builds 14 also compile on both platforms. Current builds: batch 18 Win32/high; low and guard need refreshed builds after batch 16 compile errors (already repaired).
- Standalone fixed allocator regression passes on Win32 and x64 (64-bit backing buffer at 0x0000023934760000), covering distinct allocations, exhaustion, payload preservation, free/reuse, reset, invalid interior free rejection and buffer bounds. Source: tools/port/test_fixed_hunk.cpp.
- Fresh full warning count, remaining folder audit, silent truncation/Win32 stride audit.
- Final Win32/high/low/guard route gates.
- Windowed high-address game with sound enabled on the Sony monitor; verify address-space usage with `vmmap.ps1`.
- Only after these pass: LOW_VA default OFF, docs/ledger update, commit and push. Then temporary diagnostic removal, glass renderer and script debugger cleanup.

Last complete comparable warning count: 710 unique `src`/`tl` locations (714 including DemonWare) after initial fixes, down from 2,507. Subsequent incremental build counts do not represent a full remaining count.

## Operational notes

- Keep the user's Asus screen free. Current tests launch hidden.
- Never blanket-kill `BO1Zombies` or `cdb`; another test may be running. `hrun.ps1`, `hi.ps1`, and `cdbh.ps1` were corrected. Older window helpers still need review before use.
- Symbolize the executable/PDB in the **run directory**, not the latest build output. `symz.ps1` now does this.
- Read changing build logs with a Python snapshot. PowerShell `Get-Content`, even with `-Tail`, can stream the entire build unexpectedly.
- Preserve root scratch files; exclude them from commits. Batch edit scripts and build logs live in ignored build directories or ignored logs.
- Keep windowed low-address runs on `+set nosnd 1` until the high-address sound gate is complete.

## Batches 10–18 (uncommitted)

- Typed client demo buffer access and byte count API; native HUD sorting/material lookup, ragdoll handles, player controllers and turret angle pointers. Native server configstring model/FX fields. Stance UI colors use an actual float array. Hinge creation now records its handle.
- Effect free-list status and atomics use the container member; rewind arrays/profile records keep full pointers. Waypoint sprite count is a typed union member.
- XAudio2 buffer release and stream return queues use native pointer atomic exchange/CAS. Sound notifications use their actual subtitle/playback union members. Reverb frame count is an integer and channel offsets are ptrdiff_t.
- Render surface command pointers use narrow handles; native image list callbacks/shutdown arrays and COM resource release. Native monitor enumeration/window procedure pointers, refresh rate integer API, light visibility callbacks, surface lookup and cached lighting pointer. Pixel cost keys keep a 32-bit material handle.
- Renderer thread handoff uses a native pointer atomic exchange. Vehicle free-list ID lookup and matrix access are typed.
- Crash/stack/thread diagnostics preserve native addresses. DemonWare bdArray pseudo-pointer return values use uintptr_t.
- Latest full warning compilation (batch 11, before later fixes): 329 src/tl sites, plus four DemonWare sites. Five compile errors in that snapshot were repaired in subsequent batches. Full successful remaining count still pending.
- Helper bld.ps1 now caps compiler parallelism at four to reduce interference while the user plays on Asus.

## Windowed sound test (batches 19-22)

- Sony window helper is tools/port/window.ps1: fresh run snapshots, native sidecars, 1024x576 at (-1850,30), sound enabled unless -NoSound, no blanket process kills or UI input. The real-window harness requires both bo1_testclient_realwindow=1 and bo1_testclient=1.
- x64hi_sound18 loaded Pentagon but crashed in R_GetAverageLightingAtPoint. R_LightGridLookup cleared its pointer array with 4 * cornerIndex bytes, producing partially cleared x64 pointers. Fixed to sizeof(*cornerEntry) * cornerIndex.
- x64hi_sound19 progressed past lighting and crashed in Dvar_FindVar from GetDvarBoolValue. Expression operand stack/result/list copies silently used the union int conversion and dropped pointer upper bits. All eight raw-union copies now copy the union, and string validity checks use the string member.
- x64hi_sound21 loaded and connected with sound enabled, no runtime crash; harness was omitted from this launcher, corrected in sound22. Native memory query showed committed buffers at 0x26E37D50000 and above, 1,059 MB committed private plus 4,270 MB reserved private. The 4 GB reservation is allocator address space, not 4 GB physical memory. Do not count this as a full gameplay sound pass yet.
- Computer Use sky screenshot capture repeatedly times out for the Sony game window; accessibility inspection works. UI screenshot validation is pending.
- Batch18 Win32 route passed 25 steps, guard passed24, each exit0. Low route has25PASS and high soak26PASS, no failures; exits still to collect. Later lighting/UI fixes need refreshed gates.

## Audio gate correction

User reported silence. win_main.cpp initializes s_nosnd by substring presence, so +set nosnd 0 actually DISABLES sound before dvars initialize. All sound18-22 runs were silent and are NOT audio gate passes. window.ps1 now omits nosnd completely for sound-enabled runs. User also authorized Asus monitor, so next real audio run uses -X60 -Y60. God mode is explicit for automated route tests; -NoGod is available for separate damage verification. x64hi_audio23 preparation failed because the full rebuild temporarily removed the build exe; no process launched. Use a new run name when retrying.


## Current audio crash and batch 29

- User confirmed sound on Asus in x64hi_audio24. Audio25 reached 18 PASS, then crashed in tlAtomicMutex::Lock from Snd_StreamGetRequest/Snd_StreamUpdate. The crash was absent from console_mp.log but present in main/bo1_emergency.log and Windows Application event1000 at RVA88EBC1. symz.ps1 now falls back to the emergency log.
- Stream arrays still allocated 0xFA0 bytes for ten snd_stream records and 0x15E0 bytes for twenty snd_buffer records. Batch29 uses native sizeof for both; the old x64 allocations allowed records to overlap adjacent memory. Batch28 also decodes the on-disk 56-byte stream header field by field instead of copying into the wider native snd_asset. This addresses rejected music/dialogue (incorrect length); runtime confirmation is pending.
- High build29 compiles with zero errors. Fresh x64hi_audio29 uses the original plan (without the extra box step), real sound, god mode, on Asus at (60,60). Win32/low/guard are rebuilding. The overlapping high builds27/28 caused compiler output-file contention; a sequential rebuild succeeded. Do not build the same directory concurrently.
- Expanded gate25 failed: w32 29 PASS/2 FAIL; low24 PASS/1 FAIL; guard25 PASS/1 FAIL. All failed the added box step while the player and box were on different floors; Win32 also failed an inserted portal observation. These are not accepted final gates. Original handoff plan omits box.
- Latest successful full rebuild native_full22.log: 221 src/tl truncation sites plus two DemonWare, zero errors. Batch18 low is28 PASS, guard24 PASS, Win32 25 PASS, all exit0. High batch18 soak26 PASS/0 FAIL reached autoquit; final exit collection pending.
- Audio25 native memory query: 1969 MB committed private, 391 MB committed image, 30 MB mapped; buffers above2 GB. Four GB reserved address space is not physical RAM usage.
- Current ledger STATE.md has been rewritten to remove obsolete headless-only/loading-stuck and nosnd0 sound claims. Earlier sections above are historical snapshots, not final verification.


## Verified sound route; manual grenade failure (batch30)

- x64hi_audio29:26PASS/0FAIL, Pack-a-Punch complete at340900ms, planned quit3000ms later. User confirmed music/dialogue and observed the normal close. No stream errors or repeating allocation crash. A process exit code was not captured by the launcher for this run.
- x64hi_damage30: god OFF, injected G_Damage decreases100->94->88->82 (retail scaling), no crash/assert, timed quit exit0. Refreshed original-plan w32_gate29 and x64hi_gate29 each24PASS/0FAIL exit0; low/guard gate29 exits0 also collected (counts pending summary).
- Batch30 compiles all four variants; full successful native_full30.log has182src/tl sites plus2DemonWare (184total). Log normalized from UTF16 to UTF8 for warnsum.py. Remaining includes loose XAnim36, water31, script debugger11; silent casts/strides still need audit.
- Batch30 typed notetrack animation fields and print counters; packed console/HUD icon materials remain4byte slots using narrow encode/decode at all three readers/writers. Box planner now inserts an elevator ride when the box is on another floor. Expanded box30 runtime still cannot reach some box interaction points; not a passed gate.
- window.ps1 now supports -Wait (records exitcode.txt), -ManualPlay (bot/god OFF, real mouse input), and -Extra for diagnostic dvars. Manual play30 launched with sound, no god, no quit timer.
- **User manual play30 then crashed after a grenade throw.** C0000005 atRVA74D523 in SND_PlaySoundAlias reading SoundFile.type [rax+8]; exact pointer/cause still unknown. Preserve the play30 snapshot. Grenade30 debug test failed to throw because frags were not yet granted at spawn; grenade31 plan qr throw_frag throw_frag waits through QuickRevive and runs under cdb. Do not count route-only success as full manual gameplay stability, and keep LOW_VA default ON pending fix/retest.


## Grenade sound-pointer fix (batch32)

- Manual play30 and debugger grenade31/35/36 reproduced the sound-on explosion crash. Alias `wpn_grenade_explode_flux_r` had `soundFile` upper bits overwritten with 0x1194 (4500). Grenade36 breakpoints inside optimized SND_PatchApply confirmed the metadata move_time write at alias+36 during DB loading; that offset falls inside the native pointer at alias+32. The packed patch field indices are valid, but the 53 target offsets were literal Win32 offsets.
- src/sound/snd_db.cpp now uses offsetof(snd_alias_t, member) for every alias patch field, preserving patch field ordering. Snapshot metadata initialization now clears all 64 native records using sizeof and computes attenuation offsets from the native member.
- All four variants build with zero errors. Focused real-window, sound-ON tests w32_grenade_fixed32 and x64hi_grenade_check32 each have3PASS/0FAIL, two actual grenade_fire events, 20seconds of continued play after the last throw, and exit0. This verifies the reproduced explosion failure is fixed; it is not a general long-play stability claim. First high test x64hi_grenade_fixed32 also survived two explosions/exit0, but two extra requests exhausted the initial grenade ammo and produced2FAIL, so it is not a clean route pass.
- Collected original-plan gates29: Win3224/high24/low25/guard24PASS, all0FAIL/exit0. These precede batch30/32. Expanded box30: Win3228PASS/1FAIL and high27PASS/1FAIL, both box interaction timeouts despite reaching the target floor; these remain failed expanded gates.
- Fresh original-plan w32_gate32 is running. LOW_VA default remains ON. Remaining 182src/tl warning sites, silent casts/strides, expanded box navigation, and final gates still require work before default-OFF/commit/push.


## Weapon-sweep checkpoint: late client VM crash (2026-10-11, batch40)

- Win32 has completed all120 planned cases across10 round-based maps, including52 upgraded cases. Shared w32_weapsweep38_zombie_pentagon completed73PASS/0FAIL/exit0. Earlier47 map-exclusive cases and retries also passed.
- High-address shared x64hi_weapsweep38_zombie_pentagon crashed after67 completed PASS cases, during case68 (base ballistic-bowie) at level.time1257900, approximately21 game minutes. Its partial cases are not accepted as a clean batch. FAIL-0004 preserves the evidence. Genuine x64_diag assert stack is GetNewVariableIndexInternal3 -> VM_Execute_0 -> client CG_DrawActiveFrame; do not assume the weapon itself caused the bug.
- Scr_InitVariables allocated/cleared4*VARIABLELIST_SIZE bytes for const char** varUsage debug metadata. That is half native x64 storage. Batch40 uses sizeof(*varUsage) and alignof(const char*) for both VMs; Win32 layout is unchanged. This is separate from four-byte VM value slots, which correctly use narrow handles.
- All four batch40 builds compile0errors. Focused sound-ON real-window checks w32_weapsweep40_ballistic_bowie and x64hi_weapsweep40_zombie_temple each2PASS/0FAIL/exit0. The latter checks Shangri-La's large client script set against the allocation change. Full x64hi_weapsweep40_zombie_pentagon all73-case repeat is RUNNING; wait for complete73/0 plus exit0 before accepting all120 x64 cases.
- Earlier native fixes in this sweep include flameTable disk expansion, zero-glass snapshot handling, trigger_radius named bounds, typed DB_FreeUnusedResources entries, full native weaponInfo cache clearing and WeaponFullDef/WeaponVariantDef allocation/copy sizes. Win32 Shangri-La developer heap is now48MB with explicit allocation-failure diagnostics. See ledger FACT-0069/0070 and WARN-0006 for the details and developer loose-field-table limitations.
- weapon_sweep.md is regenerated from saved logs with tools/port/weapon_report.py. It retains failed attempts and only accepts PASS cases from complete clean runs. Scope: shared definitions once per architecture plus map-exclusive/different definitions on the respective map; catalogs compare all10 maps. Direct upgraded grants do not exercise every Pack-a-Punch machine pair. God mode is intentional; sound is ON. No valid visual screenshots.
- Dead Ops Arcade remains stopped by unsupported GetDvarVector on both architectures after its native flame loader was fixed. LOW_VA default is still ON; no commit or push. Continue the long repeat and update final evidence.


## All-installed-map weapon sweep (batches33-39, 2026-10-11)

- User requested every installed Zombies map. Ten round-based maps have20 matching Win32/high-address catalogs,825 registered asset observations and zero differences in checked metadata. The runtime plan has120 cases (119 names; Nacht's Thundergun has different metadata), including52 upgraded cases. Final accepted coverage is Win32120/120 and high-address x64120/120. Each accepted case passed in a completed run with exit0 and no emergency crash.
- Shared Pentagon runs w32_weapsweep38_zombie_pentagon and x64hi_weapsweep40_zombie_pentagon each73PASS/0FAIL/exit0. Map-exclusive/different definitions provide the other47 accepted cases per architecture, across preserved batches34/36/37/40 and targeted retries. See weapon_sweep.md for every case/run and retained failures.
- Runtime checks use the real rendered client, sound ON and ordinary usercmd input, with god mode for automation. Each case lasts18 game seconds, checking activation plus ADS/reload/switching where applicable. Actor damage/death counts are observations during the case, not guaranteed weapon-specific attribution. Shared definitions are exercised once per architecture, not on every map. Catalogs compare every map separately.
- Upgraded weapons are directly granted with retail gold camo; this does not test the Pack-a-Punch machine for every base weapon. Earlier route gates test the machine separately. No valid visual screenshots were collected; the attempted captures showed Codex and were removed from these test folders.
- Native fixes: expand476-byte flameTable disk records into528-byte native records; do not consume a glass snapshot section on zero-glass maps; write trigger_radius through named pose.general bounds instead of shifted player union aliases; use typed XAssetEntry fields in DB_FreeUnusedResources; clear all2048 native weaponInfo records; use sizeof for WeaponFullDef allocation and WeaponVariantDef copy.
- Win32 Shangri-La client compilation exhausted its23MB developer heap. Both hosts now use48MB with explicit allocation-failure diagnostics. Win32 retry passed both Shrink Ray variants and exited0. The initial sticky-grenade failure selected a competing spawn offhand; the harness now removes other held grenades in the same slot and the targeted Win32 retry passes1/0/exit0.
- Oversized explicit Pentagon startup list was truncated by the engine, losing devmap and leaving the frontend active. Use all or shorter batches; weapons.ps1 rejects oversized command lines. The high frontend run exposed the genuine DB_FreeUnusedResources Win32-layout bug. Original failed logs remain preserved.
- Batch38 high shared sweep crashed after67 completed PASS cases during the base ballistic-bowie case, at approximately21 game minutes. The real client VM's varUsage debug pointer table allocated and cleared4bytes per entry instead of native sizeof(const char*). Batch40 repairs allocation/clearing size and alignment on both VMs; Win32 bytes stay equivalent. The full high-address repeat completed all73 cases cleanly. The old crashed batch remains unaccepted, and the failure is not established as weapon-specific.
- All four batch40 builds compile with0errors. Focused batch39 real-window sound-ON cleanup regressions each2PASS/0FAIL/exit0 on Win32/high x64, for m1911_upgraded_zm and hk21_upgraded_zm. Focused batch40 Win32 ballistic-bowie and high Shangri-La regressions each2PASS/0FAIL/exit0. Earlier complete map batches are preserved older binaries; do not imply all120 cases ran on final batch40 source.
- Low/guard original route gates38 completed24/26PASS,0FAIL,exit0 with all four SaveGame readback checks valid. Original Win32 gate32 was collected31PASS/0FAIL/exit0. Manual play32 exited0 but is not a long manual grenade stability proof.
- Dead Ops Arcade is a separate non-Pack-a-Punch mode. Its native flame loader failure was fixed; both architectures then reach unsupported server GetDvarVector. Its gameplay remains uncovered. Loose/developer weapon and flame field tables still contain Win32 offsets (WARN-0006); ordinary fastfile cases do not verify those paths.
- LOW_VA default remains ON. No commit/push. Remaining warning audit, silent casts/strides, expanded mystery-box planner failures, final-source gates, client glass renderer and script debugger work remain open.


## Checkpoint publication - 2026-10-11

User explicitly requested committing and pushing the current batch40 checkpoint before continuing the native x64 audit. This changes the earlier commit deferral. LOW_VA remains default ON; remaining audit, fresh final route gates, client glass and script debugger work are not represented as complete. Round-map weapon coverage remains 120/120 on each architecture, with the scope and limits in weapon_sweep.md. Dead Ops gameplay is out of scope at user request.
