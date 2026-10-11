# Windows x64 Vulkan bring-up

## Locally built DXVK lifecycle fix

The official 3.1.1 package reproduces one leaked `VkPipeline` at `vkDestroyDevice`
on both Pentagon and Moon, even with implicit layers, screenshot capture and the
DXVK HUD disabled. `DxvkMetaCopyObjects` creates input-attachment image-copy
pipelines but does not destroy that cache in its destructor. The tracked
`dxvk-patches/free-input-copy-pipelines.patch` adds the missing destroy loop.

`tools/port/build_dxvk.ps1` checks out exact upstream release commit
`b1a1c99ab52b687cf950d62c88bc2fa316b41663` and its pinned submodules, applies
only the recorded patch, and builds D3D9 with MSVC x64. It requires MSVC and
Vulkan SDK/glslang available on this host.
The fix must be reassessed against the selected Android DXVK revision. Meson 1.12.1 is isolated in an ignored
Python environment; the compiler and actual Ninja are recorded. No Android-only
conditional is introduced: the lifecycle fix can also apply to DXVK Native.

The build writes `build/deps/dxvk-win64-patched/package.json` with commit,
submodules, patch/DLL hashes, compiler versions and build time. Pass that manifest
explicitly to the launcher; the default manifest still selects the official
package for comparison:

```powershell
.\tools\port\build_dxvk.ps1
.\tools\port\dxvk.ps1 -Name dxvk_patched_fresh -PackageFile build\deps\dxvk-win64-patched\package.json -Validation -Wait
```

Initial patched Pentagon grenade test `x64hi_dxvk44_patched_frag` completes
3 PASS / 0 FAIL / exit 0 and no Vulkan validation errors, including teardown. Patched Moon `x64hi_dxvk44_patched_moon` also completes5/5, exit 0 with no
Vulkan validation errors through teardown. Only the expected debug-utils warning
appears. Final `x64hi_dxvk44_patched_route` completes 26 PASS / 0 FAIL
(including recovery), receives `ak74u_upgraded_zm` through the real machine after weapon recovery,
continues 30 seconds after completion and exits 0. All three patched runs pass
all four SaveGame checks and have no Vulkan errors through shutdown.
The aggregate `build/run/native_brush42_dxvk44_results.json` retains earlier
failed launch and validation attempts. No game remains running. This is a locally modified
upstream build, not an unmodified official binary.

## Reproducible setup

Batch46 restores default client glass on native x64. Native arena/header sizes,
client outline allocation and a silent Win32 stride in glass texture streaming
are repaired. The final default-enabled Vulkan route adds a real pane-shattering
step and completes25PASS/0FAIL/exit0 through actual upgraded shotgun and clean
Vulkan teardown. An additional unrestricted guard build passes the glass
client check3/0 and its original headless route24/0. See
[native glass evidence](glass_native.md). The older low-address guard real
window runs out of texture address space before glass; that failed attempt is
retained and does not replace the unrestricted guard result.

`tools/port/dxvk.ps1` launches a fresh snapshot of the unrestricted native x64
build (`build-x64-hi`, `BO1_X64_LOW_VA=OFF`) with the existing D3D9 renderer
translated to Vulkan. No x86 executable, Wine or CPU emulation is involved.

The official DXVK 3.1.1 Windows package is pinned in
`tools/port/dxvk-package.json`. Its SHA256 is checked before extraction; the
launcher verifies both EXE and D3D9 DLL are x64 PE files. Only `d3d9.dll` is
installed beside this disposable run's executable. System libraries and the
normal game installation are unchanged.

Each run preserves `launch.json` (binary hashes, source HEAD/dirty flag, arguments
and environment), EXE/PDB, `modules.json` / `all-modules.json`, DXVK and engine logs, shader cache,
PID and, with `-Wait`, the actual exit code. Environment changes are scoped to
the launching process and restored after the child starts. Empty `dxvk.conf`
uses upstream defaults. The disposable profile seeds the hardware state before
the engine's recommended-settings check; command-line variables alone are
overwritten by startup config and do not prevent its dialog.

From the repository in PowerShell:

```powershell
# Asus window, sound ON, automated god mode, two frags and 20 seconds after completion.
.\tools\port\dxvk.ps1 -Name dxvk_frag_fresh -Wait

# Original Pentagon route through the Pack-a-Punch machine.
.\tools\port\dxvk.ps1 -Name dxvk_pap_fresh -Plan 'qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power jugg speedcola doubletap pap' -AutoquitMs 900000 -Wait

# Matched system-D3D9 control (same EXE and launch settings, no local D3D9 DLL).
.\tools\port\dxvk.ps1 -Name d3d9_frag_control -NativeD3D9 -Wait

# Manual play uses ordinary mouse input, no bot and no god mode.
.\tools\port\dxvk.ps1 -Name dxvk_manual_fresh -PackageFile build\deps\dxvk-win64-patched\package.json -ManualPlay -AutoquitMs 1800000 -Wait
```

