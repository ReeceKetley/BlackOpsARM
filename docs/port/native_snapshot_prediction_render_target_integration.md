# Native snapshot, prediction and render-target integration

2026-10-13. Genuine Com_Init remains incomplete. No tablet run or asset transfer.

Full bo1_native_database now selects seven whole canonical sources:
sv_msg_write_mp, sv_snapshot_mp, cg_predict_mp, g_load_utils, cscr_tempmemory,
r_rendertarget and rb_draw3d. These supply real server snapshot/message ownership,
client prediction state, entity spawn parsing, per-thread temporary hunk ownership,
render-target creation/release and draw/game-time helpers. No substitute algorithm,
dummy success, readiness flag or assertion bypass was added.

Native snapshot formatting uses the existing legacy CRT adapter. Inspection found
SV_BuildClientSnapshot copying only the original 9892-byte player state. Native
playerState_s is 10152 bytes (its existing header assertion), so Android now copies
the typed entire ps member from SV_GameClientNum and reads clientNum/origin/
viewHeightCurrent/viewangles/leanf by name. Snapshot metadata after ps is not
overwritten. Windows retains the original copy and field expressions. This is
runtime object copying; message serializers and disk records are unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link228.log preserves the missing native
  underscore formatter compile failure. link229 compiles all selected sources;
  link230 compiles the final Android snapshot correction and fails real links.
- native_registry_closure/snapshot-prediction-target-definitions1.log confirms
  actual archive definitions of snapshot construction, prediction global, ground
  entity writer, spawn parser, temp allocator, extra-camera targets and time vector.
- takeover_snapshot_prediction_render_targets_windows1: seven whole normal TUs
  pass x86/x64 exit0. takeover_snapshot_native_copy_windows1 repeats only the
  final modified snapshot source; both ABIs pass exit0.

No initialized snapshot serialization, client prediction, spawn parser, temporary
hunk use or render-target/device lifecycle has executed. Retained decompiled byte
offsets and command-buffer copying require further caller/layout/ownership audit.
The full entry still lacks genuine audio/cinematic/live/profile/game/client/
renderer/network/recovery dependencies. No complete Com_Init or Com_Frame proof.
