# Native tag matrix translation rows

Actual full425 passes model tracing and continues real level script execution into `ScrCmd_LinkTo2697`, `G_EntLinkTo785`, `G_CalcTagAxis1131` and `G_CalcTagParentAxis1118`. It stops134 at155.801021s, after the real map command at40.077894s, because `(*parentAxis)[9]` crosses the declared first float[3] row. Evidence is `build-android-audit/native_registry_closure/full-engine425-honor-zombies1.log`; matching production ELF BuildId is `350c641a33949960a00c2e6c9d9a48051e59ca6e`.

Only canonical `src/game_mp/g_utils_mp.cpp` changes in this batch. Six Android ARM64 offset sites become typed translation-row access under `defined(__ANDROID__) && BO1_NATIVE_64BIT`: the tag-parent transform output uses `parentAxis[3]`, its untagged fallback assigns `parentAxis[3][0..2]`, and world-tag/world-bone transform outputs use `tagMat[3]`. Original Windows expressions remain verbatim in `#else` branches. All calculations, conditionals, assertions, DObj/bone/tag lookups, position/angle inputs, matrix composition and link behavior remain unchanged.

The actual `G_CalcTagAxis` caller owns local parentAxis[4][3]. Same-file `G_SetOriginAndAngleForLinkedEntity` and `G_SetPlayerFixedLink` callers also own actual four-row matrices; output translation is the fourth float3 row. World-tag and world-bone producers already produce orientation rows0..2 before transforming the translation into row3. Sampled real caller owners in actor senses, item tracing, zombie exposure, client vehicle linkage and vehicle matrices are actual float[4][3] objects. No temporary pointer is retained, allocation added, lifetime extended, structure padded or flattened float pointer introduced.

The same-file copy audit finds one `_QWORD` vector copy in `G_TraceBulletPathForVehTurret`, at original line1533. Its source is the fourth row of actual local flashMtx[4][3], after the world-bone matrix producer, and its destination is actual local start[3]. Both three-float regions only promise float alignment. The native path now uses `std::memcpy(start, flashMtx[3], sizeof(start))`, exactly12 bytes, with a guarded `<cstring>` include. Original Windows 8+4-byte expressions remain unchanged. This is a nominal copy-contract repair; no runtime misalignment at this site has been observed. No fourth lane exists in either float3 region. Subsequent end-point calculation, inverse transform, pose calculation and trace calls are unchanged.

The bounded same-file inventory of first-row constant offsets finds only these six executable sites; typed parameter declarations are unchanged. The sole `_QWORD` memory-copy family is the muzzle-origin copy. The existing whole worldAxis/axis memcpy operates on actual equal-size float[4][3] locals and remains unchanged. The `G_irand` `(__int64)` expression is a numerical conversion and remains unchanged. No other source, header, shared math, CMake or runtime service changes belong to this batch.

Verification under `build-android-audit/native_tag_matrix_rows`:

- `scope.json`, `g_utils_mp.cpp.before`, `source.diff`: exact CP1252/CRLF readback and bounded branch delta. The complete Windows source projection equals both the frozen before snapshot and read-only BlackOps64 oracle byte for byte. Before/oracle SHA256 is `20B9361F0ED409E011F289269F3573F2E54B954DBD9B09FD626A27622E0721BB`.
- `native-command.json`, `native-tu.log`: exact production ARM64 whole-TU command retains UBSan and no recovery, with output redirected only to this evidence object; exit0 once.
- `x86` and `x64`: normal Windows whole-TU checks, native selector absent, each exit0 once. Commands, logs and result JSON retain existing unrelated warnings.
- `compile-results.json`: the final source hash is unchanged throughout all three builds. `git diff --check` passes for this source.

No synthetic numerical fixture was added: this batch maps literal row offsets and one exact float3 copy without new arithmetic. Previously verified actual matrix helpers retain their implementation; root's genuine production426 run supplies the next integration evidence. No engine/device run, asset/library transfer, Ledger mutation, commit or push was performed by this source owner. All prior source batches remain frozen.

Frozen source SHA256: `FC75BFDABAEFCD5D075B4B5DE33625C72E290792F6B78C51E0CB1FAC568DC2A7` for `src/game_mp/g_utils_mp.cpp`.

Reproduction helpers under `build-android-audit/native_registry_closure` are `repair_tag_matrix_rows.py` and `build_tag_matrix_rows.py`. The repair helper preserves original bytes and refuses an existing evidence directory. Report/evidence JSON encoding is UTF-8; canonical source remains CP1252/CRLF.
