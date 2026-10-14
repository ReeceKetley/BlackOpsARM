# Existing SP startup, client and animation/actor integration

2026-10-13. Genuine Com_Init remains incomplete. No tablet run or asset transfer.

Six existing whole sources now belong to full bo1_native_database: g_sp_levelstart,
g_sp_client, g_anim_commands_sp, cg_anim_commands, cg_actor_model_state and
cg_actors_mp. Original SP policy/script startup, client command handling, server
animation messages and client animation/actor snapshot behavior remain intact.

Android local-fight command publication uses a real std::shared_mutex with exclusive
writers/shared readers at the original acquire/release boundaries. TU-local adapter
functions preserve call sites; Windows retains SRWLOCK. The two numeric-only secure
scans use actual native sscanf (no string-capacity arguments). Client animation
diagnostic labels use bounded snprintf, handling would-truncate lengths before
incrementing offsets; Windows secure formatter/copy expressions remain unchanged.

Attempted g_sp_measure compilation in link268 reveals real Windows intrinsics/
process/thread resource counters. That source is not yet selected in the final
native list; its functions remain genuinely unresolved. Next adapt it using actual
native timing/CPU/resource observations, preserving truthful metrics and failure
reporting. No fake counters or successful endpoints were substituted.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link268.log preserves native lock/numeric
  scan/formatter and measurement Windows-header failures. link269 compiles the
  six corrected owners and reaches genuine remaining links.
- native_registry_closure/sp-start-client-anim-actor-definitions1.log confirms
  actual SP policy/script load/local command, animation messages/replay, actor
  models and snapshot owners in the native archive.
- takeover_sp_start_client_anim_actor_windows1: six whole normal TUs pass
  x86/x64 exit0 after final source changes.

No initialized SP/client/animation/actor runtime or native command lock lifecycle
has executed. Command publication/retirement, buffer bounds, native offsets and
snapshot lifetime require runtime audit. SP measurement needs native porting;
other links include omitted original owners and real Android platform services.
No completed Com_Init/Com_Frame or gameplay claim; earlier probes not repeated.