Fresh names are required. Automated tests use god mode and an explicit test
score. Sound is enabled by omitting `nosnd`, not by passing `nosnd 0`.
The default window is 1024x576 at (60,60) on the authorized Asus screen.

## Presented-frame capture and validation

On this host, Vulkan SDK 1.4.357.0 supplies the registered
`VK_LAYER_LUNARG_screenshot` and `VK_LAYER_KHRONOS_validation` layers.
`-CaptureFrames '1000,3000,6000'` enables the screenshot layer only for the child
and saves actual presented Vulkan images to `frames/*.ppm`. Frame numbers are
presentation counts, not milliseconds. Convert PPM to PNG with Pillow to view
in Codex. These captures do not copy the monitor; BO1's normal front-buffer
screenshot path previously captured the desktop and is unsuitable evidence.
Capture requires the layer installed on the host. `-Validation` enables the
Khronos layer and DXVK's validation callback; inspect the DXVK log for `err:`,
`error:` and `warn:` messages. The validation debug-utils performance warning is
expected, but Vulkan VUID errors must be investigated. With `-Validation -Wait`, the launcher
records `validation-errors.txt` and throws on `err:`/`error:` messages even when
the engine exits0.

By default the launcher disables implicit Vulkan layers for this child using
`VK_LOADER_LAYERS_DISABLE=~implicit~`. This isolates tests from system-wide
capture/overlay hooks while preserving explicitly requested screenshot and
validation layers. No registry or other app settings are changed.
`-AllowImplicitLayers` restores the host's usual implicit-layer behavior for
comparison. This filter requires a Vulkan loader with headers1.3.234 or newer.

`-Weapons 'name,name' -Map zombie_moon` reuses the existing runtime weapon
sweep with a rendered client and sound. It does not imply all weapons or maps
are tested on Vulkan.

## Verified evidence (2026-10-11)

- `x64hi_dxvk43_smoke3`: official DXVK 3.1.1, NVIDIA RTX 3070 / driver596.36;
  actual Vulkan swapchain1024x576. Loaded modules confirm run-local DXVK D3D9,
  system Vulkan loader, NVIDIA ICD and screenshot layer. Captured images show
  Round1 pistol/HUD/map geometry and the Quick Revive machine.
- Three planned steps pass, including two actual `grenade_fire` events. The game
  continues20seconds after completion and exits0. Audio initializes on Realtek;
  this run has no new user confirmation of audibility.
- `x64hi_dxvk43_smoke` and `smoke2` were deliberately stopped at the optimal
  settings dialog before renderer initialization. They are failed launch
  attempts, not Vulkan runtime failures or accepted tests.
- `x64hi_dxvk43_route`:24 PASS / 0 FAIL / complete / exit 0, real Pack-a-Punch upgrades
  the shotgun to `rottweil72_upgraded_zm`. All four SaveGame checks pass. Sound
  ON/god mode,30seconds after completion, no DXVK warning/error messages.
  Presented frames show shotgun/lower-floor lighting and the Pack-a-Punch machine.
- `x64hi_dxvk43_moon_validation`:5/5 weapon cases pass and exit 0, but the run is
  **not a clean Vulkan validation gate**. Overwolf's layer lacks an entry point;
  null-renderPass framebuffer errors occur with OBS/Overwolf/Twitch implicit
  layers loaded. An otherwise matching repeat disables those implicit layers.
  Isolated repeat5/5, exit 0 removes those framebuffer/loader errors but reports
  a distinct one-pipeline teardown leak. Validation-only Moon and HUD-off
  Pentagon reproduce that leak, so screenshot capture/HUD are not its cause.
- Moon's missing `p_zom_moon_cassimir_plate` precache script error also appears
  in prior Win32 and native D3D9 runs. Weapon passes do not validate the affected
  sidequest or every map feature. Captured FPS values during compilation or
  validation/concurrent regressions are not a benchmark.
