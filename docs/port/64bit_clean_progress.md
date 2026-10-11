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


## Native config fields - batch41

Checkpoint17003a7 was pushed first at user request. Follow-up batch41 converts748 weapon and119 flame config offsets to native offsetof with retained compile-time Win32 checks; fixes sizeof(flameTable) allocation; snapshots/commits developer backing arrays and restores original pointers. Standalone Win32/high-address tests pass867 mappings and838 generic field writes, guarded initialization and array edits; old weapon/flame tables fail separate negative controls. See weapon_fields.md for reproducible tests and limitations.

All four builds compile0errors. Fresh original Pentagon routes complete through Pack-a-Punch: w32_fields41 24PASS, x64hi_fields41 32PASS (extra thief recovery), x64low_fields41 24PASS, x64guard_fields41 24PASS. All0FAIL/exit0, no recorded assert/emergency crash, and all four SaveGame readback checks passed in each. These are headless fastfile regressions with god mode, not full loose asset pipeline or manual audio/visual validation. Full native_full41 rebuild completes194s/0errors,184 warning sites (182src/tl plus2DemonWare), unchanged from full30. LOW_VA staysON; batch41 work is uncommitted. Next audit target: common/brush_edges.cpp packed queue and pointer copies still use Win32 widths.

## Native brush geometry and Windows Vulkan - batches42/43

Batch42 replaces packed x86 brush BFS storage and pointer-sized array copies with typed native access, plus a real PartitionEdges output array. Focused Win32/x64 tests pass at0x180000000 with a1024-point guarded cycle. Original Pentagon routes w32_brush42/high/low/guard complete25/31/24/31PASS, all0FAIL/exit0 and all four SaveGame checks. Full native_full42 rebuild216s/0errors has178unique truncation sites (176src/tl plus2DemonWare), down184. See brush_edges.md. LOW_VA default remainsON.

Windows Vulkan now renders the unrestricted x64 game with official pinned DXVK3.1.1. x64hi_dxvk43_smoke3 passes qr/two-frag3/0 and exit0; full x64hi_dxvk43_route passes24/0 through actual Pack-a-Punch, all SaveGame checks, no DXVK warn/err/error and exit0. Both use soundON/god automation. LunarG presented-frame images show actual gameplay; these are valid evidence, unlike the earlier desktop screenshots. Source, package/binary hashes, logs, modules, profiles and shader caches are preserved per run.

First Moon validation sweep passes5/5 weapon cases but has Vulkan VUID errors with OBS/Overwolf/Twitch implicit layers. Same-map repeat disables only child implicit layers with Khronos loader filtering; result pending. No mobile compatibility claim: Samsung S25 EU/Honor X8a are later targets; Windows Vulkan is current focus. See vulkan_windows.md for scripts, evidence and the eventual native DXVK/WSI boundary.

## DXVK lifecycle correction and final Windows Vulkan gates - batch44

Official DXVK3.1.1 validation exposed implicit overlay conflicts and a distinct one-pipeline leak during device destruction. Per-child Khronos loader filtering removes the OBS/Overwolf/Twitch framebuffer/loader errors. The leak remains without screenshot capture or HUD. Upstream DxvkMetaCopyObjects omits destruction of its input-attachment-copy pipeline cache; the tracked three-line patch adds it. tools/port/build_dxvk.ps1 checks out the exact release/submodules, verifies the sole patch, builds MSVC x64 D3D9 and records source/patch/DLL/compiler identity. Source, binaries and build logs remain ignored dependencies; the reproducible scripts and patch are tracked working files.

