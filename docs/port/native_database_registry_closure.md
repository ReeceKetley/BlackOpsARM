# Native database registry and async file integration

Later expansion/resource integration and current remaining dependencies:
[native_database_resource_closure.md](native_database_resource_closure.md).

The full original registry and file loader now compile as ARM64 production
objects, including all 43 asset pool declarations, initializers and dispatch
tables. This is compilation evidence; full pool initialization and zone loading
have not executed through Com_Init. The current runnable entry still exits78 at
DB_InitThread. No Com_Init completion, Com_Frame or gameplay claim.

## Implemented dependencies

- TL recursive reader/writer lock uses actual Android thread IDs, yielding and
  fixed-width atomic operations. Native physics read-release sites use an atomic
  release. Original Windows paths and structure widths remain selected there.
- Canonical DSP parameter/state declarations moved into snd_dsp_types.h, avoiding
  an XAudio device dependency for CPU data. Windows XAudio classes are retained.
  Native playerState uses the checked10152-byte runtime layout; Win32 retains9892.
  SDK RECT/POINT and native OVERLAPPED declarations retain checked ABI sizes.
- Native CRT strdup, stricmp and itoa implement real allocation, comparison and
  conversion, including INT_MIN and nondecimal two's-complement output.
- Native CreateFileA/ReadFile/WriteFile/CloseHandle/GetFileSize/GetFileAttributesA
  use real POSIX files. Existing ASCII game paths support Windows case lookup,
  with ambiguous aliases rejected. Sharing checks precede truncation and read
  counts preserve short-read/EOF behavior. These are scoped file services, not a
  complete Windows API implementation.
- ReadFileEx queues actual pread work on a process-owned IO thread. It captures
  the requested64-bit offset and queues completion on the issuing thread.
  SleepEx dispatches callbacks only in that thread's alertable wait. Nonalertable
  waits and other threads cannot consume those callbacks. Buffers and OVERLAPPED
  storage must remain live until completion is dispatched. This matches the
  original DB_ReadData/DB_WaitXFileStage mechanism; it is not synchronous pretend
  async IO or a fabricated completed event.
- Pending IO cancellation is unfinished. CloseHandle rejects pending callbacks
  with ERROR_BUSY170; drain them before close. General overlapped ReadFile/
  WriteFile, CancelIo, cross-process Windows sharing, full Unicode case folding,
  complete attribute emulation and IO worker shutdown remain unsupported.
  The existing DeleteFileA service does not enforce share-delete policy.
- Full loader progress counters, minimum-file readiness, DB initialization state
  and disc owner state use native atomics; Windows storage is preserved. The
  native prefetch uses the compiler's actual ARM operation.
- Original disc suspend/resume/owner queries, two database readiness queries,
  notification and sync bodies are selected from threads.cpp. ThreadOwner has
  one canonical declaration in thread_owner.h. Sys_SyncDatabase retains its
  cinematic/resource calls; those dependencies have not been bypassed.
- The selected SocketRouter_EmergencyFrame is the existing BlackOps64 body,
  which only queries main-thread identity. This does not implement networking.

## Production build closure

`BO1_NATIVE_FULL_DATABASE=ON` selects the full registry/file loader/asset-name/
memory/auth/stream/stream-load sources, original memory tracker, existing native
zone heap and database diagnostics. It removes the selected registry and tracker
objects from the other libraries, keeping one canonical set of storage and
functions. Its matching six engine zlib C sources use the modified z_stream ABI;
Android system libz must not replace them.

The full database library compiles with assertions and UBSan. The engine link
still fails honestly on81 undefined dependencies, recorded in
build-android-audit/native_registry_closure/full-engine-link8.log and
full-engine-unresolved.json. This number is a symbol inventory, not81 separate
tasks. Remaining groups include actual asset expansion/dispatch, resource
release/post-load/reorder, renderer/cinematic synchronization, filesystem/global
state and engine recovery. Implement/link their real source and platform services;
do not add unavailable endpoints merely to make this target link.

To reproduce the integration attempt in a fresh directory:

```powershell
tools/port/build_native_engine_entry.ps1 -BuildDirectory build-android-audit/full-db-next -FullDatabase
```

This currently retains a failed engine link as evidence. Within a configured
ON directory, `cmake --build <directory> --target bo1_native_database` builds the
production library alone. OFF retains the runnable diagnostic entry and its
explicit DB_InitThread stop. Current incremental native_engine_entry1 is OFF;
all earlier full-build objects and link logs remain preserved.

## Verification and limits

Evidence directory: build-android-audit/native_registry_closure.

- services-arm1 and services-{x86,x64}/run.log match exactly for TL concurrency,
  DSP widths,20 itoa and16 stricmp records plus binary file/size/sharing/case/EOF.
- async-services-arm2 and async-services-{x86,x64}/run.log match exactly,
  including original offset reads, short counts,64-bit EOF and issuing-thread
  callback dispatch. Final ARM UBSan async-services-arm3 adds explicit pending
  close rejection; all remaining records match the Windows oracle.
- database-services2 passes13 focused checks through a labelled finite callback:
  actual role13/TLS, disc ownership/suspension/publication, original initial events,
  distinct completion/readiness phases and genuine join. This does not run the
  original endless DB_Thread or load assets.
- takeover_database_wrappers_windows1 compiles six normal production units for
  x86/x64. Earlier takeover_tl_dsp_windows1 covers registry and CPU/XAudio DSP.
- current-entry-build2 passes; current-entry-run1 reuses the installed assets and
  reaches actual Com_Init/Stream_Init, then DB_InitThread exit78. Only changed
  binaries were transferred. No asset retransfers or full asset scan.

Preserved failures: initial async probe needed static libc++ linkage; first full
engine attempt found a duplicate inherited DB_FileExists endpoint; first expanded
database probe needed a test-owned fatal assertion handler. Corrected evidence
uses fresh logs; none of those failures is represented as a pass.

Next work remains full production integration, including PMem low/high ownership
and real worker/loader/publication/recovery, then original startup MP71. The main
thread's earlier bounded ZoneMemory adapter must not be weakened or substituted
for worker-owned PMem loading. Keep installed game assets unchanged. The APK
asset import/application-UID storage prerequisite remains TASK-0030.
