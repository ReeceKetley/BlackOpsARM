# Actual ARM64 script-header gate

Completed 2026-10-11 after the user's request to move on from Phase 1. This is
a bounded source/header milestone; Android engine startup and gameplay remain
deferred. Phase 1 evidence is preserved under `evidence/arm64_phase1/`.

## Result and source selection

The NDK r30 / Clang 21, API 28, arm64-v8a, Debug C++20 target now compiles five
unique actual T5 source files from the existing CMake inventory:

- `src/universal/com_encode.cpp`
- `src/universal/com_math_anglevectors.cpp`
- `src/universal/timing.cpp`
- `src/universal/profile.cpp` (new selection)
- `src/clientscript/cscr_instance.cpp` (new selection)

`cscr_variable.h` and `cscr_debugger.h` are included directly, with their real
dependencies and assertions enabled. This extends the earlier extracted
declaration probe. Tests assert VM/parse size and alignment four bytes,
VariableValue eight, VariableValueInternal 28, stack payload offset 13,
scr_block_s 536, scr_localVar_t/PrecacheEntry/RefString eight. Actual handle
traversal and FastFile -1/-2 sentinel widening pass. Fixed engine assertions
remain enabled; this does not certify every FastFile record or execute the VM.

The real `cscr_instance.cpp` utility is unchanged. The harness supplies its
engine-owned pointer slots and verifies both instances' reset/clear paths and
distinct backing buffers. No allocator/string-table/VM service is stubbed.

## Focused compatibility changes

`platform/profile_services.h` selects Windows or native Android services.
Windows retains TSC ticks, GetCurrentThreadId on x64, the original FS thread ID
on Win32, Interlocked arithmetic and SRW locking. Genuine Windows flag checks
move to the platform interface rather than becoming ARM64 architecture flags.
The Phase 1 per-site audit remains historical evidence, not a current source map.

Android implements CLOCK_MONOTONIC nanoseconds, gettid, real atomic counter
loads/adds, and mutex registration with release/acquire zone-list publication.
The shared `profile.cpp` supplies actual zone, first-use and server profiler
state. Its Android first-use deduplication/name collection uses a mutex and
bounded copying. Clock failures abort explicitly. Profiling is exercised while
enabled; no silent profiler replacement was used.

Enable flags and owner thread IDs must be established before workers start and
remain stable until they join. Live toggles/concurrent report resets are not yet
ported. Android readers must use `BO1_ProfileZones()` for acquire publication and
the 1,000,000,000 ticks/second native timebase; Windows reports currently assume
TSC. LP64 `long` telemetry fields are not a serialized ABI and are not certified
against unported report consumers. Tracy integration remains outside this gate.

The actual headers exposed a libc collision: T5's `double random()` conflicts
with Android's POSIX `long random()`. The engine symbol is now `BO1_Random` and
its zero-argument callers use that name. Windows retains the original entry
point as a forwarding function. The RNG calculation and call order are unchanged;
there is no macro alias that could corrupt libc declarations. Caller changes are
name-only and do not compile/port the wider gameplay modules.

`AxisToSignedAngles` now reads the second matrix row using `axis[1][0..2]`,
instead of indexing past the first three-element subarray. Its other arithmetic
is unchanged. The host oracle extracts only the original function at 76edfd5
into an ignored test include. All 5,845 cases are bit-identical on Win32/x64.
The actual source passes angle/axis roundtrips on ARM64 at 0.002 degree tolerance.
Gimbal-lock singularities are outside the tested pitch range of -89..89 degrees.

MSVC resolves the unused AxisToQuat dependency before dead-code elimination.
The host axis harness has an explicit aborting definition for accidental use;
it is test-only, is never linked into the engine/APK and does not claim quaternion
support. NDK removes the unused function section. The assertion harness also
aborts on failures. Neither replaces tested behavior with a silent success.

## Compiler diagnostics and evidence

Full logs/commands/hashes are in `evidence/arm64_script_headers/`.

| Pass | Errors | Warnings | Meaning |
|---|---:|---:|---|
| script_headers_pass1 | 3 | 169 | Two libc collisions; missing test initializer_list include |
| script_headers_pass2 | 3 | 169 | Same diagnostics; incomplete edit script stopped on file encoding |
| script_headers_pass3 | 0 | 169 | Actual headers/utility compile; three maths bounds warnings retained |
| script_headers_pass4 | 0 | 176 | Maths row correction and active profiler/maths tests |
| script_headers_final | 0 | 186 | Added production RNG-body probe |
| script_headers_validated | 0 | 186 | Final harness compatibility verified |

