# Independent read-only audits — TASK-0011

Lead: /root, owns implementation, builds, device/host tests, integration and
Ledger. Reviewers made no edits, builds, commits or Ledger changes.

Windows policy reviewer: /root/zone_policy_audit. Checked actual db_memory.cpp
and physicalmemory.cpp, genuine retained oracles, native debug Hunk/first-fit
layout, checked planner and production Windows probe. Confirmed existing
200/190/135 plus65536 policy, +15 nonphysical requests, alignment4096, exact
logical/backend totals and low/high cursor arithmetic. Windows pool328/492MiB
uses MEM_COMMIT, bump allocations do not touch payloads, release retains pool.
Native mappings protect then memset requests. Single-owner reference avoids the
unvalidated native PMem_FreeIndex previous-entry8-byte stride; signed freeTot
diagnostic reset is fixture-only. Native64 plan/source provenance passes.
Empty tracker callbacks match disabled production tracking; g_mem supplies
ownership observations. Reviewer recommended DB_InitStreams as next boundary.

Adversarial/methodology reviewer: /root/checked_zone_review. Checked genuine
metadata source, overflow/alignment/derived-plan checks, rollback, timing and
process-memory attribution. Required separate decoder/allocation processes,
raw samples with31 warm after3 warmups /20 process-cold, monotonic clock and
explicit HWM/RSS limitations. Genuine block sum/externalSize must not be inferred
from decoded payload length. Seven32-bit fields cannot overflow uint64; test
capacity crossing32-bit separately from the checked-add primitive.

Findings corrected before final measurements:

- Persistent sampler reset could publish an earlier iteration's RSS afterward.
  Sampling/reset/include now synchronize on an epoch mutex; synchronous endpoint
  snapshots provide a peak lower bound. Final metrics use measure_pass2.
- Native Begin previously published active before potentially throwing map
  registration. It now publishes only after insert and destroys the Hunk on
  exception; a controlled post-reservation bad_alloc fixture verifies cleanup.
  mmap registration exceptions also unmap rather than orphan their mapping.
- Rollback release previously allocated a temporary string key. It now scans
  existing keys without allocation. Reviewer explicitly confirmed resolution.
- Unrelated48MiB debug startup arena removed from this allocation-only probe.
  Windows initial retained-pool transition is recorded separately from raw
  counter equality; logical rewind remains required. Warm metadata uses actual
  configured counts. Begin(false)/throw is atomic; End/Release nonthrowing.

Both reviews found expansion, alignment/tail capacity and genuine-header chain
sound. No remaining integration blocker from these reviews. total_phase_ns is
the sum of three timed sections, not complete wall-clock. Observed process RSS
is not exact heap usage; QPC100ns /1ms sampling limitations remain documented.
No new stream-init, parsing, fixup or rendering boundary enters this gate.
