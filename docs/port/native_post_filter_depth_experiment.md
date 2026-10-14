# Native filter depth attachment experiment

2026-10-14. Experimental helper only; no measured performance improvement or production adoption is claimed. The build option must default OFF. Root owns CMake, engine hooks, actual APK/device execution and ledger/state integration.

## Source evidence and rationale

`r_rendertarget.cpp:154–195,316–353,385–445` attaches the shared display-sized single-sample depth surface to reduced post-effect, bloom and pingpong targets through `RENDERTARGET_USAGE_RENDER`. DXVK Native `d3d9_device.cpp:5404–5448` includes any compatible-sample bound depth surface without checking ZENABLE. `dxvk_context.cpp:4045–4080` defaults bound depth/stencil to LOAD; `dxvk_renderpass.cpp:106–140` requests STORE. `d3d9_common_texture.h:343–355` normally selects DEPTH_STENCIL_ATTACHMENT_OPTIMAL even when depth writes are disabled.

This is a bandwidth candidate, not proof of wasted full-resolution transfers. DXVK `dxvk_framebuffer.cpp:133–158` computes framebuffer dimensions as the minimum attachment dimensions; `dxvk_context.cpp:4010` uses that reduced render area. An unused attachment may also be optimized by the device driver. Source inspection cannot establish physical bandwidth cost.

## Contract

New `src/platform/android/post_filter_depth.h/.cpp` expose noncopyable `FilterScope` and `DrawScope` in `bo1::android::post_filter_depth`. Only BO1_NATIVE_FILTER_DEPTH_UNBIND=1 activates the helper. OFF constructors/destructors make no D3D calls and emit no diagnostics. Thread-local nesting prevents filter activity on one thread from admitting another thread's draws.

Root must enter FilterScope only around canonical filter work, and create DrawScope immediately around the final draw after all material/pass and shader argument changes. Pixel-cost work must pass nullptr or avoid DrawScope. Within a filter, nullptr skips safely and has a distinct diagnostic counter. Outside filters the draw scope makes no D3D calls.

An admitted draw queries actual device ZENABLE, ZWRITEENABLE and STENCILENABLE and requires all three FALSE. Raw material/state bits are insufficient: `r_state.cpp:1604` skips hardware depth changes for negative stateOverride while updating cached bits. No render states, shader resources, retail shaders, textures, dimensions, formats, quality dvars or constants are changed.

GetDepthStencilSurface obtains the exact attachment with its owned AddRef. D3DERR_NOTFOUND with nullptr means no work. Eligible attached draws bind nullptr, then restore the exact saved surface before releasing the owned reference. Every other unexpected HRESULT, including positive non-S_OK results, reports the operation/result and aborts. An unbind failure attempts exact restoration before releasing the reference and aborting. Restore failure also releases the reference and aborts; it never continues with the target cache disagreeing with device state.

Restoration is essential: `R_HW_SetRenderTarget` compares target metadata to cached renderTargetId and may omit a depth rebinding when targets share the surface. The helper leaves no override at canonical draw return. It assumes the caller's device lives for the synchronous scope; it does not retain a separate device AddRef.

ON logs cumulative per-thread checked, skipped, eligible, no_surface, unbound/restored and outside/null_device counters at most once per second using steady_clock. Logging occurs on admitted/skipped filter work, not each outside-filter draw. A matching unbound/restored count confirms completed helper ownership events, not render success, image equivalence or FPS gain.

## Validation

Evidence: `docs/port/evidence/native_post_filter_depth1/results.json`, exact commands/logs and frozen-source-manifest.json. Both OFF and ON production helper translation units compile against actual DXVK D3D9 headers using NDK30, ARM64/API28, O0/debug, Wall/Wextra/Werror, UBSan and nonrecovering errors (exit0). No master build or device execution was performed by this agent.

The host mock fixture includes the production helper unchanged and compiles ON/OFF with MSVC W4/WX. Normal runs exit0 and check outside-filter no calls, nullptr no calls, each individual state exclusion, nested filters, exact attachment restoration, AddRef/Release balance, and no-surface behavior. OFF performs no device calls, changes no ownership and emits no helper log. Intentional state-query, surface-query, positive surface-query HRESULT, unbind and restore failure subprocesses each exit3 through abort; acquired surface references are released for each applicable error, and unbind failure logs its restoration attempt before release. These are simulated API contracts, not actual GPU runtime failures.

Initial OFF compilation diagnosed unused private storage under clang Werror; storage now has maybe_unused attributes so ON/OFF share one header layout. The first host failure subprocess required disabling CRT abort dialogs in the fixture; a precisely identified fixture process was stopped and failure runs repeated with noninteractive CRT reporting. Production failure behavior was not weakened.

## Acceptance still required

Actual APK profiling must confirm eligible/unbound/restored counters, no fatal/UBSan output, unchanged output and warmed same-scene performance relative to the existing fine-pass profile. Per-draw unbind/restore can add framebuffer changes, FlushImplicit(FALSE), pass spills, hazard work and CPU overhead; a slower or neutral result is entirely possible. Retain default OFF unless measured benefit and visual correctness support adoption. DOF sampling of FloatZ/INTZ remains independently required and unchanged.

## Root actual experiment decision

SDK474 executed6040eligible draws with balanced unbind/restore and no observed helper failure/UBSan. SDK475 restored OFF; mean GPU spans110.351ms ON and110.236ms restored OFF show no convincing gain. Not adopted/defaultOFF, hooks absent in OFF engine. Full A/B/A table and caveats in native_gpu_post_followup.md.
