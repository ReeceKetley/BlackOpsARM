# Next database platform boundary

Read-only source audit after TASK46 initial integrated startup-service smoke,
12 October2026. This supplements the existing production startup dependency map.
No database worker loading has been implemented or certified by this audit.

`qcommon/threads.cpp:379-402` creates an initially false auto-reset wake event,
then initially true manual-reset completed, completed2 and resumed events. It
creates suspended context13 and resumes it only after registration. Thread entry
at232-251/281-294 must initialize genuine TLS before invoking its callback.

`database/db_registry.cpp:4432-4459` supplies actual `DB_Thread`, which validates
context13, installs its genuine error jump target, then repeatedly waits and
calls `DB_TryLoadXFile`. Preserve that endless production loop. Its recovery
reaches `Com_ErrorAbort` and fatal `Sys_Error`; this is not recoverable loading.

Queue publication at `DB_LoadXZone`4364-4419 fills names/flags, sets loading
count, resets completed2 then completed, publishes zone count and signals wake.
`DB_TryLoadXFile`4464-4536 captures/clears the count, loads queued zones, finishes
reorder work and signals completed. `DB_SyncXAssets`3708-3725 waits, then executes
`DB_PostLoadXZone`3606-3685, which performs post-load initialization and signals
completed2. These two completion phases must remain distinct.

`Sys_DatabaseCompleted` in threads.cpp593-599 sets server-stop state and waits
for server completion when that event exists, then signals completed.
`Sys_SyncDatabase`616-627 retains cinematic/socket pumping and conditional render
resource updates. Ready queries also invoke SocketRouter on success. Do not
replace those dependencies with manufactured readiness. Disc-read suspension
405-454 controls a separate resumed gate which actual file reads must respect.

Implement real mutex/condition event state with manual-reset broadcast,
auto-reset single consumption and signal coalescing. Preserve zero-time polls,
finite timeouts and indefinite waits. Successful waits must establish memory
publication. Audit volatile queue/stop access outside those synchronized handoffs
before integrating actual DB/server publication.

The hard loading barrier is allocator ownership: Android zone_memory.cpp23-38
and58-88 requires immutable main identity. Hunk_UserDebug Init/Alloc/Free/Destroy
in mem_userhunk.cpp107/152/182/235 and native reset135 have the same restriction.
Actual loading allocates on role13 at db_registry.cpp4682/4711 while publication
and unload happen on main. Do not re-register main identity or globally weaken
checks. A bounded owner/transfer design must account for worker allocation,
published lifetime, quiescence, main-side destruction and fatal recovery.

Recommended first milestone: native one-shot role13 spawn and real event
primitives, selecting actual database wrappers. Validate with a clearly labeled
finite callback and real TLS. Optionally prove actual DB_InitThread/DB_Thread
reaches its genuine parked wait in a dedicated child process. Do not wake that
loop through a fabricated loader. Its lifetime remains process-owned because
the production loop contains no shutdown branch.

Acceptance uses an independent Windows event oracle for initial states,
manual-reset repeated waits/broadcast, auto-reset single-consumer/coalescing,
reset, finite timeout and payload publication. Verify TLS initialization before
callback, explicit duplicate spawn failure, finite callback retirement/join,
and exact distinct completion ordering. Never destroy events with live waiters,
reset/reuse the one-shot role, or longjmp across owning C++ objects. Resolve
allocator ownership and actual file-loader closure before queuing a genuine MP
startup zone through production DB_LoadXAssets.
