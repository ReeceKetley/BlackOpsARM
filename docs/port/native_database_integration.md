# Production database integration in actual Com_Init

Current authentic engine entry still stops at `DB_InitThread`. Do not call this
milestone complete until the original database worker, initialization, loading,
publication and post-load path execute together. Never redirect Com_Init to a
finite fixture callback or report completed loading from a manufactured event.

## Implemented native services

`database_worker.cpp` creates a real one-shot context-13 thread, initializes the
canonical TLS before invoking its callback and supports retirement followed by
an actual join for callbacks that return. The original DB_Thread is endless;
this implementation does not claim its shutdown or restart is solved.

Events preserve the reference initial states: wake is auto-reset/nonsignaled;
completed, completed2 and resumed are manual-reset/signaled. The six selected
database event wrapper bodies are unchanged against BlackOps64. Android's server
stop flag is atomic at all its source access sites; Windows retains its int.

The separate finite-callback probe passes ten checks on HONOR with UBSan: actual
role/TLS, original event states, phase resets, suspended reads, completion, memory
publication, distinct post-load completion and genuine join preserving main
identity. This proves platform services, not asset loading. Evidence is in
`build-android-audit/takeover_entry_run1/database-services-probe1.log` and
`database-events-reference-proof.json`. Full normal threads.cpp compiles x86/x64.

## Remaining production closure

Latest full-source/async-file closure is recorded in
[native_database_registry_closure.md](native_database_registry_closure.md).
Full43-pool registry declarations and original file loader now compile ARM64;
they have not yet executed initialization/loading in the actual entry. The full
startup link still reports real unresolved dependencies. Original disc ownership,
readiness/notification/sync bodies are now selected, and native async file reads
have issuing-thread completion evidence against both Windows ABIs.

The real chain is Com_InitCodeXAssets -> DB_InitThread/DB_Thread -> DB_LoadXAssets
-> DB_Init/DB_LoadXZone -> DB_TryLoadXFile/DB_TryLoadXFileInternal -> DB_LoadXFile
-> DB_SyncXAssets/DB_PostLoadXZone. Preserve all queue/error/lifetime contracts,
the two completion phases, early prezone loading before FS initialization and
the full reference asset-pool policy. Current selected registry tables are not
evidence of full production pool initialization.

The main-only ZoneMemory/HunkUserDebug adapter belongs to earlier bounded loading
work. It must not be weakened, nor mistaken for the real PMem path. The original
DB_TryLoadXFileInternal opens a PMem allocation scope on the worker and loads into
that pool; native physicalmemory.cpp now exists in the engine entry. Inspect and
validate the actual low/high owner transitions, native synchronization, publication
and main-side release before using it concurrently. Retain fatal/recovery behavior;
do not longjmp across live C++ owners or held locks.

The full loader's file/refill/cancellation contracts, registry/pool closure,
reorder work and retained socket/cinematic services still need implementation.
Native streaming currently parks at the real sound gate; texture/audio update
endpoints are still unfinished and cannot be reported as implemented.

No new success stubs, shortened DB loop, fabricated defaults or skipped assets.
Persistent game data stays at `/data/local/tmp/blackopsarm-game-data` for native
development, with the APK import/location prerequisite retained in TASK-0030.
