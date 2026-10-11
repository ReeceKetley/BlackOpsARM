# Native water renderer audit — batch47

Windows Vulkan remains the current target. These engine fixes retain D3D9 as
the renderer interface and remove assumptions that would also break ARM64.

## Repairs

- `R_RenderWaterModel` allocates `sizeof(GfxModelWaterSurface)` and writes named
  fields. The surface is 44 bytes on Win32 and 56 bytes on x64; its three COM
  buffer pointers retain their full width. The draw key remains a bounded
  four-byte-unit offset into the backend surface arena.
- `R_SurfaceAlloc` reserves alignment padding and the object with a compare
  exchange loop. Another surface producer may leave a four-byte boundary;
  native water surfaces require eight-byte alignment. Capacity checks avoid
  overflow and reject oversized/full-arena requests without advancing it.
- `R_WaterSimulationSkinMesh` uses a typed mesh pointer, a native vertex-output
  pointer and named mesh/vertex members. The source channel is an integer ID,
  never a fabricated pointer. Packed GPU vertices remain 16 bytes.
- The water worker declaration uses `JQ_CMD_SIZE(WaterSimulationCmd, 84u)`.
  Win32 keeps its 84-byte payload; x64 copies the full 152-byte command, including
  all 16 mesh pointers, through the existing large-payload queue path.
- Swizzle constants initialize the integer lanes of `float4`. The old aggregate
  initialized its float lanes, converting packed byte indices into float values
  and causing invalid byte lookups on both architectures.
- `FlushBuffers` returns void. Its callers only request cache flushes; the old
  return mixed unrelated cache pointers and fabricated integer pointers.

## Focused production tests

Run `tools/port/check_water_native.ps1 -NegativeControl`. Its generator extracts
current production surface allocation/rendering, channel lookup, command types,
worker declaration, queue copy and queue lookup. GPU/culling/queue allocation
are fixture stubs; this does not instantiate a D3D9 device or the actual worker
heap.

Win32 and x64 pass:

- GPU pointers, scalar fields and draw-key round trips at native addresses
  above 4 GB on x64;
- all four mesh-ring positions, near/far LOD and interpolation;
- exact-fit/full/oversized surface allocation and 2,000 concurrent allocations
  with alignment and non-overlap checks;
- both water channels across the wrapping seam and the XZYW byte swizzle;
- all 16 mesh pointers preserved by the production queue copy/lookup, with
  native heap payload and Win32 inline payload selection.

Checkpoint `17003a7` negative controls fail the native worker size check and surface alignment
check. The separate Win32 lookup control fails the channel/swizzle check. These
are expected failures, retained in `build/run/test_water_native_baseline_*.log`.

## Batch47 verification limits (before batch49)

The focused tests do not execute the full simulation or vertex skinning math.
Existing decompiled skinning still contains stack aliases and suspect vector
loads: three-float storage receives a four-lane write, and some vector loads
start inside a `float4`. Those paths need a separate reconstruction and a test
that actually exercises water cells. Passing a Zombies route does not establish
that dynamic water was rendered. This batch does not certify every map's water
behavior or Android GPU compatibility.

## Batch47 build and runtime evidence

All five configurations compile without errors: Win32, unrestricted x64,
restricted x64, restricted guard and unrestricted guard. Full unrestricted
rebuild: 206 seconds, zero errors, 144 unique truncation sites (142 in src/tl
plus two DemonWare), down from 175. No water truncation warning sites remain;
the warning count does not detect the vector-math issues above.

The fresh final-source original Pentagon routes complete26/24/24/24PASS on
Win32/high/low/guard, all0FAIL/exit0 and all four SaveGame checks. The initial
Win32-first route also completes26/0/exit0. These are headless/god regressions.

