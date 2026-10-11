# Windows-reference RNG and script lookup gate

Completed 2026-10-11 after the user's continuation. This bounded gate resolves
the first-party CRT RNG range/stream blocker and adds one actual script utility.
It does not start the engine, execute the VM or integrate Android backends.

## RNG contract and implementation

The installed Windows SDK UCRT source establishes a calling-thread state of
unsigned 32-bit width, default seed 1, wrapping update `state * 214013 + 2531011`,
and output `(state >> 16) & 32767`. Seeding overwrites only that thread's state.
Read-only source paths/hashes are recorded in `windows_crt_reference.json`:
`ucrt/stdlib/rand.cpp`, `internal/per_thread_data.cpp` and
`inc/corecrt_internal.h`, SDK 10.0.26100.0. The executable oracle compares against
the actual MSVC CRT, not another implementation of the formula.

`platform/engine_random.h` exposes explicit BO1_CrtRand/BO1_CrtSrand services.
Windows forwards to its existing CRT. Android's implementation uses thread_local
uint32_t state and unsigned wrapping arithmetic, so no shared atomic RNG state
or cross-thread draw ordering is introduced. Existing BO1_Random divides by
32768 and crandom scales by two/subtracts one; their arithmetic is unchanged.
Direct and normalized engine draws now advance the same per-thread stream.

All 48 first-party CRT callsâ€”44 draws and four seed callsâ€”in 23 T5 files route
through these services. `rng_call_migration.json` records every original line,
reason and before/after SHA256. Reversing only the call names and the added
include reproduces each original file byte-for-byte, including comments and
line endings. No gameplay algorithm, serialized layout or call order changes.
The wider caller modules have not been compiled as a complete engine target.

Libc symbols are not overridden and there is no rand macro. Third-party
libtommath and Speex remain byte-identical and keep libc behavior on Android.
Their source hashes/classification are recorded in `vendor_rng_boundary.json`.
Windows vendor draws can share CRT state with engine draws on a thread, whereas
Android's explicit first-party stream is separate. Assess actual vendor call
interleaving, seed/time/input schedules and thread assignment before claiming
gameplay replay equivalence. This gate certifies the same first-party per-thread
seed and draw sequence; it is not a full determinism certification.

## Actual script source and ABI

Unchanged `src/clientscript/cscr_readwrite.cpp` joins the existing CMake inventory
selection. All three functionsâ€”FindVariableIndexInternal,
FindVariableIndexInternal2 and FindLastSiblingâ€”compile and execute in the probes.
The fixture supplies the actual engine global/table types and aborts on engine
assertions; there are no allocator/VM success stubs.

Its 33 checks cover direct hits, collision-chain hits, absent keys, empty buckets,
last-sibling empty/nonempty cases and different hash/value record indices.
Runs cover both script instances with retail pool sizing and an expanded server
pool. Assertions retain VariableUnion four bytes, VariableValueInternal 28,
Variable eight and scrVarGlob_t 128. Earlier direct header, Narrow32 and fixed
FastFile sentinel probes remain enabled. This is fixture-backed lookup execution,
not full VM or malformed-table handling certification.

The target now compiles six unique real T5 units: com_encode.cpp,
com_math_anglevectors.cpp, timing.cpp, profile.cpp, cscr_instance.cpp and
cscr_readwrite.cpp. No engine source file is duplicated. BO1_Random/crandom
production bodies are extracted into ignored test code; **the full com_math.cpp
unit remains uncompiled**. Previous engine selections and source inventories
are reused, with no root engine restructure.

## Verification

| Check | Result |
|---|---|
| Actual CRT oracle, Win32 and x64 | 1.62 million stream comparisons per ABI; default/edge seeds and eight threads |
| Captured CRT fixture | Both Windows ABIs produce identical 12 seed streams of 128 draws |
| ARM64 engine RNG | Captured streams, default/reseed, 15-bit range, eight independent threads and parent-state preservation pass |
| Production normalized bodies | Captured streams, [0,1)/[-1,1) bounds, mixed raw/normalized draws and Windows legacy entry point pass |
| Unchanged script lookup source | 33 fixture checks pass on ARM64 and Win32/x64 |
| Existing native/host probes | Timing/TLS, profiling, maths, four-byte layouts and 483 guard parity checks pass |
| HONOR NDL-L09, API 36 | All nine native executables exit zero |
| NDK final build | Exit zero, 0 errors / 322 retained warnings, no array-bounds warnings |
| Scaffold APK | assembleDebug/lintDebug pass; SHA256 identical to setup |
| Preservation | Windows/DXVK/root Ledger and earlier evidence unchanged; no commit/push |

