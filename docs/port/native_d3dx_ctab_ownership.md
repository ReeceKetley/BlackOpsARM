# Native owned CTAB metadata

2026-10-13. Full-engine-link123 compiles selected sources and resolves
D3DXGetShaderConstantTable. D3DXCompileShader and wider engine services still
prevent linking; actual Com_Init/Com_Frame remain incomplete.

Native extraction walks encoded shader instructions/comments to END, finds the
actual CTAB FourCC and validates the copied payload's header/version, record ranges,
terminated strings, type/member offsets, recursion/cycles and default-value range.
Disk metadata remains fixed32 with16-byte type/8-byte member structures. Recursive
types are bounded at64 levels; allocation errors propagate. The bounded native
entry accepts an authoritative word length for the coming compiler adapter and
rejects truncated input before advancing beyond that extent. Unsized SDK wrapper
still requires readable END-terminated bytecode; scan ceiling is not buffer proof.
SM2/3 are handled; earlier models explicitly unsupported.

The owner copies actual CTAB bytes into the previously implemented native buffer,
supports atomic lifetime/refcount and releases backing storage at final release.
No alias into source shader memory or fabricated constants are published. Native
ID3DXConstantTable declarations currently expose only the inherited buffer methods
consumed by the original loader. IUnknown/buffer queries are supplied; complete SDK
constant-setting/reflection methods and their IID are not advertised or claimed.
Extend the declaration/real methods if future callers need that broader interface.

Actual Windows x64 D3DX9_43 validates the88-byte matrix-constant fixture's exact
contents, copied ownership after source overwrite, retained/final references and
IUnknown. It returns D3DXERR_INVALIDDATA (0x88760b59) with null table when CTAB is
absent. Initial Windows1/2 tests exposed an incorrect success/null assumption;
the implementation and expectation were corrected, and Windows3 passes. Failed
evidence remains. This is measured SDK behavior, not reliance on an assumption.

HONOR UBSan native build1/run1 passes the same identity/ownership/reference/absence
cases, plus bounded truncated-comment, invalid string and type offsets. Other
recursive/member/default-value variants, forced allocation failure, native IID
buffer comparison with SDK and every retail CTAB are not runtime-proven. Only the
test executable was transferred; staged assets were reused. The tests call the
production CTAB/buffer sources, not substitutes. Shader compiler output has not
yet gone through this CTAB owner or actual material loading.

Next connect the proven upstream CPU compiler to D3DXCompileShader with real
profile/entry/flags/macros/include behavior, owned bytecode/diagnostics and this
bounded CTAB extraction. Keep existing Vulkan/DXVK requirements. Wider actual
audio/cinematic/client/UI/memory/game/renderer/network and production execution
remain required; this metadata service does not constitute startup completion.
