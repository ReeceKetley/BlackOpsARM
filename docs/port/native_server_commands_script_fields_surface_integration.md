# Native server/client commands, script fields and surface loader

2026-10-13. Actual Com_Init/Com_Frame remains incomplete; no tablet run/transfers.

Full bo1_native_database now selects seven whole canonical sources: sv_client_mp,
sv_ccmds_mp, cl_net_chan_mp, cg_playerstate, cscr_main, brush_edges and
r_xsurface_load_obj. Real client drop/operator commands, channel transmit/profile,
respawn, script generic fields, brush adjacency and surface loading owners are
retained. No dummy success, readiness or independent engine implementation.

Android surface triangle ordering uses std::min/max for uint16 indices, retaining
original comparisons. The live surface loader had allocated/copied XRigidVertList
as 12-byte elements, although collisionTree is a native pointer. Android now uses
sizeof(XRigidVertList) for those runtime objects and initializes the local161-entry
array. The existing count assertion remains. Zero-count copying is skipped.
Collision-tree options clear sizeof(options), replacing a partial12-byte native
clear. Original serialized reads, index widths and Windows expressions remain.

Evidence in build-android-audit:

- native_registry_closure/full-engine-link246.log retains missing min/max compiler
  declarations; link247 compiles the adapter and all selected sources.
- link248 compiles the final native rigid-list/options correction and fails actual
  remaining engine links.
- native_registry_closure/server-client-script-surface-definitions1.log verifies
  actual archive definitions of drop/operator commands/transmit/respawn/script
  fields/brush adjacency/model surface loader.
- takeover_server_commands_script_fields_surface_windows1: seven whole normal
  TUs pass x86/x64 exit0. takeover_surface_native_rigid_list_windows1 repeats only
  the modified final surface loader; both ABIs pass exit0.

No initialized client/network/script/brush/model runtime or final native copy has
executed. Surface input bounds/alignment, array count invariants, collision-tree
ownership and retained native offsets still need runtime/caller audit. The full
entry still needs genuine audio/cinematic/live/profile/game/client/renderer/
network/recovery services. No successful startup result; earlier probes not rerun.
