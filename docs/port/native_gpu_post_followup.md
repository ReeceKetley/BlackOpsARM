# Post-effects profiling and quality-preserving candidates

2026-10-14. Real HONOR Pad X8a/Adreno610/Vulkan1.1.128 frontend room. Original assets, providers, shaders, dimensions and visual settings preserved. User aims for30FPS while preserving game appearance; playable Zombies and30FPS remain incomplete.

## Actual fine timings and experiment outcome

SDK473/stage45 BuildId0f22c76c932b954d961199b653e864b2b823fa50 provides the baseline post profiler. Seven existing safely owned marks now surround initial scene resolve, BloomLDR, second resolve, the selected conditional effect, remaining filters and final resolve. Profile mode selects either coarse0..6 or post100..106; no added query count. Default-OFF PASS/POST options are ON only for current diagnosis. Legacy profiler remains false. Actual emitted profile=post/mask0x7f/phase_valid1 verified.

SDK474/stage46 BuildId3d62d7ac0f60df699304fc99f0c2b84fcb58332c tests a guarded depth-attachment optimization: only inside canonical RB_Filter, after actual pass setup, and with actual device depth-test/write/stencil all disabled. The exact attachment is retained, temporarily removed, restored before the next pass and released. Actual counters reached6040eligible/unbound/restored at the follow endpoint with no skips/no-surface cases; no helper fatal/UBSan was observed. Screens show the same room/effects, though animation/noise differ and screenshots are not bitwise equivalence proof.

SDK475/stage47 BuildId4fd227868d79e8f8eec691633083e17de7b125d1 restores OFF. Experimental engine hooks are excluded entirely when OFF; default remains OFF. Its source and fixture remain available for future evidence, but the experiment is not adopted.

| Adjacent elapsed GPU interval | Baseline OFF median ms | Experiment ON median ms | Restored OFF median ms |
|---|---:|---:|---:|
| Whole frame | 113.329 | 104.958 | 106.476 |
| Earlier rendering before post | 67.480 | 60.023 | 61.779 |
| Initial scene copy | 2.314 | 2.036 | 2.112 |
| Bloom chain | 17.016 | 16.397 | 16.650 |
| Copy before conditional FX | 6.249 | 5.758 | 5.720 |
| Selected FX (DOF in this room) | 7.889 | 7.708 | 7.694 |
| Remaining conditional filters | 0.000 | 0.000 | 0.000 |
| Final scene copy/publication | 11.467 | 10.853 | 10.897 |
| Tail/UI | 1.631 | 1.651 | 1.635 |

Baseline15valid warmed rows seq2475–2600; ON21rows2384–2571; restored21rows3091–3281. Each excludes first100sequences after its real room-entry landmark. These rate-limited logs are not every rendered frame. Component medians do not necessarily sum to the total median. Full-frame means120.171→110.351→110.236ms: the initial difference is dominated by earlier rendering outside the changed scope. The enabled and final-disabled means are effectively the same; no convincing isolated gain is established, so OFF is the decision. Different launches/windows/thermal history and unmeasured profiling overhead prevent a precise percentage-gain claim.

Bloom is about16.7ms in the restored run. Copy boundaries initially2.1ms, before conditional FX5.7ms and final10.9ms are substantial elapsed intervals. They are not standalone transfer costs: DXVK copyImage ends the render pass and completes preceding rendering/barriers. Deleting a copy cannot be claimed to save its entire interval. See native_dxvk_hotpath_audit.md.

## Real shader capture and concrete next candidate

SDK475 later ended after a debugger capture setup failure. The first host invocation lacked the bundled Python DLL environment (exit3221225781); the second connected but failed cached-module loading and exited batch1, after which PID20510 was absent. Failed commands/maps/logs/dirtyguard are preserved; this is capture-procedure failure, not proof of a game renderer bug or clean shutdown. Explicit aarch64 local-symbol loading was then validated offline before any further attach.

The unchanged installed SDK475 was relaunched through the observed enabled Launch button, without a build, package, install or asset transfer. SDK476 is the new observation label, not a new binary: same stage47/BuildId4fd..., actual CA_ACTIVE70343ms, PID29221/marker46 live. Normal stale-owner handling replaces the old guard; no guard was erased. USB stayawake restored0.

Two bounded successful read-only LLDB snapshots capture only bloom_apply_ldr/postfx_color and their selected technique4 pass data. Snapshot3 contains616B/980B pixel programs; snapshot4 adds two336B vertex programs, all6/13 material arguments and matching state words. Both pixel programs and both vertex programs are preserved, not replaced. The vertex programs are byte-identical; both PS programs sample the same interpolated UV without neighbor offsets/control flow. Film/color constants have overlapping destination registers with different phase values and must be separated in any fusion. Matching static state disables blend/alpha/depth/stencil and writes RGBA. Later mutable source snapshots are explicitly not draw-boundary sampler/constant proof.

Each successful snapshot uses a finally-detach path; LLDB exit0/detachsuccess, actual TracerPid0 afterward, servers ended and forwards19476/19478 removed. Current process preserved. Initial forward19475 removed after failure. Raw data sdk476-post-shaders3/4 and exact offline shader assemblies in docs/port/evidence/native_post_shader_fusion1. Report native_post_shader_fusion_audit.md.

Next implement and compare a derived bloom-application/color-grading shader with the original fallback. The original path has two full-resolution draws and an intervening copy; compatible fusion uses two original input samples in one draw. Prove actual sampler/sRGB state, image format, pixel-center conventions, dynamic film/color constants and intermediate quantization before adoption. Offline assembly alone is not GPU acceptance, image equivalence or FPS gain. Preserve instruction ordering/partial-precision behavior and alpha; do not simply pass unrounded bloom output to color grading.

Native Vulkan selected passes remain worthwhile candidates, not an automatic speedup. This DXVK version exposes interop for D3D11/DXGI, but not D3D9. A narrow ARM-owned provider bridge would need ordered recording, image barriers, owned resource retirement and state restoration; private COM layout casts are forbidden. Compute also spills render passes and ordinary RTs lack storage usage. A graphics two-subpass design with a UNORM intermediate input attachment could retain hardware conversion while avoiding an external copy, but needs actual format/limit/coordinate/synchronization proof and new bridge work. Vulkan1.1 remains the target. [Vulkan render-pass specification](https://github.khronos.org/Vulkan-Site/spec/latest/chapters/renderpass.html).

## DXVK audit

Read-only native_dxvk_hotpath_audit.md identifies copies/pass spills, repeated framebuffer creation, duplicate effective constant uploads and full-overwrite color LOAD elimination as candidates requiring bounded counts/proofs. Existing descriptor dirty gating, dynamic UBOs, recycled pools and217-entry shader cache are already active. Default constant-buffer memory likely already selects device-local coherent memory on this tablet. Do not disable general hazards, alter float/sampling semantics, upgrade Vulkan or change the protected SUBOPTIMAL fix. No provider edit or performance gain is claimed by the audit.

## Evidence and constraints

Raw SDK473/474/475 unique builds/packages/startup/follow logs, screens and JSON summaries in native_registry_closure; current observation476 reuses475 as recorded in sdk476-reused-installed-build.json. Engine hook inverse-byte proofs and CMake snapshots in native_gpu_post_profile1/native_filter_depth1; native helper ON/OFF compiles and production mocks (including explicit failures) in native_post_filter_depth1. Windows bodies unchanged through Android guards. Debug/assertions/UBSan/nonrecovering errors retained; previous source-scoped reverbO2 unchanged. No repeated broad headless suite, asset copy/hash sweep, commit/push or protected-tree write.
