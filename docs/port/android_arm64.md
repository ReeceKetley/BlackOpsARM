# BlackOpsARM Android ARM64 development

## Scope and source baseline

This is an Android scaffold with a bounded Phase 1 engine compilation audit.
It is not a running engine port. `android-arm64`
starts at `76edfd5e75976c41bfab5a5db262b8b593432518` (2026-10-11, native
water/material fixes and Windows Vulkan validation). `../BlackOps64` stays on
`main`. Both worktrees share Git history/object storage but have independent
working directories, HEADs and indexes. No commit or push was made for setup.

`BlackOpsARM` was not empty: it contained a newly initialized `.ledger` only.
To preserve it, setup created an adjacent worktree with `--no-checkout`, moved
only the newly created `.git` pointer into this folder, ran `git worktree repair`,
populated its independent index with `git read-tree HEAD`, and materialized
tracked files with non-forcing `git checkout-index --all`. No existing Ledger
file was moved, deleted or replaced by checkout.

## Structure

```text
BlackOpsARM/
  AGENTS.md
  .ledger/                         Android memory, linked to ../.ledger
  android/                         Gradle wrapper and APK packaging
    app/src/main/                  Manifest and development Activity
  src/platform/
    android/                       NDK environment target and atomic narrow handles
    win32_compat/                   Minimal timing and thread-local error shims
    windows/                       Existing Windows adapters, unchanged
  docs/port/android_arm64.md
  CMakeLists.txt                    Existing Windows engine build, unchanged
  cmake_files.cmake                Shared inventory; opt-in inventory-only mode
  src/, tl/, DemonWare/            Shared sources; reviewed native-width guards
```

The development APK loads `libblackopsarm_environment.so`, whose only purpose
is to prove Android/aarch64, 64-bit pointers, C++20 and JNI packaging. Its screen
explicitly says the engine and renderer are not connected. No assets, SDL, DXVK
binary, Windows shims or gameplay are packaged in this milestone.

## Toolchain and build

