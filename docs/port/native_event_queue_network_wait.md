# Original event queue and real native network wait

2026-10-13. Full-engine-link193 compiles shared queue/wait services but fails
genuine remaining links. Actual Com_Init/Com_Frame unexecuted.

Canonical win_main event queue insertion/shutdown and Sys_GetEvent wrapper now
selected in native headless boot. No independent engine queue copied. Native
Win_GetEvent branch consumes the same256-slot queue under the actual recursive
CRITSECT_SYS_EVENT_QUEUE, transfers payload ownership on pop, clears the consumed
slot, and reports SE_NONE with the actual timestamp only when that queue is empty.
It leaves the lock before real terminal Com_Quit_f on a latched quit request.
If terminal shutdown unexpectedly returns it aborts rather than inventing an event.

Native queue counters are unsigned32 and shutdown compares inequality, preserving
FIFO through wrap. Original overflow behavior discards oldest event and frees its
real tagged11 allocation. Native shutdown drains owned queued payloads; popped
payloads belong to the caller. Normal Windows body/queue/counters unchanged.

Canonical NET_Sleep now selected by boot. Android calls the existing real native
Sleep boundary (nanosleep/sched_yield), Windows original Sleep call unchanged.
It is a wait/yield service, not a replacement network socket implementation.

Evidence:

-193 compiles original selected queue/wait bodies and reaches remaining links;
  Sys_GetEvent and NET_Sleep no longer missing. Platform_InputFrame still required.
- takeover_event_wait_windows1 passes two whole normal Win_main/Win_net sources
  on Windows x86/x64 exit0, manifests/commands/build-result.json retained.
- event-build1 compiles actual win_main/win_net queue/wait, production native
  critical sections, zone heap, clock and quit services with ARM64 API28 UBSan
  minimal runtime. Fixture captures overflow diagnostics; unexpected Com_Quit_f
  fails the probe. No fixture claims complete terminal engine shutdown.
- event-run1 on HONOR proves270 tagged payload insertions drop/free14 oldest,
 256 FIFO pops transfer actual payloads to caller and free to baseline, empty
  absence, six events crossing unsigned counter wrap,128 distinct events from
  four concurrent producers, idempotent shutdown draining three owned payloads,
  and actual10ms network sleep plus zero-duration yield.

Only changed test executable transferred, assets reused. Native retrieval does
not emulate the Windows message queue or terminal-console poll: SDL/application
input and console producers still require integration. Existing SDL lifecycle
service demands a real consumer/owner; Platform_InputFrame must translate actual
input and pump that owner, not be an empty successful function. Canonical full
Com_EventLoop/Com_Frame and renderer/network/quit lifecycles remain unexecuted.
Continue real input/event producer and remaining authentic startup dependencies.
