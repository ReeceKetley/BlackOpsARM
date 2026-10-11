# Phase 1 ARM64 compilation audit — 2026-10-11

Three existing T5 sources compile with Android NDK Clang into genuine ELF64
little-endian AArch64 objects: `src/universal/com_encode.cpp`,
`src/universal/com_math_anglevectors.cpp`, and `src/universal/timing.cpp`.
Selection reuses `UNIVERSAL_FILES` from `cmake_files.cmake`; an opt-in
`BO1_SOURCE_INVENTORY_ONLY` return avoids creating the Windows executable.
The root Windows CMake configuration is unchanged. No engine source tree is
copied, no gameplay function is rewritten, and no full engine is linked.

The Android APK still contains only the original environment JNI library.
The utilities and compatibility probes form a separate CMake project at
`android/cmake/arm64-audit/`. The existing setup work is preserved.

## Reproduce the compilation and probes

From `D:/Desktop/Black Ops 2 Android/BlackOpsARM` in PowerShell:

```powershell
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/audit-new
.\tools\port\check_narrow32.ps1 -PythonExe python
D:/Android/Sdk/platform-tools/adb.exe devices -l
.\tools\port\run_arm64_audit_tests.ps1 -Serial <selected-serial> -BuildDirectory build-android-audit/audit-new
```

Use a new directory each time; the builder refuses to overwrite diagnostic
logs. SDK root is configurable with `-SdkRoot`. Pinned configuration: NDK
30.0.16248370 (r30), Clang 21.0.0, CMake 3.22.1, Ninja, `arm64-v8a`, Android API
28, Debug, C++20. Host Python 3 generates probes. Engine flags include
`-Wall -Wextra -Wpedantic -Wcast-align -Werror=return-type -ferror-limit=0
-ffp-contract=off -fms-extensions -fsigned-char`; `_DEBUG=1` retains engine
assertions. No BO1_PORT_X64, Windows relaxation header, warning suppression,
global packing or fake `_MSC_VER` is used. Compatibility implementation files
compile with `-Werror`. Exact commands are archived in
`evidence/arm64_phase1/pass10_final.compile_commands.json`.

Windows host probes compile the unchanged Windows handle implementation from
this Android worktree, with MSVC `/W4 /WX`, once as x86 without the flag and
once as x64 with BO1_PORT_X64. They never build/write the Windows checkout.
The existing narrow-slot/ARM ABI probes are retained and extended with
production declaration extraction following the existing `check_*` test
convention. Extraction emits only ignored build-directory test code, with
source hashes; it does **not** prove that full script headers or the VM compile.
The Windows profiler transitive dependency is intentionally still visible as
the next compilation blocker.

APK recheck (JDK 21.0.1, Gradle 8.9, AGP 8.7.3, SDK/build tools 35):

```powershell
$env:JAVA_HOME = 'C:/Program Files/Java/jdk-21'
$env:ANDROID_HOME = 'D:/Android/Sdk'
Push-Location android
.\gradlew.bat --no-daemon :app:assembleDebug :app:lintDebug
Pop-Location
```

## Dependency inventory

The baseline scan covers 1,583 Git-tracked engine C/C++ files in `src`, `tl`
and `DemonWare`; 690 vendor files are excluded. Comments are removed, strings
and inactive branches retained. Counts overlap and are approximate lexical
scope, not numbers of compiler errors or proven bugs. “Unique root causes”
counts the named pattern groups found, a lower bound on semantic root causes.
Full roots, affected paths and representative lines are in
`evidence/arm64_phase1/dependency_inventory.json`.

| Category | Unique root causes | Affected files | Severity | Recommendation |
|---|---:|---:|---|---|
| Windows headers | 1 | 68 | Compile blocker | Small scoped headers for selected services; native Android backends for platform modules. |
| Windows API types/constants | 2 | 101 | Compile and ABI risk | Fixed-width Windows scalars, opaque ownership, no global conflict with DXVK types. |
| Memory allocation/virtual memory | 2 | 7 | Compile and runtime risk | Preserve reserve/commit/protect, alignment and allocation/free pairing; no allocation-success stubs. |
| Files/filesystem paths | 2 | 20 | Compile and runtime risk | Define handles, seek/IO errors, paths/case and async behavior before adapting. |
| Threads/synchronization | 3 | 141 | Compile and memory-model risk | Native lifecycle; specify event reset/wait semantics and replace volatile publication with atomics. |
| Timing/performance counters | 2 | 25 | Compile and clock-unit risk | Monotonic clock, matching frequency/counter units; remove CPU-cycle assumptions separately. |
| Thread-local storage | 1 | 21 | ABI/runtime risk | C++ TLS for state/errors; no raw Windows TEB offsets. No live TlsAlloc family match in this scan. |
| MSVC extensions/intrinsics | 3 | 1,072 | Compile blocker | Bounded spelling support demonstrated; map each intrinsic by semantics. |
| x86/x64 assembly/SIMD | 2 | 34 | Compile and runtime risk | Gate by instruction set; scalar oracle or explicit ARM implementation. |
| ABI/calling conventions | 3 | 1,356 | Compile and ABI risk | Native ARM calling convention; audit LP64 types, varargs and Windows-only gates. |
| Pointer/integer conversions | 2 | 380 | Runtime corruption risk | Native pointers versus fixed offsets/IDs; retain four-byte region handles. Cast matches require type review. |
| Packing/alignment | 3 | 417 | Compile and runtime risk | Fixed disk views plus explicit expansion; retain layout checks, review aliasing and alignment. |
| Direct3D 9/D3DX | 2 | 74 | Compile/backend blocker | Future separate DXVK Native/SDL2 backend; no integration in Phase 1. |
| Windows audio | 2 | 9 | Compile/backend blocker | Future native Android audio; no XAudio2 stubs or audio port here. |

