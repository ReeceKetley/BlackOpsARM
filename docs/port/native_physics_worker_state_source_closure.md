# Original physics worker state and task manager integration

2026-10-13. Full-engine-link151 reaches linking but fails on remaining genuine
engine/platform dependencies. Actual Com_Init and Com_Frame remain incomplete.

Selected complete canonical sources in the full native database target:
phys_traverse, win_workercmds, statmonitor, r_foliage, cg_event,
phys_task_manager, surfaceflags and rb_debug. Their existing bodies and assertions
are preserved. The first five sources are unchanged by this selection; the last
three also compile without source adaptation. No new placeholder was introduced.

These supply the collision geometry visitor, physics worker command/module,
warning monitor, foliage worker wait, ragdoll trajectory query, physics task
manager, surface friction/bounce logic and debug brush/patch callback owner.
The original task manager queues at most six batches through the original
IW_task_manager_add_batch and drains them through IW_task_manager_flush.

The full-build inherited R_InitWorkerThreads diagnostic endpoint is excluded
because win_workercmds now supplies the actual initializer. Diagnostic OFF retains
its endpoint. Original R_InitWorkerCmds performs jqInit, enables processor mask
12, sets batch storage, starts workers, submits setup jobs on queues 4 and 8,
flushes, initializes worker contexts and worker command state. Selecting that
body is not evidence of executing its startup/TLS/shutdown lifecycle.

Evidence:

- full-engine-link149 preserves the diagnostic duplicate; link150 shows its
  correction and remaining genuine dependencies.
- full-engine-link151 compiles the three additional sources and reaches linking.
  llvm-nm confirms defined ARM implementations of the task manager, friction,
  bounce, debug callback and worker initializer in libbo1_native_database.a.
  The task manager, surface functions and debug callback are no longer reported
  as unresolved in that link.
- takeover_physics_state_windows1 has exit 0 for five whole translation units
  on both normal Windows x86 and x64 builds.
- takeover_physics_task_windows1 has exit 0 for the three additional whole units
  on both normal Windows x86 and x64 builds. Commands and source manifests remain
  alongside each build-result.json.

No new runtime probe, tablet binary or asset transfer. Previous queue tests
remain valid for their recorded scope; they do not prove this original worker
initializer, initialized physics tasks or renderer callback execution. Remaining
audio/cinematic, event/input, client/UI/expression, physics/ragdoll, game/script,
renderer/network and recovery dependencies still prevent running full startup.

Next canonical owners include com_expressions_eval for UI expression evaluation
and cg_hudelem for scoped placement stack constructors/destructors. Implement
native platform boundaries with real behavior as they become retained. Then run
the production entry; do not substitute fixture initialization for Com_Init.
