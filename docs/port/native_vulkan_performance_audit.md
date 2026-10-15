# Native Vulkan CPU audit and BSP binding repair

2026-10-15. User requested a source audit, fixes and actual HONOR tablet verification against the reported D3D9 7.8–11 FPS. This task does not claim the native backend or 30 FPS objective complete.

## Evidence and fixes

The current checkout already contained SDK601/stage170, including the earlier native-library O2 change. Actual PID11532 was sampled with Android simpleperf, not inferred from CPU wall-time spans. The render-thread sample attributed 20.94% to the atomic-load helper, 19.37% to `__cyg_profile_func_enter`, 15.75% to its atomic wrapper and 5.30% to `__cyg_profile_func_exit`. Remaining byte-vector operations were also expensive. Captures, perf.data, callgraph and reports are under `build-android-audit/native_registry_closure/sdk601-audit-*`.

Stage82/SDK602 optimizes only `native_engine_trace.cpp` with O2. It retains the same atomic switch, startup breadcrumbs, explicit overrides, UBSan and symbols. Actual generated ARM64 code replaces the disabled-path nested atomic calls with a direct load/check. Existing engine function instrumentation remains enabled. The scene still renders; one overlay capture changes 0.9 FPS/1078.71 ms to 1.8 FPS/549.65 ms. Small sequential samples are not a controlled thermal comparison.

Stage83/SDK603 eliminates per-byte vector construction/destruction and the extra full-plane copy in cinematic uploads. `UploadBytes` owns immutable raw storage filled completely using memcpy/memset before publication. Pitched L8 planes copy straight into this snapshot, preserving decoder pitch, clear values, actual image validation and the existing upload/fence path. Non-compressed general image updates also use bulk copies. Compressed decoding transfers its existing vector allocation without another copy. No image contents, shaders or passes are substituted. Canonical Windows cinematic code outside the guarded native helper is byte-identical. CPU fixture on the actual ARM64 tablet passes 3,145,801 checks including padded rows, clear values, immutable retained snapshots, decoded storage adoption and overflow/invalid-source rejection. This fixture is not GPU or gameplay proof.

Stage84/SDK604 uses Clang function instrumentation after inlining only for the already optimized native renderer sources in bo1_native_database. Ordinary engine sources retain Debug/O0 and their original instrumentation. This avoids tracing each inlined STL helper while retaining actual out-of-line function entry/exit hooks; disassembly of the real material draw confirms both hooks. All 36 native rendering sources retain UBSan/no-recovery, debug symbols and assertions, without NDEBUG or fast-math. See `stage84-compile-proof.json` and `stage84-draw-instrumentation-disassembly.txt`. Compiler behavior is documented in the [official Clang option reference](https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-finstrument-functions-after-inlining) and verified with the installed NDK compiler, including the combined flags used by this build.

## Crash exposed by sustained runs

SDK601 and SDK603 both abort in `R_HW_SetSamplerTexture` at r_state.cpp:2385 from `R_DrawTrianglesLit`; SDK604 reproduces the same failure. This predates the new upload implementation. Exact BuildIds, PIDs and actual debuggerd backtraces are retained in `sdk601-603-crash-logcat.txt` and `sdk604-crash-logcat.txt`, with matching final-entry logs. SDK603's final screenshot shows the importer after the crash and must not be used as room/visual proof. Its completed pre-crash frame samples remain valid only in that scope.

Stage85/SDK605 adapts all four BSP direct texture-binding sites to real native reflection/lightmap GfxImage owners and the actual command-state owner. Packed world GfxTexture arrays are COM aliases and cannot be cast to GfxImage. The selected canonical reflection, primary, secondary and secondaryB image branches (including override/conflict/sun direction behavior) pass through the existing native R_SetSampler provider with canonical sampler states. D3D calls remain in their original else branches. No COM fake, assertion suppression or early-ready path is added. Native-disabled BSP token comparison passes 4,848 tokens; engine build passes. The initial missing reflection-probe declaration build failure is preserved as `native-bsp-images-stage85-build1.log`, and the corrected build is `build2.log`.

