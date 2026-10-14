# Native client FX axis rows

2026-10-14. Scope: `src/cgame/cg_scr_main.cpp` only. Source frozen/released for root-owned SDK451 integration. Existing VM, script callback, debug and other source adaptations remain untouched.

## Actual evidence and contract

SDK450/stage22 engine BuildId `3850c57b9fa3ab40d4ca7b47098475224bed6919` reaches client FX scripting after the previous repairs. [sdk-native-entry450-follow1-only.log](../../build-android-audit/native_registry_closure/sdk-native-entry450-follow1-only.log) reports `cg_scr_main.cpp:5133:46: runtime error: index 6 out of bounds for type 'float[3]'`, the first up-vector source in `CScr_SetFxAngles`' two-axis dot product.

The current BuildId tail of [sdk-native-launch450-follow1-crash.txt](../../build-android-audit/native_registry_closure/sdk-native-launch450-follow1-crash.txt) separately records a server-thread SIGSEGV at `gjk_query`, reached through spectator movement. That backtrace does not establish that this FX array fault caused the final process signal; the root coordinator was notified of the distinction. Older retained crash segments are not evidence for this current array report.

The helper's canonical header and definition take `float (*axis)[3]`. Both callers provide actual local `float axis[3][3]` storage. `CScr_PlayFX` writes and normalizes the script's forward argument in row zero, then writes and normalizes the optional up argument in row two before calling `CScr_SetFxAngles(2u, axis, angles)`. Its count-zero/count-one calls use the same actual array. `CScr_PlayLoopedFX` similarly fills row zero from the forward argument and row two from the optional up argument, then passes its computed axis count and real matrix to this helper.

## Repair and unchanged behavior

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, only the two-axis arithmetic block and its vector row arguments change. First-row flattened indices zero through eight now use their canonical rows/lanes; row-two normalization receives `axis[2]`; the cross product receives `axis[2]`, `axis[0]`, and output `axis[1]` in that exact order. The existing native-width header supplies the guard. The entire Windows/legacy arithmetic block remains unchanged.

The original negative forward/up dot product retains all casts, grouping and operation order. Row-two components are orthogonalized in the original X/Y/Z update order with unchanged casts. A zero normalization result retains the exact parallel/opposite-vector error text and `Scr_Error(SCRIPTINSTANCE_CLIENT, error, 0)`. `AxisToAngles` remains outside the guard and is unchanged.

The count-range assertion, count-zero output `(270.0f, 0.0f, 0.0f)`, count-one `vectoangles((const float *)axis, angles)`, and all three final `AngleNormalize180` calls remain exact. No script argument parsing, registration, caller, FX playback, angle-output storage or assertion change.

Typed indexing preserves intended physical component offsets while selecting the actual three-float row. The highest component is row two/lane two, offset 32 in the caller's 36-byte matrix. The normalized up vector and cross-product destination now retain their correct row objects through the helper calls.

## Audit and validation

The whole-file direct first-row-array search finds this family only in `CScr_SetFxAngles`. Remaining flattened expressions are solely its guarded legacy block. Other same-file local matrix callers already select row zero/row two and need no repair.

[check-results.json](../../build-android-audit/native_client_fx_axis_rows1/check-results.json) records the results. [native_client_fx_axis_rows1](../../build-android-audit/native_client_fx_axis_rows1) preserves source snapshot, exact diff, manifest, commands, full logs and native/Windows preprocessed bodies.

- Production Android whole translation unit: exit 0 using the unique `cg_scr_main.cpp` command from `native_engine_entry1/compile_commands.json`, with output redirected to the dedicated evidence object. ARM64 NDK30/API28 O0/debug/PIC/instrumentation/UBSan/nonrecovery retained.
- Windows x86/x64 whole translation units: both exit 0 using the established Android-worktree MSVC comparison templates.
- Native before/after preprocessing: both exit 0. The entire new helper body equals the old preprocessed body after substituting only component indices and the two-axis vector-row pointer arguments. This proves all math, dot/update order, error behavior, count branches, assertion, angle conversion and final normalization remain exact after address correction.
- Explicit native checks verify no flattened body access, row-two normalization, `Vec3Cross(axis[2], axis[0], axis[1])` order, the original one-axis branch and `AxisToAngles`, and all three angle-normalization calls.
- Six Windows preprocessing runs (before/after/read-only BlackOps64 oracle for both architectures): all exit 0. The complete active helper body equals both before and oracle without diagnostic-location normalization.
- All original CP1252 high bytes and CRLF line endings preserved; no concurrent source changes during validation.

Frozen source SHA256: `2C5C3300EA38C25344A81358111738D780834D4B5D89C405883B66399DD11308` (390732 bytes). Before SHA256: `10F6507466B5F867E5F7901D17A2DDCDFC33FE0F8D6B18D677225D081FECF04C` (390046 bytes).

No effect fixture, mock engine, broad test repetition or hardware/device action was used. These source and Windows-preservation checks do not establish completed rendered gameplay. Root owns the next actual APK integration and both runtime failure follow-ups.
