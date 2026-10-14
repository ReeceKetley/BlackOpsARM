# Original physics path, convex hull, automatic bodies, IK and ragdoll integration

2026-10-13. Latest full-engine-link201 compiles newly selected sources and fails
remaining genuine links. Actual Com_Init/Com_Frame unexecuted.

First corrected the preceding raw-helper batch: seven sources had accidentally
been added to the occlusion probe rather than bo1_native_database. Its original
ARM compilation claim was incorrect. Link200 moves them to the full library,
actually compiles all seven unchanged and resolves consumed color/pixelcost/
outdoor helpers. Windows checks remain valid. FACT-0209 is superseded by corrected
FACT-0210; old logs and the exact handoff archive remain preserved.

Now selected whole phys_convex_hull, phys_auto_rigid_body,
phys_collision_multithreaded, nitrous_vehicle_constraint, ik_layers,
xanim_load_obj and tl/physics/rbc_def_ragdoll. Original algorithms, assertions,
allocation/state owners and interfaces retained. All compile ARM unchanged;
no synthetic geometry/constraint/IK/animation success or new placeholder supplied.

Evidence:

-201 compiles all seven original sources and reaches linking; llvm-nm confirms
  actual brush/partition convex hull, path destroy, IK layers and animation loader
  definitions. These named gates no longer unresolved in201.
- takeover_physics_path_ik_ragdoll_windows1: seven whole normal TUs pass x86/x64
  exit0 with commands/manifests/build-result.json.
- Corrected raw batch retains matching earlier seven normal Windows whole-source
  results; no shared source changed in that wiring correction.

No new runtime probes or transfers. Native geometry/constraint/automatic-body/
IK/animation loader execution and decompiled pointer/stride/ownership assumptions
remain unverified. Prior physics arena/queue/IK math probes keep only their
recorded scope. Full startup still needs genuine server/game/client/state,
audio/cinematic, renderer/input/network/recovery dependencies shown in201.
