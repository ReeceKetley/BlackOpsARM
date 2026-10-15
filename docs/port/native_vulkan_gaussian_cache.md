# Gaussian blur weight cache — 2026-10-15

## Change

Android-only, thread-local single-entry cache in `RB_SetBlurConstants`, keyed by the clamped sigma. The original expf arithmetic and normalization order remain unchanged. A changed radius recomputes all weights; NaN does not hit the cache. Texture scaling, bilinear tap calculations and all four dirty constant uploads still run every call. Windows retains uncached behavior. No precision, rounding, skinning or render-pass changes were made in this task.

## Correctness

`build-android-audit/gaussian-cache-stage206/comparison.cpp` contains the exact before/after function bodies extracted from the working copy, with recording stubs for shader constants and a counted expf wrapper. Built using NDK30 clang++ for aarch64-linux-android28, -O2 -fno-fast-math -ffp-contract=off, and executed on HONOR AKWJ9X5116G04618.

PASS: 20,154 before/after output comparisons, bitwise equal finite outputs (NaNs compared by classification); 12,120 cache hits executed zero expf calls. Tests cover radius changes, repeated radius/different scaling, clamping, infinities/NaN and separate thread initialization. Dirty constant counts matched. Evidence: `build-android-audit/gaussian-cache-stage206/arm64-result.txt`. Original working-copy backup is `rb_postfx-before.cpp` in that directory. Final source function tokens match the tested body; only blank-line whitespace was cleaned after build.

## Full application

SDK639/stage206, native_engine_perf1, 720p. Engine and APK builds passed; packaged providers checked against staging manifest before install. Engine BuildId: af5fb9a39245172333f538214ddb3f08f980b339.

Observation: 106.11 seconds, same PID17232; 285 room profiles spanning 99.754 seconds; sustained 28.470 FPS. Mean CPU recording 24.469 ms, GPU 33.021 ms, 21 passes and 857.8 draws. Four screenshots retain the room, lights/shadows, menu and changing television content. No matched-launch fatal/assert/UBSan report. Thermal status 0 at start/end. Existing missing glass depth-prepass warning remains.

Evidence: `build-android-audit/native_registry_closure/sdk639-gaussian-cache-a.json` and its entry/logcat/screens/thermal files.

## Interpretation and limitations

Earlier SDK638 measured 28.513 FPS, CPU recording 25.432 ms and GPU33.748 ms with 20 passes. The pre-existing BO1_NATIVE_POST_FUSION_TRIAL=ON build setting is present in the current build and this run records 21 passes. This is not a controlled A/B: do not attribute the CPU/GPU timing differences to this cache or claim an FPS gain. The isolated device test proves removed repeated work and equivalent outputs. This is menu validation, not full gameplay validation. No live baseline was available at the start: the engine process was absent despite a stale running guard. Left SDK639 running; did not alter the existing trial option.
