# BlackOpsARM fresh-agent handoff

Prepared 2026-10-11. The bounded debugger/allocation task is complete. No new
porting work was started during handoff. The user requested a local commit of
the accumulated Android changes; pushing remains unauthorized. The commit
containing this handoff is the checkpoint: `git log -1 --oneline`.

## Orientation

Work only in D:/Desktop/Black Ops 2 Android/BlackOpsARM on android-arm64.
Read AGENTS.md, .ledger/AGENT-INSTRUCTIONS.md and .ledger/STATE.md, then run:

```powershell
Set-Location 'D:/Desktop/Black Ops 2 Android/BlackOpsARM'
git status --short
git log -1 --oneline
ledger context 'TASK-0007 normal-to-debugger expression compilation boundary'
ledger show WARN-0007
ledger show TASK-0007
```

This is a linked worktree with shared BlackOps64 history and an independent
directory/index. Windows ../BlackOps64/main stays at
76edfd5e75976c41bfab5a5db262b8b593432518, with its nine untracked scratch files.
../DXVK-Native is independent master at
0e271c6be16ded1a8638d117360c2e65d9df4603. Neither checkout was built, changed or
cleaned. Root ../.ledger owns shared T5/Windows history; local Ledger owns Android
progress. No scratch files or failed runs were deleted.

## Completed milestones and latest results

1. Linked Android worktree and Gradle/APK environment scaffold. Renderer target
   remains separate DXVK Native 1.9.2 with tested SDL2/Vulkan, original T5 assets/
   FastFiles and Xbox controller input. APK has no engine or renderer.
2. All 504 BO1_PORT_X64 references / 483 guards audited per site. Reviewed
   architecture paths use BO1_NATIVE_64BIT; Android has native pointers and
   four-byte Narrow32 region handles. No blanket flag replacement.
3. Actual script headers, profiler services, timing/error and maths probes.
4. Windows-reference first-party CRT RNG and actual cscr_readwrite lookup gate.
5. Actual normal parse constructors/default HunkUser, with real Android virtual
   memory, native page handling, arena ownership and typed dispatch.
6. Debugger handle/prepend/buffer fixes; whole actual evaluator compilation and
   selected cleanup/canonical/numeric/boolean paths. Real zeroed Z ownership,
   first-fit alignment/synchronization and debug HunkUser main-thread identity.

Latest build: build-android-audit/debugger_pass10; NDK 30.0.16248370 / Clang21 /
API28 / arm64-v8a / Debug C++20. Ten actual T5 units, zero compiler errors,
successful links, 1261 retained warnings and 55 verified AArch64 ELF artifacts.
All thirteen HONOR probes exit zero. UBSan on actual parsetree/HunkUser/evaluator/
first-fit objects and new probe bodies reports no findings on passing paths.
Host x86/x64 debugger, first-fit, parse/default HunkUser, six header/profiler/
maths/RNG/lookup probes, Narrow32 and flag guards passed. APK assemble/lint passed;
SHA256 remains DFA4C3BD6C8FDDF121CF87EE9B5847367FB6C29CDB5EAD8FC164F3A31F1135B5.

No compiler/native test processes remain active on workstation or HONOR. ADB
server and a Gradle daemon are background services. All launched checks completed;
see evidence/agent_handoff/process_state.json and archived test results.

## Exact HONOR reproduction commands

HONOR Pad X8a is observed as NDL-L09 / API36 / ARM64, serial AKWJ9X5116G04618,
native page size 4096. Use a fresh output directory; scripts refuse to overwrite
test logs. These commands install no APK and preserve previous runs.

```powershell
$ErrorActionPreference = 'Stop'
Set-Location 'D:/Desktop/Black Ops 2 Android/BlackOpsARM'
$handoffSdk = 'D:/Android/Sdk'
$handoffPython = 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH = (Split-Path -Parent $handoffPython) + ';' + $env:PATH
$handoffBuild = 'build-android-audit/handoff-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff')
& "$handoffSdk/platform-tools/adb.exe" devices -l
& "$handoffSdk/platform-tools/adb.exe" -s AKWJ9X5116G04618 get-state
.\tools\port\build_arm64_audit.ps1 -SdkRoot $handoffSdk -BuildDirectory $handoffBuild
if ($LASTEXITCODE) { throw 'ARM64 build failed; retain configure.log and build.log' }
.\tools\port\run_arm64_audit_tests.ps1 -SdkRoot $handoffSdk -Serial AKWJ9X5116G04618 -BuildDirectory $handoffBuild
if ($LASTEXITCODE) { throw 'HONOR probe failure; retain device-tests.log' }
.\tools\port\check_debugger.ps1 -PythonExe $handoffPython -VcVars 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat'
.\tools\port\check_parsetree.ps1 -VcVars 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat'
.\tools\port\check_script_headers.ps1 -PythonExe $handoffPython -VcVars 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat'
.\tools\port\check_narrow32.ps1 -PythonExe $handoffPython -VcVars 'C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat'
git diff --check
ledger doctor
```

