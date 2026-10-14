# Native renderer state matrix rows

Updated 2026-10-14. `r_state.cpp` is frozen and released for SDK449/stage21.
This batch repairs two confirmed matrix row violations found by source audit.
Neither site is claimed as an observed runtime failure. Root owns the linked
APK and actual tablet integration result; completed active 3D rendering and
playable Zombies remain unproved.

## Confirmed scope and canonical owners

SDK448 passed the prior ADS Z-scale repair and subsequently stopped in the
separately owned FX view/frustum code. Ahead of the next 3D shader path, the
source audit found two functions in `r_state.cpp` treating the first four-float
matrix row as a flat sixteen-float allocation:

- `R_GenerateWorldOutdoorLookupMatrix` passes `&(*outMatrix)[12]` to `Vec4Add`.
- `UpdateVPosToWorld` reads `(*projM44)[5]` and `(*invViewM44)[4..7]`.

The canonical declarations use `float (*)[4]`, and `GfxMatrix::m` is actually
`float[4][4]`. `GfxCodeMatrices` contains 32 `GfxMatrix` objects. The outdoor
generator receives `activeMatrices->matrix[baseIndex].m`. `R_GetCodeMatrix`
returns `source->matrices.matrix[matrixIndex].m[firstRow]`; both projection and
inverse-view fetches in `UpdateVPosToWorld` request row 0. Thus both pointer
casts refer to real four-by-four owners, with all later rows present.

The same-file search found no additional expressions in this class outside
these two functions. Signature declarations such as `float (*projM44)[4]` are
row types, not index operations. Existing row-0 lanes 0..3 are within bounds.
The typed native `BO1_MATH_ELEMENT` implementations in canonical matrix math
are already correct and remain unchanged.

## Native substitutions

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, outdoor lookup now calls
`Vec4Add(outMatrix[3], worldOffset, outMatrix[3])`. This remains an in-place
four-float addition into the same final matrix row.

The six adjacent VPos statements retain their original casts, scalar
multiplication, division and statement order. Only matrix expressions change:

| Previous expression | Native typed expression | Float offset |
| --- | --- | --- |
| `&(*outMatrix)[12]` | `outMatrix[3]` | 12 |
| `(*projM44)[5]` | `projM44[1][1]` | 5 |
| `(*invViewM44)[4]` | `invViewM44[1][0]` | 4 |
| `(*invViewM44)[5]` | `invViewM44[1][1]` | 5 |
| `(*invViewM44)[6]` | `invViewM44[1][2]` | 6 |
| `(*invViewM44)[7]` | `invViewM44[1][3]` | 7 |

Both uses of the projection diagonal change identically. All original valid
row-0 expressions remain unchanged. The VIEW_MODE_3D condition, inverse size
calculations, matrix fetches, output arrays, vector transformation, constant
updates, assertions, memcpy and matrix version assignments retain their
original behavior. Matrix dimensions, layout, alignment, allocation, shader
constants and retail formats remain unchanged. Windows retains the original
expressions and function behavior.

## Validation and freeze

The complete production TU compiles with its actual Android shared-engine
command: NDK30/API28, O0/debug/PIC, function instrumentation and UBSan with no
recovery. Windows x86 and x64 comparison compiles also pass in this worktree.
No GPU fixture, device operation, shared master build, Gradle, benchmark,
provider change or disk-format modification was performed by this agent.

Native before/after preprocessing of both selected functions matches exactly
after only the table's documented matrix substitutions. Windows x86 and x64
selected functions match the preserved pre-edit bodies exactly. Comparisons
remove empty lines and preprocessor line directives, and normalize the single
assertion's relocated source filename; assertion expression and line number
remain exact. No casts, arithmetic, constants or other expressions are
normalized. The row-major offsets in the table match the original physical
coordinates without indexing outside a four-float row.

Reversing only the two scoped guarded additions recovers the exact pre-edit
source SHA256 `6AA933BFF499AF42410803340298D7E8E9C6B8E0CC99DB1E584B75B7E35D1337`.
That pre-edit oracle includes prior Android adaptations already present in
this worktree; it is not presented as a pristine Windows file comparison.
CP1252 bytes and existing CRLF/bare-LF counts are preserved. The source hash
remained unchanged through all checks.

Evidence is in `build-android-audit/native_render_state_matrix_rows1/`:
`source-manifest.json`, preserved pre-edit source, `compile-results.json`,
`audit-results.json`, complete first-row search output, native and Windows
objects/preprocessed files and reproducible edit/compile/audit commands/scripts.
`audit-harness-corrections.txt` records initial comparison harness corrections;
none required production source edits. No source outside `r_state.cpp`, assets,
providers, Ledger/STATE, commits or pushes were changed by this agent.

Frozen `src/gfx_d3d/r_state.cpp` SHA256:
`C00C9C848F10B0427E803BEFE25D17022E9CBAA851DCFA4EF649FA8B55B57C9A`.
