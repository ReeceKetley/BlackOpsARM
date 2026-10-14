# Native Android coarse GPU timestamp overlay

2026-10-14. Implementation is frozen for the next root-owned SDK integration. The root's SDK446/stage18 observation already verifies the initial overlay's real process CPU/RSS/render submission values and Volume Up off/on toggle in the same engine process. Reported loading measurements include Render 48.0 FPS / 20.81 ms interval, render CPU 2.69 ms, process CPU 194.1% and RSS 413.0 MiB. These are loading-context submission observations, not playable-game FPS or a performance target result. The initial source/report/hash evidence remains in native_performance_overlay.md and native_performance_overlay1. This extension has compile/source checks but no actual engine GPU number yet; root owns subsequent SDK/device validation.

## Coarse query source and boundaries

performance_gpu_overlay.cpp owns eight fixed query sets, each containing actual D3D9 TIMESTAMPDISJOINT, TIMESTAMPFREQ, begin TIMESTAMP and end TIMESTAMP objects. It creates no phase-mark queries and never enables bo1_frameperf, per-thread process scans, profiler vectors or DXVK HUD. The existing detailed profiler's query owners/functions and Windows bodies are preserved.

Unconditional RB_BeginFrame starts the query span before scene setup and actual scene/UI command execution. Both RB_RenderCommandFrame and the canonical direct R_IssueRenderCommands path use that boundary; real loading/remote SCR_UpdateScreen and frontend 2D frames are covered. It is independent of viewInfoCount and 3D-only branch conditions. Additional command batches before the next Present retain the open query so the span covers their commands. A successful measured span closes immediately before the real swapchain Present call, placing GPU end/frequency/disjoint commands ahead of Present and excluding the CPU's time inside Present from the query range. Skipped screenshot/resize Present paths retire measurement queries instead of claiming a result.

GPU span means actual timestamp duration between those GPU command-stream markers. It can include GPU idle/starvation within that command range; it is not a hardware busy/utilization percentage or CPU execution duration. The UI labels it GPU span in milliseconds and shows sample age. Render submission FPS retains its existing distinct meaning and should not be confused with display delivery or server simulation rate.

Only the actual successful Present path with a foreground/focused, attached, shown, non-minimized positive-size SDL window/target makes a closed set eligible for publication. Java's resumed/ready positive-size valid Android Surface gate remains unchanged. Failed/inactive Present discards every measurement query and publishes unavailable. Overlay enable epochs reject late samples across hide/pause/resume/toggle. No old resolved query can become a fresh sample just because the UI resumes.

## Nonblocking resolution and cache states

The render execution owner polls eligible closed sets in submission order with GetData flags 0, at most eight sets/four reads each. S_FALSE remains pending; no query is flushed, waited on or reused while pending. A full ring simply skips opening another measurement. Completed disjoint/frequency-zero/reversed timestamp samples become invalid/disjoint. Query creation failure is unsupported, while Issue/GetData/clock failures are query failed or unavailable. A query failure retires the partial/complete objects and disables query work until a new overlay epoch or canonical resource reset. Unsupported/pending/invalid data never becomes a fabricated zero millisecond sample. An actually resolved equal timestamp pair can legitimately measure zero span.

The existing synchronized fixed snapshot now includes GPU ms, status and age, using the monotonic CPU stamp when the query range closes rather than disguising delayed resolution as a new frame. A pending query retains the last resolved sample and its increasing age; the UI marks it stale after 1.5 s. With no resolved sample it explicitly shows pending, with age. Unsupported/query failed/invalid-disjoint/unavailable states remain explicit. JNI copies 13 fixed doubles instead of 10 and makes no GPU/proc calls. Java still refreshes at 500 ms. The CPU/RSS sampler never creates, issues, resolves or releases GPU objects.

## Query lifetime and canonical preservation

All normal query creation/Issue/GetData operations run from the actual canonical backend execution boundary; the fallback main-thread direct render path is also a serialized renderer execution path. A private owner mutex serializes those operations against lifecycle retirement. It nests only into the cache mutex; UI/cache/worker paths never call back into GPU ownership, avoiding a reverse lock order.

