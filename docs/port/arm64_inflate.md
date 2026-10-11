# ARM64 bounded deflate isolation — TASK-0010

Completed 2026-10-11. The synchronous decoder in
`src/platform/fastfile_inflate.*` validates the raw 12-byte head, decodes the
complete zlib-wrapped stream into memory, verifies its checksum/end and checks
exactly `36 + XFile.size` decoded bytes. Header/output publication occurs only
after success; failures clear the result. It does not parse assets or connect
the engine's async loader. Actual database sources are unchanged by this gate.

## Reuse and ABI

Read-only agents independently audited production loader order and test/ABI
coverage, then reviewed the implementation. They identified T5's custom zlib
stream ABI: the bundled header removes `reserved`. Linking Android's system
zlib through this header would be invalid. This gate reuses six actual inventoried
zlib 1.2.3 C files unchanged: inflate, inffast, inftrees, adler32, crc32, zutil.
The audit enables C99 alongside C++20 and passes the native stream size, never
a hardcoded 52. Observed stream sizes: MSVC x86 52, MSVC x64 80, Android ARM64
104; unsigned long is respectively 4/4/8 bytes. Disk XFile remains 36 bytes,
with four-byte fields and native runtime block pointers. No system libz is linked.

Production references: db_file_load.cpp validates raw magic/version, rejects
authenticated IWff0100, decodes XFile then file.size payload; db_auth.cpp uses
wrapped inflate initialization with sizeof(z_stream_s). The new synchronous
boundary accepts unsigned IWffu100/version473. Authenticated, raw-deflate and
gzip inputs are explicitly rejected. Original Windows loader bodies are retained.

Counters use independent uint64 values. Chunk sizes are 1..32768, so whole-file
sizes are never narrowed to zlib uInt. Allocation callback multiplication is
checked; initialized codecs are released on every return/exception. Stalled,
dictionary-dependent, corrupt, truncated and mismatched streams fail explicitly.
Memory input overlapping output storage is rejected before clearing it. File
open failures distinguish missing paths from I/O errors.

Default limits: 256 MiB whole-file bytes including raw head/padding, 512 MiB
decoded bytes including XFile, and at most 31 trailing zero bytes. Padding is an
explicit configurable policy; zero selects strict EOF. Nonzero trailing bytes,
concatenated members and more than 31 zeros fail. The two measured genuine files
use 23 and 9 zero bytes. This does not certify padding conventions for all files.
The output limit controls length, not exact vector capacity or total heap use.

## Verification

Final `build-android-audit/inflate_pass3`: NDK r30, Clang21, API28, ARM64 Debug,
zero compiler errors/linker failures, 1658 retained warnings. Twelve actual T5
units plus six existing vendor C units are selected through the original
inventory. All 75 object/executable artifacts inspected are ELF64 little-endian
AArch64. New decoder, bundled codec and probes use unrecoverable UBSan; passing
paths have no findings. No whole-engine or comprehensive native-library claim.

All sixteen native probes pass on HONOR NDL-L09/API36/4096 pages, serial
AKWJ9X5116G04618. MSVC x86/x64 inflate tests and all prior zone, debugger/first-fit,
parse/HunkUser, header/profile/maths/RNG/lookup, FastFile and Narrow32/four-byte
slot/483-guard checks pass, built only in the Android worktree.

Python zlib 1.3.2 generates independent deterministic fixtures, not the codec
under test. Probes compare complete decoded bytes/fields for payload lengths
0, 1, 127, 32767/32768/32769 and 524287/524288/524289; exercise one-byte and
split chunks, unaligned input, repeated-pattern backreferences, exact budgets
and one-byte excess, header preflight, short headers/body/trailer, checksum
corruption, dictionary requests, size mismatches, wrong wrappers, padding limits,
second members, missing paths and directory I/O errors. External/allocation fields
deliberately differ from serialized payload size; no false block-sum equality.

Both genuine assets decode on HONOR and MSVC x86/x64, matching independent Python
header fields, byte counts and recomputed decoded Adler32:

| Asset | File bytes | Decoded bytes | XFile.size | externalSize | Zero padding | Decoded Adler32 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| code_pre_gfx.ff | 13280 | 49593 | 49557 | 0 | 23 | d65f37d2 |
| zombie_pentagon.ff | 48229216 | 108009041 | 108009005 | 308074740 | 9 | 99751857 |

