# Native bullet, flame, glass, streaming geometry and AABB owners

2026-10-13. Actual Com_Init/Com_Frame remains incomplete. No tablet run or transfer.

Full bo1_native_database selects six whole canonical sources: bullet,
flame_physics, glass_load_obj, r_stream_util, r_xsurface_optimize and aabbtree.
Original bullet traces/random helper, flame physics, glass source selection,
texture triangle geometry, model optimization and AABB construction are retained.
No independent algorithm, fake readiness or unsupported successful endpoint added.

Android loose glass allocation now reserves sizeof(Glass)*1000 bytes, matching the
original 1000-element capacity (also reflected in GlassServer storage/capacity
assertion) while accommodating native pointers. The old 0x1E460 allocation was
1000*124 bytes. Native allocation occurs before publishing glassesInited, avoiding
a successful flag when allocation throws. Remaining fields are zeroed as before.
Windows original allocation/order and FastFile records are unchanged. This retains
the existing one-time raw allocation ownership and assumes existing caller lifecycle;
it does not introduce or prove concurrent initialization synchronization.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link244.log compiles six original sources;
  link245 compiles the final native glass correction and fails genuine links.
- native_registry_closure/bullet-flame-glass-aabb-definitions1.log verifies actual
  archive definitions of trace/random, flame update, glass access, triangle area,
  model optimization and AABB construction.
- takeover_bullet_flame_glass_stream_aabb_windows1: all six whole normal sources
  pass x86/x64 exit0 after final source changes.

No initialized trace/flame/glass/geometry/model lifecycle or corrected allocation
has executed. Retained native pointer/layout, arithmetic, ownership/concurrency
still require runtime/caller audit. Full entry still needs audio/cinematic/live/
profile/game/client/renderer/network/recovery owners. Prior probes not repeated;
no complete startup result claimed.
