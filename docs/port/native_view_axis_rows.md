# Native view axis rows

2026-10-14. Scope: `src/cgame_mp/cg_view_mp.cpp` only. Source frozen/released for root integration. This is a bounded repair of a proved live array-indexing defect; no claim that SDK447 has executed these killcam helpers.

## Canonical ownership and path

`LookAtAxisAndDistance` and `LookAtBothPoints` are active functions declared in `cg_view_mp.h` with `float (*axis)[3]` / `float (*lookaxis)[3]`. Their callers pass `cgameGlob->refdef.viewaxis`, declared `float viewaxis[3][3]` in `cg_local_mp.h:275`. The first helper is called from scripted and helicopter killcam camera updates; the second from explosive killcam camera updates.

`CG_DrawActiveFrame` reaches `CG_CalcViewValues` in its view-values section. The latter dispatches `CG_UpdateKillCamEntity` for the relevant camera state, which selects these camera updates. The helpers belong to the actual rendered client camera path, but entering the default frontend does not by itself prove a killcam branch was selected. The earlier bounded audit used the nonexistent `src/cgame/cg_view.cpp`; the root subsequently supplied this correct concrete owner.

Before repair, both helpers used first-row expressions `(*axis)[3..8]` / `(*lookaxis)[3..8]`, including writes, reads, row normalization, cross product output and assertion diagnostics. A first-row `float[3]` cannot express rows one/two by indexing lanes three through eight even when their physical storage is contiguous.

## Repair

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, all source and destination accesses now use the actual row and lane. Flattened index `n` maps to `[n / 3][n % 3]`; matrix/vector casts to the first row become `axis[0]` / `lookaxis[0]`, and vector arguments at flattened offsets three/six become rows one/two. The explicit existing native-width header supplies the guard. Each Windows/legacy function body remains unchanged in its guarded branch.

The first helper still writes `lookAt - origin` to row zero, normalizes it and retains the returned distance, forms row one as `(-forward.y, forward.x, 0)`, applies the exact zero-X-and-Y fallback to row-one X = 1.0f, normalizes row one, and computes row two with `Vec3Cross(row0, row1, row2)`. Its normalized-row-two assertion, original hand-authored file/line/message, diagnostic values and returned distance are preserved.

The second helper retains every original point-direction normalization, average, dot threshold, rotation, interpolation, downward-looking branch and literal before writing normalized `lookDir` to row zero. Its remaining row-one fallback, normalization, cross product, assertion and returned distance retain the same operations and order as before.

The highest scalar matrix access is row two/lane two, offset 32 within the real 36-byte matrix. Vector calls receive a row's three-float object; all arithmetic operands and destinations have the same physical offsets as the intended original operations. No float expression, precision, literal, Vec3 operation, assertion or handedness change.

## Same-file bounded audit

Both helpers contain the confirmed live flattened-first-row family and are repaired in one batch. Other inspected view-axis camera operations already select explicit rows and limit scalar lane accesses to 0 through 2. The obsolete corpse-camera pointer casts are inside `#if 0` and remain untouched. No shader/renderer/wide-copy class, unrelated camera algorithm, sound source, command provider, metric, device or configuration change.

## Validation and freeze

[check-results.json](../../build-android-audit/native_view_axis_rows1/check-results.json) records the results; [native_view_axis_rows1](../../build-android-audit/native_view_axis_rows1) preserves source snapshot, exact diff, command lines, full logs and native/Windows preprocessed bodies.

- Production Android whole translation unit: exit 0 using the unique `cg_view_mp.cpp` command from `native_engine_entry1/compile_commands.json`, with output redirected to the dedicated evidence object. NDK30/API28 ARM64, O0/debug/PIC, instrumentation, UBSan and nonrecovery retained.
- Windows x86 and x64 whole translation units: both exit 0 using the established Android-worktree MSVC comparison templates.
- Native before/after preprocessing: both exit 0. For each complete helper body, applying only the described index/row-pointer substitutions to the old preprocessed body yields exactly the new body. This proves the math sequence, literals, branches, normalization/cross order, assertion text and diagnostics remain identical after address correction; no mocked GPU or separate runtime math implementation was used.
- Native checks also explicitly verify absence of first-row flattening, row-one normalization, `Vec3Cross(row0, row1, row2)` order, and the original zero-X-and-Y fallback.
- Six Windows preprocessing runs (before/after/read-only BlackOps64 oracle for both architectures): all exit 0. Both full helper bodies compare exactly equal to both before and oracle, without diagnostic-location normalization.
- Original CP1252 high bytes and mixed line endings preserved. This source already had 20 bare LF endings among 5941 CRLF endings. All 20 preexisting bare LF endings remain; new branch/header lines use CRLF. No concurrent source changes during final verification.

The first two checker attempts are preserved. The first attempted repair stopped before modifying source because it assumed the file had only CRLF endings; that assumption was corrected to preserve the existing mixed endings. The second compiled successfully and had correct native row accesses, but its comparison substitution also transformed the required matrix parameter declaration. The final comparison restricts substitution to the body. The final complete check passes; no sanitizer weakening or failed device fixture.

Frozen source SHA256: `0101F806D23D896F4C6281A65DAFA0271E49978B8FE07FBDC0266DF86504E67D` (272253 bytes). Before SHA256: `76845B01E75CDCAD91455C70A8937015506C9AA43A300622EF25E4F5E365F3C6` (268072 bytes).

The root coordinator owns the next actual APK integration and rendered-client observation. These checks do not establish a completed frontend frame or playable gameplay.
