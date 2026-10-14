# Native shader-model3 D3DX semantics

2026-10-13. Actual Com_Init remains incomplete. D3DX input/output semantics now
have real implementations for the original loader's requested vs_3_0/ps_3_0
profiles. Constant-table extraction and HLSL compilation remain unimplemented.
No new unavailable endpoints or empty shader/metadata success paths were added.

The CPU parser reads shader stage/version, encoded instruction/comment lengths,
declaration usage/index and register types. Vertex declarations preserve order.
Pixel output register references become ascending color indices then depth,
deduplicated; comments and constant-definition literals do not become registers.
Count-only queries use the same parsed metadata. Shader pointers require valid
END-terminated readable bytecode, as the unsized SDK interface supplies no buffer
extent. A word-count scan ceiling does not prove safety for truncated pointers.
Older shader models explicitly return E_NOTIMPL instead of claiming no semantics.
Other profiles and real retail shader coverage are not claimed validated.

SDK reference: installed June2010 d3dx9shader.h/d3d9types.h. Upstream
[Wine shader services](https://github.com/wine-mirror/wine/blob/master/dlls/d3dx9_36/shader.c)
were consulted for the token interface, but observed native Microsoft DLL output
is the oracle for this fixture, including pixel COLOROUT behavior.

Actual Windows x64 D3DX9_43 in d3dx-semantics-windows2 returns:
VS inputs POSITION0/TEXCOORD1, outputs POSITION0/TEXCOORD3;
PS input TEXCOORD3, outputs COLOR0/COLOR2/DEPTH0. Native HONOR UBSan build2/run1
returns identical four rows; parity1 compares those outputs directly. Both test
paths check valid HRESULTs and count-only queries and reject null shader without
changing the count. Fixtures contain comments, DEF literal bit patterns, duplicate
color references and depth output. This verifies synthetic consumed-model token
behavior, not every retail shader or actual material initialization.

Initial native build1 omitted required -fsigned-char and failed its existing
adapter assertion, corrected in build2. Initial Windows1 test crashed on calls
with a null count pointer; those invalid fixture calls were removed before the
passing Windows2 reference. They do not establish a Windows null-count contract.
Failed logs remain. Only the native test binary was transferred; no assets copied.

Native original Material_GetShaderInfo now checks failed semantic HRESULTs before
using returned counts, retaining its original error-return path. It queries output
count before filling the original16-entry output array and rejects an oversized
result. Canonical parser/state/registration algorithms remain; no success forced.
Whole normal material-loader TU passes x86/x64 compilation in
takeover_material_semantics_windows1; final count guard is Android-only.
These loader branches have compiled, not run in full engine initialization.

Full-engine-link119..121 compile and resolve both semantic functions. Latest121
still explicitly lacks D3DXGetShaderConstantTable and D3DXCompileShader, plus
broader genuine engine/audio/cinematic/UI/memory/game/renderer/network owners.
Next implement owned/validated CTAB metadata and actual CPU HLSL-to-SM3 bytecode
compilation with real diagnostics. Preserve pinned DXVK/Vulkan requirements and
SDK buffer/COM ownership, validate against the Windows oracle, and then resume
production startup. Source/probe closure does not prove Com_Init completion.