`x64hi_dxvk47_route` shatters the pane and reaches the real Pack-a-Punch
machine, receiving `mp5k_upgraded_zm`. It continues30seconds after completion,
exits0 and has zero Vulkan validation errors through teardown. However, it has
30PASS/**1FAIL**: a portal recovery entered a normal destination without the
required pack-room notify. **It is a failed route**, preserved in
`build/run/water47_results.json`. The script planner had marked a deferred
portal armed before its switch dependencies finished. A thief/Bonfire reset
left it at DEFCON2 when those dependencies ended, and it skipped its readiness
recheck. See [portal readiness recovery](pack_rearm.md) for batch48.

## Batch49: retail-reference vertex skinning

The skinner now uses typed nine-neighbor samples, initialized vectors, explicit
derivatives and saturating byte packing. This removes the three-float/four-lane
overwrite, loads starting inside vectors, unrelated locals treated as one
swizzle array, and damaged constant assignments. Retail's summation order,
XZYW sampling, XYAB derivative packing, foam/seam bits, signed shore distance,
color lookup and 17x17 loop are retained. The original function calculated
local min/max values without exporting them; those dead calculations are omitted.

`water_retail_oracle.py` executes the installed retail executable's original
skinner at `0x6de870` and lookup at `0x6de6e0` in Unicorn. Only the device event
and COM buffer lock/unlock calls are stubbed. Original sample, derivative,
sqrt, normalization, packing and loop instructions run unchanged. The local
executable is pinned to SHA256
`2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759`;
another version fails closed. No retail executable bytes are tracked or distributed.

Reproduce the oracle and focused tests:

```powershell
python -m pip install --target build/deps/water-oracle pefile unicorn
python tools/port/water_retail_oracle.py
tools/port/check_water_skin.ps1 -BuildGpu
tools/port/check_water_skin.ps1 -NegativeControl
tools/port/run_water_skin_gpu.ps1 -Name water_gpu_native_new -NativeD3D9
tools/port/run_water_skin_gpu.ps1 -Name water_gpu_dxvk_new
```

All **6,358 packed vertices** match byte for byte on Win32 and native x64:
flat, sloped, saturated, negative/positive wrapping seams, all16 mesh slots and
zero-length derivatives. Sources, command, meshes and output use guarded
allocations; x64 addresses exceed4GB. Leading/inter-mesh/trailing output gaps
stay unchanged. Lost-device and failed-lock paths leave output untouched and
do not incorrectly unlock. The pre49 skinner fails flat output on both hosts.
The durable `17003a7` skinner control fails Win32 output and faults on x64;
this control uses current corrected channel lookup to isolate the old skinner.

The GPU helper uses a real dynamic D3D9 vertex buffer with the production
NOOVERWRITE lock parameters, reads back the packed output, and releases the
buffer/device. `water49_gpu_native` and `water49_gpu_dxvk` both match all6358
vertices and exit0. DXVK loads the pinned patched local DLL, system Vulkan,
NVIDIA ICD and explicit Khronos validation layer; zero Vulkan errors through
teardown. This checks actual buffer mapping/lifetime; it does not submit the
water material/shader or prove an installed map rendered water cells.

All five game builds compile0errors, with no truncation warnings in recompiled
files. Full warning count remains the last batch47 measurement:144.

**Remaining water work:** the active UpdateFrame rewrite still reads offsets
`v+1` through `v+5` from a256-element cached row and does not reproduce retail's
neighbor/history math. Its curl/wind/shore/foam behavior, cache layout and
simulation-to-render scheduling need their own reference tests. WARN0009 stays
active for those paths; vertex skinning is now independently verified.

### Batch49 game regressions

Fresh original Pentagon gates finish32/25/27/24PASS on Win32/high/low/guard,
all0FAIL/exit0 with all four SaveGame checks. `x64hi_dxvk49_pane2_final`
finishes26PASS/0FAIL/exit0: pane2 shatters, the machine returns
`mp5k_upgraded_zm`, and play continues30seconds after completion. Sound is
enabled and automation uses god mode. Actual module paths and presented-frame
captures are retained; the3000 capture visibly shows the broken pane. There
are no Vulkan validation errors through teardown. These are gameplay
regressions; they do not replace the focused water tests above.

`x64hi_dxvk49_pane2_route` was intentionally stopped after an unknown
`glass_client` test-step typo was noticed (13PASS, exit-1). It is **not** an
accepted gate or a crash. The corrected fresh run uses `glass`.
All six attempts are retained in `build/run/water49_results.json`; focused,
oracle and real-buffer evidence is in `water49_focused_results.json`.

Additional retail/source comparison confirms the remaining cache problem:
retail short/char cache data begins16bytes into each cache; current compiled
Win32 and x64 data begins at2/1bytes, with metadata at516/260 instead of
retail532/276. Float4 cache layout agrees. The retail update is at`0x6daa30`,
accepts the command inEAX, and executes through`0x6dc518`. Its original channel
cache/simulation code is the next reference to reconstruct. No update/cache
fix is included in batch49.

## Batch50: native cache layout and complete retail wave update

The production channel template now aligns each cached row to16bytes,
including short/char data, and sizes its native pointer header to retain a
16byte cache offset on both hosts. Trailing alignment comes from the cache
type rather than a second hand-computed padding array. Cache strides are
4144/544/288/1056 for float4/short/char/Color on Win32 and x64.
`check_water_cache.ps1` exercises4000 dirty operations per type, including
hits, eviction, flush, row halos and guarded allocations above4GB on x64.
Both hosts pass; pinned1fca062 fails the expected layout checks.

`R_WaterSimulationUpdateFrame` now uses typed source neighbors and the
destination history cell. Explicit wrapped indices replace accesses beyond
the row halo. It restores retail damping, curl reduction, signed shore mask,
tilted wind, foam and signed-zero behavior. Wind config uses X/0/Z axes,
matching the original config function at0x6dd120.

`water_update_oracle.py` executes the installed SHA-pinned retail update at
0x6daa30 through0x6dc518, including its original cache, memcpy, normalization
and flush code, without stubbing those routines. Retail assets/executable
bytes are not tracked. Seven cases check flat/history/curl/wind/shore/flatten
and three repeated combined seam steps. Win32 and guarded native x64 match
all36864 updated cells bit-for-bit, and both full65536-cell banks match,
including untouched cells. Both /fp:strict and the game's /fp:precise pass.
Pinned1fca062 update with corrected cache fails both hosts; separately pinned
old wind config fails both hosts. Refreshed skinner tests still match6358
packed vertices on both hosts.

Reproduce:

```powershell
python tools/port/water_update_oracle.py
tools/port/check_water_cache.ps1
tools/port/check_water_cache.ps1 -NegativeControl
tools/port/check_water_update.ps1
tools/port/check_water_update.ps1 -FloatingPoint strict
tools/port/check_water_update.ps1 -NegativeControl
tools/port/check_water_update.ps1 -NegativeConfig
tools/port/check_water_skin.ps1
```

All five final game configurations compile with0errors and no truncation
warnings in recompiled files. The latest full warning count remains144;
these incremental builds do not establish a new full audit count.
Runtime regressions are recorded separately below when they finish.

**Remaining water work (WARN0010, supersedes WARN0009):** the active render
scheduler disagrees with the preserved original implementation in update
row start, previous-row lookup, update timestamps, FIFO occupancy and mesh
source/world coordinates. TrackView and whole simulation-to-render
integration still need reference tests. Focused wave/vertex tests and normal
Pentagon gameplay do not prove water material submission on a map with
actual water cells. This batch does not change those rendering functions.

Retail scheduler entry is also located at0x6dd640 (time inEAX);
its config call is0x6dd700 and worker dispatch is0x6dd92f. The saved
ignored water_scheduler_retail50.asm confirms the16-cell row starts,
FIFO head/tail test and source/world coordinate copies listed above.
This is inspection evidence for the next repair, not an executed scheduler
or whole-water integration test.

Batch50 final observations: Win32 is31PASS/1FAIL/exit0, failed ammo_pm63
recovery despite a real refill. Keep FAIL0005; later completion does not
accept it. High/low/guard finish24/25/25PASS, all0FAIL/exit0/all4Savechecks.
Vulkan50 finishes27/0/exit0, glass2 shattered, actual upgradedAK74U and
30seconds after completion, soundON/god and zero validation errors through
teardown. Batch51 corrects the harness measurement and repeats the final
gates. See ammo_target.md and build/run/water50_results.json.

Next-step scheduler preparation: water_scheduler_oracle.py executes retail
0x6dd640 across14 cases, retaining its original timing/row/FIFO/expiry/free
list/mesh/queue-payload instructions. Config/TrackView/model/worker helpers
are intercepted; debug drawing is off. Output water_scheduler52.json is
local ignored reference data, not native scheduler verification or a fix.

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
