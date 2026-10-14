# Native FX spawn axis rows

2026-10-14. Scope: `src/EffectsCore/fx_update_util.cpp` only. Source frozen/released for root-owned SDK452/stage24 integration.

SDK451/stage23 engine BuildId `0931b1612e23b44a7a7390c04127749c6a7404e4` passes the previous client scripting and server movement gates. Its fresh [follow1 native segment](../../build-android-audit/native_registry_closure/sdk-native-entry451-follow1-only.log) reports `fx_update_util.cpp:718:52: runtime error: index 6 out of bounds for type 'float[3]'` in `FX_AnglesToOrientedAxis`. The separate current server `cm_world.cpp:898` misaligned load is owned by another agent; this repair does not attribute that failure to the axis helper.

The helper and its canonical header receive `float (*axisOut)[3]`. `FX_GetElemAxis` forwards the same typed matrix after computing the original sampled angles. Its callers `FX_SetPlacement` and `FX_SpawnPhysics` provide local `float axis[3][3]`; `FX_DrawElem_RotatedSprite_NoCull` directly calls the angle helper with the same actual matrix shape. `FX_OrientationDirToWorldDir` writes exactly three float components to each destination and reads the existing typed rows of `orientation_t::axis[3][3]`.

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, the three destination arguments now select `axisOut[0]`, `axisOut[1]`, and `axisOut[2]`, respectively. The latter two previously formed `&(*axisOut)[3]` and `&(*axisOut)[6]` from the first three-float row. The original physical destinations remain offsets 0, 12, and 24 in the actual 36-byte matrix; each three-component write now stays inside its selected row. The existing native-width header supplies the guard. Windows/legacy calls retain their complete original expressions.

All angle reads, sine/cosine evaluations, casts, grouping, local-vector component update order and world-direction call order remain exact. No signature, matrix/owner layout, caller, assertion, random sampling, quaternion conversion, playback, renderer or scheduling change. The bounded whole-file matrix/axis audit finds this first-row destination family only in `FX_AnglesToOrientedAxis`; the file's other local, orientation and frustum row accesses already select their actual rows. Remaining flattened destination expressions are confined to the inactive native legacy branches.

[check-results.json](../../build-android-audit/native_fx_spawn_axis_rows1/check-results.json) records the checks. [native_fx_spawn_axis_rows1](../../build-android-audit/native_fx_spawn_axis_rows1) preserves the before snapshot, diff, manifest, commands, complete compiler logs and selected preprocessed bodies, using the prior client FX/camera axis checker workflow.

- Production Android whole translation unit: exit 0 using its unique command from `native_engine_entry1/compile_commands.json`, redirected to the dedicated evidence object. NDK30/API28 ARM64 O0/debug/PIC/function instrumentation/UBSan/nonrecovery retained.
- Windows x86/x64 whole translation units: both exit 0 using established Android-worktree MSVC templates.
- Native before/after preprocessing: both exit 0. The entire active helper body equals before after substituting only the three destination-row arguments; explicit checks confirm destination row order 0/1/2 and no flattened body access. `FX_GetElemAxis` is exactly unchanged.
- Six Windows preprocessing runs (before/after/read-only BlackOps64 oracle for both architectures): all exit 0. Complete helper and caller bodies match both before and oracle exactly, without diagnostic-location normalization.
- CP1252 high bytes and all original CRLF endings preserved; source unchanged during validation.

Frozen source SHA256: `886FB06BAD68CA72B1568853FA1CF0FDC8FD2D9BF3088C342EBD81412A47902F` (31309 bytes). Before SHA256: `9993472611514BA59B63194606598FA32C9877AC3E9C7B087C673B98C6BD9194` (30906 bytes).

No master engine/device build, asset transfer, effect runtime fixture, global flag change, test weakening, commit or push. These source checks do not establish runtime execution of the repaired helper or completed rendered gameplay. Root owns actual integration, ADB, Ledger and STATE updates.
