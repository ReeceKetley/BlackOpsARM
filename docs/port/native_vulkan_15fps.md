# Native Vulkan 15 FPS work

## Verified result — 15 FPS menu target reached

SDK626 / stage193, engine `0f936bf5d297af347ac488ae2bf2f53bc40c2ae7`, sustains **16.1356 FPS at 1280×720** in the actual main-menu room. The observer runs187.372 seconds with PID18582 unchanged.290 profiles span2890 submissions over179.107363 seconds. CPU scene recording averages39.280ms (median38.620), GPU scene51.750ms (median51.120), frame begin7.845ms. Pipeline-key time is1.051ms, versus2.557ms in SDK625. There are23 passes and about861 draws, with zero steady pipeline misses/creation. Six screenshots preserve the room, lighting, shadows and changing TV playback. Thermal status0 at both ends; no actual fatal/assertion in the current launch log.

| Measured configuration | Resolution | Sustained FPS | CPU recording mean | GPU scene mean |
| --- | --- | ---: | ---: | ---: |
| Previous diagnostics SDK621 | 720p | 8.71 | 70.74ms | 53.18ms |
| Optimized + depth omission SDK624 | 1080p | 10.43 | 47.79ms | 93.60ms |
| Plus sampler reuse SDK625 | 720p | 13.54 | 45.14ms | 55.53ms |
| Plus owned shader keys SDK626 | 720p | **16.14** | **39.28ms** | **51.75ms** |

Sequential observations have uncontrolled CPU/GPU clocks and scene animation. Do not attribute the entire difference to any one change. This meets the requested menu15+FPS milestone at acceptable720p; it does not establish15FPS at1080p, gameplay performance, or full lifecycle/backend validation. The last measured1080p build predates sampler/key changes. No rendering effect was removed in these stages.

Evidence: `build-android-audit/native_registry_closure/sdk626-ownedkeys720p1.json`, matching logcat/entry/crash/thermal files, six screenshots, pass-summary JSON and `sdk-package-verification193.json`. The overlay still labels its legacy GPU query unavailable; native GPU numbers above come from actual Vulkan timestamps in the log.

The installed APK remainsSDK626 at720p. Source matches its executable changes; the pipeline-key header has a later comment clarification only. Default correctness build flags remain unchanged, but these latest source changes have been built in the separate performance configuration, not rebuilt into the default diagnostics engine. Focused device fixtures use UBSan.

Next work: main scene and postprocessing GPU cost for1080p, remaining memory copying/descriptor work, then actual gameplay and lifecycle validation. Do not restart the live process merely because the observer finished.

## Objective and baseline

2026-10-15: the user requests sustained15+FPS on the HONOR tablet. Continue the same main-menu scene, preferring1080p with720p acceptable. Preserve recognizable lighting, shadows and atmosphere; efficient native effect variants are authorized. Original assets remain unchanged. The prior milestone measured8.061FPS at1080p and8.707FPS at720p with identical engine7b6566fcb8bc0dde7b24bee1b3063fe41fbdcdcb. See native_vulkan_d3d9_parity.md for evidence and measurement scope.

15FPS requires a frame every66.667ms. The prior1080p GPU scene average was96.477ms, so CPU optimization alone cannot establish the preferred-resolution target. At720p GPU scene53.182ms and CPU scene recording70.736ms identify a plausible CPU bottleneck. Both omit additional work outside their defined scopes; actual sustained submission rate determines success.

## Stage99: separate performance configuration

`BO1_NATIVE_PERFORMANCE_BUILD` is opt-in and defaults OFF. The separate `build-android-audit/native_engine_perf1` configuration applies O2, debug symbols, strict floating point (`-fno-fast-math -ffp-contract=off`) and disabled strict aliasing to the nine engine/library targets. Assertions and existing ABI/feature definitions remain. Engine UBSan instrumentation is disabled with the final `-fno-sanitize=all`; function instrumentation flags are removed. Imported providers/assets are unchanged. This experiment does not change the default correctness configuration or claim that diagnostics overhead explains all frame time.

Actual generated compile commands cover878 source entries across the selected targets. The first proof script failed to normalize Windows command-path separators and matched zero entries; its artifact is retained. Corrected stage99-compile-flags2.json verifies all selected commands. A fresh configure of the default build leaves all1114 compilation commands byte-equivalent as parsed records and preserves the existing engine SHA256. Evidence: stage99-debug-preserved.json and stage99-debug-commands-before.json.

The launcher now accepts an explicit `--build` directory within this worktree; its default remains the diagnostics directory. Full build, APK provenance, real startup, sustained scene performance and appearance are required before attributing any improvement to the new profile.

