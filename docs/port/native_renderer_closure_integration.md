# Canonical renderer closure batch

2026-10-13. Source compile/CPU policy proof only. Actual GPU fence, image touch,
collision rendering, compositing, command execution, initialized device memory,
NVAPI-positive capabilities and complete Com_Init remain unexecuted.

Select canonical src/gfx_d3d/{r_caps,r_texturemem,r_cmds,r_setstate_d3d,
rb_compositing,rb_showcollision,rb_stream,rb_imagetouch,r_hw_nvidia}.cpp.
The only companion edit is rb_backend.cpp's native stereo capability block.
No extra compile definitions or SDK failure/success substitutes are required.
Windows bodies, CP1252 and accepted Windows command behavior are preserved.
The reference's intentional imagelist BLOPS_NULLSUB registration remains as-is.
No central CMake, diagnostic, ledger, protected DXVK/Vulkan/SDL or assets changed.

All ten whole TUs compile with the production Android toolchain/instrumentation.
Evidence build-android-audit/native_registry_closure/<name>-closure-audit2.log,
<name>-closure-audit2-command.json, <name>-closure-{defined,undefined}.txt and
renderer-closure-frozen-hashes.json. Root reported full396 selecting the nine
sources passes production compilation and reaches its remaining real44-symbol
linker closure. Root owns final normal Windows10 comparison and diagnostics.

Changes:

- Caps retain every original required-bit/min/max check and warning/fatal policy.
  Native reads validated fixed 4-byte words with memcpy instead of loading all
  fields through D3DDEVTYPE (invalid enum/aliasing). Exact304-byte D3DCAPS9 and
  DeviceType offset0 are asserted against the real SDK. Fatal callbacks abort
  if they unexpectedly return, preventing continuation through rejected caps.
- R_AvailableTextureMemory invokes the actual initialized D3D device's
  GetAvailableTextureMem. Protected DXVK explicitly defines this as a coarse
  estimate based on its actual available-memory state. The separate Windows
  DirectDraw physical-video-memory probe is unavailable on Android: R_VideoMemory
  reports that fact and uses its original unknown=0 fallback. No DirectDraw
  initialization, physical VRAM size, desktop window or device readiness is
  synthesized. The original actual D3D fallback is retained.
- Fence wait requires actual owned device/query, polls real GetData with FLUSH,
  yields while S_FALSE and returns only on S_OK. Other HRESULTs produce explicit
  fatal error; they can no longer masquerade as completed GPU work. No positive
  GPU/provider fixture was used to certify this path.
- Native image touching uses the actual qsort callback signature, validates list
  count and each image pointer, preserves the drawing/scene/resource algorithms,
  and aborts on unexpected return from BeginScene/EndScene fatal callbacks. Two
  zero-semantic images compare equal, restoring comparator antisymmetry; canonical
  semantic/memory ordering otherwise remains. Its unused accumulated memory
  value uses uint64_t instead of signed overflow. No GPU image touching executed.
- Native collision code validates debug mode index, inputs and actual tessellation
  capacities, then uses actual points[row] instead of indexing beyond the first
  inner array. The original frustum, polygon, line and draw algorithms remain.
- Native compositing retains the original commands/state/target algorithms. Its
  final identical-source/destination memcpy is omitted: it has no state effect
  and violates memcpy's nonoverlap contract. Windows retains the original call.
  refSamplerState is the first field of the containing state, so this was an
  identity-copy overlap issue, not a proven past-the-object overflow.
- RB_StreamInit, R_RegisterCmds/R_UnregisterCmds and NV_UseShadowNullColorRenderTarget
  retain their canonical bodies. Native r_init already truthfully leaves the
  unavailable Windows NVAPI capability/handle uninitialized. Native rb_backend
  rejects any unexpectedly published nonnull stereo handle rather than invoking
  unavailable Windows APIs or claiming stereo activation. Standard D3D and the
  existing accepted non-NVAPI IntZ policy remain; Windows NVAPI code is unchanged.

Durable focused source tools/port/renderer_closure_fixture.cpp consumes actual
r_caps.cpp directly to access its private policy table/read helper and links the
actual image comparator/stream owner. It does not copy a renderer algorithm or
mock positive device/GPU readiness. Its isolated logger/error callback records
policy; a returning fatal callback runs in a child and must terminate SIGABRT.

HONOR AKWJ9X5116G04618 UBSan, no recovery:
renderer-closure-fixture-build1.log/command.json and
renderer-closure-fixture-run1.log PASS. The actual fixed caps policy accepts a
constructed table-consistent input; missing dynamic-texture requirement remains
fatal, missing optional gamma remains warning. Unaligned fixed-word bytes avoid
invalid enum/alignment loads; null/partial-word inputs reject. Returning fatal
callback cannot continue. Actual image comparator matrix is antisymmetric,
including zero-semantic/self pairs; actual qsort gives descending memory before
zero-semantic entries. Canonical stream bit arrays clear. This proves CPU policy,
ABI and bounds only, not measured GPU capabilities.

Only the new fixture binary was transferred, with no new library or assets:

    adb -s AKWJ9X5116G04618 shell /data/local/tmp/renderer_closure_fixture

Earlier native audit1 errors for genuine missing DirectDraw declarations and
Windows CRT qsort typedef remain preserved. Positive graphics, real command
consumers, resource lifecycle/recovery and the separate Java/input/SDL owner
prerequisites remain part of real production startup work.
