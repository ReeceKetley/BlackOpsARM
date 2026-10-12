# TASK-0020 — initialized cell strings and real offset conversion

The actual raw-string loader and `DB_ConvertOffsetToPointer` now initialize all
cell strings in both genuine first StringTables on HONOR and Windows x86/x64.
Each ABI passes 129 adversarial cases and 12 genuine low/high allocation cycles.
The optimized ARM build also passes with assertions and UBSan enabled.

| Original | Inline strings | Actual offset conversions | Inline source bytes | Source end before indices | Block4 disk/native64 end |
|---|---:|---:|---:|---:|---:|
| code_pre_gfx.ff |398|2|6005|9607|9537 /14388|
| zombie_pentagon.ff |1545|49|22415|62490|62423 /95724|

Every converted pointer equals an earlier successfully loaded inline cell string
in the same live zone. All original tags, resolved kinds, lengths, Adler digests
and disk/native target coordinates match the independent oracle. Inline loading
compares every byte against the SHA-validated original prefix, including its
first NUL; offsets reuse those already proven bytes without consuming input.

The complete bounded string frame is planned before zone allocation. Ordinary
tags must target an earlier validated inline start in block4. Runtime provenance
is recorded only after the actual raw read, byte/NUL checks and exact mapping
checks succeed. The guard checks owner identity and generation, logical disk and
native bounds, the recorded native address and first NUL at the declared end.
It scans that initialized extent, never unused reservation capacity, padding,
cell descriptors or zero-filled pages. Null0 remains absent; inline FFFFFFFF is
loaded; FFFFFFFE and other-block or unknown/interior/future starts are rejected.

Before the actual converter writes the **owned cell field**, its original widened
tag and actual exact mapping must match the proven target. Win32 uses the
corresponding base-plus-disk-address contract. Afterwards the pointer must equal
the recorded owned address. No drift fallback is accepted as initialization
proof. This certifies the exact earlier-start cases in these files, rather than
all possible FastFile suffix or other-block references.

Caller-owned records establish initialization and liveness; a pure bounds checker
cannot independently discover freed memory or distinguish forged trusted records.
Records are cleared while the owner is live, before reset or release; generations
change on reuse. Controlled cases exercise stale generations, ambiguous starts,
bad owner/address/bounds, missing/early NUL and mapping mismatch with zero converter
calls or slot mutation. Actual word and empty-string targets convert correctly.
No global allocator or engine source duplication was introduced.

The original parent4/header0/child4 stack keeps the header live. Its dimensions,
field aliases and unresolved index tag remain intact; TEMP pop restores native
block0 workspace0 while retaining native64 shadow20. Every owner returns to the
logical baseline after mapping detach and release. Further TEMP reuse, general
duplicate-alias ordering and transactional recovery after post-read allocation
failure remain unvalidated. The finite reader is a fixture, not full async/APC.

## Verification and evidence

Authoritative debug `stringtable_strings_run2` and optimized
`stringtable_strings_optimized_run1` pass all three ABIs. Fresh debug build:
0 errors, 6024 retained warnings, 225 AArch64 artifacts and 28 executables.
Optimized target: 34 AArch64 artifacts and one executable, `-O3 -g`, assertions
and nonrecovering UBSan. No sanitizer errors occurred on passing paths. All tests
finished. Host code uses actual Windows implementations with `/O2`.

All 163 actual expander bodies, two declaration headers and six stream-loader
bodies, including the converter, are unchanged against6e3528a. New code is the
bounded initialization guard plus test/build selection. Gate-off Windows
expander evidence from TASK-0019 remains valid because no production TU changed.
Existing four-byte Narrow32 and real859-ID pool evidence remains intact.

FAIL-0016 preserves a runner failure after native output validation: a coordinate
variable shadowed the native executable path, causing TypeError during final SHA
recording. It was renamed; debug run2 and the optimized run completed. Run1 is
retained as an overall failure. Its original source SHA was not captured; the
actual traceback and invocation/stdout/stderr records are preserved honestly.

`docs/port/evidence/arm64_stringtable_strings` freezes all failed/successful logs,
numeric per-cell evidence, independent references, source/symbol/architecture
hashes and review. Original asset text/hex, decoded buffers, FastFiles and
executables stay out of Git. Actual decoders run in separate processes and
release their single vector before zone tests. Small9607/62490-byte prefixes
and the already SHA-verified device originals are reused without extra raw copies.
Windows2528 files, independent DXVK2552, root Ledger15 and2123 older frozen paths
are unchanged. No APK install or game-owned graphics. No new timing/RSS claim;
TASK-0011 decoder/allocator measurements remain separately frozen. Logical release
does not imply instant recovery of allocator caches or resident pages.

## Original index contract finding

Read-only audit confirms valid permutations and matching stored hashes for all
400/1594 genuine cells, all ASCII. The indices are **neither signed nor unsigned
monotonically sorted by hash**. Signed inversions occur at386/1100; unsigned at
160/1454. Adjacent ordering matches an explicit wrapping32 model of `CellCompare`;
this supports an inference about sorting, not certification of the retail tool.

Independently modeling current `FirstHashIndex`/row lookup against a linear scan
of retail index order predicts24/400 and796/1594 existing-query misses. No native
lookup function has yet run. Preserve original indices and Windows behavior:
replace the earlier signed-sort prerequisite with recorded diagnostics, and
verify actual reference execution before diagnosing or changing lookup behavior.
Com_HashString's signed overflow/negative-char ctype and CellCompare's signed
subtraction remain issues for a future isolated UBSan lookup gate.

## Reproduce

From `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, choose fresh directories:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/stringtable_strings_repro1
./tools/port/build_stringtable_strings_host.ps1 -OutputDirectory build-android-audit/stringtable_strings_host_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_strings_gate.py --android-build build-android-audit/stringtable_strings_repro1 --host-build build-android-audit/stringtable_strings_host_repro1 --output build-android-audit/stringtable_strings_repro_run1 --serial AKWJ9X5116G04618
./tools/port/build_stringtable_strings_optimized_arm64.ps1 -BuildDirectory build-android-audit/stringtable_strings_optimized_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_strings_gate.py --android-build build-android-audit/stringtable_strings_optimized_repro1 --host-build build-android-audit/stringtable_strings_host_repro1 --output build-android-audit/stringtable_strings_optimized_repro_run1 --serial AKWJ9X5116G04618
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/audit_stringtable_indices.py --output build-android-audit/stringtable_index_readonly_repro1.json
```

Runner verifies original raw/full decoded/prefix/executable SHA, every fixture/
cycle/measurement and Windows x64/HONOR parity. Its original root is
`D:/Games/Call of Duty - Black Ops`, cached device manifest
`asset_directory_run1/results.json`. New device scratch is unique; older outputs
are retained. The index audit is read-only host modeling, explicitly separate
from actual native execution. Its source SHA and numeric findings are frozen.

DEC-0035, FACT-0060/0061/0062, FAIL-0016, WARN-0020. Local verified checkpoints
approved DEC-0026; no push. Exact paths: `arm64_stringtable_strings_changes.md`.

Next three: TASK-0021 original signed16 index bytes with range/permutation,
source/alignment/capacity/cleanup checks; isolate genuine hash arithmetic with
Windows bit equivalence; execute actual lookup leaves on both platforms and
investigate the modeled differences before registration. Keep original indices,
disk layouts and Windows behavior. Full async/VM/registry/backend, threaded pool,
multiple owners/TEMP reuse,16KB pages and S25 remain open.