Representative roots: `timing.cpp` includes Windows.h; `physicalmemory.cpp`
uses VirtualAlloc and Windows command-line APIs; `database/db_file_load.cpp` and
`qcommon/files.cpp` expose file/path assumptions; `qcommon/threads.cpp` and
`tl/jobqueue/jobqueue_all.*` expose waits, locks and Interlocked operations;
`universal/profile.h` uses intrin.h, __rdtsc and __readfsdword;
`clientscript/cscr_variable.h` and `cscr_debugger.h` contain four-byte unions;
`database/db_expand.cpp` defines explicit packed `_32` disk views;
`gfx_d3d/rb_resource.cpp` uses D3D interfaces; `sound/snd_driver_xaudio2.cpp` exposes
the XAudio2 backend. Refer to the JSON for verified exact source locations.

## Compiler findings and fixes

Full configure output, verbose diagnostics and compile commands from ten
passes are retained in `evidence/arm64_phase1/`. Earlier logs describe the
source/configuration at that pass, not a claim that current `-Raw` recreates
the original unmodified source. A separate transient configure syntax failure
was repaired and its local `build-android-audit/final/configure.log` retained.

| Pass | Errors | Warnings | Outcome/root cause |
|---|---:|---:|---|
| pass0_raw | 165 | 4 | Failed: MSVC integer spellings cascade; Windows.h and x86 intrin.h unavailable. |
| pass1_spelling | 5 | 51 | Failed: defining Clang builtin __debugbreak as a function plus the two headers. |
| pass2_foundation | 0 | 21 | Three real utility objects and two compatibility files compile. |
| final_validated | 1 | 25 | Added probe was too strict: Clang __int8 is char, not the signed-char type. |
| pass4_signed_char | 0 | 26 | Signed-width contract verified with -fsigned-char; separate narrow-slot probe still failed 8 == 4. |
| pass5_native_width | 0 | 26 | Native-width selector and Android atomic handle backend; original narrow-slot probe passes. |
| pass6_script_slots | 0 | 33 | Extracted production union/layout probes compile and link. |
| pass7_device / pass9_guard_matrix / pass10_final | 0 | 33 each | Device probes and complete guard matrix; final retained build. |

The final 33 warnings are 29 MSVC language-extension tokens (17 in engine
utilities, five in the ABI probe, seven in declaration probes), one GNU
anonymous struct and three array-bounds warnings in
`com_math_anglevectors.cpp:162–164`. Engine utility warnings remain 21.
The three accesses cross a three-float subarray via `(*axis)[3..5]`; this is a
real diagnostic and an aliasing/object-boundary risk, not an observed device
math/gameplay failure. No warning was disabled and no broad maths refactor made.

Small include changes isolate required dependencies: Android angle-vector
code includes `com_math.h` instead of transitive `q_shared.h`/Windows profiling;
timing includes the scoped time compatibility API on Android. Windows includes
and function bodies remain unchanged. `msvc_compat.h` supplies only native
__cdecl spelling and Clang's real debug trap, with explicit compiler/ABI guards.

## Legacy flag classification

Every baseline BO1_PORT_X64 reference was inventoried: **504 references in 98
files, including 483 conditional guards in 91 files**. See
`legacy_flag_references.json` for all uses, including CMake, historical docs and
Windows probes. `flag_guard_audit.json` records every guard's original file/line,
classification, original/current expression and branch-body SHA256.
The follow-up explicitly authorized this guard audit; it expands the initial
three-source compilation scope without claiming the newly selected modules build.

371 reviewed guard expressions change. This is a per-site migration, with
diagnostic/workaround exceptions inside the same database files:

