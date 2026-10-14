# Native sun-shadow and spotlight row audit

2026-10-14. Source frozen/released for root-owned SDK456/stage28 integration. Only `src/gfx_d3d/r_sunshadow.cpp` changes in this task; `r_scene.cpp` was audited and remains byte-identical to its task-start snapshot. No rendering algorithm, matrix declaration, light/shadow layout, shader, material, assertion, disk/VM/wire format or global compiler flag changes.

## Owner and active-branch proof

Current SDK455 reached client CA_ACTIVE and stopped at `R_FrustumClipPlanes` in `r_dpvs.cpp`. The DPVS agent owns that actual failure. The eight sun-shadow accesses repaired here were confirmed in active native source during the immediate 3D caller audit; separate execution or a device failure in these helpers has not been observed. Prior SDK row failures establish the bounds class, not execution of these conditional lighting paths.

`R_SetupSunShadowMaps` owns `float sunAxis[3][3]`. `R_GetSunAxes` receives its whole typed 3x3 matrix, constructs forward/right/up axes through the existing vector operations, and passes the same object to `R_SetupSunShadowMapProjection`. That function's typed whole-matrix parameter is converted by its existing cast to `const float (*)[3]` for `R_SetupSunShadowMapViewMatrix`. The helper's six original reads `(*sunAxis)[3..8]` crossed the first three-float row despite the valid larger owner.

The actual point producer in `R_SetupSunShadowMapProjection` is `float frustumPointsInSunProj[2][8][2]`. Its first four near points are projected into typed point rows; near point four stores the view origin. The far partition starts from the projected corner rows and extrapolates rows four through seven. The only two bounding-poly calls pass partition zero with pointCount5 and partition one with pointCount8. `R_SetupSunShadowBoundingPoly` receives `float (*)[2]`; its unchanged near-point loop starts at4 and ends before pointCount. Both original reads `(*frustumPointsInSunProj)[2 * nearPointIndex (+ 1)]` crossed the first two-float row. The corrected reads select the same actual point and its lane0/1.

The two initially suspected `r_scene.cpp` pointers `&(*axis)[3]` and `&(*axis)[6]` belong solely to the `#if 0` legacy `R_AddSpotLightToScene` body. The active implementation already sets `dir = axis[0]` and passes `axis[0]`, `axis[1]`, `axis[2]` to `SpotLightViewMatrixDir3`. Native preprocessing confirms one active function definition and no legacy row pointers. Its actual FX caller passes `draw->orient.axis`; `orientation_t` owns `float axis[3][3]` in `universal/com_math.h`. The immediate matrix consumer reads only lanes0..2 of each supplied vector. No scene edit was needed.

## Repair and bounded audit

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, six sun-axis reads now use rows1/2 and lanes0..2; the two near-point reads use `frustumPointsInSunProj[nearPointIndex][0/1]`. Windows/legacy branches retain the complete original expressions. The native-width header explicitly supplies the existing ABI guard.

All matrix assignments, signs, literals, casts, nested arithmetic grouping, loop limits, convex-hull call, point comparison/near tagging, assertions and snap-delta calculations remain exact. The first-row axis reads at indices0..2 remain valid and unchanged. Physical float offsets are equal: axis4*n equals4*(3*row+lane); point4*(2*index+lane) equals8*index+4*lane for every actual near/far loop index.

The bounded first-row audit covered direct axis/point expressions in both owned files and the immediate producer/consumer contracts. Other sun-shadow arrays receive pointers to whole declared matrices/polygons and index those actual rows; `R_ClipSpaceToWorldSpace` is bounded to four points. Scene lerp helpers stay within their declared three/four-float rows and `R_PvsLock_GetViewAxis` uses `AxisCopy`. No additional active direct first-row crossing was found within this scope. Debug collision polygons and unrelated byte-copy/layout issues remain outside it.

## Validation and frozen sources

[native_render_light_rows1](../../build-android-audit/native_render_light_rows1) preserves task-start snapshots, source diffs/manifests, actual commands/logs, objects, preprocessed files and extracted function bodies. [check-results.json](../../build-android-audit/native_render_light_rows1/check-results.json) and [audit-results.json](../../build-android-audit/native_render_light_rows1/audit-results.json) record the final checks.

- Both whole translation units pass Android ARM64 using their unique production `native_engine_entry1/compile_commands.json` commands, with only output redirected to dedicated evidence objects. Existing NDK/API, O0/debug/PIC, instrumentation, UBSan and nonrecovery settings remain.
- Both whole translation units pass Windows x86 and x64 using the established comparison templates in this Android worktree. All six TU compiler exits are0.
- Four native preprocessing runs exit0. Target function bodies equal their task-start bodies after only the eight documented coordinate substitutions; the active spotlight body is exactly unchanged.
- Twelve Windows preprocessing runs exit0. All three target bodies match both before snapshots and the read-only BlackOps64 oracle exactly for both architectures.
- Native explicit checks confirm six axis substitutions, two point substitutions, no remaining crossing in the active repaired helper bodies, and the existing three typed spotlight arguments.
- Reversing the native branches and added include recreates each complete task-start source byte-for-byte. CP1252 high bytes, original line endings, BOM state and all prior source changes/guards are preserved. Sources remained unchanged during validation.

The first checker mistakenly treated the parameter declaration `(*sunAxis)[3]` as a component read. Restricting substitution/crossing detection to the function body corrected that checker without source changes or compiler reruns. The initial checker result is preserved separately; all existing compiler/preprocessing exits were already0.

Frozen SHA256:

- `src/gfx_d3d/r_sunshadow.cpp`: `BB76E1F0D966B0A914ABE19692E29F126B9B4117391DF04F94CFF41E5623EE76` (49918 bytes). Before: `D5B1BC8D3A8356EF7CE4529868328F53E4422382E81ED976B8BAF8B7B800B909` (48778 bytes).
- `src/gfx_d3d/r_scene.cpp`, unchanged: `3DF703E656553FB8FEFB511B68DD7F6BE12259F2DF4651D14741240E8D2D2BB9` (257319 bytes).

Root owns combined integration and device observation. Conditional lighting execution, complete visible3D and playable Zombies are not established by these source and compile checks.
