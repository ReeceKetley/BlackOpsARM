# Native Vulkan backend: audit and migration plan

2026-10-14. Requested read-only architecture audit, following the user's proposal to replace Android D3D9/DXVK. No native backend implemented by this audit. Windows BlackOps64 remains the validated read-only reference. Retail asset bytes and the working DXVK provider are preserved. Target remains Adreno 610 / Vulkan 1.1.128.

## Recommendation

Build an optional engine-owned Vulkan backend beneath T5's existing material, draw-list and frame scheduling code. Retain the D3D9 backend as a selectable reference until feature parity and measurements justify changing the Android default. The main potential benefit is explicit knowledge of pass lifetimes and dependencies, allowing fewer copies, attachment loads/stores and pass breaks. Replacing API calls without changing that scheduling need not deliver a large GPU gain.

Do not promise 40–60 FPS. The cited 110 ms to 16–25 ms prediction implies eliminating roughly 77–85% of the current GPU span; no measurement establishes that proportion as avoidable memory traffic. GPU intervals also include synchronization/drains and profiling perturbation. CPU and GPU work overlap, and CPU/audio/movie/simulation bottlenecks can become limiting after GPU improvements.

## What the proposed optimizations actually permit

| Proposal | Verdict for this engine | Evidence and conditions |
|---|---|---|
| Transient/lazy depth attachments | Useful only for attachment-only data whose lifetime is contained in a render pass. No guaranteed 100% on-chip residency. | The tablet's native spike allocated a lazy transient attachment using memory type 6. This proves allocation support, not GMEM residency or bandwidth savings. Transient image usage cannot include ordinary sampled/transfer usage. |
| Discard depth after the prepass | Incorrect if subsequent scene passes consume that depth. | `rb_draw3d.cpp:804` sequences prepass before lit, decals, lights and emissive. `r_rendertarget.cpp:1214` explicitly shares scene depth with another target. Keep compatible stages in one render pass; discard only after final attachment use, and preserve any depth output required by later sampling. |
| Discard shadow maps after generation | Incorrect for the sampled shadow output. A separate unsampled scratch attachment may qualify. | `rb_backend.cpp:6312` publishes the sun shadow image; `rb_draw3d.cpp:607` binds spot shadow texture for scene drawing. Shadow projection samples other coordinates, so ordinary same-pixel input attachments cannot replace this read. Store the shadow output until its final reader; audit atlas reuse/partial updates before deciding next-frame loads. |
| Make all temporary post targets transient | Incorrect for filtered images read in later passes. | `rb_postfx.cpp:1349–1428` samples bloom mip/ping/pong and resolved-scene images in downsample, convolution, blur and composition. `rb_backend.cpp:6325` publishes FloatZ as a texture. Their temporary lifetime does not make them attachment-only. |
| Direct UMA vertex/index mapping | Appropriate, but already present for the relevant DXVK buffer class. | T5 `r_buffers.cpp:98,201` requests usage 520 (`DYNAMIC|WRITEONLY`), DEFAULT pool. DXVK `d3d9_common_buffer.h:91` selects DIRECT mapping for these; `d3d9_common_buffer.cpp:86–95` requests HOST_VISIBLE, HOST_COHERENT and DEVICE_LOCAL. A new backend still needs ring allocation, alignment and completion-based reuse. Coherence does not permit overwriting GPU-owned ranges. Actual selected memory types/upload bytes remain to be measured. |