## Actual run tracking

Builds SDK602/stage171, SDK603/stage172, SDK604/stage173 SDK605/stage174 and SDK606/stage175 package and verify all seven actual libraries. Only changed binaries are deployed; installed retail assets are reused. The final installed engine is SDK606/stage175, BuildId `69f4e550391ad534be1e1ac07ccb78ba66502153`, PID20688. Current build and staged engine SHA256 match the verified APK manifest. `observe_native_vulkan_performance.py` follows one existing PID and launch-marker count, records complete profile output and only accepts screenshots if the same PID survives capture. It never restarts the engine. GPU timing covers the offscreen scene and excludes image uploads/window presentation. Native GPU overlay remains unwired.

Final sustained measurements are below.


SDK605 passed the previous D3D call but then failed closed with an unbound sampler in `wc/rus_metal_wire_fence / sw_cod7_chainlink_w_sun`. Exact final entry and EXIT_SELF/status2 evidence are retained as `sdk605-sustained1-*`; this is not a successful sustained run. Source review found that the native R_SetSamplerState path omitted the canonical `samplerTexture[slot] = 0` invalidation. Stage86/SDK606 restores that metadata invalidation so BSP cannot skip an actual image bind based on a stale raw texture pointer. Independent native sampler values and real resource validation remain intact. Failure messages now include missing register/stage/image/sampler identity. SDK606 passes sustained runtime verification within the menu scope below. The only r_state.cpp change from this audit's starting snapshot is the three added native-guarded invalidation/comment lines; all other bytes match.


## Verified final measurements and limitations

SDK606 remained in the same PID20688 throughout the 185.624-second observation. The 63 room profile samples span frames1730–2350 (620 scene submissions); 62 consecutive ten-frame intervals give median3.517 scene submissions/second, range3.101–3.968. Actual screen captures show3.4FPS/293.18ms initially and3.3FPS/304.66ms at the last capture, with the room, lighting and changing TV video present. The fresh launch segment contains no fatal/assertion/UBSan failure; server simulation reaches69,850ms beyond the earlier failure section. Android reports thermal status0 in the captured snapshots. These are sequential development measurements, not a controlled thermal A/B comparison or a long-duration stress test.

| Measured phase | Mean milliseconds |
| --- | ---: |
| Begin | 3.335 |
| CPU scene recording | 100.834 |
| GPU offscreen scene | 139.866 |
| Scene fence wait | 142.603 |
| Pipeline key work (part of recording) | 4.069 |

Average scene workload is1,079 draws/26 passes; program creation is0ms during the sample. GPU time and fence wait overlap and must not be added together. GPU scope excludes uploads/window output. This roughly fourfold visible FPS gain from the initial0.9FPS capture still falls below the user's reported D3D9 7.8–11FPS. Thirty FPS, full gameplay, frontend interaction and lifecycle recovery are not verified.

Evidence: `sdk606-sustained1.json`, its `-entry.log`, `-logcat.txt`, thermal snapshots and six same-PID screenshots; `sdk606-profile-poll.txt` provides additional timings. The original following logcat output was buffered until completion, not missing. The observer now polls and flushes the ring, excludes pre-observation lines and uses explicit-year timestamp parsing. Source hashes and the r_state exact-scope proof are in `stage86-final-source-proof.json`. Historical crash-buffer entries are retained as evidence and must not be mistaken for a new SDK606 crash.

The next performance work is actual per-pass GPU timing and mobile attachment/pass policy, followed by the remaining roughly100ms of CPU recording. The current synchronous scene-fence/presentation structure also limits overlap. Any changes need real resource lifetime checks and visual comparison; neither resolution nor effects were reduced here. Windows reference, original assets and prior evidence remain untouched. Changes remain uncommitted.
