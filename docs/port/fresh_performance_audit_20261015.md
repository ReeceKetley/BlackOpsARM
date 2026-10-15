# Fresh performance audit — 15 October 2026

Source audit of the current working tree for ARM64 / Adreno 610 / Vulkan 1.1. Three independent agents reviewed pass/synchronization, draw/shader dispatch, and visibility/shadows. All fresh findings were formed without reading the ledger or earlier reports. The root then reconciled them against the ledger and SDK656/660 evidence. No engine code, build, device settings, or assets were changed. No new device benchmark was performed; estimated FPS gains would be speculation.

## What the evidence supports

The remaining problem has at least two components: sustained dense-view CPU recording cost and first-use pipeline creation hitches. SDK656's existing roaming capture has 183–2350 packets and CPU recording 6.151–57.292 ms; offscreen GPU time is 13.838–35.138 ms. SDK660's fixed dense view records about 57 ms even with compatible descriptor reuse. Its largest captured pipeline burst still totals 242.382 ms across 114 creations. These historical observations explain why lower resolution alone cannot stabilize all views. CPU/GPU times overlap and must not be added; scene submissions are not display FPS.

Evidence: `docs/port/native_vulkan_pipeline_cpu_kino_startup.md`, `build-android-audit/native_registry_closure/sdk656-roaming540-30s/report.md`. These are prior measurements, not measurements of changes proposed here.

## Findings and recommended order

### 1. Correct the visibility-byte / shadow-bitmask mismatch

`src/gfx_d3d/r_dpvs_static.cpp:1156–1173` scans `staticSurfaceCount / 32` entries after every static cell job. It treats each `g_surfaceVisData[k]` as a 32-bit visibility mask. The declared type is a byte pointer (`r_dpvs.h:475`) and producers write one byte per surface (`r_dpvs_static.cpp:1317,1354,1442`). A byte value of 1 makes `R_CalcSurfaceNoDynamicShadow` update surface `32*k+31`, rather than surface k. Most intended entries are consequently missed.

The modified flag is bit 57 (`r_dpvs_static.cpp:1054–1068`); `r_draw_material.cpp:191–192` uses it to select a technique without sun-shadow sampling. Incorrect flags can retain unnecessary shadow sampling or incorrectly remove shadows. This is a concrete correctness issue; its GPU cost depends on distances, materials and lighting. The repeated CPU scan is also concrete, but has not been timed separately. Static jobs have concurrency limit 1 (`r_workercmds.cpp:54`). Cull-group surfaces are added after the scan (`r_dpvs_static.cpp:1095`), another reason to resolve this at a completed visibility boundary.

Repair direction: calculate flags using actual surface indices once after the relevant visibility jobs complete, before draw-surface emission. `r_scene.cpp:3809–3822` is a candidate boundary, subject to verifying all view dependencies. Do not replace the existing tail with a full byte scan in every cell job. Validate surfaces 0, 1, 31, 32 and the final partial group, then visually compare shadow-distance boundaries and moving views. Correctness repair may change workload in either direction; no speedup is guaranteed.

### 2. Reuse pipelines across compatible render passes

`attachment_pass.cpp:82–84` creates distinct render-pass handles for load/store policies. `pipeline_key.cpp:81,126,168` keys graphics pipelines by the exact handle. Consequently, otherwise identical draw state can create separate pipelines for compatible CLEAR/LOAD or preservation variants. Creation is synchronous (`graphics_program.cpp:97`). The 512-entry cache (`engine_frames.cpp:14`) evicts the first unborrowed entry rather than a least-recently-used entry (`program_cache.cpp:61–67`).

