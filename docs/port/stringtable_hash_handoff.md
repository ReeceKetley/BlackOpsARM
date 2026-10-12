# Canonical hash checkpoint handoff

TASK-0022 complete. Actual canonical `Com_HashString`, unchanged StringTable hash
wrapper and earlier bounded loading leaves run on HONOR ARM64. Both first real
StringTable payloads are loaded through original indices; all1,994 stored hashes
match original Windows bits. The fixture reader is finite; full asynchronous
loader, dispatch/registration/zone initialization and graphics are not certified.
There is no engine-owned graphical output.

12,857 vectors (7,974 overflow) and162 adversarial cases/12 genuine cycles pass
per ABI, including both historical MSVC controls and optimized ARM assertions/
UBSan. Full normal Windows bot/StringTable/hash translation units compile on
x86/x64. Native tests pass; source-verifier assumptions failed before correction
and remain preserved separately. No new latency/RSS measurements were taken.
No in-scope hash work remains incomplete. Scratch/assets/prefixes/binaries and
original control source remain ignored and retained. See the report for exact
HONOR commands, frozen evidence and path inventory; current Git/STATE supply the
checkpoint SHA and any subsequent pending work. Local verified commits are
authorized, no push. Protected Windows/DXVK/root and all older evidence are intact.

Next three logical milestones:

1. Actual binary-index row/column lookup, retaining original retail indices and
   actual case-insensitive comparison. Acceptance: exhaustive1,994 existing
   queries, absent/collision/column boundary fixtures, Windows/HONOR identical
   results, linear-reference comparison and explanation of shared misses.
2. Resolve the reference ordering/lookup discrepancy using original code evidence
   if real execution confirms it; no speculative sorting or fallback. Acceptance:
   provenance-backed behavior, preserved disk data and verified Windows/ARM parity.
3. The smallest real first-asset ownership/registration boundary selected after
   lookup findings. Acceptance: actual production code, TEMP lifetime and native
   pointers, failure/release behavior and real Windows/HONOR tests before further
   asset dispatch or rendering.

High-byte/locale ctype behavior, broader hash callers at runtime, full async,
post-read OOM rollback, general offsets/pool lifecycles, retained allocator pages,
multiowner/TEMP reuse,16KB pages/S25 and graphics remain unvalidated. Never fake
query success, IDs, assets or frames.