Raw SHA256 values match the device's sha256sum and host assets. Full hashes and
independent decoded SHA256 values are retained in the two oracle JSON files.
Decoded SHA256 was computed by Python; native probes compare Adler32, counts and
fields. Original retail directory, located read-only by the audit agent:
`../BlackOps64/build/Release/zone/common` (86 common + 72 English FastFiles).
Only the two named assets are decoded here. Game bytes remain outside durable
evidence/Git/APK; the small previously pushed asset was retained locally in an
ignored directory and the larger file pushed to this fresh native test directory.

Initial failed builds remain preserved: ARM64 signed-char adapter assertions
and x64 Windows min/max macro expansion. Fixes select signed char on new targets
and parenthesize C++ names; no Windows/global macro changes. All three NDK attempts,
three host attempts and final regressions are archived separately in
`evidence/arm64_inflate`, including source/ELF/log hashes, independent fixture
manifests, oracles and local ignored artifact locations. No scratch was deleted.

Windows checkout/files/index/branch, independent DXVK, root Ledger, Android
HEAD/index and all 595 earlier evidence files are unchanged. No commit/push.
Scaffold APK SHA remains unchanged; no new assemble/lint/install was performed.
Exact changes/full uncommitted inventory: arm64_inflate_changes.md and
evidence/arm64_inflate/preservation.json. Ledger: DEC-0019, FAIL-0005,
FACT-0029/0030, WARN-0010. Every compiler/test session has completed.

## Reproduction

Run from `D:\Desktop\Black Ops 2 Android\BlackOpsARM` in PowerShell:

```powershell
$taskPython = 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH = (Split-Path $taskPython) + ';C:/Program Files (x86)/Microsoft Visual Studio/Installer;' + $env:PATH
$taskBuild = 'build-android-audit/inflate-repro-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff')
.\tools\port\build_arm64_audit.ps1 -BuildDirectory $taskBuild
if ($LASTEXITCODE) { throw 'Build failed' }
.\tools\port\run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 -BuildDirectory $taskBuild
if ($LASTEXITCODE) { throw 'HONOR probes failed' }
$taskGame = 'D:/Desktop/Black Ops 2 Android/BlackOps64/build/Release'
.\tools\port\check_inflate.ps1 -PythonExe $taskPython -GenuineFiles @(
  "$taskGame/zone/common/code_pre_gfx.ff", "$taskGame/zone/common/zombie_pentagon.ff")
```

The CMake generator prepares independent fixtures beside the executable; the
runner pushes them and sixteen probes into a unique native directory, installs
no APK and retains everything. To repeat the genuine device checks using that
new directory, without touching older runs:

```powershell
$taskAdb = 'D:/Android/Sdk/platform-tools/adb.exe'
$taskDevice = (Get-Content "$taskBuild/device-tests.log" -First 1).Substring('Device test directory: '.Length)
foreach ($taskAsset in @('code_pre_gfx.ff','zombie_pentagon.ff')) {
  & $taskAdb -s AKWJ9X5116G04618 push "$taskGame/zone/common/$taskAsset" "$taskDevice/$taskAsset"
  if ($LASTEXITCODE) { throw 'Asset push failed' }
  & $taskAdb -s AKWJ9X5116G04618 shell "sha256sum $taskDevice/$taskAsset; $taskDevice/bo1_inflate_tests $taskDevice/$taskAsset"
  if ($LASTEXITCODE) { throw 'Genuine decode failed' }
}
.\tools\port\check_zone.ps1
.\tools\port\check_debugger.ps1 -PythonExe $taskPython
.\tools\port\check_parsetree.ps1
.\tools\port\check_script_headers.ps1 -PythonExe $taskPython
.\tools\port\check_narrow32.ps1 -PythonExe $taskPython
.\tools\port\check_fastfile.ps1
```

## Remaining limits and next task

This decoder is isolated; genuine headers have not yet been fed to actual zone
allocation. Block/expansion/pool validation must precede that connection. Async
staging, authentication, asset-list expansion/remapping, strings/callables/operators
at runtime, full VM, production PMem tracking/ordering, whole engine/backends,
actual 16KB hardware and S25 remain unvalidated. Prior owner-thread and stale-handle
rules still apply. Existing missing engine services stay aborting probe fixtures.

TASK-0011: connect validated genuine XFile to actual DB_AllocXBlocks/DB_InitStreams.
Acceptance: checked uint64 native expansion/per-block/aggregate budgets before
any allocation; reject malformed/oversized sizes and unsupported flags/types
without leaks; preserve original disk fields/zero slots; genuine code_pre_gfx
bootstrap on HONOR and MSVC x86/x64; repeat/reset/detach/free/release returns all
owners/reservations to baseline; all sixteen current probes remain green;
preserve failures and protected checkouts. Stop before Load_XAssetListCustom,
asset deserialization or async integration. TASK-0011 is not started here.