Khronos guidance: [render-pass attachment lifetime](https://docs.vulkan.org/samples/latest/samples/performance/render_passes/README.html), [memory allocation and UMA](https://docs.vulkan.org/guide/latest/memory_allocation.html), [image creation usage restrictions](https://docs.vulkan.org/refpages/latest/refpages/source/VkImageCreateInfo.html). These techniques exist in core Vulkan 1.0/1.1; do not depend on newer dynamic rendering, synchronization2 or Qualcomm extensions without actual device support.

## Actual engine seams and larger work items

| Area | Current source seam | Native responsibility |
|---|---|---|
| Window/device/lifecycle | `r_init.cpp`, Android SDL integration | Own Vulkan instance/device/queue/surface/swapchain, acquire/present semaphores, resize/surface loss and teardown. Preserve the proven SUBOPTIMAL behavior without ignoring genuinely incompatible surfaces. One backend owns an entire frame and its resources. |
| Images and render targets | `r_image.cpp`, `r_image_load_common.cpp`, `r_rendertarget.cpp` | Format support, mips/cube textures, uploads, views/samplers, attachment identity/aliasing, depth/FloatZ/shadow formats, copies and layout tracking. Query exact format/memory capabilities. |
| Buffers and geometry | `r_buffers.cpp`, `rb_shade.cpp`, `r_drawsurf.cpp` | Persistent mapped upload ring, vertex layouts, indexed/UP draws, bounds/base offsets, fences and deferred destruction. Static assets upload once; preserve installed assets. |
| Materials/shaders | `r_material_shader_load.inl`, `rb_shade.cpp:515–536`, `db_material_graph_types.h` | Existing shader load definitions are SM3 bytecode, not SPIR-V. Translate bytecode, preserve constants/register layouts, sampler bindings, specialization behavior and pipeline keys. Packed material state needs Vulkan mapping, including alpha-test shader behavior. |
| State/passes | `r_state.cpp`, `rb_state.cpp`, `rb_draw3d.cpp`, `rb_postfx.cpp` | Convert material state to cached pipelines/descriptors, schedule render passes/subpasses and barriers, identify final consumers and full-overwrite opportunities. |
| Observability | Android GPU and performance overlay providers | Native timestamp availability/fence ownership, equivalent GPU phase boundaries, CPU attribution and presented-frame counters. Preserve labels distinguishing submission rate from displayed FPS. |

D3D interfaces are embedded in runtime shader, texture and primitive structures. This is more than changing the final Draw call. Introduce backend-neutral runtime handles or sidecars at these seams; do not reinterpret Vulkan handles as COM objects, change retail FastFile layout, or enable global x64 packing changes. Keep Windows behavior through the existing backend.

The initial shader path should investigate extracting the existing pinned DXVK **DXSO-to-SPIR-V compiler**, rather than rewriting every game shader. `DXVK-Native/src/d3d9/d3d9_shader.cpp:92–95` shows compilation depends on the D3D9 constant layout and module options. It is not a drop-in standalone compiler API: isolate those contracts, reflected resources, stage interface and generated compatibility logic, and preserve licensing. Reusing its compiler does not require retaining its D3D9 runtime. Handwritten shaders can follow for measured hot passes after matching original sampling/math/alpha semantics.

Avoid a second independent Vulkan device for isolated native passes sharing DXVK images: raw images cannot simply cross device ownership. The current pinned D3D9 provider exposes no public Vulkan resource bridge. Prefer an optional full-frame backend; a hybrid alternative first requires an explicit same-device bridge and ordered synchronization, not an assumed interop path.

## Ordered implementation milestones

1. **Complete the baseline and dependency capture.** Warm the real room with console/IME closed; record GPU phases, copies (source/destination/rectangles/bytes), pass breaks, attachment consumers and presented-frame count. Finish direct and call-chain CPU attribution for DXVK/driver per thread. Record profiler overhead. Produce a bounded resource-lifetime table for one actual frame, then check differing gameplay/shadow/effect paths. Do not test every asset individually.
2. **Create the selectable backend seam and native foundation.** Add an Android Vulkan option and an engine-owned backend interface. Keep D3D9 default. Bring up SDL surface/present, real capability checks, fences and teardown in the actual APK; no game asset reimport. Stubbed success is not an implementation.
3. **Prove one real material end-to-end.** Translate an existing T5 VS/PS; load its real texture, constants and vertex declaration; draw it through native Vulkan. Compare output against the D3D9 reference including alpha, half-pixel placement, orientation and sRGB behavior. The spike's generic pattern alone is insufficient.
4. **Reach the genuine frontend scene.** Implement asset resource creation, full material state, depth/FloatZ, draw streams and the scene/post/present sequence. Initially use conservative correct stores and barriers. Unsupported required features fail explicitly; retain the working backend for comparison.
5. **Optimize known lifetimes.** Merge compatible prepass/scene stages, discard only dead attachment data, avoid proven redundant copies, alias non-overlapping intermediates and cache pipelines/descriptors. Start with scene/pass boundaries: previously measured GPU scene work exceeds any one post copy. Retain sampled shadow/blur data. Change one family at a time with paired whole-frame timing and representative visual checks.
6. **Cover gameplay and adopt.** Validate Kino, moving camera, dynamic shadows, particles, DOF/bloom, UI/movie, map restart, pause/resume, surface recreation and cleanup. Change Android default only after correctness and repeatable total-frame improvement. No guaranteed FPS target based solely on the API choice.

First implementation deliverable: **optional native Vulkan window/present plus one actual T5 material compiled from its existing bytecode**, leaving the working D3D9 renderer selectable. This resolves shader/backend feasibility before committing the whole engine to the replacement.

### User refinement and checkpoint policy

The user subsequently clarified that the Vulkan backend should be portable to PC, with mobile-focused defaults and device-capability-driven memory/pass/scheduling policies. Share material and rendering semantics; isolate Android surface/lifecycle code from the renderer core. Avoid treating reduced visual quality as a mobile optimization default.

Pixel identity is a diagnostic tool, not the adoption requirement for the new backend. Accept small bounded differences in arithmetic/filtering after representative static and moving-scene inspection. Preserve all intended geometry, lighting, shadows, effects and UI; reject missing content, corruption, flicker and synchronization faults. Establish acceptable image differences from inspected outputs rather than choosing a permissive threshold to pass a failed experiment. Existing triangle/fusion mismatch evidence remains valid; it does not by itself establish unacceptable visible quality.

The user authorized a local pre-native Vulkan commit/checkpoint branch on 2026-10-14. This snapshots the current Android port, build integration, vendored source dependencies, durable reports and ledger. Generated objects/executables and ignored APK/build directories remain on disk. The separate DXVK-Native experimental checkout and accepted packaged providers remain unchanged by the BlackOpsARM commit; their existing reports and hashes identify the dependency state. No remote push requested.

## Existing experiments and current handoff

The prior native spike already compared separate passes versus merged lazy transient subpasses on this tablet: medians 2.972943 vs 2.995417 ms, no repeatable gain. It proves Vulkan 1.1 support and correct ownership, not a real-game speedup. See `../DXVK-Native/docs/mobile-native-subpass-results.md` and the existing `native_dxvk_hotpath_audit.md`.

The triangle integration also requires correction before adoption: SDK482 rejected second-frame comparisons (bloom 4,100 pixels; colour 3,648). Original onscreen draws remained active. SDK483/stage53 inadvertently reinstalled the same SDK482 BuildId because a host quoting error prevented the requested default-OFF source edit before that build. The edit was subsequently applied and compiled locally, but is **not installed**. Current source runtime control defaults false, CMake compilation option ON. Do not report this as an adopted game optimization. SDK479's compile failure and all SDK480–483 evidence are preserved. No production provider or assets changed.

At audit time engine PID30744 is live, SDK483 BuildId65e78fc4763e9a961c4323e6fba0edac3cef3fdb, marker52. No profiler/observer/build process remains. The source default-OFF engine needs a fresh SDK484/stage54 deployment before the final clean profile. SDK480 simpleperf evidence is provisional and console/IME contaminated; it is not a final estimate of DXVK-added frame latency. The user's fresh profiling request remains pending alongside this architecture plan.