Stage99 actual SDK623/stage190 engine c367cca828128524cf3eb9fbbd196a0c7d67082e passes startup and124.339seconds of observation with the same PID5969. There are113 room profiles,23 passes and approximately861 draws. Actual1120 submissions over116.984429seconds give9.573924FPS at1080p. CPU recording averages57.335ms (median52.983), GPU99.161ms (median97.527), begin11.652ms. This improves sustained throughput over SDK6228.061FPS, but does not reach15FPS. Four captures preserve room appearance; GPU clocks are not controlled, so small GPU differences are not attributed to this CPU-only build change. Evidence: sdk623-perf1080p1.json/logcat.txt/screen1..4.png, sdk-package-verification190.json. Default diagnostics compile commands and engine binary remain unchanged by the profile experiment.

## Stage100: omit unused depth attachments

A new colour pass omits depth only when canonical draw state disables depth testing, writes, front/back stencil and polygon bias. Already-open depth passes remain intact. A later depth-dependent draw or depth/stencil clear ends a colour-only pass and reopens the actual depth attachment; image tracking retains its prior contents and layout. Material validation and profiling use the attachments actually present. This changes no effect shader, resolution or draw count.

The focused actual-Adreno test inserts colour-only attachment work between depth production and resumed LOAD. All96 depth/stencil readback pixels across three cases pass, including the untouched half after a partial overwrite. Seven negative checks and submission journal checks remain. Evidence: stage100-depth-fixture.txt; both fixture and full-engine builds pass. This fixture tests GPU preservation, not full game draw selection. SDK624/stage191 engine5eec5a75f6517e6694d32f212c10c67b5932783e is deploying for1080p integration and measurement.

## Next measured candidates

- Omit depth attachments from eligible postprocessing draws while preserving actual depth contents for later readers.
- Eliminate depth copies only where producer/consumer tracking proves no relevant contents changed.
- Cache immutable samplers and shader identities with device and lifetime correctness.
- Efficient native depth-of-field/bloom variants if necessary, with actual visual comparison.

No15FPS completion claim is made until a sustained real-scene run reaches the target.

Stage100 integration SDK624 completes123+seconds with unchanged PID12995 and125 room profiles. It sustains10.434168FPS at1080p over118.840332seconds, CPUrecord47.788ms (median47.114), GPU93.604ms (median92.671),23passes. Post passes19/21/23/28 now have no depth attachment. Main room appearance is preserved in four captures. Pass19 remains about6.1ms and final DoF pass28 about8ms; omitting depth is not the large GPU saving hypothesized. Sequential device variation also affects the comparison. Evidence sdk624-depthless1080p1*, including pass summary.

## Stage101: reuse live samplers

Sampler::Acquire uses a bounded256-entry weak cache keyed by device, decoded state, mip count, exact bias bits and comparison mode. The cache owns no Vulkan resource lifetime: immutable captures retain samplers until their real users/fences finish. Expired entries are pruned at capacity; metadata eviction cannot invalidate captured users. Invalid inputs still fail through normal validation. Actual Adreno test proves reuse, distinct mip/bias/comparison identities, invalid-input rejection,300 live borrowers across cache eviction, and expiry after the last real owner. The first fixture summary incorrectly labelled this as a shader draw; corrected fixture2 explicitly records real_shader_material_draw_tested=0. Both pass results are retained.

Full engine and focused builds pass. SDK625/stage192 engine62608a895ede3df891c998cfccfb66597441e31e is deploying at720p. This changes resolution and sampler reuse together, so results cannot isolate the cache speedup.

Stage101 actual SDK625/stage192 completes150+seconds with PID14375 unchanged:203 room profiles and2020 scene submissions over149.142786seconds yield13.544068FPS at720p. CPUrecord45.142ms (median43.054), GPU55.529ms (median54.740), begin9.930ms;23passes and about862draws. Room appearance remains recognizable at720p. Evidence sdk625-sampler720p1*. Goal not yet reached.

## Stage102: owned shader pipeline keys

Pipeline descriptions may provide the actual immutable registered shader records. Keys validate that the supplied bytecode pointer and size match those owners and retain shared ownership, avoiding bytecode copies/hashing/comparisons per draw and preventing address-reuse aliases. Keys from raw preview/test descriptions retain owned byte copies and value equality. All render state, vertex layout and render-pass identity remain part of the key.

Actual ARM64 UBSan fixture verifies owner reuse, different-owner separation, mismatched bytecode rejection, retained lifetime until keys retire, and unaligned raw-copy fallback. Full engine and fixture compile. Evidence stage102-key-fixture.txt and stage102 build logs. SDK626/stage193 intended next720p measurement.

Stage102 actual integration completed successfully as recorded above. Reproduction for the installed process: `python tools/port/observe_native_vulkan_performance.py --sdk 626 --suffix NEW_UNIQUE_SUFFIX --seconds 180 --min-room-draws 700`. For a new changed build, use `run_native_vulkan_engine.py` with `--build build-android-audit/native_engine_perf1`, fresh SDK/stage numbers (next627/194), and `--height 720`; preserve existing evidence.
