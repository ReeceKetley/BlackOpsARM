# ARM64 parse-tree and HunkUser gate

Completed 2026-10-11 on android-arm64, based on 76edfd5. This is the bounded
continuation of TASK-0005, recorded in DEC-0012/0013 and FACT-0014/0015.
No engine startup, gameplay or backend integration was attempted.

## Result and implementation

The existing CMake inventory now selects eight actual T5 translation units,
adding cscr_parsetree.cpp and mem_userhunk.cpp. Production parse function bodies
are unchanged; tests call them directly. There is no copied engine source tree.

Initial compilation exposed unused database/UI/D3D9 dependencies, then three
missing animation declarations. Android-only include narrowing removes those
dependencies from the selected files. memory_services.h contains six existing
allocation declarations; com_errors.h contains the original error enum. The
broad Windows headers include these lightweight declarations, retaining their
interfaces, enum values and Windows dependencies.

The real default HunkUser allocation policy is reused. Typed forwarding adapters
replace incompatible function-pointer casts in its dispatch table. Windows keeps
its original 4 KB page policy and legacy exact-end rounding. Android uses native
sysconf page size and correct ceiling rounding, including an allocation ending
exactly at the reservation boundary. Logical arena sizes and maximum alignment
remain unchanged; the OS reservation can round to a larger native page.

The Android Z_VirtualReserve/Commit/Decommit/Free boundary uses mmap(PROT_NONE),
mprotect, madvise(MADV_DONTNEED) and munmap. A mutex protects reservation metadata
and OS operations. Invalid ranges, interior releases, type mismatches and OS
failures abort explicitly. There is no low-address allocation requirement.
Default HunkUser remains confined to its owner thread; separate arenas may use
the native registry concurrently. Caller-owned buffers are never unmapped.
Android creation rejects nondefault schemes and tracking until separately
validated. Allocation rejects negative sizes and invalid alignment.

## Validation

- Final NDK r30, Clang 21, API 28, arm64-v8a, Debug C++20: **0 errors, 685
  retained warnings**. No disabled slot/disk assertions or BO1_PORT_X64 define.
- Eleven native probes pass on HONOR NDL-L09, serial AKWJ9X5116G04618, API 36.
  Native page size measured by the probe is **4096 bytes**.
- Actual node0..8, raw signed/unsigned/float/null/handle copies, node2_, list
  end/prepend/append, both instances and 10000 retained nodes across arena growth
  pass. sval_u remains four bytes with four-byte alignment; decoded real Android
  pointers exceed 4 GB. Client storage survives server shutdown/reinitialization.
- Actual HunkUser passes 2000 payloads with alignments 1..4096, growth/reset,
  recommit zeroing, string copying, caller-buffer lifetime/SetPos and 800
  independent arenas across eight threads. Reservation counts return to zero.
- Android child processes verify uncommitted/decommitted page faults, discard
  zeroing, exact-end allocation and explicit failure paths. Expected signals
  concern only these probe children. The fixed-size OOM child reaches the
  diagnostic-stack fixture's explicit abort; this does not validate the engine's
  complete error/diagnostic handler.
- Both new probes pass with MSVC on **Win32 and x64**, using real VirtualAlloc/
  VirtualFree boundaries inside this Android worktree. Windows exact-end behavior
  is preserved, so the corrected exact-end test is Android-only.
- Existing direct-header/profiler/maths/RNG/lookup probes and Narrow32/483 guard
  comparisons pass on both host ABIs. APK assembleDebug/lintDebug pass; APK SHA256
  remains DFA4C3BD6C8FDDF121CF87EE9B5847367FB6C29CDB5EAD8FC164F3A31F1135B5.

The first device run caught reversed append_node arguments in the new test;
the fixture was corrected without changing the production list operation.
Windows probes required the root include path and data-only arrays for unused
network tables initialized by legacy headers. No networking service is provided.
Unavailable evaluator, heap, tracking, main-thread and nondefault allocator
functions are test-only aborting fixtures. These are absent from the APK.

## Evidence and reproduction

Frozen evidence is in [evidence/arm64_parsetree](evidence/arm64_parsetree/compiler_summary.json):
six compiler passes, successful host commands/results, earlier failure logs,
device runs, source hashes and 44 verified ELF64 little-endian AArch64 artifacts.
Final local build: build-android-audit/parsetree_pass6. Host directories:
host-parsetree-20261011-173541247, host-script-20261011-173654806 and
host-narrow-20261011-173654805 under build-android-audit.

From BlackOpsARM in PowerShell, choose a fresh output directory:

```powershell
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/my-parse-run
.\tools\port\check_parsetree.ps1
.\tools\port\check_script_headers.ps1 -PythonExe python
.\tools\port\check_narrow32.ps1 -PythonExe python
.\tools\port\run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 -BuildDirectory build-android-audit/my-parse-run
```

Requirements remain SDK D:/Android/Sdk, NDK 30.0.16248370, CMake/Ninja 3.22.1,
host Python 3, MSVC Build Tools and Windows SDK. Override VcVars/PythonExe on the
host scripts when needed. Probes go into a unique /data/local/tmp directory;
they install no APK. The separate packaging scaffold still contains no engine.

## Limits and next task

WARN-0006 records unresolved debugger operations: Scr_ClearDebugExpr and
Scr_FreeDebugExpr reinterpret native pointers into four-byte values;
debugger_prepend_node dereferences/arithmetic-transforms encoded addresses;
debugger_buffer's returned value needs review. These functions compile but were
not executed. Z_Malloc/Free, first-fit/fixed/debug schemes, tracking, evaluator,
full VM and engine thread registration remain outside this gate.

Narrow32 registration does not revoke stale handles when an arena is released.
All nodes/handles must remain within their owning arena's lifetime; tests never
decode released nodes. Region-table limits remain unchanged. Native page size
support is implemented, but actual **16 KB Android hardware/emulator and Samsung
S25 behavior remain untested**. Wider RNG replay, profiler synchronization and
graphics/audio/input work remain deferred.

Recommended next bounded task (TASK-0006): audit debugger handle and buffer
operations, establish genuine Z_Malloc/Free ownership and validate the required
first-fit/debug allocation boundary before exercising actual cscr_evaluate.cpp.
Keep Windows and four-byte ABI probes as references.

Windows main/its scratch files, independent DXVK-Native, root Ledger, all three
earlier evidence directories and APK are verified unchanged. Neither worktree's
HEAD/index was advanced; nothing was staged, committed or pushed. See the exact
[incremental file inventory](arm64_parsetree_changes.md).

Subsequent bounded gate: [arm64_debugger.md](arm64_debugger.md) records the
completed debugger/allocation work and WARN-0007. The evidence and limits above
remain the historical results of this parse-tree milestone.
