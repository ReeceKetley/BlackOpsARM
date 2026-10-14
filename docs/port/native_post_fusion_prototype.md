# Offline Bloom/color shader fusion prototype

2026-10-14. This is an assembled derived shader candidate, **not an adopted
engine change or a GPU performance/equivalence result**. No engine/CMake,
protected DXVK, original assets, APK, device or Ledger mutation was performed.
Evidence is in [native_post_fusion_prototype1](evidence/native_post_fusion_prototype1).

## Inputs and construction

The two actual SDK476 selected pixel shader captures are from
`build-android-audit/native_registry_closure/sdk476-post-shaders3`:
`bloomApplyLDR-pass0.ps.bin` and `postFxColorMaterial-pass0.ps.bin`.
Their existing Microsoft disassemblies, and the common byte-identical VS
captures from snapshot4, are recorded in
[native_post_shader_fusion_audit.md](native_post_shader_fusion_audit.md).
The prototype manifest records hashes and byte sizes of all four originals.

The source generator retains every original Bloom instruction in its original
order, including `_pp` modifiers, with these register substitutions only:

- Bloom's embedded `def c0,64,8,0,0` becomes `c1`; color's original `c0` remains.
- Bloom dynamic film constants `c5,c6,c7` become `c64,c65,c66`. These require
  canonical constants121..123 captured **before** color correction overwrites
  them. Color dynamic `c5..11,c20..23` bindings remain unchanged.
- Bloom output `oC0` becomes a dedicated `r5`, preserving RGB and scene alpha.
- Color's initial `texld_pp r0,v0,s0` becomes `mov_pp r0,r5`. Every remaining
  color instruction and modifier stays in its original order.

Only the original Bloom inputs are sampled: `s0` is code image35, the final
blurred Bloom image; `s1` is code image34, the original resolved scene. Both
texture instructions and the original shared `v0.xy` UV are retained. The common
VS and its matrix binding do not change. None of the preceding Bloom downsample,
blur or streak passes is fused or removed by this candidate.

No local constant collisions are present. Shader model3 allows the selected
registers; highest dynamic constant is66. Raw output uses temporaries through
r5, the quantized variant through r7. The assembled/disassembled instruction
order and register references, including modifiers, are checked against the
generated source. Source literals are independently recorded by the assembler
in the disassembly; the round-trip check permits literal formatting changes.

## Actual offline assembly result

