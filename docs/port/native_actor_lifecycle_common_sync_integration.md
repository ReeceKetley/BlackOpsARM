# Existing actor lifecycle and common synchronization integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete; no tablet run/transfers.

Full bo1_native_database selects eight existing whole actor_state/spawner/scripted/
orientation/senses/physics/death/corpse sources unchanged. Canonical common.cpp
also selects original Com_GetTimeScale, Com_SetTimeScale and Com_SyncThreads for
native boot, using actual renderer/worker/server declarations. Function bodies,
state values, synchronization calls and Windows behavior remain unchanged. No
independent AI or successful wait/ready substitute introduced.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link284.log compiles eight actor sources;
  link285 compiles common selection and fails genuine remaining links.
- actor-lifecycle-common-definitions1.log confirms actual actor think/spawn/state
  and common time-scale/thread-sync owners in native database/startup archives.
- takeover_actor_lifecycle_common_sync_windows1: nine whole normal TUs pass
  x86/x64 exit0 after final source changes.

No initialized state/spawn/scripted/senses/movement/death/corpse, time-scale or
worker/server/render synchronization runtime has executed. Native trace/state
offsets, physics/thread-local global use, references/pool lifetime and ordering
remain runtime/caller gates. Source compilation does not prove actual worker or
renderer readiness. Full startup still needs remaining original owners and genuine
Android platform services. No Com_Init/frame/AI completion or repeated probes.
