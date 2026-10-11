# TASK-0012 — Genuine checked blocks to production stream initialization

Completed 2026-10-11 on HONOR Pad X8a, Windows x86 and Windows x64. Both genuine
`code_pre_gfx.ff` and `zombie_pentagon.ff` drive unchanged production allocations
and actual `DB_InitStreams`. No shared engine/platform allocator source changed
in this gate. No asset payload was deserialized, pointers fixed up, or zone/game
initialized. Android HEAD remains `6f0787ab08e2576a342964f0297c310801ec9d11` on
`android-arm64`; all TASK-0011/0012 changes remain uncommitted. No commit/push.

User continuation authorized this bounded successor to TASK-0011. Read DEC-0023,
FACT-0035/0036 and WARN-0012; historical TASK-0011 scope and measurements remain
frozen in [arm64_checked_zone.md](arm64_checked_zone.md).

## Evidence and results

[Final machine-readable results](evidence/arm64_stream_init/stream_init_run2.results.json),
[artifact/source manifest](evidence/arm64_stream_init/manifest.json),
[linked production symbols](evidence/arm64_stream_init/stream_init_symbols.txt),
[preservation](evidence/arm64_stream_init/preservation.json) and
[exact inventory](evidence/arm64_stream_init/file_inventory.json) are retained.
Both initial/final build and run logs, full commands, stdout/stderr, device
metadata and Windows regressions are archived. Initial run passed; the final
run adds independently observable pending stride and nonzero drift checks.
There were no compiler/linker errors or failing tests in this task. Prior failed
tests/diagnostics remain in their original archives and scratch directories.

| Check | HONOR ARM64 | Windows x86 | Windows x64 |
|---|---:|---:|---:|
| Genuine asset ownership cycles | 12 | 12 | 12 |
| Actual initializer calls | 61 | 61 | 61 |
| Rejected preflight requests | 6 | 6 | 6 |
| Injected partial failures | 6 | 6 | 6 |
| Fresh full decode processes | 2 | 2 | 2 |
| Existing checked allocation fixtures | 67 | 66 | 66 |

Each asset has three rounds for both allocation types, alternating small/large
assets with one owner at a time. All seven cursor positions, zero-block positions,
push/pop including null slots, current-slot null convention, stack/delay indices,
native Win32 shadow positions, exact commit maps, alias maps and drift reset pass.
Native pointer width is eight bytes; x86 remains four. Genuine native64 logical
block sizes match Windows x64 exactly; x86 retains original disk block sizes.

The final NDK30/Clang21/API28 Debug C99/C++20 build compiles twelve actual T5 and
six unchanged matching codec C units, plus test/services. It has zero compiler or
linker errors, 2129 warnings (retained decompiled/header diagnostics), 91 verified
AArch64 objects/executables and eighteen test executables. Sixteen previous
device probes passed from pass1; the checked allocator and new stream gate run
separately with arguments in pass2. Fresh MSVC x86/x64 zone and four-byte Narrow32,
actual script-slot declaration and 483 native-width guard checks pass. The old
Windows zone regression uses the native-owner host fixture; this new gate uses
actual Windows `physicalmemory.cpp` PMem instead. Passing instrumented paths
produce no UBSan findings; whole-engine/native-library coverage is not claimed.

## Provenance and memory boundary

Original retail inputs are read-only under
`../BlackOps64/build/Release/zone/common`. Raw host/device hashes, successful full
wrapped decode/checksum/end/length, full decoded Adler against independent prior
oracles, all nine XFile fields and extracted 36-byte header SHA are checked by
`run_stream_init_gate.py` on every ABI. Headers are extracted from decoded bytes;
block descriptors are never synthesized from payload length or externalSize.
Only that verified header and decoded length reach the new executable. The
separate decoder process exits before allocation/stream tests start; the new
probe has no decoder, retail reader, full decompressed buffer or asset loader.
No original game files, extracted binary headers or payload buffers are archived
or added to Git/APK; diagnostic header fields/hex remain in provenance records.

Disk blocks remain `[36,0,0,0,49092,0,0]` and
`[2098316,4538228,0,0,93836449,0,7605494]`. Native64 logical sizes remain
`[65608,0,0,0,163720,0,0]` and
`[4052336,8688169,0,0,126744742,0,14515974]`. The large native backend request is
154001266 bytes and first-fit capacity is154017792 bytes, unchanged from TASK-0011.
Native allocation still zeroes its requests; Windows bump allocation retains
the 328/492MiB production pool without touching payloads. Reported zero payload
read/write counters mean the **stream fixture itself** does no payload I/O;
they do not describe allocator memset or decoder work.

This is a correctness gate with no new latency/RSS comparison. TASK-0011 warm/cold
samples, decoder versus allocator peaks, retained Windows commit, Android
protection/touched/RSS distinctions, clock quantization and sampling limits remain
the performance baseline. Do not interpret the new run as a matched-touch CPU
benchmark or a gameplay performance prediction.

## Reset, failure and release contract

