# Native movement, flame and model collision loader integration

2026-10-13. Genuine Com_Init remains incomplete; the full executable still fails
to link. No assets were transferred and no tablet runtime is claimed here.

The full bo1_native_database source list now selects nine canonical whole sources:
bg_pmove, cg_servercmds_mp, destructibledef_load_obj, physconstraints_load_obj,
xmodel_load_phys_collmap, and the chunk/fire/smoke/drips flame classes. This
connects original movement/sprint, server config parsing, destructible/constraint
loading and flame initialization/update behavior rather than diagnostic endpoints.

Inspection found original fixed allocation sizes in the collision-map text loader.
Android now uses sizeof(Collmap), sizeof(PhysGeomList), sizeof(PhysGeomInfo),
sizeof(BrushWrapper) and sizeof(cbrushside_t) for these native runtime objects.
Array clearing uses the same allocation byte count. Widened array multiplications
check the signed INT_MAX allocator limit before multiplying; excess size reports
the actual Com_Error(ERR_DROP) and aborts if that unexpectedly returns. No disk
layout, asset bytes, assertions or Windows allocation expressions were changed.
Existing float-vector and plane allocations retain their original sizes.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link221.log compiles the first three
  sources; link222 compiles the remaining six; link223 compiles the final native
  allocation correction. All reach genuine unresolved engine links.
- native_registry_closure/movement-flame-model-definitions1.log records actual
  defined symbols from libbo1_native_database.a, including PM_IsSprinting,
  CG_SetConfigValues, both loader entry points and all four flame initializers.
- takeover_movement_flame_model_loaders_windows1: nine whole normal sources pass
  x86 and x64 (exit0). takeover_collision_map_allocation_windows1 repeats only
  the changed collision loader after the Android correction; both pass exit0.

No initialized movement, server-command, flame, script, destructible, physics,
collision-map parser or GPU path has executed. Remaining links include real live/
profile, audio/cinematic, game/client, renderer, network and recovery dependencies.
The loader still requires actual initialized runtime validation and further
parser/ownership audit; compilation alone does not prove its full correctness.
