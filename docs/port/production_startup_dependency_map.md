# Genuine T5 headless startup dependency map

Read-only source/evidence audit, 12 October 2026, anchored at `db6c0e4`.
This maps the production path; it does not certify Android engine startup.
Research agent `production_startup_audit` made no edits, builds or device calls.
Root checked the cited startup, thread and recovery entry points in this worktree.

## Actual entry and call order

`src/win32/win_main.cpp:2419` establishes main-thread identity/TLS, localization,
parse state, actual `Dvar_Init`, timing, machine information and a mutable command
line before calling `Com_Init` at the end of that initialization section. Its
main loop calls actual `Com_Frame` (`win_main.cpp:2539`). A native entry must
provide equivalent required platform setup; calling an invented boot function is
not production T5 startup.

`src/qcommon/common.cpp:1729` establishes the calling thread's error recovery
point through `Sys_GetValue(2)`, calls the inner initialization body and then
processes startup commands, optional common/UI loading and dedicated metadata.
Calling only `Com_Init_Try_Block_Function` is not equivalent to `Com_Init`.

The inner body (`common.cpp:1932`) orders server/client SL initialization,
Cbuf/Cmd, startup variables and Dvars, IK, PMem and stream initialization,
`Com_InitCodeXAssets`, filesystem initialization, command/configuration/table
services, dedicated setup/hunks, script VM, XAnim/DObj, SV/NET, workers and server
thread creation. It finishes the `$init` owner and publishes full initialization.
Even dedicated mode retains local-client CPU bookkeeping and some Live calls.

**The first zone loads before `FS_InitFilesystem`.** Preserve that order:

```text
Native platform entry (equivalent required setup)
  Com_Init
    Com_Init_Try_Block_Function
      SL -> Cbuf/Cmd -> Dvars -> PMem/Stream
      Com_InitCodeXAssets
        DB_InitThread -> real database worker/TLS
        DB_LoadXAssets(code_pre_gfx_mp, sync=0)
        DB_SyncXAssets
      FS_InitFilesystem -> configuration -> dedicated setup
      script VM -> XAnim/DObj -> SV/NET -> server/worker threads
    remaining startup commands and dedicated metadata
  Com_Frame -> event/Cbuf/task processing -> SV_Frame -> dedicated physics
```

## First missing services and ownership contracts

| Boundary | Required production dependency | Existing ARM64 evidence / gap |
| --- | --- | --- |
| Before `Com_Init` | Main-thread/TLS, actual Dvar registration, timing, command line | Selected thread identity/timing/string services pass; whole entry/Dvar closure does not |
| Cbuf and every frame/error | General critical sections | Native script bridge supports only 27, 28, 74; `CRITSECT_CBUF` and `CRITSECT_COM_ERROR` remain missing |
| `Com_InitCodeXAssets` | Actual DB initialization, command registration, worker and work publication | Entry initializer/pools/selected registry operations pass; whole `DB_Init`/worker path does not |
| First DB file load | Early path/language/patch lookup, file completion/refill/cancellation | Complete synchronous decoder passes; production async pipeline remains unported |
| Dedicated bootstrap | Genuine postgfx/dev/patch/common assets and retained CPU initialization | First default graph/default resolution pass; full postgfx asset coverage remains incomplete |
| Runtime | Script variables/VM, XAnim/DObj, SV/NET, server roles/threads | Selected script functions execute; whole VM/server initialization is unverified |
| Map/gameplay | FS restart, map/world/collision, game scripts, server frames/physics | Not yet executed as an integrated native engine |

`src/qcommon/threads.cpp:100` and `:281` register thread context/TLS before engine
work. `src/universal/q_shared.cpp:682` binds slots 1=va state, 2=error buffer,
3=trace state, 4=command arguments. `q_shared.cpp:35` already uses actual native
`jmp_buf g_com_error[15]`; do not reinstate a fixed Win32 integer buffer.

`threads.cpp:379` requires database wake auto-reset/initially false and completion,
completion2/resumed manual-reset/initially true events. `Sys_SyncDatabase` at
`:616` waits while pumping retained cinematic/socket services; render-resource
pumping is conditional. A successful no-op cannot replace these semantics.

`src/database/db_registry.cpp:3803` lazily invokes whole `DB_Init` (`:4140`),
manages ownership/publication and signals the worker. `DB_InitThread`/`DB_Thread`
at `:4440`/`:4446` require worker context13/TLS/recovery before `DB_TryLoadXFile`.
The selected serial loader tests do not close those dependencies.

