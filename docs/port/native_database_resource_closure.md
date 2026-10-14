# Full asset dispatch and resource-source integration

Later actual full DB_Init and queue execution evidence:
[native_jobqueue_integration.md](native_jobqueue_integration.md).

Continuation of native_database_registry_closure.md. The previous turn made
verified progress on actual async IO; this turn brings the original asset
expander/dispatcher and their resource dependencies into the production library.
The requested Com_Init work remains active and incomplete.

## Current result

Full native database library builds with assertions and UBSan. In addition to the
original registry/file-loader/memory/auth/stream sources, it now includes complete
db_expand.cpp/db_load.cpp, script-string fixups, StringTable normalization, full
material/image/texture-stream/resource sources, original world state and full
com_math.cpp. It uses the original SDK declarations from read-only DXVK Native.
No BO1_PORT_X64 override, discarded layout assertion or successful dependency stub.

Native renderer state is selected from the canonical r_init.cpp declarations;
this provides genuine initially-zero globals. It does not initialize a device or
claim a graphics backend. Windows keeps the full normal source. The DirectDraw
declarations in r_texturemem.h are Windows-only; native video-memory queries still
need their real backend implementation, with no fake VRAM value supplied.

The full engine still fails to link on86 real undefined symbols in
native_registry_closure/full-engine-link15.log. That is an expanded dependency
inventory, not a separate task per symbol. A full DB_Init execution probe is
implemented but has NOT run: full-pool-build4.log still requires actual jqPoll,
jqLockData and jqUnlockData support. Do not report this as completed pool
initialization. Its expected43 capacities were independently read from BlackOps64
and are recorded with source hash in pool-reference-counts.json.

The runnable diagnostic entry builds and reaches actual Com_Init/Stream_Init,
then exits78 at DB_InitThread. Evidence current-entry-build3/current-entry-run2.
The main incremental build is restored to BO1_NATIVE_FULL_DATABASE=OFF; ON keeps
the larger honest integration attempt. No Init/Frame/gameplay completion claim.

## Implemented changes

- Original language globals/getters and load-profile begin/entry/end bodies are
  selected from their canonical sources. Native profile calls check main identity
  before reading main-owned state, avoiding worker reads of that state.
- r_resource_action now has typed callback union members, retaining exact24-byte
  Win32/40-byte native layout. Producers and dispatch call the matching function
  type; no object/function-pointer conversion or invocation through a wrong type.
- Native resource queue count and stream disable/init state use real atomics.
  Counter underflow preserves the original signed assertion. Native prefetch
  uses the corresponding locality hint; Windows instructions remain selected.
- Resource update timing calls the already-real nanosecond monotonic service,
  retaining its scoped compatibility counter type and Windows SDK ownership.
- jqBatchGroup's original constructor moved once from phys_broad_phase.cpp into
  its canonical class definition. It still sets BatchCount=0. The type retains
  an explicit8-byte size assertion. This does not create workers or complete jobs.
- GetCommandLineA can read actual /proc/self/cmdline before main, as required by
  the original texture-stream global initializer. It reconstructs Windows CRT
  argument quoting, including empty arguments, embedded quotes and trailing
  backslashes. Explicit engine-policy installation remains one-shot before
  workers start, with stable process-owned buffer storage.
- Native engine paths validate/canonicalize the actual designated game-data
  directory before workers start. Sys_DefaultInstallPath returns that root,
  independently of the development executable's directory. Sys_Cwd reads the
  actual working directory into per-thread storage. No fabricated module path.

## Evidence

- command-line-arm1 passes actual pre-main argument acquisition and stable policy
  installation. Actual Windows x86/x64 CRT parser processes reconstruct every
  argument exactly from that native command line; command-line-{x86,x64}/run.log.
- Final command-line-arm2 additionally verifies the existing staged game-data
  directory through the real path service. It transfers only its executable.
- takeover_asset_resource_windows1: seven full normal TUs pass x86/x64.
  takeover_resource_globals_windows1: full r_init/physics broad phase/stream pass
  both ABIs after constructor relocation. takeover_world_math_windows1: four
  full world/math TUs pass both ABIs. These are source/ABI checks, not runtime
  certification of rendering, scheduling or every math operation.
- full-library-final1 passes the complete selected production library build.
  The pool initializer probe and complete engine link remain unresolved as above.

Preserved failures include duplicate inherited R_StreamSetInitData diagnostic
endpoint (removed only in full source selection), a test's conflict between scoped
counter alias and SDK counter type (fixed by its actual startup interface), and
initial native path return qualifier mismatch (corrected to the canonical char*
signature). No failed attempt is recorded as execution success.

## Next integration work

Implement/select the real job-queue primitives and allocator/synchronization
dependencies. Sources are tl/jobqueue/jobqueue_all.cpp and
src/universal/com_workercmds.cpp. Their native compile reports are
full-worker-0/1-compile1.log: Windows atomic/thread calls and tick declarations
must be properly ported, not replaced by empty polls or dummy worker-data APIs.
The worker command's out-of-line data path frees actual jq heap allocations;
preserve that ownership. Then run actual full DB_Init and continue the original
database worker/loader/post-load integration, including PMem ownership/recovery.

Cinematic source compilation still needs real native Bink/platform declarations
or a properly implemented native backend; do not invent its service readiness.
Renderer events, filesystem, post-load/resource lifecycle and scheduler remain
part of the full startup closure. Retain original headless policy and runtime
branches; do not bypass initialization or skip assets to produce a completed flag.

Installed assets stay /data/local/tmp/blackopsarm-game-data, untouched/reused.
APK user-manageable import/application-UID storage remains TASK-0030.