Patched Vulkan runs x64hi_dxvk44_patched_frag / patched_moon / patched_route complete3/5/26 accepted cases respectively, all0FAIL/exit0 and all four SaveGame checks each. Full route uses the real PaP machine and waits30seconds after completion; grenade test sees two real grenade_fire events and waits20seconds after completion. Explicit Khronos validation and LunarG screenshots remain enabled; no Vulkan err/error/VUID messages occur through teardown. The only warning is expected debug-utils performance overhead. No FPS benchmark or new user sound confirmation is claimed. Genuine presented-frame images and aggregate native_brush42_dxvk44_results.json preserve evidence and failed earlier attempts.

Native D3D9 Moon sound-on control also completes5/5/exit0. Private-desktop native back-buffer images show the same weapon-HUD checker texture as Vulkan. Moon cassimir plate script precache errors also occur on Win32/native D3D9. Those baseline engine/content issues remain open; the Vulkan milestone does not certify all map features, all native cold paths, client glass, script debugger or mobile compatibility. Native audit count remains178 and LOW_VA default remainsON; tests use unrestricted build-x64-hi/OFF. Batches41/42/44 follow-up changes are uncommitted after pushed17003a7. Current user focus remains Windows Vulkan; native Android on S25 EU/Honor X8a is later.


## Batch45: Moon HUD material correction

The frozen Win32 batch42 client reproduces the same HUD checker square, ruling
out an x64-only regression. CG_Init stores eleven HUD registrations in the
wrong named members: the equipment highlight receives a missing night-vision
material and ammo materials are shifted or overwritten. Corrected assignments
preserve asset names and drawing behavior. All four builds compile0errors;
corrected Win32 and unrestricted x64 Vulkan each pass the five Moon weapon
cases (including both upgraded Wave Guns), all four SaveGame checks and exit0.
Real back-buffer/presentation captures show the square is gone. Patched DXVK
has zero validation errors through shutdown. See hud_materials.md and
build/run/hud45_build_results.json. SoundON/god automation for Vulkan; muted
private-desktop Win32. Cassimir plate script errors and wider native audit remain
open. Batch45 is uncommitted with the prior follow-ups; LOW_VA default remainsON.


## Batch46: client glass restored under native x64/Vulkan

Typed glass arena alignment/header recovery, native pane/outline sizes, split
pointer clearing and proper list destruction are repaired. Enabling the client
first exposed a silent Win32 glass stride in R_StreamUpdateDynamicModels;
named glassBrush fields replace those accesses. Glass now initializes normally
on x64. Focused production allocator tests pass6000 guarded operations on x86
and above4GB on x64, including std::list reuse; the old x64 implementation is an
access-violation negative control. Win32 real pane-shattering3/0/exit0 and the
final default-enabled Vulkan route25/0/exit0 through actual upgraded shotgun
pass, including clean Vulkan teardown. Original headless Win32/high/low/guard
routes26/26/26/31PASS, all0FAIL/exit0, all four SaveGame checks each. New unrestricted
build-x64-hi-guard (LOW_VA OFF, GUARDHEAP ON) compiles0errors and passes its
original route24/0/exit0 plus real Vulkan glass3/0/exit0 with clean validation.
The existing restricted-address guard real window still runs out of texture
address space before glass; that failed attempt is retained. Full high rebuild
231s/0errors/175unique sites (173src/tl+2DemonWare), down178. No game remains
running. See glass_native.md and build/run/glass46_results.json. Batches41-46
remain uncommitted. Wider cold-path audit/script debugger, Moon sidequest issue
and later Android remain open; LOW_VA default staysON.


## Batch47: native water renderer pointers and worker payload

