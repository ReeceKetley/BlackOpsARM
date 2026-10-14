# Existing sentient, path, bad-place and string-helper integration

2026-10-13. Actual Com_Init/Com_Frame remains incomplete. No tablet run/transfers.

Full bo1_native_database selects five whole existing sources: sentient, pathnode,
pathnode_load_obj, actor_badplace and g_mapkit_path. Original entity/team/path-node
load/count/bad-place/mapkit path behavior is retained. Existing startup string-pool
owner now also selects Scr_SetStringLiveUpdateSafe and SL_GetStringForInt in FULL
using BO1_NATIVE_STRING_UTILS. OFF pool selection stays unchanged; no duplicate
pool or independent string implementation. Bodies are unchanged.

Android path code uses compiler read-prefetch at the original address, FILE for
the legacy unused stdio local, and std::_Exit for the original intentional
g_connectpaths>=2 path-generation tool exit. Sys_NormalExit and stdout flush
remain before termination; no cleanup or initializer bypass. Windows calls remain.
The actual Sys_NormalExit service still requires genuine dependency closure.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link281.log retains missing prefetch/file/
  process declarations; link282 compiles the corrections and reaches genuine links.
- sentient-path-badplace-definitions1.log confirms actual native sentient allocation/
  team, path init/spawn, bad-place update and restored string-helper owners in their
  database/startup-service archives.
- takeover_sentient_path_badplace_string_utils_windows1: six whole normal TUs pass
  x86/x64 exit0 after final source changes.

No initialized sentient/path/bad-place/string-update/runtime or tool shutdown
has executed. Map/path layout, native pointer ownership, live-update failure and
reference-count cases, cleanup and concurrency remain runtime/caller audit gates.
Full startup still lacks other original owners and genuine native platform
adaptations. No successful Com_Init/frame/game claim; prior probes not repeated.
