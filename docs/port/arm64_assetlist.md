# TASK-0013 — Genuine 16-byte asset-list envelope

Completed 2026-10-11: both genuine FastFiles pass on HONOR ARM64 and Windows
x86/x64. Actual native `DB_ExpandLoad_XAssetList` reads exactly16 bytes at decoded
offset36; the Win32 test executes its original raw16-byte envelope-read branch.
Counts/tags are preserved, aliases address persistent final fields, block cursors
do not advance, and repeated detach/free/named release restores ownership.
No strings, asset arrays, fixups, registry/zone initialization or rendering run.

DEC-0024/0025, FACT-0037/0038, FAIL-0007 and WARN-0013 record scope, decisions,
results and limits. Android remains `android-arm64` at
`6f0787ab08e2576a342964f0297c310801ec9d11`. TASK-0011/0012/0013 work is uncommitted;
no commit/push. [Exact changes](arm64_assetlist_changes.md) and
[machine inventory](evidence/arm64_assetlist/file_inventory.json) include prior
uncommitted status and local retained artifacts.

## Genuine fields and results

| File | String count/tag | Asset count/tag | Minimum disk/native table bytes |
|---|---|---|---|
| code_pre_gfx.ff | 0 / 00000000 | 37 / FFFFFFFF | 296 / 592 |
| zombie_pentagon.ff | 860 / FFFFFFFF | 1045 / FFFFFFFF | 11800 / 23600 |

Native minima apply to ARM64/x64; Win32 minima equal disk minima. These are only
minimum table prerequisites, not complete asset sizes, array allocations or
estimates of full loading memory. Runtime envelope size is32 bytes on native64,
16 on Win32; disk size remains16. Root nonzero tags mean presence here. Only
FFFFFFFF/FFFFFFFE sign-extend; zero, ordinary offsets,80000000 andFFFFFFFD
zero-extend. None is dereferenced or fixed up by this gate.

Per ABI:72 controlled fixtures and12 genuine alternating small/large low/high
owner cycles, one16-byte read each. Negative/INT_MIN/INT_MAX counts,32-bit-sized
arithmetic hazards, short/null-sized prefix ranges0..51, decoded-length mismatch,
disk exact/one-over and native-minimum footprint bounds, unaligned/trailing input,
all six tag classes, null-positive count semantics and explicit short finite-read
exception are covered. Rejection snapshots output, reader counters, ownership,
seeded aliases, all cursors/shadow positions and a nonzero delay marker **before
reset**. No preflight failure reaches the expander or publishes output.

Actual normal Windows x64 expander TU compiles with no gate macro. Actual full
registry layout checks pass x86/x64; light envelope declarations also pass x64
with the historical flag off. The final NDK30/Clang21/API28 Debug C99/C++20 build
has0 compiler/linker errors,2495 retained warnings,102 verified AArch64 artifacts
and19 test executables. Twelve previously selected full T5 units plus the bounded
expander subset and six unchanged custom-ABI codec C sources compile. This does
not certify the full Android expander translation unit. Passing instrumented
paths have no UBSan findings; whole-engine/native-library coverage is not claimed.

Previous16 HONOR probes,67 device/66 each Windows checked allocation fixtures,
12-cycle/61-Init genuine stream gate, Windows native-owner zone fixtures and
four-byte Narrow32/actual script declarations/483 native-width guards pass.
New envelope/stream references use actual Windows PMem, one owner at a time;
old zone fixtures use the native-owner host adapter. These scopes remain distinct.

## Implementation and provenance

Three existing production declarations (`XAsset`, `ScriptStringList`,
`XAssetList`) moved byte-identically from `db_registry.h` to lightweight
`db_assetlist_types.h`. Four-byte enum/int, runtime pointer width and field offsets
have assertions. Normal registry inclusion and Windows behavior are retained.
`BO1_ASSETLIST_GATE` is a standalone test compile selection in `db_expand.cpp`,
excluding unrelated graphics/physics includes and other expanders. The actual
envelope/read/pointer-store helper region is byte-identical to baseline, proven
by [source preservation](evidence/arm64_assetlist/source-preservation.json).
No extracted/independent engine source tree is maintained.

`PlanAssetList` is an additional reusable checked gate policy, currently exercised
by the standalone probe. It revalidates genuine XFile/zone arithmetic, requires
nonnegative counts, computes present-array disk minima with uint64 (4-byte tags,
8-byte disk assets) and native minima using actual sizeof(pointer/XAsset), and
checks decoded remaining/disk block4/logical block4. It adds no arbitrary count
limit, expansion or clamping and allocates nothing. Positive counts with zero
tags remain absent arrays, matching legacy presence behavior. The legacy normal
Windows expander has no new count checks; no claim of full loader integration.

Read-only originals are under `../BlackOps64/build/Release/zone/common`. An
independent Python system-codec streaming decode retains at most its current
chunk and52-byte prefix, verifying full decoded SHA256/Adler/length against prior
frozen oracles. Each native ABI independently fully decodes with the unchanged
bundled custom codec, verifies prefix/Adler/length, extracts52 bytes and releases
its only decoded vector. That process exits before allocation/read tests begin.
Raw host/device and executable/prefix hashes match. Allocation processes retain
only verified prefixes; no duplicate full decoded buffers. Original game files,
binary prefixes/headers and payload buffers stay local/device, outside Git/APK.
Diagnostic fields/hex and provenance are frozen.

