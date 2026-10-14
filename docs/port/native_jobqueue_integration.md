# Native job queue and full registry initialization

The standing Com_Init goal remains active. This continuation made verified
progress: the real full DB_Init executes on HONOR, and the original queue runs
background jobs with real native threads, TLS, heap ownership and shutdown/join.
Full Com_Init/zone loading/Com_Frame remain unfinished.

## Verified execution

full-pool-run1.log executes actual full DB_Init, real Dvar registration and actual
asset-entry initialization: all43 types comprise28 linked pools,6 singletons and
9 absent types. Every linked pool's capacity matches independently captured
BlackOps64 constants, with native pointer/stride/order checks. The retail free
entry chain preserves the first16 reserved entries and32752 remaining entries.
This is now initialization evidence, superseding earlier compile-only claims;
it is not evidence of DB_Thread loading or publication through Com_Init.

jobqueue-run2.log passes17 focused HONOR UBSan checks. Original jqInit/Start/Add/
worker loop/flush/Stop/Shutdown run with sparse processor mask0x0d: main plus two
real background workers. Two start/stop cycles process48 copied132-byte payloads
through the actual out-of-line heap path and sum600; four actual worker init
callbacks observe correct engine TLS. Both group/global completion counts drain,
the64KiB heap returns to full availability and genuine thread joins permit role
reuse. Queue nodes, worker arrays, heap and bitmaps return to allocation baseline.
Callback-switch tests additionally prove each allocation retains its original
free/realloc provider. Those providers are explicitly test-owned allocators.

The modified event wait backend passes the existing13-check finite DB callback
probe (database-services3.log). It is not the original endless database loop.
Normal Windows x86/x64 builds of four whole queue/TL/worker-command/physics units
pass in takeover_jobqueue_windows2. First Windows build lacked cstdio's std
declarations; corrected and preserved as takeover_jobqueue_windows1 evidence.

## Production changes

- Full original jobqueue_all.cpp and com_workercmds.cpp are selected in the
  database library. Native fixed-width atomic operations preserve full barriers;
  mutex ownership/recursion and shared-state reads use real ARM atomic operations.
  The queue link between separately locked producer/consumer uses release/acquire
  publication. Native global queue counters/stop state are atomic. Windows paths
  and runtime pointer/parameter layout remain preserved.
- Native job workers use actual std::thread creation and original jqWorkerThread/
  jqWorkerLoop callbacks. TLS is installed before execution; startup waits for
  actual worker identity, and join requires actual TLS retirement before admitting
  a new worker owner. Engine roles2..9 are admitted; other existing roles keep
  their restrictions. Native thread names use pthread_setname_np.
- Existing native events gain manual pulse epochs and quiescent destruction.
  A pulse releases current waiters and does not become a sticky future signal.
  Auto-reset pulse is explicitly unsupported. Destroy requires no waiters and a
  caller guarantee that all users are quiescent. Actual queue shutdown joins its
  workers first; it does not manufacture a finished state.
- Original TL service source now provides native monotonic nanosecond ticks,
  real debug output, bounded varargs formatting and matching va_end calls,
  real file reads and physical-memory/peak-RSS queries. Native scratch ownership
  is synchronized. File reads preserve real size/count/error checks rather than
  reproducing the decompiled stack-address filename concatenation.
- Native TL allocation uses actual aligned allocations or installed providers,
  tracks size/alignment and retains original ownership when callbacks change.
  Failed reallocation preserves the original allocation; unknown/double frees
  abort. Native live allocation counters are atomic. Provider setup must occur
  without concurrent callback-table changes; individual realloc/free requires
  exclusive ownership of that pointer, as with normal allocators.
- Native heap fixes release the mutex on oversized allocation failure, reject
  out-of-range frees, check live allocations before heap replacement, release
  old backing/metadata and free global queue storage at shutdown. The duplicate
  explicit heap destructor call is excluded on native; normal C++ member
  destruction remains. Stopped temporary-worker pointers are cleared.

## Evidence and current scope

Evidence: build-android-audit/native_registry_closure/{full-pool-build5.log,
full-pool-run1.log,jobqueue-build3.log,jobqueue-run2.log,database-services3.log,
full-library-worker-final1.log}. Earlier failures and logs remain intact.

The full engine link still fails honestly on82 real dependencies in
full-engine-link16.log, including cinematic/resource/renderer synchronization,
filesystem, image upload/release and post-load services. No new successful or
unavailable endpoints were added to make it link. The inherited tlPrintf endpoint
is removed only when the original TL implementation is selected.

The incremental diagnostic entry is restored to FULL_DATABASE=OFF and builds.
Actual Com_Init still stops at DB_InitThread exit78; no completed Init/Frame/map/
gameplay claim. The production full database library builds separately with all
new real implementations, assertions and UBSan. Configure ON and build the named
full database initialization/queue test targets to reproduce their respective
scope; the complete engine executable still requires the unresolved services.

Focused execution is not certification of every job command, processor mask,
heap failure/recovery case, or producer scheduling/reconfiguration pattern.
Continue actual startup integration and PMem worker low/high ownership/recovery;
do not weaken the earlier main-only ZoneMemory adapter or replace full loading
with the standalone probe. Preserve the original headless runtime policy.

No game assets were scanned or retransferred. Only changed binaries were pushed.
Persistent staging and the TASK-0030 APK import/app-UID storage prerequisite remain.
