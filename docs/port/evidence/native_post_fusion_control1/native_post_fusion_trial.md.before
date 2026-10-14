# Experimental native bloom/color fusion trial

The new Android helper provides a default-original, optional D3D9 shader fusion
trial. It uses the frozen selected retail bloom/color shader programs as gates
and an independently owned derived PS3 program. Retail shaders, materials, assets,
DXVK and SDL are not changed. No native Vulkan/private provider ABI is used.

Only three new source files belong to this helper subtask:
`post_fusion_trial.cpp`, `post_fusion_trial.h` and
`post_fusion_trial_bytecode.h` in `src/platform/android`. Root owns CMake,
canonical hook placement, installation and actual device verification.

With `BO1_NATIVE_POST_FUSION_TRIAL=0`, APIs do nothing, Try returns false and
ObservePass returns true. With the helper enabled, the canonical int dvar
`r_nativeBloomFusion` defaults to0, with these explicit experimental modes:

- 0: original pipeline; rearms a later validation request.
- 1: up to three bounded paired GPU comparisons, stopping immediately on any
  incomplete or rejected result. Original on-screen rendering continues.
- 2: derived shader only after three successful pairs, for the same qualified
  fingerprint. Any incompatible case falls back to the original pipeline.

## Hook contract

`RegisterControls()` runs on the main thread at canonical renderer dvar setup.
`TryBloomColor(view)` runs immediately before the original bloomApplyLDR filter,
after film controls and code images34/35 are ready. A true result replaces the
original bloom-apply/resolve/color tail; a false result executes it normally.
The gate requires RB_UsingPostEffects(view), so the caller's immediate genuine
RB_GetResolvedScene publishes the final scene before a subsequent shader consumes
code image10. There is no manufactured intermediate publication.

`ObservePass(device,material,passIndex)` runs immediately before the actual draw,
after ALL pass arguments and render states have been applied. **The root hook
must condition the draw on its bool return.** Original material draws always
return true, including unsupported/incomplete validation. Only the helper's
owned fused material can return false, suppressing that experimental draw.
Try then restores source controls and returns false to execute the canonical
original tail. No skipped original draw is counted as successful fusion.

`EndOriginalColor(view)` runs after the original color filter's actual draw.
`Release()` runs at the canonical pre-reset/pre-device-release lifecycle owner,
including shutdown. Overlay visibility changes do not release this owner.

## Runtime qualification and ownership

Exact original pixel/vertex GetFunction bytes, selected names, one-pass technique,
arguments and state words must match the two captured programs. The trial is
restricted to a full-screen, full-viewport scene, no state override and matching
UNORM8 source/output sizes with no MSAA. Code-image render-target overrides must
be inactive. Original source texture COM identities are retained independently.

At actual original bloom/color draw boundaries, the helper reads the applied
PS/VS, relevant pixel constants, four VS matrix rows, textures, sampler states,
viewport/scissor and blend/alpha/depth/stencil/color-write/sRGB states. It verifies
the exact canonical transposed half-texel matrix and original image bindings.
Sampling uses base level only: mip filter NONE, mip bias0, max mip0; color and
bloom sampling must have no sRGB decode. Output sRGB writes are disabled. Both
original stages use RGBA writes with blend/alpha/depth/stencil effects disabled.

Mode1 copies both original inputs into owned format/dimension-matched render-target
textures before the original resolve overwrites the scene input. It preserves the
actual original final scene in another owned render target, draws the derived
shader into a separate matched target, then reads back both complete images.
The derived draw uses the captured matrix and shader constants, an owned matching
VS and declaration, captured bloom samplers and original geometry/indices.

All direct trial device mutations are enclosed in D3DSBT_ALL capture/apply plus
explicit retained restoration of every supported MRT and the depth surface.
Depth/MRT restoration precedes stateblock Apply, which restores the viewport and
stream/shader/constant/sampler states after SetRenderTarget's side effects.
Canonical engine caches are not mutated by that direct offscreen draw.
Unexpected restoration or unlock failure logs the operation and aborts explicitly.
Unsupported capture/draw/readback reports incomplete, releases temporary objects
and leaves original rendering active. No custom Vulkan fence/queue wait is added;
the actual D3D9 readback is deliberately bounded verification work.

Every RGBA byte is compared, with a complete absolute-difference histogram.
Acceptance requires alpha exactly equal and RGB max difference at most one code
value across all pixels of all three pairs, plus identical film/color controls
across the pairs. This is an explicit conservative visual trial criterion,
**not a claim of general bitwise equivalence**. The half-up UNORM emulation is
never assumed correct without those actual comparisons.

Mode2 retains a strict device, target-format/size/MSAA, source identity, code
sampler and film/color fingerprint. It captures film121..123 before canonical
color setup overwrites121..131, binds film separately as stable literal pixel
constants64..66 and uses the original color mapping. The material/technique/pass/
arguments/PS wrapper are owner-lifetime clones; original metadata is immutable.
Type7 film literals follow all type5 code pixel constants in the stable tail.

After canonical pass setup but before any fused draw, the bool hook additionally
checks the actual applied fused PS/shared VS, original bound textures, complete
sampler fingerprint, matrix/viewport/scissor, render states and all relevant
color/film constants. A mismatch suppresses only the experimental draw. Try
requires exactly one allowed fused draw; otherwise it restores all11 overwritten
source controls through R_UpdateCodeConstantFromVec4 and returns original fallback.
These updates deliberately refresh canonical constant versions for reupload.

Temporary surfaces/textures/stateblocks are released on every completed or failed
attempt. Two source texture references and persistent derived shader/VS/declaration
remain owned until lifecycle Release, which also invalidates validation approval.

## Verification and limits

Evidence is `docs/port/evidence/native_post_fusion_trial1`, including frozen
bytecode sources/hashes, exact normal Android ON/OFF compile commands and logs,
and focused production-code extraction fixtures. ON and OFF compile successfully;
ON retains normal legacy header warnings, debug/assert/UBSan/instrumentation flags.
No global renderer optimization flag is introduced by this helper.

The Ref/StateScope fixture passes MSVC /W4 /WX compilation and execution: exact
reference ownership, partial capture cleanup, all MRT/depth restoration order,
stateblock Apply last. Injected target restoration failure produces the expected
explicit abort (exit3). The actual ScanRGBA fixture passes pitched-row/full-RGBA
histogram checks, RGB one/two-value boundary, alpha mismatch and invalid readback
pointer/pitch rejection.

No device acceptance, speedup or30FPS is claimed by these host/native fixtures.
Root must observe mode1's real accepted/rejected/incomplete result and only then
measure a stable mode2/original A/B/A. Validation frames contain copies, readbacks
and extra GPU work and must be excluded from performance samples. Mode2 also
performs conservative boundary checks, whose overhead belongs to trial measurements.
Future adoption would require broader changing-scene/film testing and an evidence-
based reduction of checks. Original fallback remains available.