Water model surfaces use native sizeof/named COM fields and aligned bounded
atomic arena reservations. Water skinning retains full mesh/output pointers and
integer channel IDs; packed GPU vertices stay16bytes. Water worker payload is
now sizeof(WaterSimulationCmd) on x64 (152bytes), preserving all16mesh pointers,
with the84byte Win32 path retained. Swizzle constants initialize integer lanes
instead of converting the packed byte indices to floats. Focused extracted
production tests pass on Win32 and above4GB on x64, including2000concurrent
allocations, ring/LOD/scalar fields, channel seams and queue copy/lookup. Pinned
17003a7 negative controls fail native payload size/alignment and Win32 swizzle.
All five variants compile0errors; full high rebuild206s/0errors/144unique sites
(142src/tl+2DemonWare), down175. Final original Win32/high/low/guard routes
26/24/24/24PASS, all0FAIL/exit0/all four SaveGame checks. Vulkan47 reaches
actual upgraded MP5K and glass, exit0/clean Vulkan teardown, but30PASS/1FAIL
for portal recovery; this is a failed route. The planner marked the original
portal armed before its DEFCON dependencies completed, then a thief/Bonfire
reset invalidated that readiness. Batch48 defers that flag and is under test.
Full water vertex math remains unverified (WARN0009), including suspect vector
stack writes; route passes do not prove water-cell simulation. See water_native.md,
pack_rearm.md and build/run/water47_results.json. LOW_VA remains defaultON;
current focus is Windows Vulkan, with Android deferred.


## Batch48: portal prerequisite readiness and fresh Vulkan gate

PackCheck now leaves the original portal unarmed when inserting DEFCON pulls.
A thief/Bonfire reset during the dependencies therefore triggers another check
before entering the portal. Focused extracted production PackCheck/InsertStep
tests pass Win32/x64, including reset, ready state, Bonfire policy, failure cap
and floor ordering; pinned17003a7 fails the deferred flag check. All five builds
compile0errors/no new pointer warnings. Fresh original Win32/high/low/guard
routes26/26/25/25PASS, all0FAIL/exit0/all four SaveGame checks. Final Vulkan
x64hi_dxvk48_pane2_route explicitly selects glass2, shatters the pane, completes
32PASS/0FAIL/exit0 through real m14_upgraded_zm, then30seconds more and clean
Vulkan teardown. Actual module paths and presented-frame captures confirm
DXVK/Vulkan; soundON/god automation. The earlier pane0 observation has1FAIL
(11.9health after5shots at long range/20seconds); intentionally stopped/exit-1,
not an engine crash, retained with intentional-stop.txt. All six attempts are in
pack48_results.json; no game remains live. Full warning count stays144 from47.
See pack_rearm.md and vulkan_windows.md. Water math WARN0009, wider cold paths
and script debugger remain open. LOW_VA default staysON; Android deferred.
Batches41-48 remain uncommitted after pushed17003a7.

## Batch49 - water skinner compared with retail

Typed vertex math replaces unsafe stack aliases and overlapping vector loads.
6,358 packed vertices match original retail instructions exactly on Win32 and
high x64, including all16mesh slots, saturated values, wrapped coordinates and
zero derivatives. Guarded sources/output above4GB and lost-device/failed-lock
paths pass; old code fails as a negative control. Real systemD3D9 and patched
DXVK vertex buffer tests also match and exit0, with clean Vulkan validation.

All five builds compile0errors/no new pointer warnings. Fresh original routes
32/25/27/24PASS on Win32/high/low/guard, all0FAIL/exit0/all four SaveGame checks.
Final Vulkan49 is26PASS/0FAIL/exit0 through actual upgradedMP5K and shattered
pane2,30seconds after completion, clean validation through shutdown. A test
step typo in the first Vulkan49 attempt caused an intentional stop; it is
retained and excluded. See water_native.md and water49_results.json.

Checkpoint1fca062 is pushed and includes batches41-48 plus initial49 source.
Full water simulation remains WARN0009: current UpdateFrame reads beyond its
one-element row halo, and compiled short/char cache data starts at2/1 rather
than retail16bytes. Retail update0x6daa30 is identified for the next repair.
Script debugger/cold-path audit/LOW_VA default change remain unfinished.

## Final batch50/51 gameplay verification

