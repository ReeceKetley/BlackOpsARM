# SDK463 command-submission failure and persistent stall

2026-10-14. Bounded source/binary audit plus root's live LLDB capture of actual engine PID22952. SDK463/stage35 engine BuildId `813ae076f2c9caaff639c3c1d4a5e2cd773ed789`. The persistent stall is explained by a confirmed DXVK error-recovery self-deadlock. The initial `vkQueueSubmit` return `VK_ERROR_INITIALIZATION_FAILED` (-3) remains unproved.

## Actual observation and matched stacks

`native_registry_closure/sdk-native-entry463-follow1-only.log` records CA_ACTIVE at78104ms, then the stencil-export fallback diagnostic and one command-submission failure. The process stayed alive, with growing render/GPU sample ages and no subsequent submissions observed. No new UBSan failure was observed in this run. These are stalled-run observations, not playable 3D or a gameplay benchmark.

Root attached LLDB to this same process and saved `sdk463-lldb-backtrace1.txt`, `sdk463-live-maps2.txt` and `sdk463-live-symbolized1.txt`. Thread40, named SDLThread, was inside `std::mutex::lock` -> `DxvkSubmissionQueue::synchronize+0x28` -> `DxvkDevice::waitForIdle+0x20` -> `DxvkSubmissionQueue::submitCmdLists+0x2dc`. Thread30, bo1-render, waited in `synchronizeSubmission` -> `waitForSubmission` -> `D3D9SwapChainEx::PresentImage` -> `Present` -> engine `RB_SwapBuffers`. Thread42's DXVK CS worker was blocked entering `DxvkSubmissionQueue::submit`; thread23 waited for the renderer in the canonical frame path. This establishes the recovery stall and its renderer propagation.

Initial debugger stop reasons in Java/JIT threads were SIGSEGV stops. They are not evidence of a new engine crash; root detached and resumed the process. After preserving diagnosis, root intentionally force-stopped the stalled APK and removed its owned ADB forward19063. The dirty guard is retained; clean canonical shutdown is not claimed. This audit owner performed no device action.

## Why recovery self-deadlocks

Protected `../DXVK-Native/src/dxvk/dxvk_queue.cpp:123` reacquires nonrecursive `m_mutex` after the real submit. At129 the failed command list is logged;131 calls `m_device->waitForIdle()` while that mutex is still owned and before135 pops the submission entry. `dxvk_device.cpp:252` calls `lockSubmission()`, whose `dxvk_device.h:429` implementation calls queue `synchronize()`. `dxvk_queue.cpp:65` then attempts the same mutex. Android's `dxvk::mutex` is `std::mutex`, not recursive. The submission thread cannot reach queue pop, completion notification, or the eventual Vulkan device-idle call.

Stage35 packaged machine code independently confirms this order:

| Module offset | Operation |
| --- | --- |
| `0x1e7fb4` | submit worker locks its submission mutex |
| `0x1e80c8` | stores asynchronous submission error |
| `0x1e80d0` | calls `DxvkDevice::waitForIdle` |
| `0x1e80d8` | queue pop, after the blocked call |
| `0x1c7b28` | idle routine calls queue `synchronize` |
| `0x1e7da8` | synchronize attempts `std::mutex::lock` |

Actual captured return PCs `0x1e80d4`, `0x1c7b2c`, `0x1e7dac` match these call sites exactly. The same source sequence exists in the protected spike. The provider was not changed. Potential resource-idle spinning follows from `DxvkResource::waitIdle`, but the observed roughly one-core process activity has not been attributed to that function by this stack capture.

## Initial failure: bounded candidates and limits

This -3 log comes from command-list submission, not presentation alone: the logging branch accepts a command list or DEVICE_LOST, and a present-only -3 would not enter it. `DxvkCommandList::submitToQueue` forwards `vkQueueSubmit` status. Actual graphics/transfer families both0 exclude the dedicated-transfer branch for this device. Which recorded graphics command caused rejection is unknown.

The concrete newly exercised difference from the working spike is multisampled depth resolve. SDK463 says `Using 4x anti-aliasing`; the spike's zero-initialized presentation parameters select no multisampling. Engine `rb_backend.cpp::R_ResolveIntZ_PC` binds FLOAT_Z and emits POINTSIZE `0x7fa05000` RESZ. Protected `D3D9DeviceEx::ResolveZ` routes a multisampled source to `resolveDepthStencilImage` using SAMPLE_ZERO for both aspects. Its framebuffer fallback selects a depth-only shader when stencil export is absent, producing the observed diagnostic. That diagnostic does not throw or return -3. Temporal proximity makes this command family a focused candidate, not a proven cause.

Engine D24FS8 and spike D24S8 both map to Vulkan D24_UNORM_S8_UINT; the differing D3D format name does not establish a Vulkan-format fault. The saved spike compat result checks INTZ format support, draws simple scenes, switches targets, resets and presents. It does not perform RESZ or prove actual 4x depth/stencil resolve. Its toy rendering therefore cannot establish support for this engine operation. Other engine scene shaders/resources or command-list synchronization remain possible initial causes.

Root selected the ordinary engine `r_aaSamples1` configuration for SDK464/stage36 as a focused follow-up to separate the multisample-resolve path. Its runtime result is pending in this report; neither success nor initial-cause proof is claimed. No Vulkan upgrade, provider patch, asset mutation or broad test was proposed. The user SynchronizePresent SUCCESS/SUBOPTIMAL acceptance remains preserved.

## Frozen evidence

Dedicated evidence is `build-android-audit/native_dxvk_submission_error_stall1/`: selected actual log lines/stacks, packaged recovery disassembly/symbols, bounded source/evidence SHA256 manifest and read-only `capture.ps1`. Raw root evidence remains in `native_registry_closure`; the report references it rather than replacing it. Stage35 DXVK SHA256 `6E546EE31B8E45AF75EC0385E7B0B33A0BFEFD4810F9DD9B7237BB3FF7D9CF18` and SDL SHA256 `11C28146F62F5BB13CEE863C1203D574324EF83931F1C668A3A181B470235E11` match the saved working baseline.

This owner wrote only this report and its dedicated evidence. Root owns build/device/Ledger/STATE; no providers, engine sources or configuration were edited by this owner, and no additional broad tests were run.
