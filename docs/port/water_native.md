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

## Verification limits

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