| Classification | Guards | Action |
|---|---:|---|
| Native runtime layout | 95 | Native-width condition for pointer-bearing runtime structs, not fixed disk records. |
| Native-width expansion/sizing/access | 271 | Enable existing expansion, native allocation/command sizing and pointer access on ARM64. |
| Fixed-layout/count assertions | 54 | Keep checks active on Android; preserve legacy Windows assertion policy. |
| Bring-up diagnostics/workarounds | 43 | Retain legacy gates; do not import corruption-tolerance or debugger-UI policy as a pointer-width requirement. |
| x86 MMX fallback | 3 | Add ARM64 scalar selection; existing Windows conditions unchanged. |
| Windows platform/compiler/diagnostics | 12 | Retain Windows context, profiler, vendor and diagnostic behavior. |
| Windows handle implementation | 1 | Retain original source and locking unchanged. |
| Native-width handle ABI | 1 | Select Narrow32 on ARM64 by default. |
| Windows-host probes | 3 | Retain Windows tooling guards. |

`src/platform/bo1_native_width.h` derives `BO1_NATIVE_64BIT` from actual pointer
width outside Windows. Windows retains its existing BO1_PORT_X64 comparison
selection, so Win32 and flag-off experiments keep their previous branches.
Android AArch64 requires native-width layouts and rejects BO1_NARROW_HANDLES=0.
The negative compiler check confirms this explicit error (exit 1), retained in
`rejected_low_address_mode.log`.
The Windows-only `bo1_arch.hpp` CONTEXT/Rip/Rsp helpers are not repurposed for ARM.
Windows CMake, profiler, exception handling, vendor stubs and Steam paths are
untouched. Fixed checks include VM values, stack offsets, scalar maths, network
records, pointer-free SP extension fields and save headers. The mixed
pathnode/pathlink assertion has an additional Android check for the fixed link
portion. No fixed FastFile `_32` assertion is suppressed.

All 483 old/new guard selections compare equal when compiled for Win32 and
Windows x64 with the legacy flag both enabled and disabled. No full Windows
engine regression/gameplay run was
performed. Enabling the shared branches is not an ARM correctness certificate
for every expanded asset or allocator; those modules remain uncompiled here.
Existing low-address assumptions in q_shared partial-access helpers and Windows
diagnostics still need call-site review before wider compilation.

## Compatibility semantics and memory ordering

Only selected timing APIs and their error support are implemented:

- `QueryPerformanceFrequency`: BOOL success, fixed 1,000,000,000 Hz; a null
  output returns failure and sets ERROR_INVALID_PARAMETER. Valid calls preserve
  this thread's last-error value. No counter is implemented; a future counter
  must use matching monotonic nanoseconds. Existing CPU raw-timer integration
  remains unverified.
- `Sleep`: zero yields via sched_yield; finite sleeps use nanosleep and resume
  remaining time after EINTR; INFINITE does not return after signals. Unexpected
  POSIX failures abort explicitly because the API has no error return. Windows
  priorities, APC/alertable waits and exact tick scheduling are not emulated.
- `GetLastError`/`SetLastError`: full DWORD thread-local storage, initially zero.
  Definitions remain in `bo1::win32`; no global Windows/DXVK header replacement.
- BOOL/DWORD are four bytes, and the limited LARGE_INTEGER supplies only an
  eight-byte QuadPart with alignment eight, not the entire Windows union API.

Android narrow regions are immutable and never reclaimed during the process.
Registration is mutex-serialized; key publication is release, fast lookup is
acquire, and raw-slot decoding acquires the atomic base. The index is atomic,
published before the key. No volatile/x86-ordering assumption is carried into
the Android implementation. Existing Windows tables and SRWLOCK code are retained.
Null remains zero; handle bits remain region index 1..255 in the high byte and
24-bit region offset. Exhausting the process-wide 255-region table aborts
explicitly. Handles are process-local and are not serialized as persistent
pointers. Four-byte runtime slots and original asset/save formats remain distinct.

The Android SVAL_PTR_AT branch decodes both the slot address and its stored
handle; direct address casts cannot decode region-tagged handles. Its production
macro is exercised by the extracted declaration probe. Windows macro behavior
is retained pending a separately authorized Windows implementation review.

## ABI findings and validation limits