- Moon's checker texture on part of the weapon HUD also appears in the native
  system-D3D9 control and its true back-buffer screenshots. This is an existing
  engine/content issue, not established as a Vulkan regression. The ordinary
  native sound-on control passes the same 5 weapon cases/exit 0. **Batch45 resolves
  the checker square:** Win32 also reproduced it, and `CG_Init` assigned HUD
  materials to the wrong `cgMedia` members. Corrected Win32 and unrestricted
  x64 Vulkan each pass5/5/exit0 with clean captured HUDs. Vulkan validation is
  clean through teardown. See [HUD material registration](hud_materials.md).
  Windows.Graphics.Capture
  failed twice; native image evidence instead uses the existing private-desktop
  headless-client COPY swapchain/back-buffer capture (1920x1080, AA1), so it is
  not a pixel-for-pixel comparison of the windowed AA4 Vulkan run.

## Boundary for the later Android port

Windows success checks BO1's D3D9 calls, shaders and resources against DXVK. The
engine still has Windows windowing, XAudio2, D3DX and vendor stubs. A Windows DLL
does not provide a native Android implementation.

Keep gameplay and the D3D9 renderer as the compatibility baseline. Later native
ARM64 work must move D3D9 creation/device and window/surface ownership behind a
platform boundary, use DXVK Native with a native WSI backend, and port the
remaining Windows/D3DX/audio dependencies. Upstream DXVK Native supports SDL3,
SDL2 and GLFW backends; native WSI selection is separate from this Windows DLL
experiment. Do not add Android-specific workarounds to gameplay or pretend a
desktop capability set is available on mobile.

User targets: Samsung S25 EU and Honor X8a with Mali, deferred while Windows
Vulkan is the active focus. Native Android DXVK version/features and any mobile
patches must be selected from actual device capability data. RTX3070 validation
does not establish compatibility or performance for either phone.

Primary references:

- [DXVK 3.1.1 release](https://github.com/doitsujin/dxvk/releases/tag/v3.1.1)
- [Upstream DXVK usage and Native WSI](https://github.com/doitsujin/dxvk)
- [LunarG screenshot layer](https://vulkan.lunarg.com/doc/sdk/latest/windows/screenshot_layer.html)
- [Khronos loader layer filtering](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md#layer-filtering)


## Latest native renderer and planner regressions — batches47/48

Native water surface/COM fields and vertex/mesh pointers are full width; the
worker command now copies its native152bytes and all16mesh pointers. Packed
swizzle indices initialize integer lanes. Focused production tests pass on
Win32 and above4GB on x64. Full high rebuild206s/0errors/144warning sites,
down175. Full water simulation/vertex math is not certified by these tests;
see [native water audit](water_native.md) and WARN0009.

Vulkan47 reaches actual PaP and exits with clean validation but has a portal
recovery failure after a thief/Bonfire reset; it remains a failed route. Batch48
leaves deferred portals unarmed until their switch prerequisites finish, so the
readiness check runs again. Current-production fixture tests reproduce the
reset and pass on Win32/x64; the pinned old code fails.

Final original Win32/high/low/guard routes26/26/25/25PASS, all0FAIL/exit0/all four
SaveGame checks. Final `x64hi_dxvk48_pane2_route` explicitly selects glass pane2,
shatters it, completes32PASS/0FAIL/exit0 through actual upgraded M14, continues
30seconds after completion and has zero Vulkan validation errors through
teardown. SoundON/god automation, real presented-frame captures and confirmed
Vulkan modules. Earlier pane0 glass observation fails its20second budget and is
intentionally stopped; that is retained as failed, not a crash or a pass. See
[portal recovery and evidence](pack_rearm.md), `water47_results.json` and
`pack48_results.json` under the ignored build/run folder. All test games have
ended. Source follow-ups remain uncommitted; Android remains deferred.

## Batch49 water vertex reference and runtime regression

The water skinner matches6,358 retail packed vertices on Win32/high x64,
including wrapping seams, saturation, all16slots and zero derivatives.
The same production function matches through real systemD3D9 and patchedDXVK
dynamic vertex buffers. The DXVK helper confirms the local DLL, Vulkan/NVIDIA
driver and Khronos layer, and exits0 without validation errors. It checks
buffer mapping/lifetime; it does not submit a water material shader.
See [water reference tests](water_native.md).

`x64hi_dxvk49_pane2_final` completes26PASS/0FAIL/exit0 with actual
`mp5k_upgraded_zm`, shattered pane2 and30seconds after completion. SoundON/god
automation, actual presented-frame captures and clean Vulkan teardown are
verified. Original native routes complete32/25/27/24PASS on Win32/high/low/guard,
all0FAIL/exit0/all four SaveGame checks. The earlier49 run's unknown
`glass_client` step caused an intentional stop; it remains a rejected attempt.
Results: `build/run/water49_results.json`, `water49_focused_results.json`.

Full water simulation/cache/scheduling remains open; these tests do not certify
every map's water rendering or Android GPU support.

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