Use a canonical render-pass compatibility identity in the pipeline key while retaining real render-pass objects and all actual pipeline state. Load/store and initial/final layouts can differ for compatible passes; formats, sample counts, attachment references, and other compatibility requirements still matter. This is explicitly supported by [Vulkan render-pass compatibility](https://docs.vulkan.org/spec/latest/chapters/renderpass.html#renderpass-compatibility).

Measure unique compatibility keys, total creation time and evictions. Warm-up/precreation of observed scene variants can address remaining first-use stalls. Existing captures demonstrate compilation bursts but do not establish cache-capacity thrashing. Increasing 512 blindly is not the first fix. Validate reuse through actual GPU draws with CLEAR and LOAD passes and incompatible-format negative cases.

### 3. Reuse unchanged constant uploads independently of draw state

`material_dispatch.cpp:113–122` uploads required constant banks whenever the overall draw fast path fails, including geometry-only or texture-only changes. `material_bindings.cpp:66–92` trims uploads to shader-visible row counts, but does not reuse an unchanged stage allocation. Changed offsets also force binding work. `constant_state.cpp:19` copies the complete two-stage 8 KB snapshot on the first changed constant write while `BindCache::last.constants` retains the previous snapshot.

Use per-stage revisions and retained per-frame uniform allocations, with immutable source snapshots or equivalent lifetime-safe change tracking. Stage splitting can avoid copying an unchanged 4 KB bank. Compare sampled texture slots rather than whole texture snapshot identity (`material_dispatch.cpp:46`) to avoid unrelated bindings forcing the slow path. Include shader constant layout/range in reuse validity; never reuse an allocation after its frame arena resets.

This targets steady CPU cost. SDK656 already attributes roughly 5.5 ms to upload and 10.9 ms to descriptor processing in one dense frame; those totals are not estimates of recoverable savings. SDK660 descriptor compatibility alone saved only about 0.5 ms, so merely reducing descriptor writes further should not be sold as the complete fix.

### 4. Extend far-shadow caching to frontend static work

The backend skips far static caster draws on cache hits (`r_draw_sunshadow.cpp:144–155`), but `R_SunShadowMaps` still traverses both partitions (`r_sunshadow.cpp:1160–1177`) and merges their draw lists (`:1228–1232`). Far-cache mode deliberately disables visible-cell filtering (`r_dpvs.cpp:3836`) and scans world cells against its frustum; selected cells schedule four worker commands (`:3926–3935`).

Cache the static visibility/list work using the existing world-locked cache identity, while continuing dynamic casters. Persist actual owned data, not pointers into recycled frame allocations. Invalidate on relevant world, light, cache-region and static-content changes. This is additional frontend work outside the GPU draw saving already measured in the ledger. Measure frontend wall time and worker time separately: it will not directly remove the entire 57 ms backend recording cost.

### 5. Distinguish depth changes from stencil changes

`pass_recording.cpp:342–343` invalidates depth-copy reuse for any stencil-enabled draw, even with depth writes disabled. `ResolveDepth` copies only the depth aspect (`:178`). Separate content generations can prevent unnecessary depth copies when only stencil changed. Two depth resolve sites exist in `rb_draw3d.cpp:897,948`; a real copy ends the pass and subsequent rendering reloads attachments. Track all real depth writers, clears and copies, preserving combined depth/stencil synchronization requirements.

This is more precise than the already-existing approximate `r_nativeDepthResolveOnce` option. Benefit is conditional on intervening stencil-only work; count suppressed copies before claiming it affects the slow view. [Khronos attachment guidance](https://docs.vulkan.org/samples/latest/samples/performance/render_passes/README.html) explains why extra attachment loads/stores matter on tile-based hardware.

## Secondary opportunities

- `material_dispatch.cpp:63,139` creates a temporary geometry-retention vector; `geometry_bindings.cpp:90` pushes into it and it is immediately destroyed. Remove redundant allocation only after verifying retained packet/last-snapshot ownership for the reused-draw path. Small CPU cleanup, not a plausible sole 30 ms win.
- `buffer_assets.cpp:103–122` MapDiscard bypasses the spare pool used by Write/UpdateRange. Retained readers trigger fresh VkBuffer/memory allocation; stream2 additionally preserves the full payload. Callers include fullscreen quad rebuilding and TerrainScorch, so classify it as event-dependent until counted, not generic per-frame mesh traffic. Reuse retired backings without overwriting live readers.
- `attachment_pass.cpp:73–78` and `image_usage.cpp:83–94` use broad vertex/fragment/transfer dependencies. Track actual producers/consumers before narrowing. Do not remove vertex-stage coverage globally: vertex sampling is supported. This is a GPU hypothesis, lower priority than measured dense CPU pressure.
- Per-pass timestamp/logging runs every tenth frame. An opt-in comparison could quantify observer overhead; existing ledger evidence did not show a material gain from earlier sampling changes.

## Candidates excluded after ledger reconciliation

Native BC texture sampling is already enabled and verified on this Adreno 610 (FACT-0521), so the RGBA8 fallback is not an outstanding bottleneck for this tablet. The separate performance build already uses O2 with engine tracing/UBSan disabled and strict FP retained. Scene submission already calls Submit(false); normal steady-state device-idle waits were not found. Driver pipeline persistence, shader-pair caching, compatible descriptors, compact constant uploads, draw merging, dynamic Write/UpdateRange spares and native post-processing already exist. The current source defaults partial-precision translation on; older STATE paragraphs describe earlier defaults and are stale.

## Focused verification sequence

First repair/test the visibility correctness issue in isolation. Then compare pipeline compatibility reuse and stage-constant reuse separately. For each, use the same good and bad camera positions, same build settings and resolution, and warmed views for sustained comparisons. Record every first-use pipeline event separately from steady frames. Capture frame-time distribution, CPU recording/frontend/worker cost, GPU scene and window work, actual draw commands versus retained packets, allocation/copy bytes and cache evictions. Preserve representative screenshots and pending-fence ownership checks. Shadow frontend caching and depth-generation refinement follow according to those results. No Vulkan upgrade or original asset modification is required by these proposals.
