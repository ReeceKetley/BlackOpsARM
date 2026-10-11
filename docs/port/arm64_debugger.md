# ARM64 debugger handles and allocation gate

Completed 2026-10-11 under TASK-0006 / DEC-0014 / DEC-0015. Ten actual T5
translation units now compile to AArch64, including the complete
cscr_evaluate.cpp and mem_firstfit.cpp. Selected debugger cleanup and numeric
evaluation paths execute on the HONOR and Windows x86/x64. This gate establishes
an allocation/handle boundary; it does not certify the full expression compiler
or VM. Exact incremental files: [arm64_debugger_changes.md](arm64_debugger_changes.md).

## Source changes

- Scr_ClearDebugExpr/Scr_FreeDebugExpr encode a native node pointer through
  sval_u.node before passing it to the evaluator. They no longer reinterpret
  the low four bytes of a native pointer as a handle.
- debugger_prepend_node decodes handles before indexing nodes and re-encodes the
  resulting link pointer. Arithmetic on the encoded address is removed.
- debugger_buffer returns the allocated node handle rather than its token slot.
  Copies, zero-length buffers, bounded sizes and power-of-two alignment are
  checked; debugger_string retains its original bytes and terminator.
- Evaluator function bodies and normal parse constructor bodies are unchanged.
  Android narrows unrelated networking/UI/SSE/D3D9 header dependencies. The
  existing animation-debug prototype is extracted into xanim_debug.h; Windows
  keeps its original broad includes. Lexer FILE declarations use portable FILE*.
- Actual first-fit policy is reused. BO1_NATIVE_64BIT aligns payload
  back-pointers and split headers, including after odd-sized allocations.
  Win32 placement and the existing reserved-tail/exhaustion policy are retained.
  Native-width free operations validate bounds/alignment before dereferencing.
- Android Z_Malloc/Z_Free use a real zeroed, naturally aligned allocation with
  synchronized ownership/type/name/byte metadata. Invalid frees and invalid
  requests abort explicitly. This implements these two entry points, not the
  entire engine Z allocator/tracker API.
- Android first-fit operations use a recursive mutex. Debug HunkUser uses a real
  registered gettid identity; call RegisterHunkMainThread on the application
  main thread before debug allocator startup. Registration binds once and rejects
  a different thread. Default/debug reserve and caller-buffer default/first-fit
  configurations are supported; tracking/fixed schemes remain rejected.

No engine source is duplicated, disk/FastFile layouts are changed or layout
assertions disabled. Android uses the native-width policy by default; the
legacy BO1_PORT_X64 flag is absent from Android commands. Windows engine Z heap,
thread services and critical-section implementation remain unchanged.

## Verification

Final NDK r30 / Clang 21 / API 28 / arm64-v8a / Debug C++20 build:
**0 compiler errors, 1261 retained warnings**, successful link, and **55 ELF64
little-endian AArch64 artifacts** checked. Existing decompiler/MSVC-extension
warnings and unused generated fixture parameters remain visible. Initial
dependency/fixture failures are retained, including pass8's linker failure
despite zero compiler errors.

All **13 native probes pass** on the authorized HONOR NDL-L09, API36, native
page size 4096, serial AKWJ9X5116G04618. The two new probes exercise:

- Actual debugger node0..4, raw four-byte slots, typed prepend links, aligned
  buffer/string copies and both instance cleanup traversals. Clear/free/ref
  operations are observed through explicitly test-owned value/reference state.
- Actual evaluator initialization, canonical comparison, script-expression
  wrapper and integer/float/negative/boolean literals for both instances, with
  real allocation cleanup. The variable manager is not ported by these fixtures.
- Actual first-fit/debug HunkUser: 400 odd payloads, requested alignment 1..4096,
  coalescing, reserved-tail exhaustion, reset/destroy and caller ownership.
  Eight threads perform 8000 shared-pool allocate/free operations. A separate
  eight-thread test performs 8000 real zeroed Z heap allocations.
- Isolated Android child processes verify wrong-type/interior/double frees,
  invalid sizes/alignment and main-thread ownership failures abort explicitly.
  Allocation count and byte count return to zero after successful paths.

