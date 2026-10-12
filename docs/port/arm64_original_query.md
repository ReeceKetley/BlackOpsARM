# TASK-0024: original StringTable registration and sorting audit

The original game normalizes StringTable indices after registration. The reconstructed
Load_StringTableAsset currently omits that step. The signed binary search agrees with
the original instructions; replacing its comparison would address the wrong cause.
This gate changes no engine source. TASK-0025 must restore the missing operation on
the returned registered header and preserve original duplicate-key ordering.

## Original evidence and ownership

Read-only BlackOps.exe SHA256:
`2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759`.
Locator run3 records PE VA/RVA/file offset, exact range hashes, instruction logs,
calling conventions and call references. Original inline pointer loader0x604f50
loads body0x5e53e0 then calls wrapper0x53edd0. That wrapper passes type37 to
DB_AddXAsset candidate0x7a3330, stores the returned pointer, then tail-calls
signed sort0x5c03f0. Inserted alias completion and TEMP Pop occur afterward.
Sort the returned header: an unconditional sort of transient input before
registration would violate this observed ordering.

FirstHashIndex0x8604b0 uses signed CMP/JGE as reconstructed. Comparator0x860500
orders signed hashes with normalized -1/+1, then column modulus. Equal hash and
column compare zero. It uses global0x26050d8. The separate subtraction comparator
0x860560/global0x26050dc belongs to the parsing/LoadObj path. Preserve that path.
Source db_registry.cpp Load_StringTableAsset lacks the first comparator's sort.
Do not add a row/index tie-break, change the query lower bound or claim the
original game has the raw-index reconstruction discrepancy.

## Actual instructions, native experiments and models

The bounded host analysis oracle runs six unchanged SHA-pinned x86 instruction
ranges: FirstHashIndex, signed comparator, sort helper, qsort0x967090,
shortsort0x967000 and swap0x966fd0. It validates instruction/write/time bounds,
caller cleanup, nonvolatile registers, untouched table/cell hashes and cleared
comparator global. There are no service stubs. This is an isolated reference
analysis, never an Android engine implementation. DB_AddXAsset and the original
row function are not executed. Per-query rows are explicitly source scan models
using actual original FirstHashIndex results and independently hashed strings.

| Genuine first table | Cells | Raw row model misses | After original sort | HONOR CRT differing positions / modeled rows |
| --- | ---: | ---: | ---: | ---: |
| code_pre_gfx.ff |400|24|0|6 /6|
| zombie_pentagon.ff |1594|796|0|14 /14|

Both actual Windows x86/x64 std::qsort experiments reproduce the exact original
normalized permutation. Actual HONOR Bionic sorting is valid by comparator but
orders equal keys differently; it has zero modeled misses yet6/14 modeled row
differences. FAIL-0023 records this portability contract. These are native sort
results plus modeled query effects, not restored engine query execution.
Original sort instruction counts170106/715731 are not latency measurements.
Duplicate-key groups8/13, largest2/18. Exact sorted index SHA256:
code `6f014d692dcef759efda4969da54ca950220385d0ba97f1236263d1c63aa571c`;
zombie `1443983124ffb7cfd3be7dc900bf6c1c21937c02874acca8b7bfc05cf5e2b203`.

Read-only independent literal integer-position algorithm audit reproduced both
permutations exactly. Original cutoff<=8; strict median swaps; asymmetric pivot
scans and movement; equal-pivot trimming; queue larger partition, process smaller
(equal sizes queue left); shortsort selects first equal maximum. Use guarded
ptrdiff_t positions rather than before-begin pointers. The signed16 domain
supports32768 entries. Original30 pending slots exceed the conservative15-range
bound for that domain. TASK25 will verify the implementation independently.

## Reproduce the authoritative tests

Run PowerShell from the Android worktree. Output directories must be fresh.
Unicorn2.1.4 is read only in the existing Windows build/deps/water-oracle analysis
dependency. Python -B prevents protected bytecode writes. Original files are read
only; numeric metadata comes from genuine fully validated decoded header/payload
provenance, not fabricated allocation descriptors.

```powershell
$queryPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $queryPython -B tools/port/run_original_stringtable_reference.py --locator build-android-audit/original_query_locator_run3/results.json --lookup-results build-android-audit/stringtable_lookup_run1/results.json --output build-android-audit/original_stringtable_reference_run4
& tools/port/build_stringtable_crt_sort_probe.ps1 -OutputDirectory build-android-audit/stringtable_crt_sort_build3
& $queryPython -B tools/port/run_stringtable_crt_sort_probe.py --build build-android-audit/stringtable_crt_sort_build3 --output build-android-audit/stringtable_crt_sort_all_run3 --android --serial AKWJ9X5116G04618
```

Latest successful evidence is locatorrun3/reference run3/CRTbuild2/allrun2.
All native and oracle tests have finished. FAIL22 preserves reference run2's
incorrect tool seed0 instead of5381, corrected before run3. Collector attempt1
wrong binary basename and attempt2 line-ending resume assumptions are retained;
the resumed collector validates existing evidence bytes, never overwrites it.
No new production performance/RSS/decode measurements are claimed.

## Preservation and next gate

Machine-readable outputs, commands, logs, source hashes, provenance, review and
280 copied records are frozen in evidence/arm64_original_query. Collector attempt2
and independent sort audit supplement that manifest. All2722 older frozen files,
Windows2528/DXVK2552/root Ledger15 files, original executable/assets and APK remain
unchanged. No asset, executable, numeric input, decompressed buffer or native binary
is committed. Ignored builds, failed diagnostics and scratch remain available.

TASK25 acceptance: restore the post-registration normalized ordering with a bounded
Microsoft-compatible signed16 sorting service; preserve native Windows CRT behavior,
LoadObj subtraction comparator and four-byte disk ABI. Execute actual production
normalizer plus existing queries on HONOR/Windows, exact original permutations and
all1994 query outcomes, controlled duplicate/extreme/domain failures, optimized
assertions/UBSan, source preservation, owner cleanup and protected snapshots.
Full registry execution/publication remains a distinct subsequent boundary.
TEMP block0 shadow/rewind/aliases and persistent registration ownership must be
audited before another asset. Async refill, full dispatch, zone init, graphics,
S25/16KB pages and historical pool/retained-memory limits remain open.