The actual CRT oracle tests 5,000 seeds with 64 draws each, five edge seeds with
100,000 draws each and eight threads with 100,000 draws each. Captured vectors
include zero, one, boundary and high-bit seeds and come from the CRT executable.
`tools/port/tests/windows_crt_rng_vectors.h` is a retained test-data fixture, not
engine source. check_engine_rng.ps1 regenerates local captures and rejects any
divergence from that fixture; it does not overwrite it.

The first build (`rng_script_pass1`) failed in Python generation on a pre-existing
Windows comment byte. It reported zero NDK compiler errors and 312 warnings but
was not a successful build. Byte-preserving Latin-1 extraction fixed the tool
without rewriting production encoding. `rng_script_pass2` succeeds with 322
warnings: 291 MSVC spelling extensions, 11 anonymous structs and four each for
nested anonymous types, legacy const declarations, logical parentheses, unused
parameters and dollar identifiers. Counts include repeated header compilations;
the larger gate is not directly comparable to prior totals. No warning suppression
or assertion removal was used. New native RNG code/probes use -Werror.

Full diagnostics/commands, CRT oracle and device/host logs, source/header hashes
and ELF headers are archived in `evidence/arm64_rng_script/`. Its manifest verifies
35 ELF64 little-endian AArch64 objects/executables, including CMake's compiler-ID
object. The APK remains an environment scaffold with SHA256
DFA4C3BD6C8FDDF121CF87EE9B5847367FB6C29CDB5EAD8FC164F3A31F1135B5.

## Reproduction

From BlackOpsARM, use fresh build outputs, MSVC Build Tools/Windows SDK and host
Python. The installed NDK is 30.0.16248370, Clang 21, CMake 3.22.1/Ninja under
D:/Android/Sdk. The target is API 28 arm64-v8a Debug C++20.

```powershell
.\tools\port\check_engine_rng.ps1
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/my-rng-gate
.\tools\port\check_script_headers.ps1 -PythonExe python
.\tools\port\check_narrow32.ps1 -PythonExe python
.\tools\port\run_arm64_audit_tests.ps1 -Serial DEVICE_SERIAL -BuildDirectory build-android-audit/my-rng-gate
```

Host scripts accept -VcVars to override the toolchain path. Run the device step
only for an authorized device. It pushes nine native probes into a fresh
/data/local/tmp/blackopsarm-phase1-* directory, without installing APKs, changing
device settings or deleting prior runs. No Samsung S25 results are claimed.

Archive a fresh run after capturing `apk-recheck.log` (Gradle assemble/lint from
android/) and `diff-check.log` (git diff --check) into its build directory:

```powershell
python tools/port/collect_script_header_evidence.py --build build-android-audit/my-rng-gate --host build-android-audit/host-script-TIMESTAMP --narrow build-android-audit/host-narrow-TIMESTAMP --rng-host build-android-audit/host-rng-TIMESTAMP --pass-prefix my-rng --output docs/port/evidence/my-fresh-rng-gate
```

Replace host directory names with script output. Archives refuse existing
destinations. This recorded run uses rng_script_pass2,
host-script-20261011-171115629, host-narrow-20261011-171215821 and
host-rng-20261011-170846140. The generator/backend are linked only to bounded
audit probes; the scaffold APK does not link the engine.

Preservation compares a fresh starting snapshot: Windows main at 76edfd5 with
2,528 files/index/status, separate DXVK master at 0e271c6 with 2,552 files, and
all 15 root Ledger files. Android remains android-arm64 at the same committed
base, with shared history and independent index/directory. All 54 Phase 1 and
61 prior header-gate evidence files are unchanged. Exact edits/local artifacts
are listed in `arm64_rng_script_changes.md` and `preservation.json`.

## Next bounded task

Compile actual cscr_parsetree.cpp to identify its concrete dependencies, then
establish the HunkUser allocation boundary needed for real node construction.
Verify alignment, ownership/lifetime, four-byte strides and handle roundtrips
before broad VM expansion. Full engine/game/backend integration remains deferred.
Ledger DEC-0010/0011 and FACT-0011/0012 record this gate; WARN-0005/TASK-0005
retain the remaining runtime/allocator boundary. Root Ledger is unchanged.

The recommended normal parse-tree/default HunkUser gate is now complete; see
[arm64_parsetree.md](arm64_parsetree.md). This report's evidence remains frozen.
