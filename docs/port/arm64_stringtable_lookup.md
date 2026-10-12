# TASK-0023 actual StringTable queries

The original C++ binary-index, row and column query leaves now execute on HONOR
ARM64, Windows x86/x64 and optimized ARM. ABI parity passes, but genuine row
query correctness has a shared discrepancy. No algorithm or retail data was
changed to manufacture successful queries.

| First genuine table | Existing row queries | Actual row misses / linear disagreements | Column disagreements |
|---|---:|---:|---:|
| code_pre_gfx.ff | 400 | 24 | 0 |
| zombie_pentagon.ff | 1594 | 796 | 0 |

These are actual native results, identical on all three ABIs and optimized ARM,
for every one of12 genuine allocation/release cycles per ABI. Every query has
an existing matching value in the independently scanned linear reference.
All1,994 column queries agree with that reference. ASCII case changes preserve
every actual result, including misses. The full records contain each original
signed hash, binary-search position, returned row/column and linear results;
there is no asset text/hex in evidence. FACT-0062's earlier models are now
confirmed for this reconstructed C++ implementation (FAIL-0020).

This does **not** establish original retail-game instruction behavior. The
verified original executable SHA256 is
`2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759`,
x86/image base0x400000, without COFF symbols. No verified original addresses
for FirstHashIndex/CellCompare/row lookup were established in this gate. Ghidra
had no running instance; no game/debugger/import was launched. Read-only source
inspection found no FastFile StringTable index normalization: Load_shortArray
preserves data; Load_StringTableAsset -> DB_AddXAsset -> DB_LinkXAssetEntry;
generic cloning copies the header, StringTable clone callback is null, and
external/postload handlers cover weapons/images. The located index sort belongs
to LoadObj's qsort/CellCompare. This is source evidence, not retail disassembly.

The six unique original bodies stay byte-identical in their original units:
`StringTable_FirstHashIndex`, simple `StringTable_LookupRowNumForValue`,
`StringTable_LookupColumnNumForValue`, `I_isupper`, `I_strnicmp`, `I_stricmp`.
`BO1_STRINGTABLE_LOOKUP_GATE` selects them for a bounded executable. Whole
default source/global/function order is recovered exactly by reversing only
the selectors; declarations and canonical hash remain unchanged. Normal full
Windows query/comparison/hash/StringTable-object translation units compile on
x86/x64 (eight objects). No CRT comparison replaces the real engine bodies.

262,180 comparison cases pass: the full256×256 byte matrix through actual
I_stricmp and nonnegative prefix lengths0/1/2, signed/ASCII predicate boundaries,
and terminated longer prefix cases. The independent oracle lowers ASCII and
compares unsigned bytes. Valid storage includes the extra readable byte required
by the original pre-count read in I_strnicmp. Null comparisons and negative
prefix lengths are not executed or certified. Hash high-byte/locale behavior
remains the separate limitation from TASK-0022.

22 controlled query cases pass, including the real folded-hash collision
`a~`/`b]`, case duplicates, a null cell, absent/empty values, zero dimensions,
signed numeric binary-search extremes and two actual upper-bound diagnostics.
Controlled fixture indices are sorted only for that fixture; genuine arrays
are never sorted or rejected for hash order. Negative comparison columns cannot
match nonnegative cell modulo; negative row input is excluded before the unsafe
original column leaf. All query strings are live, proven NUL data. The prior
signed16 permutation gate bounds count<=32768 and binary-search signed sums.
Queries do not mutate stream state, owner counters, original index bytes or
the live TEMP header. Repeated reset/release restores logical ownership.

Existing162 loader adversarial cases,12 genuine cycles,12,857 actual hash vectors
with historical Windows controls and7,974 overflow vectors also pass per ABI.
Source/file boundaries remain10407/65678 before the next asset. This is a finite
checked reader adapter, not full asynchronous/refill/dispatch/registration or
the complete StringTable loader wrapper. No engine-owned graphics were achieved.

Authoritative debug: `stringtable_lookup_pass2`, host_pass2 and run1. Full ARM
zero errors,7312 retained warnings,291 AArch64 artifacts/31 executables.
Optimized `-O3` Debug with assertions/UBSan: optimized_pass1/run1,40 artifacts/
one executable, identical query results and miss counters. No new timing/RSS
claims; historical decoder and allocator measurements remain distinct.
All tests completed. Initial Windows host linking failed because an older
fixture defined CRT I_stricmp alongside the actual engine function (FAIL-0019).
That provider is excluded only under LOOKUP_GATE; older gates remain intact.
The failed linker diagnostics are retained. The first source-proof tool assumed
uniform endings for a newer LF source (FAIL-0021); corrected proof2 passes and
the qualified tool-error record remains preserved. These tool/link failures are
separate from the actual shared row misses.

Reproduce from this Android worktree with fresh output names:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
& tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/lookup_repro_debug
& tools/port/build_stringtable_lookup_host.ps1 -OutputDirectory build-android-audit/lookup_repro_host
& tools/port/build_stringtable_lookup_windows_compile.ps1 -OutputDirectory build-android-audit/lookup_repro_default
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_lookup_gate.py --android-build build-android-audit/lookup_repro_debug --host-build build-android-audit/lookup_repro_host --output build-android-audit/lookup_repro_run --serial AKWJ9X5116G04618
& tools/port/build_stringtable_lookup_optimized_arm64.ps1 -BuildDirectory build-android-audit/lookup_repro_opt
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_lookup_gate.py --android-build build-android-audit/lookup_repro_opt --host-build build-android-audit/lookup_repro_host --output build-android-audit/lookup_repro_opt_run --serial AKWJ9X5116G04618
```

The runner rejects all unknown/missing/extra/reordered171 records and fields,
compares actual results against the independent C++ source model and linear
reference, and records parity success only after all ABIs. Parity success does
not claim every genuine row lookup succeeds. Machine-readable records and
`functional-query-findings.json`, all commands/logs/failures/source and symbol
proofs are frozen under `evidence/arm64_stringtable_lookup/`. Exact paths are in
`arm64_stringtable_lookup_changes.md`. Small original prefixes, full raw device
assets, decoder buffers, binaries and control source stay ignored; existing raw
device files are SHA-verified/reused and complete decode ends before zone tests.
No APK install, push or protected checkout writes. Windows2528/DXVK2552/root15,
APK and2601 older frozen files are verified. Local checkpoints are authorized.

Next smallest task: establish SHA/address-backed original instructions for
FirstHashIndex, row lookup and CellCompare, and verify the loading-path finding
before any correction. If original behavior differs, implement the smallest
evidence-backed correction with original data and Windows/HONOR regression
tests. Then select actual first-asset ownership/registration with correct TEMP
lifetime. Full async, post-read OOM, general offsets, broader pool/allocator
multiowner and retained pages,16KB/S25 and renderer/graphics remain open.