`src/database/db_file_load.cpp:347` uses a 512 KiB ring, 256 KiB reads and Win32
`ReadFileEx`/`OVERLAPPED`; wait at `:111` and cancellation at `:85` require genuine
completion and draining before buffer/codec/file release. A native backend can
use different OS mechanics while preserving those engine-facing contracts.

`Com_Error` (`common.cpp:740`) uses the calling thread's recovery target and
global error synchronization. `Com_ErrorCleanup` restores subsystem/DB/temporary
state. Test exceptions/aborts do not prove this recovery route. Never longjmp
over live owning C++ locals with nontrivial destructors; keep owner cleanup state
outside that extent or provide an explicit production cleanup path. Worker
shutdown/join and role lifetime must be solved before application reinitialization.

## Dedicated policy and ABI boundaries

`src/client_mp/cl_main_mp.cpp:3661` calls `R_MakeDedicated`;
`src/gfx_d3d/r_init.cpp:144` sets/latches `r_loadForRenderer=false` but still calls
`DB_LoadGraphicsAssetsForPC`. `db_registry.cpp:5913` loads MP postgfx, SP postgfx
for Zombies, dev/patch zones and optional mod. Zombies later needs common_zombie
and the map. Renderer-off does not remove CPU assets, default providers or scripts.
The next unimplemented postgfx asset boundary is Image type8.

GPU/window/backbuffer, device audio/voice, controller hardware and UI rendering
can follow existing dedicated policies. Retained CPU/bookkeeping calls still
need real implementations or verified existing reference no-op semantics.
`BO1_LIVE` DemonWare sections are optional; other Steam/Live wrappers need an
explicit source-backed local/offline policy. Loopback NET/event processing is a
hard retained dependency, not automatically optional.

Keep bundled custom zlib: stream sizes are Win32 52, Windows x64 80, ARM64 104;
ARM64 `unsigned long` is eight bytes. Check fixed32 counters/atomics, high native
pointers, OS handles and each real asset/expander layout. Native Windows PMem
third-plus same-primitive record stride remains uncertified; current tested two
owner case is contained. Android named owners use a separate real backend.

## Evidence scope and implementation sequence

TASK17 certifies actual SL/MT interning/lifetime; TASK31 certifies the canonical
entry-list helper, not whole DB initialization. TASK37 certifies the complete SP
37-asset pre-gfx zone; TASK39 certifies the first MP/SP default CPU graphs; TASK40
certifies default clone/naming/retained ownership. Their HONOR and Windows results
are strong prerequisites, not proof of `Com_Init`, `Com_Frame` or gameplay.
`src/platform/android/environment.cpp` is still an environment JNI probe.

1. Finish TASK41 reader/owner and complete genuine MP71 loading with actual
   supported loaders and explicit real caller service contracts.
2. Implement native main-thread/TLS/recovery, general synchronization and actual
   Dvar/Cmd/Cbuf/early startup services, linking real production translation units.
3. Execute actual DB worker/events/read completion, `DB_LoadXAssets`,
   `DB_SyncXAssets`, publication and cleanup for the first zone.
4. Close genuine dedicated bootstrap asset coverage and VM/XAnim/DObj/SV/NET.
5. Native entry calls actual `Com_Init` to completion and a bounded number/time
   of actual `Com_Frame` calls, then genuine quit. Require production initialization
   publication, genuine registrations, real VM/server state and frame advancement.
6. Through actual Cbuf/startup commands, `devmap zombie_pentagon` reaches
   `SV_SpawnServer` (`src/server_mp/sv_init_mp.cpp:513`), map/world/collision,
   game scripts and observable Zombies simulation before graphics integration.

A traced failure reached inside actual initialization is useful integration
evidence, but completed startup requires step5. Disabling FastFiles, returning
after the first zone or using a replacement frame loop does not satisfy it.

Windows `bo1_headless` is the dedicated reference. `bo1_headless_client` is a
rendered listen server on a private desktop (`win_main.cpp:2485`, reference
`tools/headless.ps1:35`); do not call that renderer-free validation.

Parent attribution correction: `../.ledger` FACT-0103, event EVT-00000197,
2026-10-11T15:30:37.7047118Z, records Final57 shader/native/gameplay/save results.
It does not itself establish the exact 180-second/three-round run previously
attributed to it in Android FACT121. The verified playable Windows reference
remains accepted; exact older run claims need their own specific archive citation.