`assemble_prototype.py` calls Windows System32 `d3dcompiler_47.dll`'s actual
`D3DAssemble`, then `D3DDisassemble`, with flags0. Its eight-argument prototype
was checked against the [Wine compiler implementation](https://github.com/wine-mirror/wine/blob/master/dlls/d3dcompiler_43/compiler.c).
The installed Windows SDK header documents the five-argument disassembler;
the private assembler export is absent from that public header. The manifest
records the DLL hash. The generated binary has actual SM3 version token
`0xffff0300` and final END token `0x0000ffff`.

| Variant | Assembly/disassembly HRESULT | Bytes | Executable instructions | Texture instructions | Highest temporary |
|---|---|---:|---:|---:|---:|
| Raw, no intermediate conversion | 0 / 0 | 712 | 38 | 2 | r5 |
| Saturated UNORM8 half-up candidate | 0 / 0 | 812 | 43 | 2 | r7 |

Binary SHA256:

- Raw: `f09dc0f6d194afd384c304917eb4dfdb99efcdc0b34be8a772bb4d0a6a72d529`
- UNORM8 candidate: `e079be6101e0646afa0a6c82deb0ffd6766a4d49427e7cc29b4c7deebcd5bc2f`

Generated sources, binary programs, actual round-trip disassemblies, diagnostics,
source/asset hashes and the manifest are preserved. Running the generator is
an offline Windows operation, not Android shader creation or execution.

## Intermediate conversion remains a real obstacle

The original two draws have an intervening render-target write/copy/texture
sample. That boundary clamps/quantizes and may encode/decode sRGB. The raw
variant intentionally omits this conversion and is only a diagnostic candidate.
It must not be described as equivalent.

The second variant explicitly implements saturated RGBA conversion using
`floor(f32(255*x+0.5))*f32(1/255)` with `mov_sat`, `mad`, `frc`, `add`, and `mul`.
It assumes an8-bit UNORM intermediate, half-up rounding and no sRGB transfer.
These are candidate assumptions, not measured driver behavior. Added arithmetic
is full precision; the original partial-precision modifiers remain.

`quantization_boundaries.py` performs host f32 diagnostics over the three
adjacent float32 inputs around each of255 half steps, and all256 decode values.
It preserves all765 inputs and all256 outputs in JSON. Endpoint saturation
checks pass. However:

- The candidate's f32 rounding produces a different integer from ideal exact
  half-up conversion at128 of765 boundary inputs. For example
  `x=0.5039215683937073` yields129, while exact half-up conversion yields128.
- The tested fused versus separately rounded multiply/add paths coincide for
  these765 inputs. This limited observation does not establish driver MAD or
  `_pp` semantics elsewhere.
- Multiplication by a float32 reciprocal differs from correctly rounded
  `n/255` for126 of256 normalized decode values; maximum absolute difference
  is `5.960464477539063e-08` (one float32 ULP in those cases).

These diagnostics **rule out a claim that the formula is an exact abstract
UNORM8 conversion for every float32 input**. They do not identify the tablet's
actual conversion implementation. A one-code-value threshold discrepancy can
be more consequential than the tiny decode ULP difference. Actual paired GPU
outputs remain required. No fabricated film constants were used to claim full
scene numerical equivalence.

## Unimplemented alternative retaining hardware conversion

A Vulkan render pass with two subpasses and an8-bit UNORM intermediate input
attachment could retain the actual hardware write conversion between Bloom
application and color correction. Pixel-local input reads could avoid the
external copy while preserving that intermediate format. This is a separate,
unimplemented architectural candidate, requiring a new engine-facing D3D9
bridge; the existing prototype does not provide it. It does not guarantee tile
memory residency or a GPU saving. Query actual input-attachment/device limits,
format/usage compatibility, prove pixel-center sampling equivalence and sRGB
behavior, and implement ordered submission/resource lifetime/reset retirement
before adoption. Provider and Vulkan-version constraints still apply.

## Remaining production gates

1. Capture actual Bloom/color draw-boundary sampler states, input/intermediate
   formats/sizes/sample counts, sRGB read/write states, geometry/viewport/matrix
   and texel-center alignment. The recorded later UI source snapshot is not
   valid for these facts.
2. Capture film constants121..123 before overwrite and color constants121..131
   afterwards. A production binding owner must retain both sets independently.
3. Demonstrate the actual intermediate conversion, alpha behavior and shader
   precision/rounding through paired original/candidate render outputs on the
   unchanged Adreno610/Vulkan1.1.128 provider. Cover gradients, dark/bright
   thresholds, saturation, alpha and changing vision settings. Preserve fallback
   for split-screen/overridden/unsupported state.
4. Implement bounded derived-shader ownership and retirement if a candidate is
   adopted; original retail shader bytes must remain untouched. Offline assembly
   success does not prove `CreatePixelShader` or device/reset lifetime behavior.
5. Measure selected passes and total frame time with matching scene/settings.
   At most this candidate targets the selected Bloom application, its interior
   copy and color filter; **the entire measured17ms Bloom group is not a saving
   estimate**. Fusion changes shader register pressure/compilation and may shift
   synchronization cost, so no FPS gain is claimed.

Reproduce only this bounded offline work:

```
python docs/port/evidence/native_post_fusion_prototype1/assemble_prototype.py
python docs/port/evidence/native_post_fusion_prototype1/quantization_boundaries.py
```
