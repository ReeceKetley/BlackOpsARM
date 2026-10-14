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
The color shader samples its intermediate only once. Snapshot4 subsequently
proves the two differently named/pointered vertex shaders are byte-identical,
with identical position and UV operations; the details and remaining runtime
conditions are below.

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


## Snapshot4: UV program, argument and static-state proof

Root's bounded SDK476 snapshot4 retained the same selected PS bytes and captured
both VS programs, all selected arguments and both state tables. Offline
`D3DDisassemble` returned HRESULT0. Both VS files are byte-identical: 336 bytes,
SHA256 `bb7360f2413c64e5c03907a45ed422be1dfdeddb2ea5910732470d22386a19cd`.
The names differ, but their instructions and embedded constants are identical.
Their six instructions construct position `(v0.xyz,1)`, transform it with
c0..3, and copy input texcoord v1.xy directly to output o1.xy. Both selected
passes bind the same VS matrix: argument type3, dest0, code constant219,
firstRow0, rowCount4. Thus **the UV programs are equivalent**, with no shader
offset/remap between these two passes. Equal geometry, matrix and viewport
produce equal interpolated UVs. This does not alone prove texel-center sampling
or runtime sampler/filter state.

Actual argument records resolve all selected bindings:

| Pass | Binding | Canonical source |
|---|---|---|
| Bloom | VS c0..3 | constant219, rows0..3 |
| Bloom | PS s0 | code image35 (0x23), bloom input selected by RB_BloomLDR |
| Bloom | PS s1 | code image34 (0x22), original RESOLVED_SCENE |
| Bloom | PS c5,c6,c7 | constants121,122,123 (one row each) |
| Color | VS c0..3 | constant219, rows0..3 |
| Color | PS s0 | code image10 (0x0a), newly resolved bloom result |
| Color | PS c5..11 | constants121..127 (one row each) |
| Color | PS c20..23 | constants128..131 (one row each) |

There are exactly six bloom arguments (one per-primitive and five stable) and
13 color arguments (one per-primitive and12 stable); no material literal or named
texture/constant tables are involved. Argument type4 is a code pixel sampler;
type5 is a code pixel constant. Union fields printed as fake-looking literal
pointers are alternate interpretations, not pointers to dereference for these
types. This snapshot proves mappings, not the dynamic values at each draw.

Both selected static state tables are identical:
`loadBits[0]=403867666 (0x18128812)` and
`loadBits[1]=3759013890 (0xe00e0002)`. Canonical `R_SetRenderState_0/1` and
`R_HW_Set*` decode the relevant fields as follows: RGB blending disabled
(word0 &0x700=0), all RGBA color writes enabled (word0 &0x18000000), alpha
test disabled (word0 &0x800), solid fill, depth writes disabled (word1 &1=0),
depth test disabled (word1 &2), stencil disabled (word1 &0x40=0). These static
material states therefore introduce no destination-blending obstacle to
composition. They do not prove the applied runtime state when overrides are
active, and they do not encode a proof of sRGB read/write settings.

The report's earlier requests for material arguments, static words and VS
bytecode are satisfied by snapshot4. Remaining gates are bounded:

- Actual input/output/intermediate formats, sizes and samples; code sampler
 35/34/10 filter/address/mip settings at their own pass; actual sRGB texture-read
 and render-target-write state.
- Same geometry/matrix/viewport and pixel-center alignment for the two draws.
 Fullscreen canonical filters use the same UV0..1 geometry and the same target
 dimensions here; retain fallback for unsupported/split/overridden cases until
 their corresponding conditions are proven.
- Film values121..123 before their overwrite, and color values121..131 after
 `RB_SetVisionSetColorCorrection`; preserve both in independent derived shader
 bindings. Static mapping does not authorize reading film from post-frame state.
- Intermediate quantization/clamping/encoding and actual paired output comparison.

The captured `current_source_snapshot` is a later mutable source state, containing
UI-era values such as `(6.28318548,0,1,0)` in121..124. It is explicitly excluded
as a bloom or color draw-boundary constant/sampler proof. No additional device
action was performed by this audit. Root captured and detached the debugger.
The pre-amendment report is archived verbatim with its hash in this evidence
directory; new disassemblies and `manifest4.json` preserve VS proof.