Actual parsetree, HunkUser, evaluator and first-fit object files plus the new
probe bodies use UndefinedBehaviorSanitizer with recovery disabled. Passing
paths report no sanitizer finding. Native service libraries and the whole
engine are not claimed as comprehensively sanitized.

The two new probes pass on MSVC **x86 and x64**. Parse/default HunkUser, six
header/profiler/maths/RNG/lookup probes, Narrow32 and 483 reviewed flag guards
also pass on their existing host configurations. All host builds occurred in
the Android worktree. Debugger host probes deliberately link the Android Z heap
under BO1_ZONE_HEAP_HOST_TEST; first-fit host probes use real Windows virtual
memory, thread identity and critical sections.

APK assembleDebug/lintDebug pass. The scaffold APK remains byte-identical:
DFA4C3BD6C8FDDF121CF87EE9B5847367FB6C29CDB5EAD8FC164F3A31F1135B5.
No engine/test fixtures are packaged and no APK was installed.

## Runtime boundary and next task

Fifty unavailable compiler/VM services are generated from current production
declarations for standalone probes. Every endpoint prints its name and aborts;
none is reached by passing tests. Test-owned globals and the three cleanup
reference/value callbacks are fixtures, not a substitute implementation of
the variable manager. These files must never be linked into an engine/APK.

Normal-to-debugger expression compilation, text parsing, string interning,
callable-slot registry/list traversal, nonliteral operators/variables and full
VM execution remain unvalidated. The production Windows Z heap has a legacy
164-byte prefix: it does not establish alignment for native debugger prefixes.
Testing the Android heap on Windows does not certify that Windows heap/debugger
integration. Preserve that boundary when widening compilation coverage.

Default HunkUser and the global debugger compile list are owner-thread
confined. First-fit reset/destroy require quiescent users. Narrow handles must
not outlive their owning allocations; registration does not revoke stale
handles. Tracking/fixed schemes and complete engine diagnostics remain
unsupported. An older OOM probe still reaches an explicit test diagnostic
abort, not a complete engine error handler. Real 16 KB Android hardware,
Samsung S25, replay/profiler reporting and graphics/audio/input remain untested.

TASK-0007: exercise actual normal-to-debugger expression compilation, starting
with numeric/boolean expressions. Audit list/string/callable ownership before
implementing only the services justified by that boundary. Full VM, gameplay
and backend integration remain deferred. WARN-0007 supersedes WARN-0006.

## Evidence and reproduction

Frozen current evidence: [compiler summary](evidence/arm64_debugger/compiler_summary.json),
[source contract](evidence/arm64_debugger/source_contract.json) and
[preservation](evidence/arm64_debugger/preservation.json). Ten compiler passes,
host commands/results, generated abort endpoints, earlier failed host logs and
device runs are retained. Earlier four evidence directories remain byte-identical.

Final local build: build-android-audit/debugger_pass10. Final host directories:
host-debugger-20261011-180837755, host-parsetree-20261011-180910718,
host-script-20261011-180837767 and host-narrow-20261011-180908630.

From BlackOpsARM in PowerShell, use a fresh build directory each time:

```powershell
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/my-debugger-run
.\tools\port\check_debugger.ps1 -PythonExe python
.\tools\port\check_parsetree.ps1
.\tools\port\check_script_headers.ps1 -PythonExe python
.\tools\port\check_narrow32.ps1 -PythonExe python
.\tools\port\run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 -BuildDirectory build-android-audit/my-debugger-run
```

Requires SDK D:/Android/Sdk, NDK 30.0.16248370, CMake/Ninja 3.22.1, host Python3,
MSVC Build Tools and Windows SDK. Override VcVars/PythonExe as needed. This
workstation's Python is
C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe.
Generated declarations are refreshed by fresh builds. Device probes use a
unique /data/local/tmp/blackopsarm-phase1-* directory and install no APK.

Both worktrees retain their original HEADs and indexes. Windows files/scratch
files, independent DXVK-Native and root Ledger are hash-verified unchanged.
Nothing was staged, committed or pushed.
