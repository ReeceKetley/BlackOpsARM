# Original Live_Init and file-share integration

2026-10-13. Full Com_Init remains unexecuted; latest full-engine-link348 fails.

Full target selects canonical live.cpp, live_fileshare.cpp, live_fileshare_cache.cpp
and live_fileshare_search.cpp. Full-only Live_Init diagnostic is excluded where
the original Live_Init calling Live_InitPlatform replaces it. Original offline/
online feature guards remain unchanged. Android excludes an unused DemonWare
lobby implementation header only when BO1_LIVE is disabled; enabling that backend
still requires its genuine native implementation/ABI adaptations.

Native cache allocations use sizeof actual file/rating entry times 300 instead of
fixed Windows sizes. Entries are constructed with placement new before publishing
cache pointers. Allocation failure returns false without publishing ready state;
hunk owns the raw storage. Native shutdown destroys constructed entries before
clearing owners; hunk release remains the original external lifetime owner.
Descriptor/rating copies use typed assignment, preserving object lifetimes and
native member widths instead of fixed-size/vtable byte copies. Native hash folds
the actual low/high32 fileID and location as uint32 before modulo300, preventing
negative indices for high-bit IDs. Windows original branches are unchanged.

Evidence in build-android-audit:

- native_registry_closure/full-engine-link346.log records unused backend header
  error; link347 compiles final originals/cache changes; link348 is the latest
  production link attempt after fixture configuration, unresolved other owners.
- takeover_live_init_fileshare_windows1: four normal whole TUs x86/x64 exit0,
  final source manifests preserved.
- native_registry_closure/fileshare-cache-build1.log: focused CMake target uses
  actual canonical cache source with UBSan/GC. Fixture Hunk_Alloc uses real malloc
  with byte canaries and recorded sizes; unexpected assertions abort. Production
  keeps its actual hunk/assertion providers.
- fileshare-cache-push1.log/run1.log: HONOR AKWJ9X5116G04618, ADB-shell UID exit0.
  Actual allocation sizes/construction, high-bit/full64 hash bounds, descriptor and
  rating round trips, allocation canaries and idempotent shutdown pass. Only the
  changed fixture transferred, staged game assets reused.

No actual global hunk initialization/reset ordering, allocation-failure injection,
concurrent cache access, backend fetch/persistence, file search/UI or initialized
Live_Init/Dvars/profile lifecycle was executed. Source selection and this isolated
memory fixture do not prove online services or complete startup. Renderer/audio/
cinematic/network/debug/Steam and other genuine dependencies remain unresolved.