The real expander calls an explicit **finite-reader fixture adapter** implementing
`DB_LoadXFileData`: checked16-byte memcpy, exact consumption/accounting, throw on
unavailable/short input. This is substantive bounded reading, not a success stub,
but it does not exercise production Win32 async/refill/APC code. Trace byte
accounting remains unsupported/aborting in the standalone fixture. No APK
build/install or device setting change. TASK-0011 timing/RSS baseline remains
frozen; this gate makes no new performance comparison.

Native alias slots at block0 offsets4/12 point into the persistent side-buffer
destination, including null fields. Never expand into a temporary then copy it:
aliases would target the temporary. Tests detach via a static zero descriptor
set before clearing descriptors, destination reuse and named release. These
outside-XBlock alias slots are registered correctly; later production alias
conversion may reject them. Fixup resolution is explicitly unvalidated.
Private helper disk metadata references its stack buffer until the next read
overwrites it; no later access occurs in this bounded path. Post-read alias-vector
bad_alloc rollback, initializer bad_alloc/fatal OOM recovery, retained vector/
inactive stack/diagnostic storage and native64 multi-owner legacy PMem stride
remain limitations. Fixture-only freeTot reset preserves production ownership.

## Evidence and retained diagnostics

[Final results](evidence/arm64_assetlist/assetlist_run3.results.json),
[independent prefix oracle](evidence/arm64_assetlist/assetlist_run3.independent-oracles.json),
[manifest](evidence/arm64_assetlist/manifest.json),
[production boundary symbols](evidence/arm64_assetlist/assetlist_symbols.txt),
[read-only reviews](evidence/arm64_assetlist/review-evidence.md), and
[preservation](evidence/arm64_assetlist/preservation.json) are frozen.

All three passing build/run attempts are retained. Header-check1 failed linking
unused NetField tables; existing standalone data fixtures supplied those tables.
Header-check2 passed normal ABIs but heavy legacy-off x64 hit existing
`r_font.h:32`/`r_material.h:237,401` Win32 graphics layout assertions. Guards were
retained. Checks3/4 passed normal registry ABIs, light legacy-off header and full
normal x64 expander. An initial source-hash helper compared Git LF with checkout
CRLF; corrected verification reconstructs checked baseline bytes. FAIL-0007 and
[diagnostic narrative](evidence/arm64_assetlist/verification-attempts.log) preserve
these details. No genuine envelope runtime or NDK/host gate build failed.

## Reproduction

Run from `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, HONOR unlocked/ADB authorized.
Use fresh output names; every previous local/device directory is retained.

```powershell
$probePython = 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH = (Split-Path $probePython) + ';C:/Program Files (x86)/Microsoft Visual Studio/Installer;' + $env:PATH
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/assetlist-repro-arm
.\tools\port\build_assetlist_host.ps1 -OutputDirectory build-android-audit/assetlist-repro-host
& $probePython .\tools\port\run_assetlist_gate.py --android-build build-android-audit/assetlist-repro-arm --host-build build-android-audit/assetlist-repro-host --output build-android-audit/assetlist-repro-run --serial AKWJ9X5116G04618
.\tools\port\check_assetlist_headers.ps1 -OutputDirectory build-android-audit/assetlist-repro-headers
.\tools\port\run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 -BuildDirectory build-android-audit/assetlist-repro-arm
.\tools\port\build_stream_init_host.ps1 -OutputDirectory build-android-audit/assetlist-repro-stream
.\tools\port\build_checked_zone_host.ps1 -OutputDirectory build-android-audit/assetlist-repro-checked
& $probePython .\tools\port\run_stream_init_gate.py --android-build build-android-audit/assetlist-repro-arm --host-build build-android-audit/assetlist-repro-stream --host-decoder-build build-android-audit/assetlist-repro-checked --output build-android-audit/assetlist-repro-stream-run
.\tools\port\check_zone.ps1
.\tools\port\check_narrow32.ps1 -PythonExe $probePython
```

Latest envelope: assetlist_pass3/assetlist_host_pass3/assetlist_run3. Current device
directory `/data/local/tmp/blackopsarm-phase1-assetlist-20261011-220744878356`.
Direct retained HONOR rerun (prefer full fresh runner if files changed):

```powershell
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 shell '/data/local/tmp/blackopsarm-phase1-assetlist-20261011-220744878356/probe gate /data/local/tmp/blackopsarm-phase1-assetlist-20261011-220744878356/code_pre_gfx.prefix 49593 /data/local/tmp/blackopsarm-phase1-assetlist-20261011-220744878356/zombie_pentagon.prefix 108009041'
```

## Recommended TASK-0014

Reserve block4's script-string pointer table and exercise actual
`DB_ExpandLoad_Ptr32Array` / Win32 four-byte table equivalent, before
`Load_TempStringArray` iterates strings. Both files/all3 ABIs; independent validated
table window begins decoded offset52. Code_pre's absent zero-string case consumes
no table; zombie's860 entries consume3440 disk/6880 native bytes. Verify actual
alignment/capacity, exact tag widening/no dereference, disk/native cursor and
element-stride maps/alias destinations, truncation/count/capacity rejection and
repeat detach/release. Audit existing caller/expander count policies rather than
substituting new arbitrary clamps. Stop before string bytes/interning, asset
arrays/dispatch/fixups/registry/zone init/async/rendering. Not started.
