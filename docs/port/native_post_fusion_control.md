# Independent original two-pass control for fusion validation

The SDK477 actual first paired frame rejected the candidate: root reports
2304000 compared pixels,10892 differing pixels,508 alpha mismatches and RGB max4.
Mode2 was not enabled; original rendering continued and the criteria were not
loosened. This helper revision adds an independent control before attributing
that difference to shader fusion or UNORM rounding.

Mode1 now additionally renders the **exact original two pixel shaders** offscreen
from the same frozen inputs. It uses their captured film/color constants,
samplers, shared vertex shader, half-texel matrix, viewport and identical quad
indices. Bloom writes a format/dimension-matched UNORM producer; genuine
StretchRect copies it to a matched texture; original color samples that texture
into a separate matched output. This keeps the original hardware storage/read
boundary, rather than emulating its quantization in arithmetic.

Both original-control and fused draws use the existing bounded StateScope:
retain/capture every MRT and depth surface, restore them explicitly and apply the
full stateblock last. Direct diagnostic draws do not alter canonical cache values.
Original on-screen output is copied before either diagnostic draw. Persistent
original shader objects and temporary control textures/surfaces have ordinary
COM ownership; lifecycle Release destroys the shaders alongside the existing
derived owner. No provider ABI, shader asset, Vulkan version or engine source is
changed by this helper subtask.

The control output must match the real captured original **exactly in RGBA**.
Its separate comparison record includes differing pixels, maxRGB, maxAlpha,
alpha mismatch count and the full absolute-difference histogram. If the control
fails, fusion differences are logged as diagnostic-only and validation stops
with `original_control_not_exact`. They cannot approve mode2 or establish a
quantization explanation. Only after an exact control can the unchanged fusion
criterion (alpha exact, RGB max at most1 across three qualified pairs) approve.

The first failed frame can produce exactly three diagnostic PAM files containing
explicit RGBA bytes: original, original-control and fused. They are written
under the actual Sys_EngineUserRoot with one shared capture identifier, exclusive
creation and no overwrite. There is no recurring per-frame output or automatic
retry on dump failure. A8R8G8B8 readback bytes are swizzled from BGRA to RGBA;
A8B8G8R8 bytes already have RGBA order. Paths and completion status are logged so
root can retrieve that bounded evidence and create difference heatmaps. File
failure does not fake comparison success or modify original output.

## Verification and evidence

Prior helper/header/bytecode/report bytes are archived verbatim with hashes at
`evidence/native_post_fusion_control1`; the prior helper hash is
`1d7f72df5d9726f4f5d8a7c1bc77f715e82dc7594386c2f4ca6fb89eb23d7183`.
The original and derived bytecode arrays are unchanged in this revision.

Normal Android helper ON/OFF compilations pass, preserving canonical debug,
assertions, UBSan and instrumentation flags and the existing legacy-header
warnings. Focused extracted-production Ref/StateScope ownership tests pass
compile/run, including partial capture cleanup and restoration ordering; an
injected restoration failure explicitly aborts with exit3. Windows CRT fault
reporting initially held that host-only injected abort open; a fixture-only
setting suppresses its dialog. The interruption and repair are retained in
`ownership-initial-host-fault-dialog.json`, and no production abort policy changed.

The updated extracted-production ScanRGBA fixture passes full RGBA/pitched-row
histogram checks, RGB acceptance boundaries, maxAlpha/count behavior and invalid
pointer/pitch rejection. Exact commands/logs/results and final source hashes are
in the evidence directory. An independent read-only canonical review found no
concrete control-layout/ownership defect. Actual tablet control execution remains
root's next verification, not a host-fixture claim.

Hardware UNORM rounding, slight interpolation around texel centers and arithmetic
contraction remain hypotheses for the first failed fusion comparison. DXVK1.9.2
decodes the original partial-precision modifier but the inspected shader compiler
does not use that flag for a separate precision mode; MAD translation can use
SPIR-V Fma unless longMad is selected. Those source observations do not prove the
observed failure cause. Preserve the exact control gate and alpha criterion while
diagnosing the actual readbacks. Validation work must be excluded from performance
samples; no optimization gain or30FPS is claimed by this revision.