Tests poison public pending size/stride settings, expanded commit/alias maps,
stack and delay state on owned blocks, then reinitialize. Cursor arithmetic uses
16 bytes of block0 without reading/writing contents. An equal-size commit proves
pending size reset; a separate unequal-size commit proves stale element stride
cannot survive reset (interior offset4 must have no exact mapping). Initial drift
identity is checked at offsets0 and8. These controlled cursor fixtures are not
asset parsing or pointer fixups.

Cleanup calls `DB_InitStreams` on a persistent static zero descriptor set **while
the zone is owned**, checks detached cursors/maps, calls actual `DB_FreeXBlocks`,
checks descriptors empty and owner still present, then performs named release.
Native owners/allocation counts/requested bytes/reservations return to zero;
Windows low/high positions/alloc-list counts return to baseline while its global
pool remains reserved/committed. Do not pass a temporary detached descriptor
array whose lifetime ends while globals still refer to it.

Truncated metadata, decoded-length mismatch and tampered logical plans never
invoke Begin or Init. Controlled failure at every nonzero genuine block returns
empty descriptors, restores ownership and never publishes stream globals. These
reuse the checked transaction. `DB_InitStreams` is void and native map vectors
can allocate; this gate does not make initializer bad_alloc or fatal allocator
OOM recoverable. Its stack index resets; inactive stack entry bytes, vector
capacity and cumulative diagnostic drift counters need not clear. Logical
reset is verified, not zeroing all diagnostic storage or immediate RSS recovery.
MapLookup's expected empty-commit drift-fallback diagnostics are retained and
are not test failures. Trace byte-accounting is unsupported in this standalone
probe and aborts explicitly if crossed; no fake runtime success endpoint added.

Single-owner Windows cycles deliberately avoid the unvalidated native64
multi-owner `PMem_FreeIndex` previous-entry stride. `freeTot` is reset in the
fixture only to avoid cumulative signed diagnostic overflow; production engine
ownership and formulas are unchanged. Shared engine/Windows behavior remains
untouched.

## Reproduction

Run in `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, with the HONOR unlocked and
ADB authorized. Use fresh local names for every build/run; scripts refuse
overwriting retained logs. All outputs remain in this Android worktree. The
runner creates a unique device directory, performs full fresh decoding once per
asset/ABI, verifies provenance, and runs both the new gate and old allocation
fixtures. It installs no APK and changes no persistent device setting.

```powershell
$probePython = 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH = (Split-Path $probePython) + ';C:/Program Files (x86)/Microsoft Visual Studio/Installer;' + $env:PATH
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/stream-init-repro-arm
.\tools\port\build_stream_init_host.ps1 -OutputDirectory build-android-audit/stream-init-repro-host
.\tools\port\build_checked_zone_host.ps1 -OutputDirectory build-android-audit/stream-init-repro-decoder
& $probePython .\tools\port\run_stream_init_gate.py --android-build build-android-audit/stream-init-repro-arm --host-build build-android-audit/stream-init-repro-host --host-decoder-build build-android-audit/stream-init-repro-decoder --output build-android-audit/stream-init-repro-run --serial AKWJ9X5116G04618
.\tools\port\run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 -BuildDirectory build-android-audit/stream-init-repro-arm
.\tools\port\check_zone.ps1
.\tools\port\check_narrow32.ps1 -PythonExe $probePython
```

Latest successful gate used `stream_init_pass2`, `stream_init_host_pass2`,
unchanged validated `checked_zone_host_pass5` decoder binaries, and
`stream_init_run2`. Final device directory:
`/data/local/tmp/blackopsarm-phase1-stream-init-20261011-214459226656`.
Direct rerun of retained device probe (existing extracted headers):

```powershell
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 shell '/data/local/tmp/blackopsarm-phase1-stream-init-20261011-214459226656/probe /data/local/tmp/blackopsarm-phase1-stream-init-20261011-214459226656/code_pre_gfx.header 49593 /data/local/tmp/blackopsarm-phase1-stream-init-20261011-214459226656/zombie_pentagon.header 108009041'
```

Fresh runner validation is preferred if device files have changed. Exact
invocations and executable/header/raw/oracle hashes are frozen with results.

## Recommended TASK-0013

The next smallest real loading boundary is the 16-byte `XAssetList_32` envelope
immediately after the36-byte XFile, through actual `DB_ExpandLoad_XAssetList`
on native64 and the unchanged Win32 envelope read. Its production caller
`Load_XAssetListCustom` immediately loads script strings, so calling that whole
function would exceed the proposed boundary.

Acceptance: both genuine files/HONOR/x86/x64; validated bounded decoded-byte
source; exactly16 disk bytes consumed; unchanged signed counts and32-bit pointer
tags widened according to actual production rules without dereference/fixup;
independent original-byte/Windows comparison; short/inconsistent/count-invalid
input rejected before publication; stream/reset/ownership regressions retained;
one decoded buffer at most and HONOR RAM protected. Audit the real source read
dependency rather than silently stubbing `DB_LoadXFileData`. Stop before script
strings, asset arrays, asset dispatch, registry/zone init, async/APC or rendering.
No TASK-0013 implementation has started.
