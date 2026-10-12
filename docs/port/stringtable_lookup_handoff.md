# Actual query checkpoint handoff

TASK-0023 query parity audit complete. Six real production lookup/comparison
bodies execute on HONOR and Windows unchanged. Both first original StringTable
payloads load through indices/hash/query while their header/zone remain live.
All ABIs match, including optimized ARM assertions/UBSan. Genuine row lookup
correctness is **not** complete:24/400 and796/1594 existing-value queries miss
on every ABI; all1,994 column queries agree with the linear reference. Data and
Windows behavior were preserved. This is reconstructed C++ behavior, not yet
verified original retail instructions (FAIL-0020).

262,180 actual comparison cases,22 controlled lookup cases,162 earlier loader
cases,12 genuine cycles and12,857 hash vectors pass per ABI. Full normal Windows
four TUs compile on x86/x64. Initial duplicate-fixture linker failure and source
proof assumption are fixed/retained; shared row misses remain open. No new
latency/RSS or engine-owned graphical output. Full async/loader wrapper/asset
dispatch/registration/init/rendering remain unvalidated. No in-scope parity
audit work is incomplete; functional discrepancy resolution remains next work.

See arm64_stringtable_lookup.md for exact HONOR commands and frozen query records,
functional findings, source/symbol proofs and exact path inventory. Current
Git/STATE supply checkpoint and later pending work. All scratch is retained,
Windows/DXVK/root/assets/older evidence protected. Local verified commits are
authorized, no push. A parity checkpoint does not conceal failed functional
queries or authorize resorting assets to force success.

Next three logical milestones:

1. Original-instruction audit: map FirstHashIndex, row caller and CellCompare in
   the SHA-verified retail executable; acceptance is exact PE VA/RVA/file offsets,
   bytes/instructions/xrefs and evidence distinguishing compare/subtract semantics
   plus FastFile postload/registration behavior. No speculative algorithm change.
2. Smallest evidence-backed query correction if original behavior differs.
   Acceptance: preserved original arrays/ABIs, original-behavior controls,
   exhaustive actual Windows/HONOR debug/optimized results and transparent limits.
3. Actual first-asset ownership/registration boundary. Acceptance: genuine
   production code, correct TEMP/persistent lifetime, native pointers/fixed disk
   layouts, failure/reset/release and Windows reference parity before more assets.

Do not enter original unsafe negative-row column queries or null/negative-prefix
comparison paths without a separate validated fix. Broad hash high-byte/locale,
TEMP multiasset reuse, async/OOM/general offsets/pool lifecycle, allocator
retention/multipleowner,16KB/S25 and graphics remain open.