Fresh corrected original routes Win32/high/low/guard finish24/25/25/25PASS,
all0FAIL/exit0 with all four SaveGame checks. These executables contain the
complete cache/wave/wind fixes and corrected ammo-test targets.
`x64hi_dxvk51_pane2_final` finishes33PASS/0FAIL/exit0, actual
glass2 shattering and Pack-a-Punch upgrade, then30seconds further play.
Sound is enabled; automation uses god mode. Pinned patched local DXVK,
system Vulkan/NVIDIA and explicit validation layers are recorded in the
module manifests. Presented-frame captures are retained. There are zero
Vulkan errors through teardown. All these test processes have ended.

The earlier Win32 water50 run remains31PASS/1FAIL/exit0, rejected for its
ammo-target measurement failure. `water50_results.json` retains all six
batch50 observations. `ammo51_results.json` retains both focused game runs
and all five corrected final gates; `water50_focused_results.json` retains
the retail wave/cache/skin and old-code control evidence. Focused ammo
measurement cases and their pinned controls are in `test_ammo_target*.log`.
These normal gameplay routes still do not certify full water-cell rendering.
WARN0010 remains open for scheduler/TrackView/material integration.

## Batch52: wrapping halo adjacency follow-up

Original short/char cache instructions store the left wrap value at row-2
and row-1 (retail0x6db500,0x6db5ad/0x6db649). Batch50 aligned the row but
left its scalar halo at the start of the prefix, separated by padding.
Earlier tests checked the scalar field rather than the actual boundary.
The prefix now occupies16bytes and stores the wrap value in its final
T element. The complete cache sizes/data/metadata offsets are unchanged.

The strengthened boundary test rejects the pre52 placement on both hosts.
Corrected float4/short/char/Color caches pass4000 dirty operations/type/host,
including guarded high allocations, flush and wrapped boundaries. Exact
retail wave comparison still passes36864 cells on both hosts using the game's
/fp:precise setting. Reproduce the isolated negative control with
`tools/port/check_water_cache.ps1 -NegativeHalo`.

All five current builds compile0errors. Header recompilation re-exposes
existing BSP pointer warnings (1Win32/4x64); full count remains144.
`water52_focused_results.json` records current source hashes/build-log hashes
and focused results. The successful final51 native/Vulkan gameplay runs
predate this isolated halo follow-up; no full route for final52 is claimed.
WARN0010 remains open for render scheduling/TrackView/water material
integration. The14-case local scheduler oracle is preparation for that work,
not proof that the native scheduler passes.

## Water scheduler and view tracking (batches53/54)

The complete production scheduler now matches the installed original at
0x6dd640 in14 cases/83 commands on Win32 and guarded high-address x64.
Corrections cover row start indices, update-row timestamps, mesh FIFO
occupancy and copying the tile coordinates into new meshes. Tests compare
all256 tile states, the full expiry/free queues, call order and normalized
worker payloads. The pinned old scheduler fails on both hosts.

TrackView now matches original0x6dc640 in17 cases/935 tile-fill callbacks.
This includes smoothing, wrapped relocation, expiry and teleport boundaries,
with exact float bits and complete tile/expiry state. Retail uses exponent
0.5 for movement easing, sums squared X/Z/Y, retains double precision across
sqrt/divide until the float store, and multiplies by reciprocal tile size.
The previous easing fails small_motion; reverting only the distance math
fails precise_distance on both hosts. Original sqrt/pow/expiry/teleport
instructions execute in the reference. Base-height and FillTile are mocked
in this comparison, so actual samplers are a separate remaining check.

All five current54 build variants compile with0errors and no pointer-width
warnings in recompiled files. Final53 native original routes complete
25/25/25/25PASS (Win32/high/low/guard), all0FAIL/exit0/all4Savechecks.
Vulkan53 completes26PASS/0FAIL/exit0, pane2 shattering and real upgradedPM63,
then30seconds continued play. SoundON/god automation; actual presented frames
and loaded module manifests retained; zero validation errors through teardown.
These runtime snapshots predate Track54; current54 regression runs are pending.