The final larger target retains 169 MSVC spelling extension warnings, seven
anonymous-struct warnings and two each for nested anonymous types, legacy const
type declarations, logical parentheses, unused parameters and dollar identifiers.
It has zero array-bounds warnings. New native profiler code uses -Werror; the
maths target makes array-bounds warnings errors. No global warning suppression
or assertion removal was used. Counts cover all objects/tests, including repeated
header inclusions, and are not comparable to the smaller Phase 1 target's total.

`compiler_summary.json` and `aarch64_elf_headers.txt` verify 29 ELF64 little-endian
AArch64 objects/executables, including the CMake compiler-ID object. Source/header
hashes and exact compile commands are retained. The RNG probe extracts current
production function bodies into ignored test code; **all of com_math.cpp has
not been compiled** by this target.

## Validation and reproduction

Seven probes pass on the authorized HONOR NDL-L09, API 36: existing timing/TLS,
Narrow32 and extracted layout tests plus actual headers/instance, real profiler,
maths and RNG-body tests. Profiling verifies nested main scopes, exclusive server
and builtin scopes, thread/depth filtering, 80,000 atomic adds, 10,000 deduplicated
keys, concurrent registration and bounded names. All four new host probes pass
on MSVC Win32 and x64, and existing narrow/layout/483 guard parity checks pass.

Run from BlackOpsARM; choose a fresh output directory and an authorized device:

```powershell
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/my-script-gate
.\tools\port\check_script_headers.ps1 -PythonExe python
.\tools\port\check_narrow32.ps1 -PythonExe python
.\tools\port\run_arm64_audit_tests.ps1 -Serial DEVICE_SERIAL -BuildDirectory build-android-audit/my-script-gate
```

Use `-VcVars` to override the MSVC Build Tools path. Installed tools here are
NDK 30.0.16248370, CMake 3.22.1/Ninja and the SDK at D:/Android/Sdk. The optional
BO1_ARM64_SCRIPT_HEADERS CMake switch defaults ON; OFF retains the original
utility/probe target. No engine code enters the scaffold APK.

To archive a fresh run, substitute the host/narrow directories printed by their
scripts and select a fresh evidence output path:

```powershell
python tools/port/collect_script_header_evidence.py --build build-android-audit/my-script-gate --host build-android-audit/host-script-TIMESTAMP --narrow build-android-audit/host-narrow-TIMESTAMP --output docs/port/evidence/my-fresh-gate
```

This collector requires device-tests.log, apk-recheck.log and diff-check.log in
the final build directory. Produce the latter with `git diff --check` and run
`android/gradlew.bat :app:assembleDebug :app:lintDebug` from android/, capturing
output there. Archives refuse existing destinations. The recorded run is
script_headers_validated with host-script-20261011-165155884 and
host-narrow-20261011-165123884. Devices receive only native executables in a fresh
/data/local/tmp/blackopsarm-phase1-* directory, with no APK installation/settings.

Scaffold assemble/lint passes; its APK SHA256 remains
DFA4C3BD6C8FDDF121CF87EE9B5847367FB6C29CDB5EAD8FC164F3A31F1135B5.
Windows main and its 2,528 files/index/status, DXVK's independent repository and
2,552 files, and all 15 root Ledger files match the fresh starting snapshot.
Android remains android-arm64 at 76edfd5 with independent index/directory and
shared history. Frozen Phase 1 evidence matches the snapshot. No commit/push.
See `preservation.json` and `arm64_script_header_changes.md` for exact inventories.

## Next bounded milestone

The RNG-body probe confirms RAND_MAX=32767 on Windows versus 2147483647 on
Android. The unchanged rand()/32768 normalization and libc sequence therefore
need a Windows-reference range/seeded-stream contract before gameplay. Define
and test a portable 15-bit generator or justified compatibility service; do not
silently change every random call's distribution by using Android libc.
Then inspect and compile one small real script evaluator/variable utility.
Full VM, engine startup, renderer/DXVK, audio, input and S25 testing remain later.
Ledger DEC-0008/0009 and FACT-0008/0009 record this gate; WARN-0004 and TASK-0004
record the remaining boundary and next work. Parent Ledger remains authoritative.

## Subsequent RNG/script lookup milestone

The first-party RNG range/stream gate is now complete; see
[arm64_rng_script.md](arm64_rng_script.md). The counts and evidence above describe
this historical header gate. Vendor interleaving and broader runtime behavior
remain unvalidated; the next bounded task is parse-tree allocation.
