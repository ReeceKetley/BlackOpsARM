# ARM64 actual zone bootstrap — TASK-0009

Completed 2026-10-11. The complete existing `db_memory.cpp` and `db_stream.cpp`
compile alongside the ten previously selected T5 units. The new standalone probe
executes actual `DB_AllocXBlocks`, `DB_InitStreams`, stream positioning and block
cleanup against a logical decompressed fixture. No assets are parsed or packaged.

Final NDK r30 / Clang 21 / API28 / arm64-v8a / Debug C++20 build: zero compiler
errors, zero linker failures, 1529 retained warnings. All 65 object/executable
artifacts checked are ELF64 little-endian AArch64. All fifteen native probes pass
on HONOR NDL-L09, API36, serial `AKWJ9X5116G04618`, native page size 4096. Actual
database objects, new services and probe bodies use unrecoverable UBSan; passing
paths have no findings. This does not certify every earlier native library.

MSVC x86 and x64 zone probes pass. Fresh debugger/first-fit, parse-tree/HunkUser,
direct-header/profile/maths/RNG/lookup, FastFile and Narrow32/four-byte-slot/483
guard regressions also pass from this Android worktree. Windows checkout, DXVK,
root Ledger, Android index/HEAD and all 451 earlier evidence files remain intact.
The existing scaffold APK hash is unchanged; assemble/lint was not rerun here.

## Implementation and ownership

`db_zone_types.h` extracts the existing XFile/XBlock declarations once, reused by
the original database headers. Active assertions retain a 36-byte XFile, 8-byte
block-size offset and four-byte unsigned disk fields; XBlock carries a native
pointer. Android includes avoid Windows loader dependencies. The actual database
allocation bodies match HEAD after CRLF normalization. No engine implementation is copied.

`platform/android/zone_memory.*` owns named zones with the real debug HunkUser and
first-fit allocator. Register the native main thread first. The required lifecycle
is Begin(name,type,capacity), actual DB allocations, End, detach stream globals,
DB_FreeXBlocks, then Release(name,type). DB_FreeXBlocks with ordinary flags clears
metadata; Release destroys the owning pool. Names are owned; allocations are
zeroed, aligned and counted. Only allocation types 0/1, TRACK_FASTFILE, reviewed
memory flags 4/-2147482620 and power-of-two alignments up to 4096 are accepted.
Unsupported requests, wrong-thread ownership and exhaustion fail explicitly.

This adapter is a bounded named owner. It does not implement legacy PMem low/high
pool ordering, tracking, parallel zone mutation or the complete PMem API. Windows
production PMem is unchanged. Host zone probes explicitly use the native owner
with actual Windows virtual memory/thread services. Android database diagnostics
use real stderr/optional file output and `_Unwind_Backtrace`; Windows diagnostics
retain their existing implementation.

`DB_InitStreams` previously cleared the pending Win32 expansion size but retained
the preceding zone's element stride. A stronger Windows x64 test failed when that
stride incorrectly mapped offset 4 in a subsequent non-element expansion. The
native-width path now also clears `g_expandElemW` and `g_expandElemN`. The retained
old control fails; repaired x64 and ARM64 pass. Win32 behavior is unchanged.

## Covered paths and limits

The probe checks all seven blocks/memory types, unchanged fixture fields, zeroed
payloads, 4096 alignment, real HONOR pointers above 4 GB, retained bytes, exact
request accounting, zero/sparse inputs, allocation type 1 and native expansion
factor boundaries at 1 MB/32 MB. It checks push/pop, cursor advancement, pointer
insertion, alias/shadow reset and detachment before release. Alternate Z allocation
is checked with physical blocks zero; mixed Z/physical flags remain unsupported.

Eight fresh-process Android negatives verify end/release/allocation outside their
ownership scope, invalid types/alignment, open-zone release, genuine first-fit
exhaustion and wrong-thread ownership. Fork-only controls initially inherited the
parent's cached thread identity; fork+exec establishes each child's actual main
thread and reaches the intended failure. Virtual reservation counts return to
baseline, and zone/heap ownership counts return to zero.

The fixture is a logical 48-byte preamble, not raw bytes after a retail file's
12-byte header. Genuine files remain compressed there. Malformed/oversized XFile
expansion arithmetic, full stream remapping/asset expansion, async/APC loading,
compressed-byte accounting and full error handlers remain unvalidated. Missing
PMem over-allocation and DB_XFileBytesLoaded services abort in test-only fixtures;
stream tracing that needs the latter must remain disabled. Fixtures never enter
the APK. Prior string/callable/operator evaluation/VM limits still apply. Actual
16 KB Android hardware and Samsung S25 remain untested; no game/backend claim.

## Retained evidence and reproduction

`evidence/arm64_zone/compiler_summary.json` contains source SHA256s, all three
compiler attempts, warning classes, artifact hashes and archived log hashes.
`source_contract.json` records reuse/layout checks. The archive retains the
missing-ARRAY_COUNT build failure, initial disconnected ADB attempt, reconnected
run, failing Windows stride control and final host/device regressions. The failed
Windows control opened a CRT abort dialog; only its owned zone.exe PID42520 was
stopped to complete that failure. No scratch files were deleted.
`retained_local_artifacts.json` locates ignored binaries/builds and hashes.
`preservation.json` and `arm64_zone_changes.md` record exact preservation and files.
Earlier evidence and the rotation handoff remain historical, frozen references.

Run in PowerShell in `D:\Desktop\Black Ops 2 Android\BlackOpsARM`:

```powershell
$taskPython = 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH = (Split-Path $taskPython) + ';C:/Program Files (x86)/Microsoft Visual Studio/Installer;' + $env:PATH
& 'D:/Android/Sdk/platform-tools/adb.exe' devices -l
# Confirm AKWJ9X5116G04618 is listed as device, then use a fresh output directory.
$taskBuild = 'build-android-audit/zone-repro-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff')
.\tools\port\build_arm64_audit.ps1 -BuildDirectory $taskBuild
if ($LASTEXITCODE) { throw 'ARM64 build failed' }
.\tools\port\run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 -BuildDirectory $taskBuild
if ($LASTEXITCODE) { throw 'HONOR tests failed' }
.\tools\port\check_zone.ps1
.\tools\port\check_debugger.ps1 -PythonExe $taskPython
.\tools\port\check_parsetree.ps1
.\tools\port\check_script_headers.ps1 -PythonExe $taskPython
.\tools\port\check_narrow32.ps1 -PythonExe $taskPython
.\tools\port\check_fastfile.ps1
```

The device runner pushes fifteen executables into a unique
`/data/local/tmp/blackopsarm-phase1-*` directory and records every exit code in
`device-tests.log`. It installs no APK. Exact latest successful output directories
are in compiler_summary.json; combined regression logs are archived alongside it.

## Next task and acceptance criteria

TASK-0010: isolate bounded zlib deflate decoding in production stream order after
the verified 12-byte raw header. Inspect existing zlib/loader code first; retain
Windows compatibility and reject malformed/oversized inputs explicitly. Decode
the real 36-byte XFile from a decompressed stream before claiming genuine zone
bootstrap. Keep asset parsing, async loading and full engine startup deferred.

Acceptance: deterministic fixture decode and explicit truncated/corrupt/size-limit
failures on ARM64 and Windows x86/x64; genuine retail decode with file hashes and
recorded decoded header sizes if assets are available; unchanged fixed disk ABI;
all fifteen current HONOR probes and host regressions passing; preserved failure
evidence, protected checkouts and updated Ledger. New work requires user direction;
no commit or push is authorized by this completed gate.
