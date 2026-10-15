# Native Vulkan: vertex format mapping review

2026-10-14. Read-only review of how the canonical material vertex routing maps D3D9
`D3DDECLTYPE` values to Vulkan formats. No source, CMake or ledger edits. Nothing was built or
run on a device by this review; the type-usage scan below was done by a script over the table
text, and everything else is from reading code.

## Result

1. A `D3DDECLTYPE` -> `VkFormat` table already exists inline in
   `src/gfx_vulkan/vertex_assets.cpp:14-18` (with a matching size table at line 18). Every entry
   is consistent with D3D9 semantics (checked below), so a second table would only duplicate it
   and risk drift. None was added.
2. The canonical routing never uses `UDEC3` (13) or `DEC3N` (14). The "Packed UDEC3/DEC3N requires
   native shader decoding" error at `vertex_assets.cpp:54` is therefore unreachable for the
   engine's own vertex declarations. Failing explicitly there is the right behavior if a future
   source ever adds one.
3. The CPU decoders in `packed_vertex_decode.*` are consequently a specification/oracle aid, not a
   runtime dependency of material rendering.

## What the engine actually routes

`s_streamSourceInfo[18][10]` (`r_material.cpp:29-247`) is the only source table feeding
`Material_BuildVertexDecl` (`r_material.cpp:459`) and the native `vertex_assets::Create`
(`r_material_shader_load.inl:128`). A script over all 18 rows (180 entries) found these element
types. "Slots" are source-slot indices 0-9 and "streams" the vertex stream numbers they use;
which slot feeds which semantic depends on each material's routing, which this review did not
trace.

| Type | D3D name | Source slots | Streams | Offsets seen |
|---|---|---|---|---|
| 1 | FLOAT2 | 2, 5, 6 | 0, 1 | 0, 12, 16, 20 |
| 2 | FLOAT3 | 0 (row 15 only) | 0 | 0 |
| 3 | FLOAT4 | 0, 2, 5, 6 | 0, 1 | 0, 16, 20 |
| 4 | D3DCOLOR | 1 | 0 | 8, 12, 16 |
| 5 | UBYTE4 | 0, 2, 3, 4, 5, 7 | 0, 1 | 0, 4, 12, 16, 20, 24, 28, 36, 40 |
| 8 | UBYTE4N | 7, 8 | 1 | 8, 16, 20, 24, 28, 32, 36 |
| 10 | SHORT4N | 0 (row 17 only) | 0 | 0 |
| 15 | FLOAT16_2 | 9 | 2 | 0 (rows 2-13) |

Never used: 0 (FLOAT1), 6 (SHORT2), 7 (SHORT4), 9 (SHORT2N), 11 (USHORT2N), 12 (USHORT4N),
13 (UDEC3), 14 (DEC3N), 16 (FLOAT16_4). The scan also found no element whose offset is
misaligned to its component size.

Streams per row: rows 0-1, 14, 15 and 17 use stream 0 only; row 16 uses streams 0-1; row 2 uses streams 0 and 2; rows 3-13 use streams 0, 1 and 2. The largest stream-0 extent is 44 bytes (rows 2-13) and stream 2 is always the 4-byte FLOAT16_2 element.

## Existing mapping vs D3D9 behavior

`vertex_assets.cpp` maps as follows. "Fetch" is what a Vulkan vertex fetch delivers to the shader
input; "D3D9" is the documented expansion.

