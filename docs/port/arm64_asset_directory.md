# TASK-0016 — Genuine FastFile structural front end

Completed 2026-10-12 local time. Both genuine original-game FastFiles now reach
actual script pointer-table expansion, bounded raw strings and the asset directory
on HONOR ARM64 and Windows x86/x64. No asset payload dispatch or graphics runs.
The approved structural core is complete; actual interning remains TASK-0017.

| Input | Directory source offset | Entries | Disk/native64 record bytes | File end | Block4 disk/native64 end |
|---|---:|---:|---:|---:|---:|
| code_pre_gfx.ff | 52 | 37 | 296 / 592 | 348 | 296 / 592 |
| zombie_pentagon.ff | 18885 | 1045 | 8360 / 16720 | 27245 | 27196 / 42360 |

Native runs actual `DB_ExpandLoad_XAssetArray`; Win32 runs actual `Load_Stream`.
The production helper bodies/declarations remain unchanged. Checked planning
uses actual validated XFile block capacities, uint64 arithmetic and the original
native helper1<<20 domain, rejecting invalid counts/commit widths/types/source/
capacity before allocation or writes. Root absence with positive count remains
legacy absence; presentzero still aligns. Every tag bit is preserved, including
sign extension only for -1/-2. Enum0..42 certifies directory layout, not all
possible dispatcher cases. Genuine header tags are allFFFFFFFF; first asset in
both files is type37 StringTable.

Zombie's directory aligns disk memory18833->18836 and native25639->25640.
No file bytes are skipped: source remains18885. Native internal struct padding
is zero; memory alignment-gap bytes are not assumedzero. Every record type/tag,
exact element start, rejected interior/end mapping and headerfield alias is
checked (disk+4 -> native+8). Prior raw/table bytes, maps and aliases are preserved.
Reset clears every directory and table alias/exact mapping before owner release,
restoring logical allocation counters. No post-read allocation-exception rollback
claim for private helper vectors/stream segments.

47 fixtures and12 alternating genuine low/high allocation cycles pass on each
ABI. This includes24 raw-string fixtures,16 directory preflight cases and7
actual directory cases: zero/single/mixed6tag records on aligned/+1 cursors and
shortreader failure. Late invalid enum rejects wholeplan beforewrites; failures
snapshot source, bytes, aliases, cursors and owners before reset/release.

Independent oracle verifies complete raw/decoded SHA/Adler and every numeric
record from originals D:/Games/Call of Duty - Black Ops. Host reference takes
two bounded streaming decode passes, without retaining duplicate full buffers.
Native decoders each finish/release one decoded vector in separate processes
before allocation tests, keeping only348/27245-byte prefixes. Previously
SHA-verified originals on HONOR are reused. Original strings/asset buffers stay
out of Git; only structural metadata/digests/test evidence are frozen.

Final asset_directory_pass2 NDK30/Clang21/API28 Debug:0 compiler/linker errors,
3692 retained warnings,146 AArch64 artifacts/23 test executables. Shared raw
fixture extension separately rebuilds/runs Windows x86/x64 (24fixtures/12cycles
pass). Passing instrumented directory paths have no UBSan diagnostics. Existing
production dependencies/bodies remain unchanged; normal Windows full stream TU
and prior16 HONOR probes from TASK15 remain valid. No unnecessary repeated
whole-engine/backend tests. Independent source/adversarial/oracle review passes.

Real script-pool synchronization prerequisites also pass Windowsx86/x64 and
HONOR: two recursive independent locks, contention,4-byte seq_cst atomics,
80000 atomic/80000 protected updates and explicit unsupported-case diagnostics.
A review caught the first identity probe querying an exited pthread; FAIL-0011
retains that evidence. Corrected probe holds the registered worker alive, fresh
service-only Android build has0warnings0errors and all3 native cases pass. Read
arm64_script_string_services.md and the separate corrected lifetime archive.
These are real platform services, not actual SL/MT pool certification.

## Reproduce

From BlackOpsARM PowerShell, use fresh directory names:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/asset_directory_repro1
./tools/port/build_asset_directory_host.ps1 -OutputDirectory build-android-audit/asset_directory_host_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_asset_directory_gate.py --android-build build-android-audit/asset_directory_repro1 --host-build build-android-audit/asset_directory_host_repro1 --output build-android-audit/asset_directory_repro_run1 --serial AKWJ9X5116G04618
```

Runner verifies native/device/host executable and prefix SHA, every fixture/
cycle/measurement, explicit exits and no unknown records. Cacheddeviceoriginal
manifest defaults to raw_strings_run1/results.json; originals are read-only.
Keep old outputs; missing cachedoriginals require an explicit verified transfer.
Decoder and allocation memory are separate; this gate records no new timing/RSS
performance claim. Historical TASK11 allocation/decoder measurements remain.
Finite decoded-source callbacks do not certify full async/refill/APC loader.
No encodedstringfixups/SLIDs/assetpayloads/registry/zoneinit/rendering claimed.

DEC-0031, FACT-0046/0048, FAIL-0011 and WARN-0016. Previous checkpoints583b976/
da7f496 protect pointer/raw gates; local verified checkpoints approved DEC-0026,
no push. Protected Windows/DXVK/root and1503 older archive/report/handoff files
preserved. Exact current file delta and pending state: arm64_asset_directory_changes.md.

Next3: actual SL/MT init/intern/user4release and genuine strings; first genuine
StringTable packed20->native32 header block0; bounded StringTable name/cell/index
loading beforefixups/registration. Continue toward actual native engine startup
one validated boundary at a time. No fabricated menu/frame output.
