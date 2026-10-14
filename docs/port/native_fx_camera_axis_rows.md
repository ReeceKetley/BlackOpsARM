# Native FX camera axis rows

2026-10-14. Scope: `src/EffectsCore/fx_update.cpp` only. Source frozen/released for root-owned SDK449 integration. No camera declaration/layout, ownership, scheduler, renderer or hardware FX changes.

## Actual trigger and canonical owner

SDK448/stage20 engine BuildId `121d3690157c5bf24d1d6c28c69a05876c6f5688` reaches initial client-view FX camera setup after the preceding repairs. Nonrecovering UBSan stops at `fx_update.cpp:3427:79`: `index 3 out of bounds for type 'const float[3]'`.

[sdk-native-entry448-follow1-only.log](../../build-android-audit/native_registry_closure/sdk-native-entry448-follow1-only.log) contains the exact report. The current BuildId segment of [sdk-native-launch448-follow1-crash.txt](../../build-android-audit/native_registry_closure/sdk-native-launch448-follow1-crash.txt) shows `FX_SetupCamera` through `FX_SetNextUpdateCamera`, `CG_InitView`, `CG_SetInitialSnapshot`, `CG_ProcessSnapshots`, `CG_DrawActiveFrame` and `Com_Frame`. The capture retains older launches; they are not evidence for this current failure.

The function and `fx_update.h` declaration receive `const float (*viewaxis)[3]`. `FxCameraUpdate::viewaxis` is the actual `[3][3]` object (`fx_update.h:50`); `refdef_s::viewaxis` is also `[3][3]` (`cg_local_mp.h:275`). `FX_GetCameraUpdateFromRefdefAndZFar` uses the existing native-safe `AxisCopy` to populate it. Both local setup callers, `FX_SetNextUpdateCamera` and `FX_SetExtraCamera`, pass that canonical matrix directly. The destination `FxCamera` in `gfx_d3d/fxprimitives.h:273` has its existing axis and `frustum[6][4]` storage.

## Repair and preserved math

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, every flattened `(*viewaxis)[n]` component read now indexes `viewaxis[n / 3][n % 3]`. This repairs all 30 occurrences: 27 base-frustum reads and three optional far-plane reads. The explicit native-width header supplies the ABI guard; the Windows/legacy branch retains the complete original function body.

Only component addressing changes. All float casts, nested expression grouping, arithmetic operation order, double literals, square-root calls, plane signs, origin assignments and `AxisCopy(viewaxis, camera->axis)` remain exact. The function still computes five base planes, checks each normal with the same normalization predicate and hand-authored assertion/message/diagnostic values, and computes each plane's origin dot product in the same order. For positive `zfar`, it still writes the negative forward vector, uses the original distance expression and sets the plane count to six. The camera destination bytes/field offsets, validity publication in the callers and owner lifetime remain unchanged.

The physical component offset is still `4*n`, but the expression now selects the actual row object and a lane in `[0,2]`. The largest source component is row two/lane two (offset 32 in the actual 36-byte matrix).

## Scoped audit and validation

The whole-file direct first-row-array search finds this family only in `FX_SetupCamera`. The remaining flattened expressions in source are confined to its inactive native legacy branch. Camera update/extracamera wrappers already pass actual typed matrices; no further callable same-file axis component repair was needed. No speculative wide-copy or unrelated FX class changes.

[check-results.json](../../build-android-audit/native_fx_camera_axis_rows1/check-results.json) records all results. [native_fx_camera_axis_rows1](../../build-android-audit/native_fx_camera_axis_rows1) preserves the before snapshot, exact diff, manifest, commands, complete logs and native/Windows preprocessed bodies.

- Production Android whole translation unit: exit 0 using the unique `fx_update.cpp` command from `native_engine_entry1/compile_commands.json`, with output redirected to the dedicated evidence object. ARM64 NDK30/API28 O0/debug/PIC/instrumentation/UBSan/nonrecovery retained.
- Windows x86/x64 whole translation units: both exit 0 using the established Android-worktree MSVC comparison templates.
- Native before/after preprocessing: both exit 0. The entire new `FX_SetupCamera` body equals the old preprocessed body after substituting only those component indices. This preserves all math expressions, literals, branches, unit assertions and messages exactly; no diagnostic-location normalization is needed.
- Native explicit checks confirm all 30 typed components, absence of flattened body accesses, unchanged `AxisCopy`, five base planes and the same conditional sixth plane.
- Six Windows preprocessing runs (before/after/read-only BlackOps64 oracle for both architectures): all exit 0. The complete active function body is exactly equal against both before and oracle.
- All original CP1252 high bytes and CRLF line endings preserved; no concurrent source changes during final validation.

The initial checker-preparation script stopped because a prior filename replacement had already handled an oracle path. Correcting that redundant preparation assertion did not change production source. All final compile/preprocessing checks pass.

Frozen source SHA256: `6342088C9ADB87FC146413AB870C1639757E187406FEDF44F4AFADB950A3CA4B` (147167 bytes). Before SHA256: `A9A813F3BE86F8B2CFF839D8124B9C98163912F3642198A08D38216B6F376B83` (142770 bytes).

These source and Windows-preservation checks do not establish a completed rendered-client frame or gameplay. Root owns the next actual APK integration/observation.
