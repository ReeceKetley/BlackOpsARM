# Pipeline CPU batch and automatic Kino startup

2026-10-15. Installed SDK660/stage227, engine BuildId `5ee3b25e4d962f4585fe95f572881841f13dcb9c`, native Vulkan960x540. HONOR tablet AKWJ9X5116G04618, PID6390 left alive and paused in Kino, console closed, descriptor reuse enabled. Next SDK661/stage228.

## Changes

Pipeline cache misses now log their actual frame, creation time and shader names. GraphicsProgram separates translation, layout/module setup and driver pipeline creation. Additional clock reads occur on creation, not every draw. Existing one-in-ten summaries missed bursts between sampled frames.

A bounded shader-pair cache reuses linked SPIR-V across raster/blend/depth/vertex-layout variants. Keys retain both immutable shader owners and include exact alpha comparison/reference bits, depth-sampler mask, UBYTE conversion mask and precision mode. Raw bytecode callers retain direct translation. Programs own copies of translated output, so metadata eviction cannot invalidate an in-flight program. Clear releases both caches.

Frame descriptors can be reused across identically defined SM3 layouts. Stage visibility, fixed binding/type mapping and actual resource handles/ranges/layouts remain checked; dynamic offsets remain per draw. Compatible layouts can retain unaffected sets. A layout change at set0 invalidates compatibility at set1. Consecutive changes are bound together; multi-bank layouts retain combined binding. `/r_nativeDescriptorReuse 0` restores exact-handle caching/binding; default1 enables reuse.

The tablet had `debug.graphics.gpu.profiler.perfetto=1`, loading `libVkLayer_ADRENO_qprofiler.so`. It is now0 for performance testing. Native engine GPU timestamps are unchanged. New engine mappings show the vendor layer absent. No isolated FPS gain is attributed to this setting.

The launcher has a persistent **Start Kino automatically** checkbox, default on. SDK SDL arguments carry `--kino` or `--menu`. The native main owner waits for a genuinely active frontend and queues canonical `map zombie_theater` once. It consumes the launch request, avoids reloading gameplay already active, and allows a later return to menus. Headless/presentation behavior remains unchanged. The slash belongs to interactive console commands; internal Cbuf receives ordinary command text.

## Tests and measurements

The actual ARM64/Adreno fixture exercises three pipelines, two shader pairs, compatible-layout reuse, four buffer identities, changed dynamic offsets, alpha-NEVER rejection preserving prior pixels, pending-fence reset rejection, page/pool reuse and eight float RGBA readbacks over two submissions. All checks pass and the final harness exits0. Evidence: `build-android-audit/kino_depth_shader208/manifest-pair224b.json` and `tablet-pair224b.log`; game PID6390 remained unchanged.

Earlier fixture runs completed GPU checks but aborted during vendor profiling-layer C++ teardown. Matched stacks identify `libVkLayer_ADRENO_qprofiler.so`. Disabling the Perfetto property makes the same fixture exit cleanly. Failed evidence is preserved; no success check or assertion was bypassed.

SDK657 frame1874 created114 pipelines costing272.259ms; frame1875 created73 costing237.673ms. Many short creation calls accumulate into a large frame hitch.

Captured SDK660 Kino events contain237 new pipelines and41 shader-pair hits, avoiding82 stage translations. Hits have median whole-program creation0.483ms; misses3.135ms. These are different shaders, not a controlled speedup ratio. The largest observed burst still costs242.382ms for114 pipelines, including26 pair hits. Unique translations and driver compilation remain. Cross-build burst totals are not controlled A/B: cache warmth, profiler state and pipeline sets differ.

Same SDK660 PID6390, paused fixed view, four12-second windows:

| Window | Packets median | CPU record median | Descriptor median | Set misses | Sets bound | Scene submissions/sec |
|---|---:|---:|---:|---:|---:|---:|
| Off1 | 2564 | 57.558ms | 11.291ms | 803 | 5128 | 18.24 |
| On1 | 2569 | 57.275ms | 10.765ms | 513 | 3503 | 18.70 |
| Off2 | 2564 | 57.580ms | 11.241ms | 803 | 5128 | 19.06 |
| On2 | 2569 | 57.159ms | 10.736ms | 513 | 3503 | 19.14 |

Descriptor writes/allocations fall about36%, sets bound about32%, saving about0.5ms in that phase. Total recording changes by less than0.5ms. Overall FPS spreads overlap: no substantial steady FPS gain claimed. Console/input UI during toggles adds about5 packets to enabled windows (under0.2%). GPU scene medians vary35.5–40.3ms. CPU and GPU overlap and cannot be summed. GPU timestamps exclude window output/uploads; submission rate is not measured display FPS.

Automatic startup is verified by actual launcher UI, active frontend, exactly one auto-Kino log, `Server: zombie_theater`, continued scene frames and an unblurred gameplay screenshot. No manual map command was sent for SDK660. `gameplay-confirmed660.png` shows room/weapon/lighting; `left-paused-confirmed660.png` is the final state. Existing animation/developer-config/load warnings remain.

SDK658 exposed a separate lifecycle failure: a second manual devmap after Kino was already active unloaded/reloaded the level and failed `Native vertex buffer load: Native buffer context unavailable or owner already loaded`. See `sdk658-launch-complete.log`. Ownership checks remain; same-process level reload needs repair. Avoid redundant map commands during unrelated tests.

## Remaining work and evidence

Dense-view CPU recording remains about57ms; stable30FPS is not achieved. Simpleperf identifies draw dispatch, descriptor/driver command work, uploads and sun shadows. Next: profile retained scratch storage, constant/upload reuse and shadow casters in a fixed view. Shader-pair caching alone does not remove all first-use pipeline work.

Session artifacts: `build-android-audit/pipeline-cpu-stage224/`, including source backups, CPU recordings/reports, failed/passing test context, live657.log, sdk660-live.log, pair-cache-summary.json, final-off/on logs, ab-final-summary.json, settings/mappings and screenshots. Package/build/launch evidence lives in native_registry_closure and apk-native-stage227. No retail assets transferred/changed, Windows-reference writes, commits, resets or subagents.

References: [Vulkan layout compatibility](https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html), [Android GPU profiling property](https://android.googlesource.com/platform/cts/+/a475993fbd4ed8f04f41139d9a7dcea2caed26b5%5E%21/). Loaded-layer and clean-exit findings above come from actual device evidence.
