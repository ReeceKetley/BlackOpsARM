# Native static-model LOD row traversal

SDK456/APK28/stage28 actually aborts in `R_SkinStaticModelsCamera` at the original source line824: `index 8 out of bounds for type unsigned short[4][128]`. The matching crash is PID30407/SDLThread30480, BuildId6c9f9e662cf3ec3dad2cca01700e774acac4ae45. Its stack proceeds through `R_AddAllStaticModelSurfacesCamera`, `R_GenerateSortedDrawSurfs` and `R_RenderScene`. The frustum gate reached static-model draw preparation. Complete visible3D and playable Zombies remain unverified.

Evidence: `build-android-audit/native_registry_closure/sdk-native-entry456-launch1-only.log` lines11322–11323, `sdk-native-launch456-crash1.txt` newest PID30407 segment, and retained stage28 ELF. Older PID27291 crash text in the same crash capture is separate evidence.

The three actual camera/sun-shadow/spot-shadow producers own `unsigned short[4][4][128]` lists and `unsigned short[4][4]` counts. They already append using `[StaticModelId.surfType - 2][lod]`, flush at128 entries, reset the corresponding counts, and dispatch these actual owners. The static-model ID producer emits surface types2/4/5; consumer loops visit four types2…5, and the actual XModel has four LOD records. List/count storage and list entries remain unchanged.

Six native-only substitutions repair four consumers:

| Consumers | Original crossing | Typed coordinate |
| --- | --- | --- |
| Camera and Shadow | `(unsigned short (*)[128])(*staticModelLodList)[4 * surfTypeIndex]` | `staticModelLodList[surfTypeIndex]` |
| Camera and Shadow | `&(*staticModelLodCount)[4 * surfTypeIndex]` | `staticModelLodCount[surfTypeIndex]` |
| CameraForSurface and ShadowForSurface | `(unsigned char *)&(*staticModelLodList)[128 * lod]` | `(unsigned char *)staticModelLodList[lod]` |

The correct surface-list stride is1024bytes, surface-count stride8bytes and LOD-list stride256bytes. The original outer crossing indexes a four-row subobject with4/8/12 and the inner crossing indexes a128-element subobject with128/256/384. Typed coordinates traverse the real owner without crossing those subobject bounds. The byte-list cast remains at the existing LOD callback boundary.

The bounded read-only audit covered the complete file's same list/count family and immediate `CameraForLod`, `CameraForLod_Internal`, `ShadowForLod`, `PreTessStaticModelCachedList` and delayed draw writer. `R_WritePrimDrawSurfData` already uses a byte `memcpy`; no adjacent source edit was needed. Existing model loops, assertions, charred/uncharred partition, list capacities, draw command order, surface packed bits, LOD packed bits, math, callbacks and prior narrow32 encoding remain unchanged. Windows selects the exact original expressions under four native guards.

Actual checks pass:

- SDK build compile command reused for a full native translation-unit object, preserving target flags, debug, UBSan and assertions.
- Full Windows x86 and x64 translation units compile in this worktree. Windows reference remains read-only.
- Eight preprocessing passes: native before/after, Windows x86/x64 before/after/reference.
- Four repaired native bodies differ only by the six exact coordinate substitutions; nine related producer/callee bodies remain unchanged. Native preprocessed list/count crossings are absent.
- All26 Windows body comparisons match both pre-edit and Windows reference after only generated assertion file/line and profiling identifier line normalization.
- Whole-source reversal of the four native branches plus added width header reproduces every before byte; CP1252 high bytes, CRLF and no-BOM state remain preserved.
- Ten bounded owner/representation/proof checks pass in `audit-results.json`.

The initial proof harness comparison failed because native preprocessing expands `__int16/__int8` to `short/char`, and the unchanged spot-shadow profiling macro emits line-numbered identifiers. Initial results are retained in `initial-check-results.json`. Corrected exact substitution/line normalization reuses the same successful native/Windows builds and preprocessed artifacts; engine source was unchanged. Final pass is `check-results.json`.

Frozen/released source: `src/gfx_d3d/r_add_staticmodel.cpp` SHA256 `5E56AF08D20480E4B2C0BC22285DE8E899DD084AB4B0EA9E5FAD2A7B7224D11C` (59529bytes). Before SHA256 `DCF250EA28F3943C1DDD3EEBDD207564E833ACAF03AEE57D801742629E6F6C98` (58945bytes). Evidence directory: `build-android-audit/native_staticmodel_lod_rows1/`, containing before source, diff/manifests, commands/logs/objects, preprocessed bodies, check results and bounded audit. Root owns SDK457/APK29/stage29 integration and device observation; patched device execution is pending at release.
