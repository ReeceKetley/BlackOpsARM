# Original playlist, weapon metadata and native worker context integration

2026-10-13. Full-engine-link163 still fails on real remaining dependencies.
Actual Com_Init and Com_Frame remain unexecuted.

Selected complete canonical ui_playlists, ui_feeders_mp, cl_rank, bg_emblems
and bg_weapons_load_obj. Native weapon loader uses existing legacy CRT formatter;
Windows original behavior preserved. Original algorithms/assertions remain.
No fabricated playlist, rank, emblem or weapon data supplied.

FULL inherited Playlist_Init and BG_EmblemsInit diagnostics excluded because the
original bodies now own those symbols. Playlist_Init registers original commands
and Dvars. BG_EmblemsInit retains real asset lookup, dedicated-server material
policy and cache/Dvar setup. Neither initializer has been executed on Android.

Android jobqueue_worker now supplies Sys_InitWorkerThreadContext. Windows copied
jqWorkers[1/2].ThreadId into its OS threadId table; native role queries instead
use owning TLS registered before ready publication. Native initialization now
requires main-thread ownership, a populated worker set, dedicated processor
queues4/8, correct worker ordinal/role, a ready non-retired handle and matching
actual native thread IDs. JobWorkerReady validates current TLS role and gettid
against the queue's published ThreadId, then publishes its own immutable native
ID under the owner mutex. Initialization validates those live owners under the
same mutex. This is validation of already-established native identity, not a
dummy readiness function or a fabricated Windows thread table.

Existing queue probe now calls this boundary in each of its two start/stop
cycles and checks distinct live worker IDs. This extended probe is PENDING:
worker-context-build2 compiles but cannot link after full graph expansion.
It fails on real bg_moonGravity, events/input/network and renderer dependencies.
Earlier passing queue evidence remains valid for its original scope; no new
runtime result or original R_InitWorkerCmds lifecycle proof claimed.

Evidence:

- link160 native formatter error corrected using the actual CRT adapter.
- link161 inherited initializer duplicates corrected; link162 reaches real
  dependencies. Five normal Windows whole TUs pass x86/x64 in
  takeover_playlist_weapon_ui_windows1 with commands/manifests/build-result.json.
- worker-context-build1 used an unexported queue count global; corrected to the
  canonical jqGetNumWorkers API. Build2 compiles the new native implementation
  and test but fails linking genuine engine dependencies.
- link163 reaches the remaining full entry dependencies; the worker-context
  function is no longer unresolved. New source selection is compile/link evidence
  only, not initialized playlist/rank/asset/physics/UI behavior.

No tablet binary or game asset transfers. Remaining cinematic/audio, game state,
renderer, event/input/network and broad client/UI/script/recovery owners still
prevent production startup. Continue canonical integration and relink the pending
worker context probe, then execute the production entry when it links.
