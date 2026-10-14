# Native terminal out-of-memory owner

The real Sys_OutOfMemErrorInternal owner replaces the final FULL allocation-failure diagnostic. It is terminal failure handling, not successful initialization, memory recovery or clean shutdown. Root selected the source and FULL diagnostic exclusion in full406; that target linked successfully, while actual startup is being observed separately.

## Canonical semantics and ownership

Windows remains unchanged: the original Monkey path reports/disconnects and exits -1, headless exits3 without UI, and rendered failure follows the fatal-message branch then exits -1. Native retains headless exit3 and Monkey/rendered exit255. Existing native Sys_IsHeadless reads the immutable initialized policy; Monkey_IsRunning reads its actual atomic running flag. The standalone fixture's explicit policy queries are isolated test inputs, not engine/Monkey/renderer readiness claims.

Native does not call Monkey_Send, Java UI, Com_Printf/Com_Error/Sys_Error, suspension, process-guard release or engine cleanup. Those operations can require heap, engine/stdio locks, network callbacks or incomplete startup state. No successful continuation, clean marker or phase transition is published. _Exit terminates every process thread and bypasses C++ destruction/atexit; outstanding process-owned workers are not represented as explicitly cleaned up.

## Allocation-safe diagnostic path

platform/android/oom_error.cpp uses bounded stack arrays and manual decimal/string formatting. There are no malloc/new/string/stdio formatting calls. A lock-free atomic_flag admits the first reporter; recursive/concurrent entry emits a bounded constant report and terminates immediately, without waiting for the first reporter or reacquiring allocator/console locks. Filename and operation/name strings are explicitly bounded; signed values include INT_MIN safely.

It queries actual sysinfo total/free physical bytes and bounded /proc/meminfo MemAvailable plus /proc/self/status VmRSS/VmSize. Each field is reported only when its actual syscall/parser succeeds; failed/unavailable fields say unavailable. Physical free and MemAvailable are distinct labels. Unit scaling checks overflow; quantities are evaluated before formatting, avoiding unspecified argument evaluation order. No cached readiness or invented memory value is used.

Best-effort stderr output blocks SIGPIPE in the fatal thread and sets the existing stderr descriptor nonblocking. Writes have eight bounded attempts, including interrupted attempts; full/broken pipes do not park the fatal thread or replace its OOM exit status. Proc reads have bounded buffers/retries. No engine or stdio lock is acquired. Kernel syscalls still depend on the OS: nonblocking mode does not bound regular-file/device kernel IO or scheduling under catastrophic system pressure. This is not a guarantee against an unresponsive kernel or exhausted stack; it removes application lock/heap/backpressure dependencies.

## Real allocator failure callsites

The original pre-handler logs were themselves unsafe during failure: Z_MallocFailed used Com_PrintError, and PMem high/low exhaustion used Com_Printf/Error. Com_PrintMessage takes console/RD locks and can invoke remote debug, UI, Monkey and file logging. Native failure-only branches now call actual ReportAllocationFailure directly, retaining operation, requested bytes, allocation name where known, and actual PMem over-allocation deficit. Unknown deficit is explicitly unavailable. Successful allocation, pointer/position updates, tracking and lock-publication paths are unchanged. Normal Windows retains the original logs and handler calls exactly.

Owned canonical edits are only universal/com_memory.cpp and universal/physicalmemory.cpp. win_main.cpp, normal fatal reporter, global initialization, process guard and shared scheduling were not edited. Generic virtual/hunk failure calls still reach actual Sys_OutOfMemErrorInternal; requested-size context was added only where the replaced prelogs already supplied it.

## Integration

Select platform/android/oom_error.cpp with existing actual policy owners; no new production definition or library is needed. Preserve the existing canonical Sys_OutOfMemErrorInternal declaration/ABI. Root excludes its unavailable_services.cpp definition only from FULL; historical standalone diagnostics remain. The new header exposes the narrow nonreturning context reporter used by actual allocator failure branches.

Production object undefined-symbol audit is native_registry_closure/oom-error-undefined-audit.log. It contains only existing policy reads, lock-free compiler atomic helper, memset, POSIX IO/signalmask/sysinfo and _Exit. There are no heap allocation, stdio, engine lock or cleanup symbols.

## Focused real negative evidence

Run tools/port/build_native_oom_fixture.ps1 (NDK30/API28 ARM64 UBSan). Transfer only native-oom-error-tests to the already existing HONOR AKWJ9X5116G04618 runtime, chmod755 and run ./native-oom-error-tests . there. No assets move. Logs: oom-error-native-build1.log, oom-error-honor-run1.log and final oom-error-honor-final.log.

All six cases pass. Each child establishes an actual OS file lock/running marker, registers an atexit clean-marker callback, starts a real thread holding stderr's FILE lock, sets actual RLIMIT_AS below the current address-space requirement and observes actual malloc ENOMEM. It then arms malloc/calloc/realloc tripwires before the real reporter entry. Cases prove headless3, rendered255 and Monkey255 policy exits, bounded long filename/negative line/request context, genuine recursive reporter entry, a prefilled nonblocking stderr pipe and a broken stderr pipe with SIGPIPE contained. The recursive case is a fixture-only wrapped sysinfo fault that invokes the real reporter again; it never fabricates successful telemetry. After process termination the marker remains running, atexit never writes clean, and another process acquires the same actual OS flock. These are component/OS lifetime proofs, not initialized engine teardown or physical low-RAM stress.

The final context test passes the actual failed 32MiB malloc request and reports unavailable deficit rather than inventing a kernel deficit. PMem deficit fields come directly from its actual allocator failure state in production.

Normal Windows x86 and x64 whole-TU compilation of com_memory.cpp and physicalmemory.cpp passes once with native selection absent. Reproduce build_startup_services_windows_compile.ps1 with these two units and a fresh directory; manifests/results are takeover_oom_allocator_windows1/{x86,x64}. Original CP1252 source content/line endings and Windows behavior remain.

## Frozen source hashes

- src/platform/android/oom_error.cpp: cf928dbe4766b7f8c959cf526775d1d7cd38d1ce3491d1590598ebc9346d1362
- src/platform/android/oom_error.h: 5bd28f9e1d564cd8fe183b186c74334ff88dde6912b327fbf176f07f0a76448b
- src/universal/com_memory.cpp: 567e1b73bad2b8916ff3a9e92e7838b82d5a887a64436993fbceebd8cc06bfa1
- src/universal/physicalmemory.cpp: 8b49753d91735ec418e271a647891ae0fa32b4d9839943444432c204547389fe

No post-init whole-engine OOM cleanup or Android UI/Monkey error notification was claimed. Native termination deliberately retains the dirty process marker; root owns the live engine startup outcome.
