# TASK-0021 original StringTable indices

The first genuine StringTable payload in each FastFile now reaches its original
signed16 index array on HONOR ARM64 and Windows x86/x64. This gate stops before
hash queries, asset dispatch, registration, zone initialization or rendering.
The finite fixture reader calls actual production leaves; it does not certify
the asynchronous FastFile refill pipeline or a full `Load_StringTable` wrapper.

| Original file | Index count / bytes | Decoded source interval | Block4 disk interval | Native64 interval |
|---|---:|---:|---:|---:|
| code_pre_gfx.ff | 400 / 800 | 9607–10407 | 9538–10338 | 14392–15192 |
| zombie_pentagon.ff | 1594 / 3188 | 62490–65678 | 62424–65612 | 95728–98916 |

Disk alignment uses the original mask1. Native64 retains the existing minimum
eight-byte stream alignment. Scalar stride stays two bytes on every ABI.
Win32 native intervals equal disk intervals. Alignment consumes no file bytes.
The original index SHA256 values are
`0b311eaa43dffe461622242729dd0ac307d61fd2673f8c4cb7d7ff66f615b65d` and
`06337fe408654d1cc0dc136f7f26e372310b2b969f662cfc8310f2ccf9bac7ac`.
Complete raw/decoded file SHA and Adler provenance is in the frozen runner
results. Original assets remain read-only and outside Git.

`Load_shortArray` is the unique original body in `db_load.cpp`, selected for the
probe with `BO1_STRINGTABLE_INDEX_GATE`. Its body and the default global order
are unchanged. The canonical prototype is moved once into `db_scalar_load.h`.
The source proof reverses only the documented selection/include changes to
recover the exact parent loader/header bytes. Normal full `db_load.cpp`
translation units also compile on Windows x86/x64 with the gate disabled.

The pure guard validates signed dimensions/product, signed read/commit sizes,
source, logical capacities, disk/native alignment and unsigned32 endpoints
before owner allocation. A present array must be a permutation of its cell
indices. The 32768 limit follows from nonnegative signed16 indices; it is not
an arbitrary allocator or expander limit. Absent roots retain legacy presence
semantics without reads/alignment. Present zero arrays retain alignment checks.
All nonzero root tags denote presence at this scalar boundary.

Actual loading checks every stored word and byte, byte-exact scalar mappings,
absence of pointer aliases, source/cursor endpoints and original order. The
header field is published only after the leaf succeeds. `varshort` is cleared
on success and throwing fixture returns, before stream reset and owner release.
The real TEMP header scope remains live during child loading; its aliases are
transient loading metadata, not registered persistent asset ownership.

162 cases and 12 genuine low/high allocation/release cycles pass per ABI.
The extra 33 index cases include 26 pure checks and seven actual cases:
alignment offsets0/1 with counts0/1/6, plus an 11-of-12-byte short read. Negative,
out-of-range, duplicate and full-domain permutations, truncation, invalid source,
alignment exhaustion, dimensions and read/endpoint overflow are covered. The
short-read test seeds an earlier real scalar commit and persistent envelope
alias, then compares existing/prospective mappings, bytes, stream state, reader
position and owner counters before reset. Cleanup returns logical counters to
baseline. No new allocation latency/RSS claim is made; TASK-0011 measurements
remain the historical memory/performance baseline.

Authoritative debug builds are `stringtable_indices_pass4` and
`stringtable_indices_host_pass4`; `stringtable_indices_run1` passes all three
ABIs. Full ARM build has zero errors, 6449 retained warnings, 245 AArch64
artifacts and29 executables. Optimized `-O3` ARM with assertions and UBSan also
passes all three ABI comparisons: `stringtable_indices_optimized_pass1/run1`,
36 artifacts and one executable. Earlier pass1/2 compiled before the final
fixture enhancement. Pass3 failed on a missing fixture helper declaration
(FAIL-0017), corrected by a fixture-only forward declaration; all diagnostics
remain frozen. Review also caught and corrected a string endpoint check before
authoritative execution. No failed compile is presented as a passing test.

Reproduce from this Android worktree using fresh output directory names:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
& tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/indices_repro_debug
& tools/port/build_stringtable_indices_host.ps1 -OutputDirectory build-android-audit/indices_repro_host
& tools/port/build_stringtable_indices_windows_compile.ps1 -OutputDirectory build-android-audit/indices_repro_default
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_indices_gate.py --android-build build-android-audit/indices_repro_debug --host-build build-android-audit/indices_repro_host --output build-android-audit/indices_repro_run --serial AKWJ9X5116G04618
& tools/port/build_stringtable_indices_optimized_arm64.ps1 -BuildDirectory build-android-audit/indices_repro_optimized
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_indices_gate.py --android-build build-android-audit/indices_repro_optimized --host-build build-android-audit/indices_repro_host --output build-android-audit/indices_repro_opt_run --serial AKWJ9X5116G04618
```

The runner SHA-verifies and reuses the original device files through the retained
TASK-0016 results. It pushes only fresh probes; each full genuine decode finishes
and releases its buffer before the zone allocation process. Only small selected
prefixes are retained locally. No APK is installed. Raw strings, prefixes,
binaries and scratch remain ignored. Machine-readable records, invocation
commands, stdout/stderr, failures, source proofs, symbols and inventory are under
`evidence/arm64_stringtable_indices/`.

Windows2528 files, independent DXVK2552 files, root Ledger15 files, unchanged APK
and2271 prior frozen paths are verified. The exact new change inventory is
`arm64_stringtable_indices_changes.md`. Local verified checkpoints are authorized
by DEC-0026; no push. Later Git/STATE supply the actual checkpoint SHA.

Next smallest boundary: execute a canonical `Com_HashString` leaf with explicit
modulo32 arithmetic verified against original Windows bits and every genuine
cell hash, including optimized ARM UBSan. Preserve its public signature and
length/null/ASCII behavior. Then execute the actual hash-index/row lookup leaves
with the original index arrays on both hosts and HONOR. FACT-0062's predicted
misses are still models, not native results. Do not sort/reject the original
arrays or change lookup algorithms to manufacture parity. Native reference
execution must explain the observed ordering first. Asset registration and the
next real asset remain subsequent gates.
