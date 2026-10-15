# Native Vulkan presentation — stage 3

2026-10-14. Native replacement goal incomplete. `src/gfx_vulkan/presentation.{h,cpp}` implements a reusable SDL/Vulkan1.1 device/surface/swapchain owner. A development-only entry in the real EngineActivity/SDL_main exercises it. This mode explicitly logs PRESENTATION_ONLY, clear_only=1 and Com_Init_called=0; default launch still runs the original engine. It is not game rendering, a replacement-backend completion or a performance benchmark.

## Ownership and behaviour

SDL supplies required instance extensions and the platform surface. Queue/device selection checks Vulkan1.1, swapchain extension and graphics/present support. Surface format, extent, transform, alpha and present mode come from supported capabilities. MAILBOX is a preference, FIFO the fallback. Two CPU frame slots own command buffers, acquire semaphores and completion fences. Present-ready semaphores belong to swapchain images, so reacquiring an image establishes retirement of that image's previous presentation wait. The caller receives a frame context with command, extent, slot/image and generation for subsequent material integration.

The render pass clears/discards prior color and stores output for presentation. No per-frame device/queue idle; resize/surface replacement/shutdown drain outstanding work before releasing owners. Acquire/fence timeouts defer frames. OUT_OF_DATE triggers rebuild; usable SUBOPTIMAL continues without repeated recreation. Surface loss defers rendering, retires the old swapchain/surface, recreates through SDL and rechecks present support. Rebuild generation informs higher-level render resources. This initial swapchain pass is not the final scene/post frame graph.

`native_vulkan_entry.cpp` and a development Intent extra select this mode through the actual APK. Java normal launch and original Com_Init path remain unchanged. A presentation-only overlay label prevents diagnostic clear rate being confused with game FPS; current diagnostic entry has16ms host pacing and no game input controls. Native GPU timestamps are not yet connected. DXVK library is still packaged/linked by the reference engine, but the presentation path does not issue D3D9 calls.

## Actual tablet results

- SDK485/stage54 first native window visibly presented1920x1200 clears. Returning from HOME/recents exposed VK_ERROR_SURFACE_LOST_KHR; it failed explicitly and released Vulkan resources with status1. Preserved failure log and first screenshots, not a clean-success claim.
- SDK486/stage55 BuildId7fd6687ff5d8ed5f8690c520c730fc267a2e02a8 installs/verifies all seven providers. PID10459 presented clear frames at generation2. HOME/recents return kept the same PID and recreated surface/swapchain; generation3 advanced beyond8,280 diagnostic submissions without reported native failure. Screenshot confirms native output. No FPS claim for the game.
- DXVK, SDL, environment, Steam and UBSan provider hashes match stage53. Engine and shader compiler changed, the latter for stage1 SPIR-V LRP support. No asset import/transfer. Reused committed game roots.
- Escape/BACK was not delivered as a usable SDL quit event in the installed SDK486 mode, so successful normal teardown is still pending. Added a presentation-only Java BACK/ESC route calling SDLActivity.nativeSendQuit; its Java compile passes, but this edit is NOT installed. It will be tested in the next changed APK. Original game input behaviour is unaffected by this guarded branch. Do not claim SDK486 clean exit.

Evidence `build-android-audit/native_registry_closure/`: SDK484 compile log, SDK485/stage54 and SDK486/stage55 build/package verification/install records; `sdk-native-vulkan485-presentation.log`, `485-resume-failed.log`, `486-presentation.log`, `486-resume.log/png`, `486-teardown.log` (still running), `486-exit-java-compile.log`. Native mode launcher is `tools/port/run_native_vulkan_presentation.py`; use fresh SDK/stage numbers and retain logs. Helper selects existing enabled LAUNCH GAME with developer extra, performs no import, preserves earlier logcat and filters by current PID. USB stay-awake restored0.

## Current state and next

At this handoff native presentation PID10459 remains live. Installed SDK486 has surface recovery but not the subsequent Java quit-route edit. Normal app launch remains D3D9; no native Com_Init/full-game frame. Next changed build SDK487/stage56. Combine real material pipeline/resource upload with this presentation owner, then adapt canonical engine image/shader/buffer providers and frame scheduling. Validate quit after the explicit SDK quit route is installed. Vulkan validation-layer injection not performed for this WSI stage; no claim of validation-clean synchronization beyond reviewed ownership and actual successful execution.

Full native scene, all material states/resource formats, gameplay, lifecycle beyond this pause/surface cycle, native metrics and mobile performance optimization remain outstanding. Pre-native checkpoint4247b269 is intact; current changes are uncommitted.
