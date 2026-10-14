# Original CubeMapGen filtering with native worker ownership

2026-10-13. Full Com_Init/Com_Frame incomplete; full engine remains unlinked.

Full target now selects canonical CCubeMapProcessor.cpp beside its existing image
and bounding-box owners. Native launch uses actual std::thread workers owned by
platform/android/filter_worker.h. Each native callback captures its own processor
and filter parameters instead of consuming shared global Windows argument slots.
Worker active state uses release/acquire; status and per-worker progress fields
are atomic. Slot lookup/publication/retirement uses a processor mutex. No worker
count forced zero, successful asynchronous placeholder or duplicate filter engine.

Native cancellation requests cooperative stop between rows/phases/mip levels.
Termination joins the primary before collecting/joining the secondary, so primary
cannot create a worker after retirement inspection. Both workers retire before
Clear/reset/outputs release or reuse image storage. Cancelled work cannot publish
FILTER_COMPLETED through the native completion guard. Existing real CPU filter,
edge fixup, lookup, image and mip calculations retained. Cancellation may wait for
a finite inner tap/fixup/precompute operation; no unsafe forced thread destruction.

Native status/progress formatting uses actual atomic observations and bounded
wide formatting, without Win32 desktop cursor calls. Fatal library diagnostics
use actual stderr and original terminal exit. Native thread construction errors
propagate real C++ exceptions; no successful/synchronous fallback supplied.
Control/progress/reset/input/output methods require one controlling caller per
processor; atomic worker progress does not make arbitrary simultaneous control
calls or public-field mutation safe. Windows original worker/progress/dialog
branches and structure fields retained.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link367.log records the old Windows fatal
  dialog dependency. link368 compiles final native worker/filter owner; link369 is
  latest production attempt after fixture configuration, unresolved other services.
- cubemap-worker-definitions1.log verifies actual constructor/destructor/init,
  worker launch/termination, CPU filtering, status and output owners.
- takeover_cubemap_worker_windows1: two normal whole TUs (processor plus screenshot
  header consumer) Windows x86/x64 exit0, source manifests retained.
- cubemap-worker-build1/2.log: focused CMake fixture uses actual processor/image/
  bbox and real timing/error providers with GC/UBSan; no fake thread or filter.
- cubemap-worker-push1/2.log/run1/2.log: HONOR AKWJ9X5116G04618 ADB-shell UID exit0.
  Two independent processors run two workers each; all six4x4 output faces retain
  their distinct expected RGBA colors while actual status/progress is polled.
  Larger128x128 job is stopped/joined and filtering restarted successfully.
  Final run explicitly records active=1 before stop and resulting_status=2
  (FILTER_TERMINATED), verifying cancellation rather than only completed cleanup.
  Both worker slots inactive after retirement. Only changed fixture binaries
  transferred; game assets reused.

No injected thread-start/join/allocation failure, arbitrary concurrent controllers,
all filter/edge-fixup/half/gamma modes, retail cubemap material data, long-running
cancellation latency or GPU/image-file save was tested. UBSan is not a data-race
detector; synchronization relies on the audited ownership/atomic protocol and
scoped execution. Initialized renderer/window/device/error recovery remains
unexecuted. D3DXSaveSurfaceToFileA and audio/cinematic/renderer/input/Steam/debug/
Monkey and other true dependencies remain; full Com_Init has not returned.
