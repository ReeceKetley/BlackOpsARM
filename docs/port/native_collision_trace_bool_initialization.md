# Native sound sight-trace boolean initialization

2026-10-14. Source repair frozen for root integration into SDK466; no actual repaired APK runtime claim yet.

## Actual failure

SDK465/stage37 BuildId `55aaeff532f8a35aa9f6e3bdb43158facf2a8ac3`, actual PID28989, reaches approximately200 server frames/simulation9900ms before death. The launch-only log reports `cm_mesh.cpp:1328:23` loading bool value124 from `tw->doublesided`. Root refreshed the crash buffer; the matching section identifies tid29157 `bo1-occlusion`, PID28989, and every engine frame carries the same BuildId. Earlier crash-buffer entries belong to other processes/builds and are excluded.

The actual stack is `CM_TraceThroughAabbTree_work` / `_r` / `CM_TraceThroughAabbTree` / `CM_SightTraceThroughAabbTree` / `CM_SightTraceThroughLeaf` / `CM_SightTraceThroughTree` / `CG_SightTracePointInternal` / `CG_SightTracePoint` / `Snd_LosOcclusionMultiTrace` / `SND_LosOcclusionTrace` / `SND_LosOcclusionCmd` / `SND_LosOcclusionThreadMain` / `SND_OcclusionThreadEntry`. This is sound occlusion sight testing, not an inferred renderer or server thread.

## Owner and canonical contract

There are exactly five `traceWork_t` local objects in engine sources: one in `CG_SightTracePointInternal`, and four in `CM_Trace`, `CM_BoxSightTrace`, `CM_SightTracePoint`, and `CM_TracePointDown`. The real `traceWork_t` constructor initializes vector lanes but leaves scalar flags untouched. All four qcommon producers initialize `isPoint`, `doublesided`, and either `axialCullOnly` directly or through `CM_SetAxialCullOnly`. `CM_Trace` takes doublesided policy from the real `doublesided_raycasts` dvar on both stationary/moving branches. Ordinary sight producers explicitly set `doublesided=0`.

Before this repair, `CG_SightTracePointInternal` initialized `isPoint=1` and `axialCullOnly=0` but omitted `doublesided`, then submitted the stack object to the tree/Mapkit sight consumers. Its constructor cannot supply a value. `CM_GetTraceThreadInfo` increments the thread-local global checkcount and performs a typed tail-member copy `*threadInfo=*value`; it neither overlaps nor supplies the missing flag. The native object layout and thread-local provider remain unchanged.

Native Android now assigns `tw.doublesided=false` next to the other point-trace flags. This establishes the ordinary one-sided sight contract already used by `CM_SightTracePoint` before either consumer sees the object. It does not read or normalize invalid bool storage. No byte-offset clearing, blanket initialization, constructor replacement, collision bypass, assertion weakening, or geometry/math changes were made. The material-content0x20 exception and triangle tests in `cm_mesh.cpp` remain intact.

The assignment is guarded by `defined(__ANDROID__) && BO1_NATIVE_64BIT`; Windows retains its validated reference source behavior, including the historical CG omission. The fix adds no members and changes no native layout, fixed32-bit disk/VM slots, asset data, flags, or thread-local lifetime. Existing native coordinate-copy guards remain exact.

## Verification and freeze

- Native Android ARM64, Win32, and Win64 full `cg_world.cpp` TU compilation passed using existing canonical flags.
- Eight preprocessing runs passed: native before/after; each Windows before/after/read-only BlackOps64 oracle.
- Native extracted `CG_SightTracePointInternal` differs only by the typed assignment. Its `CG_SightTracePoint` wrapper is identical. Both functions match before and Windows oracle bodies on both Windows architectures: eight comparisons in total.
- Exact byte source reversal passed; CP1252 high bytes, BOM, and existing line endings are preserved.
- Immediate five-producer flag/constructor/provider owner proof passed. This is a bounded owner check, not a broad collision audit or runtime coverage claim.

Frozen `src/cgame/cg_world.cpp` SHA256: `C964944932A3A7C82060A3FBB0ADA79328DB7217C4C9F7A6823C090EF26DD243`.

`cm_mesh.cpp`, `cm_trace.cpp`, and `cm_trace.h` were read-only. Root retains build/package/device/Ledger/STATE ownership and must validate the combined next APK. No asset/provider/configuration changes, master engine build, or device action were performed here.

## Evidence

`build-android-audit/native_collision_trace_bool_initialization1/` contains `cg_world.before.cpp`, exact substitution/diff, source manifest, native/Windows compilation and preprocessing logs/commands, extracted bodies, `check-results.json`, `owner-proof.json`, all five producer source extracts, constructor/provider extracts, and `sdk465-matched-crash.txt`.

Original failure evidence remains `build-android-audit/native_registry_closure/sdk-native-entry465-launch1-only.log` and root-refreshed `sdk-native-launch465-crash2.txt`. The matched excerpt preserves PID/thread/time/BuildId identity. Report written as UTF8 to a temporary file, strictly decoded and verified before publication.
