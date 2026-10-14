# Original draw surface and full client state integration; backend pending

2026-10-13. Current full-engine-link169 stops during compilation in rb_backend,
before linking. Full Com_Init/Com_Frame remain unexecuted. This is unfinished
platform integration, not a successful full native build.

Selected complete canonical r_add_bsp, r_add_staticmodel, r_pretess and rb_backend.
Removed splitscreen's BO1_NATIVE_REFERENCE_QUERY_ONLY source restriction; full
canonical splitscreen now supplies active-client and connection-state queries.
cg_compass retains its prior separate query-only restriction.

Three original draw-surface sources compile ARM unchanged in link167. Full
splitscreen compiles ARM unchanged in link168. Their assertions and original
single-client policy are preserved; no invented client readiness or state.

Backend is deliberately retained as pending work. Its rendering logic shares
the source with Windows profiler and window/recovery instrumentation. Native
adaptations so far only scope Win32 window-procedure/intrinsic headers to Windows
and replace the reflection readback Sleep(100) with actual chrono-based 100 ms
sleep. Windows behavior unchanged. No profiler, focus-loss, Present, message pump
or recovery behavior stubbed to force compilation.

Evidence:

- link167 preserves native missing Win32 window types and intrin.h failures.
- link168 preserves Sleep and tlhelp32.h failures after header scoping.
- link169 confirms native sleep dependency correction, but compilation stops at
  Windows tlhelp32.h in backend profiling section (line4712). Further platform
  compiler errors may appear after this first fatal include is adapted.
- takeover_draw_surfaces_client_windows1: all five whole normal Windows TUs,
  including backend, pass x86/x64 exit0 with commands/manifests/build-result.json.

No native backend object/link/GPU runtime proof; original draw-surface, active
client and connection query execution remain unverified. No new runtime probe,
device binary or game asset transfers. Prior link166 remains the most recent
complete native link attempt identifying wider real dependencies;169 supersedes
it as the current compile gate, not evidence those dependencies disappeared.

Next action: adapt rb_backend's actual profiler platform boundary. Existing
native profiling_diagnostics supplies real process virtual bytes and dladdr
symbol lookup; native per-thread/process CPU time and thread enumeration need
appropriate live ownership/lifetime behavior. Preserve D3D9 GPU query semantics,
use fixed32 frame publication counters and real timing units. Windows TSC and
Toolhelp assumptions cannot be applied on ARM. Then adapt native window/message/
focus/recovery lifecycle against the tested SDL2/DXVK path. Do not report an
empty sampler or ignored lifecycle messages as implemented behavior.
