# Original entity handles, server state/world, game utilities and sky rendering

2026-10-13. Full-engine-link215 compiles selected sources but fails remaining
genuine links. Actual Com_Init/Com_Frame/server execution unverified.

Selected whole enthandle, sv_main_mp, sv_world, g_utils_mp and rb_sky. These own
actual handle validation/lookup, canonical server/static/header state, server
startup/frame/wait logic, world entity tracing, game utilities and sun flare state.
No artificial server-ready, entity/game state or network values supplied.

Native server CRT formatting uses existing real adapter; timing counters use
native nanosecond performance-counter type/provider and thread identity uses
actual profile thread ID. Native frame-handshake counters are fixed32 lock-free
atomics; original reads/updates use seq-cst semantics, rather than mixing ordinary
reads with atomic writes. Windows original LONG/Interlocked behavior retained.
Native Sleep uses actual existing boundary; sky query loop retains real query
handling and uses actual zero-duration yield. Timing unit consumers/server reports
still need actual native lifecycle validation; no Windows TSC unit assumption
or completed server frame claimed.

FULL inherited SV_InitServerThread/SV_WaitServer diagnostics excluded only because
the original source now owns them. OFF diagnostic selection remains as before.
Actual native server spawning/context/events and subsequent game/frame execution
remain required; selecting an initializer does not implement its dependencies.

Evidence:

-213 preserves native formatter/thread/timer/atomic/Sleep compilation errors.
-214 corrects them and reaches inherited server diagnostic duplicates;215 fixes
  ownership and reaches real remaining links.
- takeover_server_handles_world_windows1 passes five whole normal TUs x86/x64
  exit0 with commands/manifests/build-result.json.

No new runtime probe or device/asset transfers. Server/handle/world/game/sky paths,
native pointer/stride/state publication and resource/worker lifecycles unexecuted.
Previous event/queue/clock/identity fixtures keep only their proven narrower scopes.
Continue genuine config/live/profile/cache/server-spawn/game/client/audio/cinematic/
renderer/input/network/recovery services from215; run authentic startup when linked.