Use check_water_scheduler.ps1 and check_water_track.ps1. Reference generators
read only the SHA-pinned local original executable; original assets/bytes stay
out of Git. water53_results.json and water54_focused_results.json under the
ignored build/run directory preserve logs, hashes, fixtures and scope.
Remaining water scope (WARN0012): actual FillTile/base-height sampling, debug
drawing and real water material/shader submission. Ordinary Pentagon passes
and real mapped vertex-buffer comparisons do not certify that submission.

## Tile and height samplers (batch55)

The independent reference executes original FillTile0x6da4f0, FindCells0x6da420,
BaseHeight0x6dfba0, ConvertWorldToGrid0x6ddf90 and global height0x6e0280 without
intercepting any engine/math calls. Both native hosts match nine tile fills
(2304 interpolated cells), all six complete65536-cell channel banks, exact
tile bounds/validity and22 float-bit height queries. Source state, input cells
and output banks use guarded native allocations above4GB on x64. Cases cover
interior/right/bottom/corner lookups, negative headers, missing/outside cells,
last source rows, signed floor offsets and disabled/enabled local sampling.

Original coordinate conversion uses float reciprocal multiplication. The old
global-height expression chooses no water (-131072) at query
(-12.500001,-.0001), where original lookup returns173. Restoring the original
operation order fixes this on both hosts; pinned1fca samplers fail that same
query on both. ConvertWorldToGrid also preserves original reciprocal math.
All five current55 builds compile0errors/no recompiled pointer warnings.
Use water_sampling_oracle.py, then check_water_sampling.ps1; add
-NegativeControl for the pinned old behavior. Original executable bytes/assets
stay out of Git. Fixtures, logs and hashes live in water55_focused_results.json.

Final54 native25/26/26/24PASS and Vulkan5426PASS, all0FAIL/exit0/all4Savechecks,
are accepted; Vulkan has zero errors through teardown and continues30seconds
after actual PaP and pane2 shattering. These snapshots predate sampler55.
Win32 final55 completes26PASS/0FAIL/exit0/all4Savechecks; x64/Vulkan55 pending.

Noninvasive live CDB inspection of Vulkan54 confirms Pentagon has zero
simulation water cells. The optional weapons.ps1 -Catalog -Debugger -WaterCells
records actual COMWORLD asset names, counts, headers and native pointers at
Load_ComWorldAsset; it requires x64 and checks the native field offset in the
production sampler fixture. Initial metadata attempts55a/b had unresolved
debugger module names and produced no water counts; retain them as unverified.
The corrected wildcard symbol lookup55c records Shangri-La count0/null cells.
This inventory is in progress. Ordinary routes are gameplay regressions, not
proof of real water material/shader submission. WARN0013 remains open for that
integration and debug drawing; full dynamic height/info/particle behavior is
outside the base-height sampler comparison. Wider native audit is unfinished.

## Final55 native/Vulkan and installed-map inventory verification

Fresh original Win32/high/low/guard routes complete26/25/28/24PASS, all0FAIL,
exit0 and all four SaveGame checks. These snapshots include the final55 sampler
fixes. Vulkan55 completes26PASS/0FAIL/exit0, actual pane2 shattering
and real pm63_upgraded_zm, then30000ms continued play (273850 to303850).
SoundON/god automation. Module manifests confirm patched local DXVK, system
Vulkan/NVIDIA and explicit validation/screenshot layers. There are zero Vulkan
errors through teardown. The actual3000 presented frame shows rendered gameplay
and the broken pane. All test processes have ended. Logs/hashes are preserved
in water55_results.json and water55_focused_results.json.

