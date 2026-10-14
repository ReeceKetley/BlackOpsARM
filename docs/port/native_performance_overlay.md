# Native Android development performance overlay

2026-10-14. Source implementation is frozen and checked for the next root-owned APK integration. No on-device overlay image, actual engine performance values, or playable-gameplay proof is supplied by this work. SDK444 proved visible real engine UI; that preceding evidence does not prove playable gameplay. Root coordinates the actual SDK446/stage18 integration and current render-resource callback repairs.

## Implemented display and real measurement sources

EngineActivity adds a compact white monospace TextView with a translucent black background at the SDL layout's upper left. It is non-focusable, non-clickable and non-selectable; the SDLSurface retains touch/controller focus. One UI Handler reads a fixed cached native snapshot every 500 ms. Volume Up toggles the development overlay and persists the selection in Activity preferences; it is initially visible. Volume Down retains its SDK handling. The visible panel explains the toggle. Showing the overlay requires successful SDK adoption, resumed Activity, a real positive-size ready SDLSurface and valid Android Surface. SurfaceHolder callbacks disable measurements on actual surface loss; pause/destroy remove callbacks and stop sampling/refresh, and destroy removes the view/callback. Existing SDK adoption/native-entry paths remain intact.

| Field | Actual source and meaning |
| --- | --- |
| Render FPS / ms interval | CLOCK_MONOTONIC between successful actual RB_SwapBuffers Present returns. At least a 500 ms accumulated interval window. Foreground/focused, attached, shown, non-minimized positive-size SDL window and render target are required. Screenshot/resize skips, failed Present, invalid surface and gaps over 1.5 s reset the window. This is render submission rate, not display/compositor delivery or 20 Hz server simulation. |
| CPU process % | CLOCK_PROCESS_CPUTIME_ID delta divided by CLOCK_MONOTONIC delta, about every 500 ms, in the actual :engine process. 100% consumes one core; values may exceed 100%. Includes native/provider/Java/sampler threads in that process. |
| CPU main ms / Com_Frame | CLOCK_THREAD_CPUTIME_ID across the actual completed canonical Com_Frame, averaged on a 500 ms window only with recent real successful rendering. Covers menu/client scopes too; no server-derived FPS. Generation validation rejects a sample across hidden/re-enabled cache reset. |
| CPU render ms / submission | Actual render thread CPU delta between consecutive successful Present boundaries on the same thread, averaged with render submission intervals. It includes that render thread's work between boundaries and excludes sleep/descheduling. Main/render scopes can overlap and must not be summed. |
| Engine RSS MiB | Memory-only /proc/self/status VmRSS in kB converted to MiB, about once per second on the sampler. Does not enumerate threads or FDs and does not substitute PSS, malloc heap, GPU allocations or physical free RAM. |
| GPU ms unavailable | Explicitly unavailable in this implementation. No GPU timing number, hardware busy percentage or CPU Present-time proxy is invented. |

Unavailable fields have negative internal values and no placeholder performance number. Each sample carries a monotonic publication stamp, copied as age in seconds; UI thresholds mark render/main stale after 1.5 s, process CPU after 2 s and RAM after 3 s. Fresh process idle CPU can legitimately measure 0%; that is an actual clock delta, not a failure placeholder. Proc/clock/JNI failures remain unavailable. JNI only copies the fixed cache and obtains its monotonic age; it performs no proc read, GPU query, renderer dispatch or thread scan.

## Ownership, overhead and preservation

performance_overlay.cpp owns one bounded snapshot, fixed accumulators, one mutex and one sleeping sampler thread. Render/main updates and worker publication use the same mutex; consumers never read the profiler's volatile GPU/frame rings. Worker generation checks discard observations during enable changes. Hidden/paused/surface-less overlays clear the cache, stop periodic work and stop UI refresh. A worker created on first enable sleeps until the next enable; destruction wakes/joins it. Thread creation failure is reported as unavailable.