One narrow __ANDROID__ hook at the start of R_ReleaseForShutdownOrReset retires measurement child objects before canonical swapchain/device/resource teardown. All inspected engine reset/full-shutdown/failed-hardware-initialization release paths pass through that function. Reset's only caller is R_RecoverLostDevice, reached after the canonical release sequence. The hook preserves every original resource, display-mode, error and assertion statement. Hidden/surface-lost measurement objects retire when canonical rendering next observes the disabled/inactive state, or through this resource lifetime hook; no GPU calls are made by Activity callbacks. A different device identity or enable epoch also retires old sets before reinitialization.

Read-only pinned DXVK source confirms Begin/End queued CS closures hold private Com query references, so dropping the overlay's COM references while work is pending retains provider command ownership. Its D3D9DeviceChild public refs also retain the parent device and release that parent reference when their external query count reaches zero. The overlay releases child refs before canonical device release. No wait/flush is added for retirement. Pinned disjoint Begin writes query1 and End writes query0, with end-before-begin invalidation; frequency comes from the actual Vulkan timestampPeriod. No protected DXVK/SDL/spike source, Vulkan version, asset, native layout, engine assert, UBSan recovery, native entry or render configuration is changed.

The GPU source joins the existing tiny noninstrumented static measurement target; required native DirectX header include directories are added only for that target. PIC/C++20/configured debug-O0 and UBSan/no-recovery remain. No canonical compiler flags or guard family changes are made. The provider emits UBSan abort references and no __cyg_profile references.

## Verification and next actual check

build-android-audit/native_performance_gpu_overlay1 retains before sources, own diffs, scoped compile/preprocess commands/logs, check-results.json, final source hashes/bytes and java-compile.log. performance_overlay.cpp, performance_gpu_overlay.cpp and bootstrap compile with the current Android toolchain. rb_backend/r_init native whole TUs and Windows x86/x64 whole TUs compile. Their selected Windows before/after bodies match after only source-file/line assertion metadata and line-generated profiler identifier normalization. Native bodies preserve original statements after removing only new measurement hooks. Every existing RB_BO1 GPU/Present profiler function and the bounded real Present observer matches its prior native/Windows body. JNI adoption itself is unchanged. Gradle :app:compileDebugJavaWithJavac against existing stage18 passes without a native build, APK assembly/install or device action.

These checks validate source integration and preservation, not actual GPU availability or measured runtime timing. The root's next SDK run must confirm genuine GPU span values on the real engine, sample freshness, hidden/resume behavior and absence of runtime/provider/UBSan failures. Continue to distinguish frontend/loading metrics from playable Zombies benchmarks. Capture actual artifact identity and screen evidence before claiming the GPU request verified. No toy/device benchmark or synthetic GPU number is supplied by this work.

## Actual engine GPU measurements

SDK447 stage19 BuildId01bff569d01b457e81f90929ed374d0d8a8d428b installs
with seven provider hashes matched. sdk447-gpu-live-3.png shows real GPU span
12.92ms with0.3s sample age. Other simultaneous values: loading render submission
17.7FPS/56.56ms, main Com_Frame CPU1415.65ms, render CPU18.29ms, process137.2%
(100%=one core) and RSS518.3MiB. These are loading samples, not a steady gameplay
benchmark or exact panel delivery. Earlier live image correctly shows stale
render/GPU data after a loading pause. Main-frame CPU sample also proves the
actual measurement boundary completed; active3D scene completion is not proved.

SDK448 stage20 BuildId121d3690157c5bf24d1d6c28c69a05876c6f5688 tests
Volume Up off/on in the same process. sdk448-gpu-toggle-off1.png and on1.png
show visibility toggle; the on panel resolves fresh GPUspan18.75ms/age0.1s
after epoch invalidation/query retirement, with loadingRender31.5FPS/process
180.8%/RSS444.0MiB. Visible preference restored. Pausing/resuming, surface loss
and canonical shutdown query release remain runtime checks for a stable engine.
Both runs later hit unrelated active-client bounds failures; no GPU error or
forced wait was observed. Raw SDK logs/screens/package checks remain in
native_registry_closure; no assets/providers were modified or transferred again.