| Type | `VkFormat` in the table | Fetch vs D3D9 |
|---|---|---|
| 0 FLOAT1 | `R32_SFLOAT` | (x,0,0,1) both |
| 1 FLOAT2 | `R32G32_SFLOAT` | (x,y,0,1) both |
| 2 FLOAT3 | `R32G32B32_SFLOAT` | (x,y,z,1) both |
| 3 FLOAT4 | `R32G32B32A32_SFLOAT` | identical |
| 4 D3DCOLOR | `B8G8R8A8_UNORM` | byte order B,G,R,A in memory; fetch delivers (r,g,b,a) = D3D9 expansion |
| 5 UBYTE4 | `R8G8B8A8_USCALED` | integer value as float, 0..255, not normalized: matches D3D9 UBYTE4 |
| 6 SHORT2 | `R16G16_SSCALED` | (x,y,0,1) as floats |
| 7 SHORT4 | `R16G16B16A16_SSCALED` | as floats |
| 8 UBYTE4N | `R8G8B8A8_UNORM` | 0..1 |
| 9 SHORT2N | `R16G16_SNORM` | see note 1 |
| 10 SHORT4N | `R16G16B16A16_SNORM` | see note 1 |
| 11 USHORT2N | `R16G16_UNORM` | 0..1 |
| 12 USHORT4N | `R16G16B16A16_UNORM` | 0..1 |
| 13 UDEC3 | `UNDEFINED` | unsupported, explicit error |
| 14 DEC3N | `UNDEFINED` | unsupported, explicit error |
| 15 FLOAT16_2 | `R16G16_SFLOAT` | (x,y,0,1) |
| 16 FLOAT16_4 | `R16G16B16A16_SFLOAT` | identical |

The size table `{4,8,12,16,4,4,4,8,4,4,8,4,8,4,4,4,8}` matches each format's byte size for all 17
entries. Missing components default to (0,0,0,1) in both D3D9 and Vulkan vertex fetch, so partial
formats such as FLOAT2 and FLOAT3 agree.

Note 1: Vulkan SNORM maps -32768 to -1.0. I did not verify what D3D9 hardware does for -32768
(it may differ by a tiny amount); the engine does not route SHORT2N and routes SHORT4N only in
row 17, so the effect, if any, is confined there.

The runtime guard at `vertex_assets.cpp:58-59` checks `VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT` on the
physical device for every routed format. I did not check this against the Vulkan required-format
list or the Adreno 610 reported properties; the check exists precisely so a device lacking a format
fails with an explicit error.

## If UDEC3 / DEC3N is ever needed

Direct fetch would be possible but not equivalent without a shader fix:
- Vulkan's `A2B10G10R10_*_PACK32` formats place the first component in bits 0-9, the second in
  10-19 and the third in 20-29, matching the D3D9 layout used by `DecodeUDec3` /
  `DecodeDec3N`. `A2B10G10R10_USCALED_PACK32` would match UDEC3 for x/y/z;
  `A2B10G10R10_SNORM_PACK32` would match DEC3N including -512 -> -1.0.
- The fourth component would differ. D3D9 expands W to 1.0; a Vulkan fetch delivers the 2-bit W
  field (0..3 scaled, or the SNORM interpretation of 2 bits). The translated shader or the vertex
  data would have to force W to 1.0. That is why this is not a drop-in format substitution.
- Until a real route needs it, keep the explicit error. Do not map to a lookalike format.

## What remains unproven

- No per-format GPU fetch test. Existing fixtures draw a captured T5 material; they do not
  exercise each of the eight routed types separately, so a device-side check of the
  `USCALED`/`UNORM`/`SFLOAT16` fetches against known bytes would add real evidence.
- The `VERTEX_BUFFER` feature check was not compared with the device's reported properties.
- Behavior of UBYTE4 consumers (the engine feeds packed unit vectors through type 5) depends on
  the translated shader unpacking floats in 0..255; that path was not traced here.

## Suggested follow-ups (not done)
1. Optional: a tiny tablet fixture that uploads known bytes for each of types 1, 2, 3, 4, 5, 8, 10
   and 15, fetches them through the real `Resolve` path, and compares against expected floats.
2. Optional: move the `formats`/`sizes` arrays into a header with `static_assert`s on the
   `D3DDECLTYPE` indices so a future edit cannot desynchronize them. This touches
   `vertex_assets.cpp`, so it is left to you.
