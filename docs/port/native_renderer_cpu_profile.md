# Native renderer CPU profiling boundary

2026-10-13. CPU sampler implemented and tested on HONOR; renderer integration
remains incomplete. Full-engine-link171 stops compiling Windows focus/window
diagnostics, before linking. Actual Com_Init/Com_Frame/Present unexecuted.

cpu_profile.cpp provides actual process/current-thread CPU clocks and enumeration
of /proc/self/task. An owned CpuThread keeps an open task directory descriptor;
each stat read verifies TID and kernel start time before publishing a sample.
Failure leaves the caller's output unchanged. Explicit CloseCpuThread releases
descriptor/object. Reads after exit fail instead of accepting a recycled ID.
Arbitrary task CPU values are actual kernel user/system ticks converted using
_SC_CLK_TCK; precision is kernel accounting resolution, not nanosecond precision.
Process/current CPU clocks use clock_gettime. No Windows TSC units fabricated.

Renderer native path uses real CPU samples converted to milliseconds. Before/
after reads must both succeed and remain monotonic; invalid CPU values retain
the existing unavailable -1 convention. Thread reports use actual kernel comm
names instead of claiming unavailable Windows start addresses. Snapshot refresh
opens new live owners, retires old descriptors, and samples only matching live
identities. Failed main-thread samples release the stale owner for later reopen.
Snapshot capacity is bounded160; enumeration overflow fails rather than claiming
a complete truncated snapshot. GPU D3D9 timestamp query logic remains original.

Renderer publication fields/counters now use BO1RenderCounter: fixed32 int on
Android, original long on Windows. Native render counter additions use seq-cst
atomics; existing buffer exchange/publication uses its canonical fixed32 fields.
Other native zone counters retain their existing separate services. This is not
certification of the complete renderer ring reader/concurrent publication path.

Evidence:

- cpu-profile-build1 compiles production cpu_profile.cpp and focused probe for
  Android28 ARM64 with UBSan minimal runtime, linked static C++/sanitizer runtime.
- cpu-profile-run1 on HONOR passes real process enumeration/worker presence,
  opened ownership/kernel name, task/process/current clock reads, measurable
  monotonic CPU work, rejection after exit with output sentinel preserved and
  invalid identity rejection. Only this changed test binary was pushed.
  Does not force TID reuse, exhaust descriptors or verify all parser/overflow/
  permission/error cases; live-owner failure behavior is tested.
- full-engine-link170 preserves initial performance-counter type conflict and
  native string-copy adaptation errors. Corrected;171 diagnostics now concern
  actual Windows window/focus/lifecycle assumptions. Full backend object still
  cannot compile, so no renderer profiler execution or GPU query result claimed.
- takeover_render_profiler_windows1 passes two whole normal rb_backend/profile
  sources on Windows x86/x64 with commands/manifests/build-result.json. Subsequent
  native-only stale-owner cleanup leaves the Windows branch unchanged.

Next: implement actual SDL2/native focus/window/message/quit/recovery behavior
in the retained backend, keeping original rendering and lifecycle semantics.
Do not bypass or claim Win32 focus-loss diagnostic behavior on Android. Compile
through that boundary, finish real remaining links, then execute production
startup and validate actual renderer lifecycle. Pinned DXVK/Vulkan unchanged.
