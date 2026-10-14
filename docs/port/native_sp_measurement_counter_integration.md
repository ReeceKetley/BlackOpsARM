# Complete native SP measurement source integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. Whole g_sp_measure.cpp
is now selected and compiles in the full native database, superseding the earlier
unselected-source limitation without invalidating its provider runtime evidence.

Native measurement reads the existing real monotonic QPC provider, with explicit
failure. Thread CPU clock samples are converted from nanoseconds to the observer's
100ns accumulation units. BO1_ProfileTicks already uses monotonic nanoseconds, so
native zone conversion is its documented frequency/1000, not x86 rdtsc calibration.
Current thread identity uses actual BO1_ProfileThreadId/gettid.

System CPU observations read aggregate /proc/stat's first eight counters, excluding
guest time already included in user/nice. Idle includes iowait. Process CPU is the
existing real process clock. Interval percentages use kernel tick frequency and
validate monotonic/same-frequency baselines. First/failed/reset samples remain -1
(unavailable); failure discards baseline. Owned proc descriptors close, field/sum
overflow fails without publishing output. Sequential samples are not atomic.

Native bounded formatting rejects overflow before append lengths advance; animation
labels handle truncation. Native fixed32 render counters are accessed through their
actual type and widened explicitly for %ld diagnostics. Native long profiler values
use real seq-cst atomic exchange/increment/CAS; report reads use acquire load and
published zone-list acquisition. Windows timer/CRT/intrinsic calls retain their
original implementations and values. This is not a blanket ring/lifecycle/race audit.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link271/272 retain count-helper/render-counter
  type failures; link273 compiles the complete source and fails genuine other links.
- sp-measure-native-definitions1.log confirms actual native G_SP_MeasureEnabled,
  G_SP_ProfileNow, G_SP_MeasureTicks and G_SP_FramePerfServer archive definitions.
- takeover_sp_measure_native_counters_windows1: whole final normal measurement TU
  passes x86/x64 exit0.
- process-resources-build2/run2: HONOR UBSan provider test reads actual system CPU
  total98450648/idle96442898 ticks at100Hz; immediate second sample unchanged,
  so no positive CPU-interval or percentage claim. It also rechecks the changed
  provider's descriptor/RSS behavior. Only changed459456-byte test transferred.
- Existing cpu_profile and monotonic provider runtime evidence remains scoped to
  those previous reports; no repeated unchanged CPU probes here.

No initialized SP measurement event/report/percentage/frame profiling has executed.
Remaining runtime gates include format overflow, thread-unit integration, interval
CPU deltas/failure/reset, zone sorting while counters change and renderer ring
publication/reuse. Full entry still needs omitted original owners and native
audio/cinematic/live/profile/game/client/renderer/network/recovery services.
No successful Com_Init, Com_Frame or gameplay claim; retail assets unchanged.
