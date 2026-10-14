from pathlib import Path
import json,hashlib
root=Path.cwd();out=Path(__file__).parent;p=root/'docs/port/native_post_shader_fusion_audit.md';before=p.read_bytes()
archive=out/'native_post_shader_fusion_audit.before4.md'
if archive.exists(): assert archive.read_bytes()==before
else: archive.write_bytes(before)
text=before.decode('utf8');old='The color shader samples its intermediate only once. This is strong evidence\nfor a local fusion candidate, but the two passes have different vertex shader\npointers; equal pixel-shader input names do not establish equal interpolants.'
new='The color shader samples its intermediate only once. Snapshot4 subsequently\nproves the two differently named/pointered vertex shaders are byte-identical,\nwith identical position and UV operations; the details and remaining runtime\nconditions are below.'
assert old in text;text=text.replace(old,new,1)
text+='''

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
'''
temp=out/'report4.tmp.md';temp.write_bytes(text.encode('utf8'));data=temp.read_bytes();assert data.decode('utf8')==text;p.write_bytes(data)
(out/'report4-manifest.json').write_text(json.dumps({'before_sha256':hashlib.sha256(before).hexdigest(),'after_sha256':hashlib.sha256(data).hexdigest(),'before_bytes':len(before),'after_bytes':len(data)},indent=2),encoding='utf8')
print((out/'report4-manifest.json').read_text())
