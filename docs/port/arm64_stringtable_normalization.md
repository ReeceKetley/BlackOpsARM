# TASK-0025: original StringTable normalization and actual query parity

Actual production normalization and existing query leaves now execute on HONOR
ARM64 and Windows x86/x64 with exact original index order. All1994 genuine row
and column queries agree, including duplicate choices and ASCII case changes.
The raw-order24/796 row misses from TASK23 are resolved at the original required
normalization boundary. Full asset registration is not yet executed by this gate.

## Minimal restoration

Load_StringTableAsset keeps its original DB_AddXAsset call and pointer replacement,
then normalizes the returned registered header. Invalid input reports ERR_DROP.
The placement follows SHA-pinned original instructions from TASK24: body load,
registration, returned header sort, inserted alias completion, TEMP Pop.
Com_stringtable_sort validates nonnegative dimensions, mathematical32768-cell
signed16 domain, span sizes/alignment/address wrapping/nonoverlap and every
permutation word before the first swap. Only indices change. The production
overload derives child extents under the existing live-registered-array caller
contract; it cannot establish allocation ownership or registry rollback.

The small platform/win32_compat/sort16 service reproduces the original Microsoft
CRT ordering specifically for this two-byte index domain on every platform.
It uses explicit context, guarded native-width ptrdiff_t positions and30 local
pending ranges, no heap allocation/global context/new row tie-break. The original
cutoff8, median swaps, pivot movement, equal-key trim, partition queue order and
strict shortsort maximum selection are retained. The independent instruction
audit and compiled Windows CRT controls verify duplicate-key order. This fixes
the HONOR CRT6/14 differing positions from TASK24 without replacing global qsort.
Same-table concurrent writes remain excluded by exclusive caller ownership.

LoadObj's wrapping comparator, all existing query/comparison/hash/type source,
actual Load_shortArray,163 expander and6 stream-loader bodies remain unchanged.
Source proof2 reverses exactly the added caller/include/source inventory and
test-selection changes against captured working bytes and Git. Full normal
Windows compilation covers seven translation units per ABI, including registry
and both new implementations, with gates disabled. Disk header20/native32,
cell8/native16, scalar stride2 and original four-byte tags/layouts remain intact.

## Verified execution

| Genuine first table | Original indices loaded unchanged | Normalized order SHA256 | Actual row misses / disagreements |
| --- | ---: | --- | ---: |
| code_pre_gfx.ff |400 /800 bytes|6f014d692dcef759efda4969da54ca950220385d0ba97f1236263d1c63aa571c|0 /0|
| zombie_pentagon.ff |1594 /3188 bytes|1443983124ffb7cfd3be7dc900bf6c1c21937c02874acca8b7bfc05cf5e2b203|0 /0|

Both final debug and optimized runs execute Windows x86/x64 and HONOR. Each
passes162 earlier loader cases,86 normalization cases including60 exact Windows
CRT controls,12857 hash vectors/7974 wrapping,262180 comparison cases and22
controlled queries. Two independent tables overlap16 normalization passes;
every permutation/header/cell byte remains correct. Controls include empty/one,
7/8/9 threshold, reverse/sorted/all equal/mixed duplicates, signed extrema,
random hashes and32768 cells. Malformed dimensions/count, missing/short spans,
alignment, address wrapping, overlapping arrays/header, negative/out-of-range
and duplicate words fail before array or actual passed-header mutation.

Twelve genuine cycles per ABI exercise both allocation types, actual original
leaves, live header scopes, resolved strings, hash, normalization and queries.
Raw scalar metadata is recorded before normalization. Full normalized indices
and every first_hash_index/row/column result match original reference outputs;
per-cycle zero misses/disagreements, owner counters, cursors, mappings and reset/
release remain verified. Native normalizer and query functions actually execute.
The original x86 row reference is a separately labeled digest-based model; original
FirstHashIndex and sorting instructions execute in the TASK24 analysis oracle.
Do not describe that host reference as an Android implementation or game emulator.

