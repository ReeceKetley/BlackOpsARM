# Native DObj model-surface stream alignment

Updated 2026-10-14. The bounded runtime surface family is repaired, verified and frozen for SDK465/stage37. APK runtime verification of this repair remains pending; this report does not claim visible 3D or playable gameplay.

## Actual failure and owner trace

SDK464/stage36, BuildId `bb858a1fe9b1c4c972c5ec2e54dd88b0fc3829dc`, reached CA_ACTIVE and subsequently reported `r_dobj_skin.cpp:548:18`: member access within `0x006e534f38f4` for `GfxModelRigidSurface`, requiring eight-byte alignment. This is the skinned branch of `R_PreSkinXSurface`, reached through `R_SkinSceneDObjModels` on the scene-entity worker path. The launch-only log and the matching BuildId section of the crash buffer are the runtime evidence; the crash buffer also contains older runs and must not be read without matching BuildId.

The DObj stream combines hidden markers (-3), rigid records (-2) and skinned records. Before the repair, hidden markers consumed four bytes while native records contained eight-byte pointers. Any odd number of hidden markers could leave the next record at four modulo eight, including the reported write before final publication. The local byte-array declaration also lacked explicit model-record alignment.

The direct XModel producer in `r_model.cpp` had the same four-byte hidden marker. Camera/shadow readers in `r_scene.cpp`, the `r_model_skin.cpp` worker, and both local DObj cache passes repeated that stride. Fixing only the crashing assignment or adding an assertion would leave the producer/reader contract broken.

## Repair and publication contract

- `r_dobj_skin.h` defines the hidden marker stride as `alignof(GfxModelRigidSurface)` on native non-Windows 64-bit builds (eight bytes on Android ARM64), and four on Windows. The marker still contains the same four-byte -3 tag; the extra bytes are unused runtime padding.
- DObj scratch storage has explicit native model-record alignment. Existing XModel scratch storage already has alignas(16). Rigid/skinned record sizes remain their canonical native sizeof values: 72 and 40 bytes.
- Both producers reserve their final block through `R_AllocModelSurfaces` on native builds. A bounded compare-exchange loop reserves alignment padding and payload in one operation on the existing frame arena counter. The initial load and interlocked operations retain sequentially consistent ordering. A competing four-byte producer or CAS failure is rechecked before the reservation succeeds.
- `GfxBackEndData` already has alignment 32 and `surfsBuffer` at offset zero, so no arena declaration or layout edit was needed. Static assertions verify base alignment, surface sizes and compatible record alignment.
- All direct hidden-marker traversals use the same stride, including both XModel camera/shadow surfId increments. Draw keys and model surfIds still use four-byte word offsets. Material indices, surface order/count, record tags, transform/bone math, skin cache, shaders, worker submission and placement data are unchanged.

The arena capacity stays 262144 bytes. Bounds checks use subtraction after checking the current cursor and size against capacity, preventing unsigned overflow. A successful reservation advances the high-water counter through both padding and payload. On exhaustion, the allocator returns the capacity sentinel, leaves the counter unchanged, and each existing caller reaches its original warning/return-zero path before publishing or copying any surface block. This replaces the old failed exchange-add's unused out-of-capacity reservation; successful reservations and frame reset/clamping remain compatible. Zero-size model blocks retain an assertion rather than publishing a fake allocation.

DObj publication still copies the complete local stream into the persistent front-end arena, publishes the existing firstSurf pointer and submits the existing skin worker with the same modelSurfs/surfCount/mat ownership. XModel publication retains its copy and surfId/lod sequence. The frame arena owns pointer-containing records until the existing renderer/frame-buffer lifecycle retires them. The new allocator does not publish or free any resources and does not alter lifetime ordering.

The production change is exactly five files: `r_dobj_skin.cpp`, `r_dobj_skin.h`, `r_model.cpp`, `r_model_skin.cpp`, `r_scene.cpp`. The shared Windows source paths keep their original exchange-add allocation and marker stride. FastFiles, retail assets, disk/VM slots, draw-word IDs, providers, config, build flags, assertions and UBSan are unchanged. The Windows oracle was inspected read-only.

## Verification

Twelve affected TU builds pass: four native ARM64 TUs plus the same four Windows x86 and four Windows x64 TUs. Twenty-four before/after preprocessing runs pass. Three separate layout compilations prove native and Windows record/arena sizes and alignment. Native compilation uses the current engine compile-command flags, including O0/debug and undefined-behavior sanitization without recovery.

Eight native body comparisons reverse only the intended alignment/stride/allocation substitutions, including an unchanged `R_PreSkinXSurface` body after normalizing generated assert locations. Sixteen Windows before/after body comparisons are identical. Exact source inverse checks preserve prior pending changes and prove no unrelated edits. CP1252 high bytes, BOM presence and every source's pre-existing CRLF/bare-LF counts are preserved.

The host fixture passes 31,660 checks using the extracted production allocator body and canonical model/arena headers. It covers capacity/exhaustion/near-UINT_MAX cursors, eight-byte starts after four-byte arena positions, 64 mixed 128-record streams, preserved native pointer payloads, copied publication, hidden-marker parity, traversal extent, four-byte draw offsets, and 400 concurrent reservations including a competing four-byte exchange-add producer. Allocation intervals remain disjoint and the final high-water counter matches the last reservation. The fixture maps only the native atomic-load spelling to a sequentially consistent host atomic load and uses Windows' real interlocked primitives. Unused transitive codec-table definitions exist only to satisfy header relocations; the fixture never reads them. This is a host fixture, not actual APK execution or a gameplay benchmark.

The first verification-harness attempts found generated assert-line normalization, extractor and isolated-link setup issues; none required a production edit. All final fixture results and production hashes pass. Conditional camera/shadow/cache paths are checked structurally and by the bounded traversal fixture; they are not separately claimed as executed on the tablet.

## Evidence and frozen source hashes

- Actual launch: `../../build-android-audit/native_registry_closure/sdk-native-entry464-launch1-only.log`.
- Actual crash buffer: `../../build-android-audit/native_registry_closure/sdk-native-launch464-crash1.txt` (match the stage36 BuildId).
- Focused evidence: `../../build-android-audit/native_dobj_rigid_surface_alignment1/` with preserved before files, exact substitutions, diffs, command/log files, native and Windows extracted bodies, `check-results.json`, `fixture-results.json`, and `source-manifest.json`.
- Reproduction: run `python build-android-audit/native_dobj_rigid_surface_alignment1/check.py`, then `python build-android-audit/native_dobj_rigid_surface_alignment1/verify.py` from BlackOpsARM. Master build/package/device/Ledger/STATE remain root-owned.

| Source | Frozen SHA256 |
| --- | --- |
| r_dobj_skin.h | `84DE6293E9D6D3FCC56FA70E989648D250F7EA28E2B4A2378936E1B2DE0EFDA3` |
| r_dobj_skin.cpp | `06D9F51749B411C66D053D81D7C69348F8A8B49AD79F4D35A310E445070B26FF` |
| r_model.cpp | `5B3D359642F433A587991DF61D8E204535A1878E6A7CB89BA87402EE2AC9A8A4` |
| r_model_skin.cpp | `B82FC77E26A687F4A564F357E6CED3BF036F3093987BE381AFAF130EEC3BF094` |
| r_scene.cpp | `9D7F90875DEE26BEB537DC20ABFBB99553674B0ECACB427B20780ED10684A07C` |

No production edits remain pending in this bounded repair. Root was notified that the source family is released for SDK465/stage37.
