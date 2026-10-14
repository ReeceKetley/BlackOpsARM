# Selected bloom/color shader fusion audit

Read-only audit, 2026-10-14. Root captured these two actual loaded shaders from
the SDK476 observation using the SDK475 BuildId beginning `4fd`, PID29221;
the capture metadata reports detached debugger/TracerPid0. This audit performed
only offline disassembly. It does not claim a fused shader or performance gain.

Windows `d3dcompiler_47.dll!D3DDisassemble` returned HRESULT0 for both exact PS3
programs. UTF8 disassemblies, their hashes and the reproducible ctypes script
are in `evidence/native_post_shader_fusion1`. Original captured bytes were read
from `build-android-audit/native_registry_closure/sdk476-post-shaders3` and remain
unchanged.

| Actual shader | Bytes | SHA256 | Texture instructions | Approximate instruction slots |
|---|---:|---|---:|---:|
| bloomApplyLDR pass0 | 616 | b92ae0c0e1a0c1e0761c18fdd70721aa7d9ac5cf052e1cf2588699711828b873 | 2 | 15 |
| postFxColorMaterial pass0 | 980 | dfec7de254c962ad7a8918761bd60e081da98ca0f5335197f25b09469a041775 | 1 | 23 |

Both selected techniques have one pass. Neither selected pixel shader has
control flow, a texture-coordinate offset, neighboring texture taps, derivatives,
a dependent texture lookup or a color LUT. Both use interpolated `v0.xy`.
The color shader samples its intermediate only once. This is strong evidence
for a local fusion candidate, but the two passes have different vertex shader
pointers; equal pixel-shader input names do not establish equal interpolants.

## Exact shader dependencies

Bloom declares 2D `s0=bloomSampler`, `s1=colorSampler`. Both samples use `v0`.
Its external constants are `c5=postFxControl0`, `c6=postFxControl1`,
`c7=postFxControl2`; its local `def c0` is `(64,8,0,0)`.

For sampled scene color C and bloom B, its RGB expression follows this operation
sequence, componentwise:

```
X = 8 * (C.rgb * C.rgb) + 64 * B.rgb
denominator = X * (X * c5.x + c5.y) + c5.z
numerator   = X * (X * c6.x + c6.y) + c6.z
output.rgb  = (numerator * reciprocal(denominator) + c7.x) * c7.y
output.a    = C.a
```

There is no shader saturation on the final bloom output; render-target storage
can still clamp, encode and quantize it. `RB_SetFilmCurveConstants` supplies the
film curve through source constants121..123 immediately before this pass.

Color declares only 2D `s0=colorMapSampler`. External constants are
`c5..c11,c20..c23` (11 float4 registers), corresponding to
`postFxControl0..postFxControlA`. Its local `def c0` is `(0.25,0.5,1,0)`.

Let Q be its sampled intermediate, L=`0.25*Q.r + 0.5*Q.g + 0.25*Q.b`,
and K=`1-L`. It constructs three RGB vectors by interpolating Q.rgb toward K
using c22.x/y/z. Each is extended with w=1, multiplied by a separate 3x4 matrix,
and saturated. Matrix rows are c5/6/7, c8/9/10 and c11/20/21. The final RGB
interpolates the first two matrix outputs with
`saturate(L*c23.x+c23.y)`, then interpolates toward the third using
`saturate(L*c23.z+c23.w)`. Alpha is Q.a throughout.

The precise assembly, including instruction ordering and `_pp` modifiers,
is the reference; this prose algebra must not justify reassociation, reduced
precision or fast-math changes.

## Fusion feasibility and conditions

`RB_BloomLDR` applies bloom into SCENE, copies it to RESOLVED_SCENE with
`R_Resolve`, sets code image10 to the resolved intermediate, then applies color
back into SCENE. A fused shader can evaluate bloom and color with two original
texture samples rather than three samples across two full-size draws, removing
the intervening copy. This preserves bloom and color effects, conditional on the
following proof rather than simply feeding the unrounded bloom value into color:

- Match the two vertex shader coordinate transformations, fullscreen/split-screen
  geometry, viewport and half-texel conventions. Establish that the color sample
  hits the exact corresponding intermediate texel. If it performs fractional
  filtering, reproducing only one producer value is insufficient even though
  its pixel shader contains one `texld` instruction.
- Preserve the intermediate storage format, clamp/quantization, channel mapping,
  sRGB write/read behavior and sampling. If it is UNORM, quantization must occur
  inside the fused shader at the removed storage boundary. Its actual rounding
  behavior needs comparison, not an unverified assumed rounding expression.
- Preserve blend, color-write mask, alpha-test and depth/stencil behavior from
  both selected material states. Destination-dependent blending would invalidate
  simple composition. Preserve partial viewport and other effect paths.
- **Remap constants.** Bloom and color use the same c5..7 register numbers with
  different phase values. `RB_SetVisionSetColorCorrection` overwrites the source
  postfx controls after bloom. Capture the three film values before this call,
  then provide them in distinct fused registers. Also remap one shader's local
  c0 definition, whose literal values differ. Do not overwrite canonical values
  or reuse the second phase's constants for the first phase.
- Preserve resource identity and downstream output publication. A later resolve
  or DOF phase still needs the same resulting scene. Keep original immutable
  materials/shaders and an explicit fallback for unsupported conditions.

Existing D3D9 `CreatePixelShader` and canonical material drawing provide the
smallest implementation route: create an Android-owned derived SM3 program and
matching argument bindings, then choose it for the proved compatible case.
The two short programs present no obvious instruction-budget problem, though
the actual fused program must compile and its runtime limits must be checked.
No native Vulkan bridge is needed for this route.

Direct Vulkan remains additional work. Protected DXVK1.9.2 implements
`IDXGIVkInteropDevice` and `IDXGIVkInteropSurface` for D3D11/DXGI, but D3D9
device/surface QueryInterface rejects them. Internal provider image/device access
is not an engine-facing ABI. A new narrow D3D9 bridge in an ARM-owned provider
copy would need ordered submission, barriers, resource retention, state restoration
and reset/shutdown retirement. Ordinary D3D9 color targets generally lack storage
usage; DXVK compute-state commitment also spills its graphics render pass. Native
compute does not automatically save the identified draw/copy or preserve quality.
This shader-fusion candidate can be tested with the current Vulkan1.1.128 baseline.

## Next bounded capture

Capture only these two selected passes at their actual draw boundary:

1. All six bloom MaterialShaderArgument records and all13 color records, including
   type/dest and the correct union member; follow literal constants where present.
   This identifies real code-image and source-constant mappings rather than relying
   only on shader parameter names. Include the selected stateBitsTable[0] words
   and their decoded blend/write/depth/stencil/sRGB states.
2. The two loaded vertex shader bytecode programs and their pass argument bindings.
   Verify transformed UVs and geometry/viewport conventions without scanning any
   other shader assets.
3. Actual sampler states and bound image identities/formats/dimensions at bloom
   s0/s1 and color s0; source and resolved render-target format/sample count;
   D3DRS_SRGBWRITEENABLE and each D3DSAMP_SRGBTEXTURE value.
4. Snapshot actual pixel constants c5..7 at the bloom draw and
   c5..11,c20..23 at the color draw, plus relevant viewport/scene dimensions.
   Static material arguments alone cannot recover dynamic film settings.

Once compatible, verify paired original/fused outputs for dark gradients, bright
bloom edges, saturation, alpha, changing film/color parameters and the actual
room viewport, then measure the bloom group and whole-frame span. The original
full17ms bloom group is not a saving estimate for these two selected passes.