Python is required by CMake declaration generators; PATH pins the installed
runtime. Requires SDK CMake/Ninja 3.22.1, MSVC Build Tools and Windows SDK.
Device runner uses a unique /data/local/tmp/blackopsarm-phase1-* directory and
leaves binaries intact. Latest retained device run:
/data/local/tmp/blackopsarm-phase1-20261011-180835019.
Scaffold rebuild uses JAVA_HOME C:/Program Files/Java/jdk-21 and
`gradlew.bat --no-daemon :app:assembleDebug :app:lintDebug` from android/.

## Active blockers and incomplete work

WARN-0007 is active. Normal-to-debugger expression compilation/text parsing,
string interning, callable slots/lists, nonliteral operators/variables and full
VM have not been validated. Fifty generated compiler/VM endpoints abort if
reached; three cleanup callbacks use test-owned value/reference state. These
standalone fixtures must never be linked into an engine/APK. Windows host
debugger tests use the Android heap; the unchanged production Windows Z heap's
164-byte prefix does not certify native debugger-prefix alignment.

Tracking/fixed schemes and complete diagnostics remain unsupported.
RegisterHunkMainThread must bind the main thread before debug allocator startup.
Default hunks/global debug list remain owner-thread confined; first-fit reset/
destroy require quiescence. Handles must not outlive allocations; Narrow32 does
not revoke stale handles. Real 16 KB Android/Samsung S25, wider replay/profiler
reporting, graphics/audio/lifecycle/input and gameplay remain untested. No
partially running task/test is left to resume; these are future work.

## Next task and acceptance criteria

TASK-0007: start actual normal-to-debugger expression compilation using
Scr_CompileExpression/Scr_CompilePrimitiveExpression and normal parse
constructors, then evaluate numeric/boolean expressions for both instances.
Inspect list/string/callable ownership before expanding. Wait for the user's
continuation before starting new porting work.

- Exercise actual production compilation functions and evaluate their trees
  for representative integer/float/negative/boolean inputs on HONOR and Windows
  x86/x64. Explicitly label fixtures and unsupported paths.
- Prove four-byte slots/native-pointer roundtrips and exact allocation cleanup,
  with no stale-handle access, native misalignment or UBSan findings on tested paths.
- Add genuine services only where justified; no success stubs, copied engine
  bodies or disabled layout checks. Account for Windows Z heap alignment.
- Keep all thirteen existing HONOR probes and applicable Windows/Narrow32 guard
  checks passing. Retain failures, compiler diagnostics and reference evidence.
- Preserve frozen evidence and Windows/DXVK/root Ledger; record exact files,
  commands, results and limits in a new evidence directory and local Ledger.
- Do not widen into full VM/gameplay/backend work or commit/push without new
  explicit authorization.

## Files, evidence and checkpoint

All accumulated durable Android changes were uncommitted before this checkpoint:
scaffold, shared/compatibility sources, tools, Ledger and five evidence archives.
Exact modified/untracked paths and hashes:
[pre_commit_inventory.json](evidence/agent_handoff/pre_commit_inventory.json).
Readable list: pre_commit_inventory.md beside it. The checkpoint includes the
listed durable files. Local builds, SQLite/lock, SDK properties and caches stay
ignored and retained; no remaining durable change is intended after commit.

Frozen evidence directories: arm64_phase1 (54 files), arm64_script_headers (61),
arm64_rng_script (71), arm64_parsetree (94), arm64_debugger (156). Failed compiler,
host fixture and linker diagnostics remain alongside reference captures. Git
attributes preserve archive bytes; never normalize/regenerate frozen evidence.
Ignored artifact catalogs retain local binary/build hashes.

DEC-0014/0015 and FACT-0017/0018 explain the latest implementation; FAIL-0002
records failures. WARN-0007/TASK-0007 define the next boundary. Rotation results
are appended in local Ledger. Post-commit verification and ID are also saved in
ignored build-android-audit/rotation-receipt.json. No push is authorized/performed.
