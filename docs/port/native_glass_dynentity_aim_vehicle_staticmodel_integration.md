# Native glass, dynamic entities, aiming, paths and static models

2026-10-13. Actual Com_Init/Com_Frame remains incomplete. No tablet run/transfers.

Full bo1_native_database selects seven whole canonical sources: glass_server,
DynEntity_server, cg_spawn, r_staticmodel_load_obj, aim_assist, aim_target and
g_vehicle_path. Original server glass/snapshots, dynamic collision/damage, client
spawning, static-model tree loading, aiming/targets and vehicle paths are retained.
Native client spawn includes the existing CRT case-insensitive comparison adapter.

Android static-model tree nodes contain native smodelIndexes pointers. The
original40-byte allocations/copies and offset36 rebasing were invalid for this
layout. Native sorting/growth use sizeof(GfxAabbTree) and alignof(GfxAabbTree).
Growth initializes the new storage and copies whole existing nodes, then rebases
nonzero child links through named signed childrenOffset fields. Zero links stay
zero. Relative address conversion validates both signed32 limits and reports the
actual Com_Error(ERR_DROP) if unrepresentable (aborts if it unexpectedly returns).
Offset fields stay signed32; Windows and serialized records are unchanged.

Evidence in build-android-audit:

- native_registry_closure/full-engine-link253.log retains missing native _stricmp;
  link254 compiles all seven sources. link255 compiles native tree sizes/rebasing;
  link256 compiles the final storage initialization and fails genuine links.
- native_registry_closure/glass-dynentity-aim-staticmodel-definitions1.log confirms
  actual archive definitions of glass snapshots, dynamic traces, spawn, tree sort,
  aim input/target updates and vehicle path initialization.
- takeover_glass_dynentity_aim_vehicle_staticmodel_windows1: seven whole normal
  TUs pass x86/x64 exit0 after final source changes.

No initialized glass/entity/spawn/aim/path/tree runtime has executed. Native tree
growth/rebasing/offset-limit failures remain focused runtime gates; compilation
does not prove these. Other retained offsets/counts/pointer assumptions and owner
lifetimes require caller audit. Full entry still needs actual audio/cinematic/
live/profile/game/client/renderer/network/recovery services. Prior probes not rerun.