The new small bo1_native_performance_overlay static target is linked through bo1_native_entry_state so both CLI and SDK resolve the narrow Android hooks. It retains PIC, C++20 and UBSan/no-recovery, with the configured debug/O0 build flags, but does not inherit function instrumentation. NDK clang rejects the attempted -fno-instrument-functions inverse option; the original failed compile/log and comparison harness results are preserved. Existing canonical target instrumentation and source guards remain intact. rb_backend/common changes use __ANDROID__ guards. Existing Present observer, native configuration, bootstrap adoption and native entry bodies remain intact. No DXVK, SDL, Vulkan, asset, runtime stub, assert, UBSan or compiler optimization policy change was made.

## Checks and outstanding validation

Evidence lives in build-android-audit/native_performance_overlay1. Final source hashes/bytes are in source-manifest-final.json; original files and own diffs are retained. New provider/bootstrap and canonical rb_backend/common native whole TUs compile. Canonical rb_backend/common Windows x86/x64 whole TUs compile. Selected before/after Windows bodies match, and native comparison preserves canonical statements after removing only the explicit new measurement hooks. Comparison normalization only accounts for __FILE__/__LINE__ assertion metadata and line-derived profiler identifiers. Windows Com_Frame and Com_Frame_Try_Block_Function also match the read-only BlackOps64 oracle. Existing renderer bodies differ from the BlackOps64 oracle because of preceding accepted Android-worktree changes; this overlay preserves their exact prior Windows behavior. check-results.json records these distinctions.

Gradle :app:compileDebugJavaWithJavac with existing apk-native-stage17 passes (15 tasks, 1 executed), without rebuilding native providers or assembling/installing another APK; java-compile.log is retained. An optional host POSIX concurrency fixture could not run: installed Ubuntu WSL references a missing ext4.vhdx. This is a test-environment limitation, not runtime concurrency proof. No device work or independent shared build was performed, to preserve root's live SDK observation/build coordination.

Next required checks are the root-owned SDK integration: confirm actual on-screen measured values on successful engine rendering, surface/pause/resume/toggle behavior and input passage. Capture the actual source artifact and screenshot. Do not report these compile checks as performance benchmarks or gameplay proof.

GPU timing remains required follow-up. Same-device protected-spike evidence already demonstrates real timestamp feasibility (19.2 MHz / 236 resolved samples), but is not engine timing. The existing bo1_frameperf ring also initializes detailed phase queries and enables process/thread diagnostics and growing run vectors. This initial overlay does not enable that profiler. Add a separate bounded coarse disjoint/frequency/begin/end query owner controlled by overlay enable, with correct render/device reset/shutdown ownership. Issue around actual renderer work, poll GetData with flags 0, preserve pending/disjoint/failure and age, never flush/wait, and publish only resolved real engine durations to this cache. Validate that path on the actual engine before claiming GPU request completion.

## Actual SDK446 tablet verification

SDK446 stage18 engine BuildIdf1a7c2d69c88559d3ad9bda61dd04e9da169d9b1
builds and installs with seven packaged providers matching stage18. The actual
engine overlay is visible: sdk446-screen-live1.png shows process235.8%
(100%=one core) and RSS294.8MiB, with render/main/GPU pending or unavailable.
The same running process toggles the panel off/on using Volume Up, captured
in sdk446-overlay-toggle-off1.png and sdk446-overlay-toggle-on1.png.
The on image shows Render48.0FPS/20.81ms submission interval, render thread
CPU2.69ms per submission, process194.1% and RSS413.0MiB. These are startup/
loading measurements, not gameplay FPS or compositor delivery benchmarks.
Main Com_Frame remains n/a before completed client frames; GPU timing is
explicitly unavailable in this baseline and its genuine timestamp follow-up
is underway. No hidden unsupported values are replaced with zero.

The actual SDK proceeds through real Com_Init and frontend client CA_ACTIVE,
then aborts on SND_SetListener first-row matrix bounds. No steady rendered
gameplay, main-frame CPU value, GPU value, pause/resume/surface-loss runtime
proof is claimed. Process23618 absent/guarddirty; USBstayawake restored0.
All assets/providers reused. Screenshot/log evidence is in native_registry_closure.