Strict174 ordered records are required. The reused terminal TASK23 schema covers
prerequisite fixtures; enclosing TASK25 schema and mandatory normalization records
distinguish this gate. Final builds: fullARM pass3 zero errors/7748 existing
warnings,319 AArch64 ELF files including CMake compiler checks; optimized pass2
zero errors/756 warnings,44 ELF files including checks. Normal Windows7TUs each
compile with no new implementation warnings. No new latency/RSS/memory-peak
measurements are claimed. Native full decode ends and releases its buffer before
zone allocation; only tiny validated prefixes and existing device FF cache are used.

## Exact reproduction

Run from BlackOpsARM in PowerShell; select fresh ignored output directories.

```powershell
$sortPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH=(Split-Path $sortPython)+';'+$env:PATH
& tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/stringtable_normalization_pass4
& tools/port/build_stringtable_normalization_host.ps1 -OutputDirectory build-android-audit/stringtable_normalization_host_pass4
& tools/port/build_stringtable_normalization_windows_compile.ps1 -OutputDirectory build-android-audit/stringtable_normalization_windows_compile_pass2
& tools/port/build_stringtable_normalization_optimized_arm64.ps1 -BuildDirectory build-android-audit/stringtable_normalization_optimized_pass3
& $sortPython -B tools/port/run_stringtable_normalization_gate.py --android-build build-android-audit/stringtable_normalization_pass4 --host-build build-android-audit/stringtable_normalization_host_pass4 --output build-android-audit/stringtable_normalization_run2 --serial AKWJ9X5116G04618
& $sortPython -B tools/port/run_stringtable_normalization_gate.py --android-build build-android-audit/stringtable_normalization_optimized_pass3 --host-build build-android-audit/stringtable_normalization_host_pass4 --output build-android-audit/stringtable_normalization_optimized_run2 --serial AKWJ9X5116G04618
& $sortPython -B tools/port/verify_stringtable_normalization_source.py --baseline build-android-audit/stringtable_normalization-baseline.json --output build-android-audit/stringtable_normalization_source_proof3.json
```

The optimized builder explicitly pins bundled Python. Preliminary configure
selected Anaconda despite PATH precedence; both successful preliminary builds
remain preserved. Final authoritative tests: debugpass3/hostpass3/run1 and
optimizedpass2/run1; fullWindowscompilepass1/sourceproof2. All tests completed.
FAIL24 preserves integration-tool LF anchor mismatch; FAIL25 preserves source
proof's invalid mixed-ending assumption. Both tooling errors were corrected;
no failed native normalization/query result. Preliminary/final logs and commands,
all earlier evidence and scratch remain retained. Frozen archive has129 copied
records plus review, source/binary hashes, preservation and build summaries.
No assets, original executable, prefixes, binaries or full decoded buffers enter Git.

## Remaining boundary and acceptance

The fixture supplies live zone spans and invokes the real normalizer; it does
not fabricate successful DB_AddXAsset or claim full registry execution. Actual
async/refill, dispatcher, registry initialization/publication/rollback, asset
ownership after TEMP Pop and game-owned graphics remain future work.
All3018 older frozen files, Windows2528/DXVK2552/root15/APK/originals unchanged.
Exact changed files and hashes are arm64_stringtable_normalization_changes.md
and evidence/arm64_stringtable_normalization/file_inventory.json. Verified local
checkpoint authorized, no push; Git/STATE identify later pending work.

Next TASK26: actual StringTable pool-header/clone ownership prerequisite. Use
real production pool/clone primitives, retain zone-owned children, pop and
overwrite TEMP storage, then prove both tables/all1994 queries survive. Exercise
pool exhaustion and balanced release without freeing block4 children. This is
a prerequisite to full registry certification, not substitute registration.
Next audit native TEMP shadow/maps/alias reuse against original stream semantics
before a second asset; then integrate the smallest real registry boundary with
actual DBHASH/initialization services. Existing native Pop retains TEMP shadow,
and historical first-header proofs certify only that limited scope. Read-only
Pop instructions retained at ignored ownership_boundary_audit_run1. Full async,
S25/16KB, hash locale/highbytes and historical retained-memory/multiowner limits
remain unverified. No genuine engine-owned graphical output yet.
