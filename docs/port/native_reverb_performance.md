# SDK471 reverb optimization and frame-cost diagnosis

The measured audio callback was the largest CPU consumer in SDK470. A live
matching-process LLDB snapshot caught SDLAudioP2 in SND_RvFrame, through the
canonical reverb effect and FAudio submix/effect chain. The render thread was
between frames at RB_RenderThread6140; one job worker was skinning a model and
another was waiting. One snapshot identifies active code/waits, not their time
distribution or the dominant frame bottleneck. The debugger detached, the
engine resumed with TracerPid0 and root removed its port19470 forward.
The initial host command quoting failure is preserved separately.

The production change is one source-scoped compiler option: rendered native
builds compile unchanged snd_radverb.cpp with -O2. The option
BO1_NATIVE_OPTIMIZE_REVERB defaults ON and can be disabled for comparison.
All other canonical engine translation units retain their existing policy.
The actual generated reverb command retains debug symbols, DEBUG, assertions,
function instrumentation, stack protection and non-recovering UBSan. No
fast-math option is added. Windows and standalone headless recipes are unaffected.

Canonical source SHA256 remains
95E08E55A8B4A1C399ABAFA7BD71F3D35FF21B371D9F4C643A3F8C025C4A3EAB.
CMake SHA256 is E0B5E954B3775858EADF6033C7FCCF2B899E51566EA1152CA887C255943175F4.
The actual ARM64 tablet fixture compares O0/O2 outputs, parameters and full
persistent state bitwise across eight scenarios, including in-place operation,
parameter changes, multiple frame rates and ring wrap.1600 calls/version,
307520 frames and1230080 output floats pass with tolerance0, exit0. See
native_reverb_optimization_equivalence.md for proof scope and preserved failures.

SDK471/stage43 BuildId9209170ec6a62da071e6c0512a2e34dfd750028c packages all seven
providers exactly and installs successfully. Actual PID8567 reaches CA_ACTIVE
at71439ms and touch OK enters the room; the existing assets are reused.
The preliminary targeted build performs5actions; packaging helper's later
build correctly reports no work. Gradle completes in6s. Helper52666 finishes
its observation and restores USB stay-awake0, leaving the engine live.

| Thread CPU,100%=one core | SDK470 O0 | SDK471 reverb O2 |
| --- | ---: | ---: |
| SDL audio callback |56.61%|41.14%|
| Renderer |32.66%|35.00%|
| Main |31.74%|31.75%|
| Worker1 |23.32%|24.54%|
| Worker2 |23.23%|24.60%|
| Server |23.08%|23.47%|
| Sum of measured threads |227.69%|218.70%|

Audio callback CPU demand falls by15.47 percentage points,27.3% relative, in
the observed room samples. The comparison uses73.02s/72.35s schedstat windows,
with matching independent tick counters. Different launches, warm-up ages,
thermal/scheduler conditions and minor UI history limit attribution precision.
This is evidence of reduced callback CPU use, not a controlled gameplay FPS
benchmark. Full-size room captures remain around8.4–8.7FPS; audio optimization
alone does not establish a material frame-rate gain.

The final clean full-size capture subsequently reads7.4FPS/134.33ms and
GPU120.04ms, with no runtime error and main8701/server8700 frames reached.
This additional variability reinforces that these screenshots do not establish
a controlled overall FPS improvement from the audio change.

A same-process reversible canonical r_scaleViewport test changes1→0.5→1.
This halves 3D viewport width/height, reducing its area to one quarter; it is a
debug viewport test, not an adopted resolution/upscaling feature. Source also
applies it to non-2D render views, so it does not isolate only the final pixel
shader or prove a single GPU bottleneck. Half-size captures show11.5FPS/87.16ms
and10.3FPS/96.89ms, GPU query spans102.79/99.13ms. Restored full size shows
8.4FPS/118.98ms and GPU116.30ms; full-size pre-test captures show8.4–8.7FPS and
GPU111.20–113.75ms. These sparse samples support significant graphics-workload
sensitivity while substantial fixed/CPU/wait costs remain. GPU span is not busy
percentage. Actual console query confirms r_scaleViewport1 after restoration.

No worker count, affinity, quality default, assets, protected DXVK/SDL provider
or SUBOPTIMAL fix is changed. Current target is renderer/pass/skinning cost
and remaining audio work, with real gameplay still required before tuning the
final workload. The extra diagnostic console is closed at handoff; overlay ON.

An independent lifecycle failure occurred when ending the old SDK470 process
via actual /quit: Native shutdown lifecycle reports rendered splash lifecycle
requires an actual Android Activity owner. PID28098 died and its dirty guard
was retained. This was not clean shutdown or an optimized-DSP failure. The next
launch performs its normal stale-process ownership handling; no marker was
manually erased. Real Android splash/shutdown ownership remains outstanding.

Evidence under native_registry_closure: sdk470-cpu-live-maps1.txt,
sdk470-cpu-lldb-backtrace1/2.txt, sdk470-cpu-live-symbolized1.txt,
sdk470-cpu-sample2/results.json, sdk471-cpu-sample1/results.json,
native-reverb-build-policy1 baseline/manifest/production-command,
sdk471-prebuild1.log, stage43 package verification, SDK471 viewport screenshots
and native log segments, and sdk470 canonical-quit1 log/dirty guard. The fixture
and all its raw failures are retained under native_reverb_optimization1.