| Issue | Classification | Evidence/adaptation |
|---|---|---|
| MSVC spellings and unavailable OS/x86 headers | Compile-time incompatibility | Actual failing compiler passes; bounded extensions and two include boundaries resolve the selected sources. |
| Original Android narrow slot was eight bytes | Confirmed ABI incompatibility | Original probe failed 8 == 4. Native selector resolves it; no slot widening. |
| Android pointers/uintptr_t 8, int 4, long 8, wchar_t 4, long double 16 | Confirmed ABI differences | ARM ABI contract compiles; Windows long/wchar differ. Review exported data/varargs before wider compilation. |
| Plain char signedness / __int8 type | Compile-time contract and runtime risk | Default ARM char unsigned; -fsigned-char preserves Windows signed behavior, without asserting identical C++ type identity. |
| Volatile narrow publication | Potential runtime incompatibility | Android uses C++ acquire/release and mutex; passing stress is evidence, not a proof for all schedules. |
| Four-byte script/parse/layout invariants | Verified for probed declarations | VariableUnion/sval_u 4, VariableValue 8, VariableValueInternal 28, stack buf offset 13; full VM not compiled/executed. |
| Maths subarray bounds, union punning, signed overflow | Potential runtime incompatibility / investigate | Three real warnings; no confirmed gameplay/runtime failure or broad semantic fixes. |
| Other shared native-width branches | Requires further investigation | Guard selection audited; Windows API, LP64/vtable/packing and event/memory-model dependencies remain. |
| Win32 event semantics, virtual memory reserve/commit | Future compile/runtime blockers | No implementations or success stubs introduced. |

On the connected HONOR NDL-L09, Android API 36, arm64-v8a, all three final native
test executables exit zero. Evidence: `honor_device_tests.log`.

- Timing/error tests: frequency and explicit null failure; error preservation;
  eight-thread TLS isolation, 80,000 updates; yield, finite sleep, EINTR resume,
  infinite sleep remains blocked after a signal (supervised child only).
- Narrow tests: size/alignment 4; null, actual-address and fabricated >4 GiB
  round trips (fabricated pointers never dereferenced), raw four-byte copies;
  eight threads perform 160,000 round trips across 32 shared regions with stable
  handles; child table exhaustion terminates with SIGABRT as expected.
- Production declaration/macro tests: script sizes/offsets and pointer round
  trips; pointer-array slot traversal and FastFile zero/ordinary/-1/-2 widening.

Win32 and x64 MSVC probes pass the same four-byte declarations and applicable
round trips. Exhaustion is device-tested; Windows child-exhaustion behavior is
not claimed. The guard parity probes compile with zero host diagnostics.
No engine, renderer, gameplay, Samsung S25, or sanitizer certification is claimed.

`compiler_summary.json` records hashes/commands and verifies AArch64 ELF headers
for the three engine objects, supporting objects and executables (17 artifacts
including CMake's compiler-identification object). Headers are retained in
`aarch64_elf_headers.txt`. The APK assemble/lint recheck passes; lint has zero
errors/four existing warnings. APK/ELF/signature setup verification remains in
`.ledger/setup-verification.json`; the APK's sole JNI library is unchanged.
Its SHA256 remains
`DFA4C3BD6C8FDDF121CF87EE9B5847367FB6C29CDB5EAD8FC164F3A31F1135B5`.

Windows main at `76edfd5` and all 2,528 tracked/nonignored-untracked files,
including nine scratch files, plus its index/status match the starting snapshot.
DXVK-Native master at `0e271c6` has a separate object store and all 2,552 files,
index/status/HEAD unchanged. All 15 root Ledger files match. Both BlackOps64
worktrees share history and retain independent indexes/directories. No commit,
push, destructive Git command, backend integration or engine-source fork occurs.
See `arm64_phase1_changes.md` for the exact created/modified file inventory and
`evidence/arm64_phase1/preservation.json` for preservation checks.

## Recommended next task

Add a bounded **actual script-header** compilation gate: isolate the Windows/x86
profiler include behind an explicit native platform interface, keeping unsupported
operations unresolved rather than stubbing them. Compile cscr_variable/debugger
headers directly, validate four-byte VM/parse slots and fixed disk records, then
add one memory-independent VM utility. Use focused oracle tests to resolve the
three maths bounds warnings. Do not begin full engine, SDL2/DXVK, audio or input
integration yet. Keep Windows regression checks in this Android worktree and
record results in its Ledger; root history remains authoritative and untouched.

Semantic references: [Android ABI documentation](https://developer.android.com/ndk/guides/abis),
[Clang debug trap](https://clang.llvm.org/docs/LanguageExtensions.html),
[Windows Sleep](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep),
[QueryPerformanceFrequency](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency),
[MSVC scalar character types](https://learn.microsoft.com/en-us/cpp/cpp/char-wchar-t-char16-t-char32-t?view=msvc-170).

## Subsequent actual-header gate

The next bounded gate is complete. See [arm64_script_headers.md](arm64_script_headers.md)
for direct-header compilation, real profiler/instance utilities, maths oracle
checks and the remaining RNG blocker. The Phase 1 counts/evidence above describe
that historical snapshot and are preserved. Full VM/game/backend certification
remains deferred.