Pinned project versions: Gradle 8.9, Android Gradle Plugin 8.7.3, compile/target
SDK 35, build tools 35.0.0, minimum API 28, NDK `30.0.16248370`, CMake 3.22.1,
and ABI `arm64-v8a` only. Use a JDK compatible with Gradle 8.9 and AGP (minimum
JDK 17); this machine has JDK 21.0.1. Java sources target Java 17.
The AGP requirement is documented in the
[official 8.7 release notes](https://developer.android.com/build/releases/agp-8-7-0-release-notes).
The official wrapper JAR was verified against Gradle's published checksum;
`distributionSha256Sum` pins the downloaded Gradle distribution. See
[Gradle wrapper verification](https://docs.gradle.org/8.9/userguide/gradle_wrapper.html).

On this workstation, the SDK is `D:/Android/Sdk`; `android/local.properties`
records that machine-local path and is ignored by Git. For another workstation,
set `ANDROID_HOME` or create its own `local.properties` with `sdk.dir=<SDK path>`.
Install the pinned SDK packages using Android SDK Manager. Equivalent commands
in PowerShell on this machine are:

```powershell
$env:JAVA_HOME = 'C:\Program Files\Java\jdk-21'
$env:ANDROID_HOME = 'D:\Android\Sdk'
& "$env:ANDROID_HOME\cmdline-tools\latest\bin\sdkmanager.bat" `
  'platforms;android-35' 'build-tools;35.0.0' `
  'ndk;30.0.16248370' 'cmake;3.22.1' 'platform-tools'

Set-Location 'D:\Desktop\Black Ops 2 Android\BlackOpsARM\android'
.\gradlew.bat --no-daemon :app:assembleDebug :app:lintDebug
```

On POSIX hosts use `sh ./gradlew --no-daemon :app:assembleDebug :app:lintDebug`
from `android/`, with the same SDK components and a compatible JDK. Build
outputs are ignored. The debug APK is
`android/app/build/outputs/apk/debug/app-debug.apk`; lint reports are under
`android/app/build/reports/`. Debug signing is for local development only.

Setup verification on 2026-10-11: debug build and lint passed; lint reported
0 errors and 4 warnings (ARM64-only ChromeOS support, backup rules, development
icon and localization). APK signing and the packaged ELF64/AArch64 library/JNI
export were verified. The pinned AGP emitted a non-fatal SDK XML-version warning
with the newer installed command-line tools. No device install/launch was run.

Optional device smoke check, after selecting the intended attached device:

```powershell
& "$env:ANDROID_HOME\platform-tools\adb.exe" devices -l
# Replace DEVICE_SERIAL with the selected device; this installs the dev APK.
& "$env:ANDROID_HOME\platform-tools\adb.exe" -s DEVICE_SERIAL install -r `
  '.\app\build\outputs\apk\debug\app-debug.apk'
& "$env:ANDROID_HOME\platform-tools\adb.exe" -s DEVICE_SERIAL shell am start `
  -n 'com.blackopsarm.dev/.DevelopmentActivity'
```

Expect the ARM64 environment text with no JNI load failure in logcat. Device
installation/launch is a separate check, not part of the build-only setup claim.

## Renderer and platform direction

The goal is the original native T5 Zombies engine on HONOR Pad X8a and Samsung
Galaxy S25 with Xbox controller-first input. Keep original game assets and
FastFile formats. Shared engine changes must remain Windows compatible.

Initial renderer target: DXVK Native 1.9.2 using the tested SDL2/Vulkan
integration in `../DXVK-Native`. That repository stays independent; neither
its source nor binaries are copied or changed by this scaffold. Its
`ANDROID-PORT-NOTES.md`, `android/DXVK-ANDROID-PROFILE.md` and baseline APK provide
renderer evidence, not proof that T5 already runs. Record external revision
`0e271c6be16ded1a8638d117360c2e65d9df4603` when consuming future artifacts.
Confirm actual device GPU/Vulkan properties directly; older root Mali/SDL3
assumptions must not override the measured SDL2 integration.

Use `src/platform/android/` for graphics/WSI, audio, lifecycle and controller
input. Use `src/platform/win32_compat/` only for small OS services where shims
avoid invasive shared changes. Unsupported functions must fail explicitly.
Use native-width pointers, preserve 32-bit disk/serialization layouts, and
implement synchronization with ARM64-correct atomics and memory ordering.

## Reusing engine CMake and first compilation task

The root `CMakeLists.txt` currently assumes MSVC, Windows libraries and executable
packaging. `cmake_files.cmake` contains source groups and an `add_executable`, not
a platform-neutral list-only module. Directly adding the root project to the
NDK build today would pull in Windows flags, headers and libraries. The scaffold
therefore uses the isolated environment target without copying engine sources.

Phase 1 now reuses that inventory through `BO1_SOURCE_INVENTORY_ONLY`. The root
Windows executable remains the default. The separate target compiles genuine
`com_encode.cpp`, `com_math_anglevectors.cpp`, and `timing.cpp` to AArch64 objects.
See `arm64_compilation_audit.md` for the dependency/flag audit and full evidence.

From this worktree in PowerShell, use a fresh output directory on each run:

```powershell
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/my-run
.\tools\port\check_narrow32.ps1 -PythonExe python
.\tools\port\run_arm64_audit_tests.ps1 -Serial <adb-serial> -BuildDirectory build-android-audit/my-run
```

The host probes require MSVC Build Tools (`-VcVars` overrides its path), Python
3 and Windows SDK. The NDK compile target also needs host Python 3 for extracting
current production declaration probes. The Phase 1 device runner pushed three native
test executables into a unique `/data/local/tmp/blackopsarm-phase1-*` directory;
it installs no APK. Inspect `adb devices -l` and select the intended device.
These are ABI/compatibility tests; engine startup, renderer, audio, input and
gameplay have not been validated on Android.

The next actual-header gate is now complete: profile.cpp and cscr_instance.cpp
also compile, with direct cscr_variable/debugger headers and four-byte checks.
See [arm64_script_headers.md](arm64_script_headers.md) for evidence and limits.
The device runner now executes seven native probes when this gate is enabled.
Run the Windows checks from this Android worktree:

```powershell
.\tools\port\check_script_headers.ps1 -PythonExe python
```

Use a fresh build directory for every retained build/device run. The CMake
option BO1_ARM64_SCRIPT_HEADERS defaults ON with compatibility enabled; OFF
selects the original bounded utility/probe suite. The APK still has no engine.
The Windows-reference RNG and unchanged cscr_readwrite.cpp lookup gate are now
complete; see [arm64_rng_script.md](arm64_rng_script.md). The device runner runs
nine probes, including captured CRT streams and real script lookup fixtures.
Use tools/port/check_engine_rng.ps1 for actual Win32/x64 CRT comparisons.
The actual parse-tree/default HunkUser gate is now complete: eight actual T5
units compile, and the device runner executes eleven probes. See
[arm64_parsetree.md](arm64_parsetree.md) for source/evidence/limits. Run
`.\tools\port\check_parsetree.ps1` in this Android worktree for Win32/x64
constructor/allocator checks with real Windows virtual-memory services.
The debugger handle/allocation gate is now complete: ten actual T5 units
compile, including the entire evaluator and first-fit source, and thirteen
HONOR native probes pass. See [arm64_debugger.md](arm64_debugger.md) for typed
handle/buffer fixes, real native heap/thread services, selected evaluator paths
and test fixture limits. Run `.\tools\port\check_debugger.ps1 -PythonExe python`
for both Windows host ABIs. These debugger probes use the Android heap; the
production Windows Z heap's alignment remains unvalidated.
Subsequent TASK-0007 actual normal-to-debugger expression compilation and
TASK-0008 FastFile raw-header/I/O gates are complete; see arm64_expr_compile.md
and arm64_fastfile.md. Genuine retail files have been surveyed/verified, but the
XFile follows the raw 12-byte header inside compressed data.
TASK-0009 actual DB allocation/stream initialization now passes: twelve actual
T5 units and fifteen HONOR probes. See arm64_zone.md for exact current device
and MSVC x86/x64 commands, native named ownership, stale-stride reset fix,
retained failures and limits. Run tools/port/check_zone.ps1 for both host ABIs.
The APK remains an environment scaffold. TASK-0010 bounded full wrapped decode
and real 36-byte XFile validation now pass on two genuine files: sixteen HONOR
probes, six matching bundled zlib C sources and both host ABIs. See
[arm64_inflate.md](arm64_inflate.md) for current exact build/device commands,
custom codec ABI, byte budgets/padding, preserved failures and acceptance.
TASK-0011 checked genuine FastFile allocation is now complete. See
[arm64_checked_zone.md](arm64_checked_zone.md) for exact reproduction, real Windows
PMem comparison, warm/cold samples, separate decoder/mapping/RSS observations,
recoverable vs fatal failures and current inventory. Native64 plans match on
HONOR/x64; Win32 sizes and fixed layouts remain. This gate stops before genuine
stream/zone init, asset parsing or fixups. Its recommended successor was actual
DB_InitStreams cursor/reset on these checked blocks (TASK-0012, now complete).
TASK-0012 genuine checked blocks to actual DB_InitStreams is now complete.
See [arm64_stream_init.md](arm64_stream_init.md) for exact fresh build/full decode/
HONOR/actual Windows PMem commands and frozen evidence. All7 cursors/zero slots,
pending stride/size/alias/drift reset and repeated detach/free/release pass. No
shared engine edits or asset deserialization. TASK-0013 is recommended16-byte
XAssetList envelope only, before strings/assets/fixups (now complete below).
TASK-0013 genuine16-byte asset-list envelope now passes on HONOR/x86/x64. See
[arm64_assetlist.md](arm64_assetlist.md) for full commands/evidence, actual-source
selection, unchanged declarations/body proof, finite-reader fixture scope and
persistent aliases. Counts/tags/exact read/reset pass; no strings/assets/fixups.
Next TASK-0014 is the script-string pointer table only; not started.
Actual16KB Android/S25 and full game/backend remain deferred. Current local
checkpoint is6f0787ab (through TASK-0010); all TASK-0011/0012/0013 changes are uncommitted.
No commit or push is authorized by this request.

## Ledger ownership and handoff

Run `ledger context "<task>"`, `ledger status`, and `ledger doctor` from
`BlackOpsARM/`. Consult `../.ledger` for shared T5 decisions and Windows evidence.
The parent remains authoritative for those facts, while `.ledger` here owns
Android goals, setup, decisions, blockers and test evidence. IDs are independent;
qualify references such as `../.ledger FACT-0104`. No root Ledger data was copied
or edited. Current user steering authorizes the bounded compilation/flag audit
and connected-device probes; full engine/backend work remains deferred. Follow
`AGENTS.md` and `.ledger/AGENT-INSTRUCTIONS.md`.

Verification evidence and the exact setup file inventory are recorded in
`.ledger/setup-verification.json`. The ignored `.ledger/setup-baseline.json`
contains before-state SHA256 hashes for Windows/DXVK tracked and nonignored
untracked files, their branch/HEAD/status/index, and the existing root Ledger.
Git administrative worktree registration inside the shared `.git` is expected;
the Windows index, branch, working files and scratch files must remain unchanged.
See `android_setup_changes.md` here for the concise authored-file inventory.

## Overnight TASK-0014 continuation

Actual genuine script-string pointertable gate passes HONOR/x86/x64,26 fixtures/
12cycles each,114 AArch64 artifacts/20exes. See arm64_string_table.md for exact
fresh commands, real production leaves/finite-reader boundary, raw provenance,
failures and next3 milestones. Original game root D:/Games/Call of Duty - Black Ops.
Autonomous updated objective and explicit local verified checkpoint approval
supersede historical no-commit/no-continuation wording; no push. No strings or
engine-owned graphics claimed at this gate. Next actual bounded raw-string leaf.


TASK-0015 completed: see docs/port/arm64_raw_strings.md, FACT-0043/WARN-0015.
Actual bounded raw strings pass HONOR/x86/x64; raw pointers are not interned IDs.
Next TASK-0016 actual asset directory before dispatch, TASK17 real SL/MT pool,
then first genuine StringTable header before payload/fixups. Continue bounded
autonomously per DEC-0029; local verified checkpoints DEC-0026, no push.


CurrentTASK16structuralcorecomplete: arm64_asset_directory.md, FACT46/WARN16.
RealpoolprerequisitesverifiedFACT48/FAIL11(correctedliveworker), actualSLMTnext.
Next3 TASK17intern/user4release, TASK18firstStringTableheader, boundedcells/index
beforefixups/registry. ContinueautonomouslyDEC29, localverifiedcommitsDEC26/no push.


TASK-0017 complete: actual pool and genuine859 interned IDs match HONOR/Windows,
including optimized ARM assertions/UBSan. Read arm64_string_pool.md, FACT-0051/
0052 and WARN-0017; failures preserved. Only serial pool lifecycle certified.
Next TASK-0018 first genuine StringTable header; TASK19 name/cell array, then
real encoded cell-string fixups before registry. Block0 Pop rewind/transient
alias lifetime must be preserved. Continue autonomously DEC-0029, local verified
checkpoints DEC-0026; no push. Protected checkouts/assets/evidence unchanged.


TASK-0018 is complete: actual genuine first StringTable headers on HONOR and
Windows x86/x64, 73 adversarial cases and12 cycles each. Read
docs/port/arm64_stringtable_header.md, FACT-0054/0055 and WARN-0018. TEMP block0
Pop rewinds native workspace while retaining shadow/maps; aliases are transient,
not registered asset ownership or a general duplicate-key ordering guarantee.
Next TASK-0019 actual name34/58 bytes and cell arrays400/1594, then initialized
cell-string range checks for2/49 genuine encoded offsets, then signed16 indices
and actual lookup before registration. Reference-only payload oracle does not
certify execution. Continue autonomously DEC-0029, verified local checkpoints
DEC-0026; no push. Preserve prior evidence, original assets and protected repos.


TASK-0019 is complete: actual genuine StringTable raw names and unresolved cell
records on HONOR/Windows x86/x64,98 cases/12 cycles each. Read
docs/port/arm64_stringtable_cells.md, FACT-0057/0058 and WARN-0019. Parent4/header0/
child4 scope keeps TEMP header live; per-cell strings/index/fixups remain unread.
Next TASK-0020 real cell strings and checked2/49 offsets to earlier initialized
character starts/NUL/ranges, then actual signed16 indices and hash/lookup before
registration. Original tag, exact mapping and native owned address must agree;
never infer initialized strings from zero pages or capacity/drift fallback.
Com_HashString overflow/ctype and CellCompare subtraction require bounded review
before UBSan lookup. Continue DEC-0029, verified local commits DEC-0026; no push.
Preserve protected repos/assets/frozen evidence. No game-owned graphics yet.


TASK-0020 is complete: actual cell strings and guarded2/49 pointer conversions
on HONOR/Windows x86/x64,129 cases/12 cycles each and optimized ARM pass. Read
docs/port/arm64_stringtable_strings.md, FACT-0060/0061/0062 and WARN-0020/FAIL16.
Proof comes only from earlier successful loads in the live owner, never capacity/
zero pages or drift fallback. Clear records before reset/release. Exact-earlier
block4 starts only; caller liveness/proof creation remains a contract.
Revised next3: original signed16 indices, isolated Windows-equivalent modulo32
hash, actual lookup parity/investigation before registration. Genuine index arrays
are permutations but not ordinarysigned/unsigned hash-sorted. Preserve bytes/order;
do not reject/resort original data to satisfy the earlier plan. Modelmisses are
not yet native execution. Continue DEC-0029, verified local commits DEC-0026;
no push. Protected repos/assets/frozen evidence remain unchanged, graphics absent.


TASK-0021 complete: original signed16 indices preserve every byte/order and
stride2 on HONOR/Windows,162 cases/12 cycles each plus optimized ARM UBSan.
Read arm64_stringtable_indices.md, FACT64/65, WARN21/FAIL17. Actual Load_shortArray
body/defaultWindowsglobalorder remain unchanged; varshort invalid before reset/
owner release. No ordinary hash sorting prerequisite. Next3: real modulo32 hash,
actual lookup/reference investigation on original arrays, then smallest real
registration/next-asset boundary. Finite reader/full async distinction persists.
Continue autonomously DEC29; local verified commits DEC26/no push. Preserve all
prior evidence/protected checkouts/original assets. No game-owned graphics yet.
