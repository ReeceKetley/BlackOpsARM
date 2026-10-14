# Original debug initialization and map-load profiler lifecycle

2026-10-13. Full Com_Init remains incomplete; no complete executable or frame run.

Canonical win_net.cpp now selects original NET_InitDebug and NET_InitDebugStreams
under the existing full-only BO1_NATIVE_UDP_ENGINE scope. Native state uses the
same inactive debug flags, socket slots, stream pointer and byte counters as the
reference. Initialization resets metadata; it does not create or authenticate a
TCP connection. Connect/listen/read/write/shutdown/restart still require native
transport adaptation. Existing UDP ownership remains separate.

Canonical com_profilemapload.cpp selects its original initialization, activation,
deactivation, tree/hotspot calculations, printing and overlay bodies through
source-scoped BO1_NATIVE_PROFILE_LIFECYCLE. The full target excludes the earlier
ProfLoad_Init diagnostic only where the actual owner replaces it. Native uses
the existing real QPC boundary; discards its success result rather than casting
it to a pointer. std::less<const char*> preserves the reference comparator's
address-based ordering. Empty native DrawTree returns zero rather than an
uninitialized result. Windows branches retain their original behavior. No typed
rewrite of all decompiled tick arithmetic, initialized profiler, UI draw or
recovery lifecycle is claimed.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link331.log and link332.log compile the
  selected original owners and fail on other true remaining dependencies.
- native_registry_closure/debug-init-profile-lifecycle-definitions1.log confirms
  both debug initializers and original profiler lifecycle/print/draw owners.
- takeover_debug_init_profile_lifecycle_windows1 has final manifests and exit0
  for both normal whole TUs on Windows x86 and x64.

No device execution, game asset transfer or unchanged broad fixture rerun was
needed for this source selection. Native debug TCP and initialized profiler/error
cleanup remain unverified. No startup readiness or successful recovery added.
