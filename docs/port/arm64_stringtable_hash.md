# TASK-0022 canonical production hash

The actual `Com_HashString` and unchanged `StringTable_HashString` wrapper now
execute on HONOR ARM64 with defined modulo32 arithmetic. All1,994 stored hashes
in the two genuine first StringTables match original Windows result bits.
This gate stops before actual hash-index queries, registration and rendering.

The single hash definition moved from `server_mp/sv_bot_mp.cpp` into
`universal/com_hash.cpp`. Its sole declaration moved to `com_hash.h`, included
at the original server-header declaration position. Existing six caller sites
remain intact and the canonical unit/header are in the explicit universal CMake
source list. No engine function was forked. The selected original StringTable
wrapper body remains byte-identical in `com_stringtable_obj.cpp`.

Only accumulation changes: a `uint32_t` accumulator, `33u` multiplication and
bit-preserving `memcpy` into the returned32-bit `int`. Null still returns0;
empty and negative-length nonnull strings return5381. Positive lengths still
bound prefixes, zero length means NUL terminated, and the original loop
evaluation order and `tolower(fname[i])` argument are retained. This fixes
signed-overflow UB without separately changing ctype behavior. Tests certify
defined ASCII in the default C locale. High-byte/signed-char and locale behavior
remain a distinct audit; they are not silently reinterpreted by this change.

12,857 vectors pass on Windows x86/x64 and HONOR: null, empty, negative/INT_MIN,
positive/max lengths, every defined nonzero ASCII byte, deterministic random
inputs, early NUL and long strings. 7,974 vectors exercise wrapping. The
independent oracle multiplies in64 bits then truncates and lowers ASCII directly.
Both Windows probes also call the exact historical MSVC `/O2` production body,
extracted from commit `1874245786ef55145c710d0016f693e123d66d1f` with only its
function name changed. That control lives only in ignored build output, is never
compiled into ARM, and has original-source/function/generated SHA evidence.
The actual wrapper is checked independently of each length-limited query.

Every genuine cell is hashed through the actual wrapper while its resolved
StringTable header and zone owner remain live. All400/1,594 stored signed hashes
match on each ABI and every one of12 allocation/release cycles. The original
index words/order remain unchanged. Existing162 adversarial cases and cleanup
checks also pass. File endpoints remain10407/65678 before the next asset.

Authoritative final strict runner results are `stringtable_hash_run2` and
`stringtable_hash_optimized_run2`; original successful run1 records are preserved.
Debug ARM build: zero errors,6877 retained warnings,267 AArch64 artifacts and30
executables. Optimized `-O3` Debug with assertions/UBSan:38 artifacts/one executable,
identical vector counts, result bits and genuine hashes. All tests completed.
Normal gate-disabled Windows x86/x64 compilation passes for full bot,
StringTable-object and canonical hash translation units, six objects total.

Source proof verifies unique definition/declaration, unchanged wrapper/default
source/global order and original callers; all other server source bytes remain
intact after the single function removal. Exact removal of the two added CMake
lines restores the captured mixed-ending baseline bytes, as well as canonical
Git content. Initial source-verifier attempts failed on uniform-ending/regex
assumptions (FAIL-0018); raw logs, argv and qualified transcription evidence are
retained. These were tool-proof failures, not native hash failures. Final proof4
passes. A reviewer requested strict fixture/summary record fields; final runner
adds exact whitelists and both final runs pass. Stream/expander bodies remain
unchanged. No sanitizer was disabled and no optimizer workaround was used.

Reproduce from the Android worktree using fresh directories:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
& tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/hash_repro_debug
& tools/port/build_stringtable_hash_host.ps1 -OutputDirectory build-android-audit/hash_repro_host
& tools/port/build_stringtable_hash_windows_compile.ps1 -OutputDirectory build-android-audit/hash_repro_default
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_hash_gate.py --android-build build-android-audit/hash_repro_debug --host-build build-android-audit/hash_repro_host --output build-android-audit/hash_repro_run --serial AKWJ9X5116G04618
& tools/port/build_stringtable_hash_optimized_arm64.ps1 -BuildDirectory build-android-audit/hash_repro_opt
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_hash_gate.py --android-build build-android-audit/hash_repro_opt --host-build build-android-audit/hash_repro_host --output build-android-audit/hash_repro_opt_run --serial AKWJ9X5116G04618
```

The runner verifies/reuses original raw device assets, transfers only fresh
probes, and separates complete genuine decode and zone processes. No duplicate
full decoded buffers, asset text/hex, original assets or binaries are committed.
No APK install or new latency/RSS claim. Historical TASK-0011 memory/timing
evidence remains separate. Frozen machine-readable measurements, source/control
proofs, symbols, command/log records and exact path inventory are in
`evidence/arm64_stringtable_hash/` and `arm64_stringtable_hash_changes.md`.
Windows2528 files, independent DXVK2552 files, root Ledger15 files, unchanged
APK and2397 prior frozen paths remain protected. Verified local checkpoint
commits are authorized; no push. Git/STATE identify the resulting checkpoint.

Next: select and execute actual `StringTable_FirstHashIndex` and simple row/
column lookup leaves with original indices, real `I_stricmp` and exhaustive
genuine queries on Windows/HONOR. FACT-0062's modeled24/400 and796/1,594 misses
remain models until then. Compare against a linear reference, explain any actual
shared misses, and preserve Windows behavior and original data. No resorting,
sorting rejection or fallback may be presented as a verified production query.
Full dispatch/registry, TEMP multiasset lifetime, asynchronous reads, post-read
OOM recovery, general offsets/pool lifecycle,16KB/S25 and graphics remain open.