All10 installed round-based Zombies maps have zero simulation water cells in
their loaded COMWORLD assets. The asset inventory records exact names, counts,
headers and null cell pointers; it does not certify visible water effects or
full gameplay on those maps. Nacht's CDB string display wraps into32-character
quoted lines; joining adjacent string/address lines recovers the full name.
Independent Win32 raw bytes and repeated high captures confirm the name/count.
The initial parser rejections are retained in water55_map_cells_initial_results.json;
final observations in water55_map_cells_results.json. Initial55a/b unresolved
breakpoints remain unverified. weapons.ps1 -WaterCells supports both architectures
with native COMWORLD offsets checked by the production sampler test.

The water simulation's cache/wave/skin/scheduler/tracking/base-sampling paths
have focused original-reference coverage; actual material draw, debug drawing,
dynamic height/info/particle paths still need separate evidence (WARN0013).
An actual simulation-water draw cannot be inferred from these installed map
routes. Windows Vulkan remains the current target. Android is deferred.
Latest full audit remains144 pointer-truncation sites from47; these incremental
builds do not establish a new full count. LOW_VA default remainsON and the
actual unrestricted build-x64-hi isOFF. Follow-ups49 through55 remain uncommitted.

## Native material storage (batch56)

- Batch56 (FACT0098/0099): native loose-material technique-set/state-map/pass/trailing-pointer/name storage and shader-argument allocation/copies/literal/compaction fixed. Scratch512arguments and130*4state pairs restored. Seven focused production cases pass on each host with guarded x64 pointers above4GB; pinned1fca062 fails all7x64 and Win32parse/state, other5Win32 cases pass. File/parser/reflection/linkage inputs controlled; full pipeline is not certified. All5builds0errors;6remaining material truncation sites on x64 versus10before, no full recount. Final56 original Win32/high/low/guard routes 25/25/32/26PASS, all0FAIL/exit0/all4Savechecks. Windows Vulkan56 26PASS/0FAIL/exit0/all4Savechecks with pane2, actual upgraded weapon, 30000ms continued play and zero validation errors through teardown. All test games ended. WARN0014 tracks remaining loose shader COM/copy/header/raw material pipeline. WARN0013 water material/debug integration stays open. Followups49-56 uncommitted. See material_native.md and ignored material56_results.json/ material56_focused_results.json.

## Native shader compiler and ownership (batch57)

- Batch57 (FACT0100/0101/0102/0103): typed D3DX diagnostics/constants and COM ownership, native shader headers, proper vertex bytecode copy and CreateShader failure buffer releases fixed. Real D3DX guarded tests15cases per host pass, x64 COM/objects above4GB; pinned controls fail12Win32/13x64 cases. Controlled generation/cache/device/failure helpers; warning is a controlled real COM buffer, invalid HLSL yields real diagnostics. All5builds0errors/no material truncation warnings. Full high426s/0errors/134sites (132src/tl+2DemonWare), down144; gfx_d3d21 remain. Final57 Win32/high/low/guard 24/32/25/25PASS, all0FAIL/exit0/all4Savechecks. Actual GPU shader checks pass Win32D3D9/x64D3D9/x64DXVK with expected pixels and clean Vulkan teardown; final57 Vulkan 28/0/exit0/30seconds continued/clean teardown. Frozen56 Vulkan soak passes 1800000game ms after actual PaP, max round16/520spawned,0FAIL/exit0/clean teardown. All test games ended. WARN0016 replacesWARN0015 for remaining full loose-file/raw-material/reflection/cache integration. WARN0013 water material/debug remains. Windows platform docs remove old Godot bridges and preserve current native DXVK direction. Followups49-57 form the verified checkpoint; publication SHA is recorded in ledger. Reports shader57_results.json/shader57_focused_results.json/vulkan56_soak_results.json;56 soak executable predates57. Initial DEVICELOST helper attempts remain preserved; the same device setup later passed, transient cause unconfirmed. GPU manifests hash actual loaded modules, including system x64 D3DX (which differs from the staged DLL) and WOW64 system paths.
