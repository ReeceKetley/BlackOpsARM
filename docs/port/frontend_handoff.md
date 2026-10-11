# FastFile front-end handoff — 12 October 2026

Both genuine files now execute complete production decoding, checked zone
allocation, stream/envelope initialization, pointer-table expansion, bounded raw
string loading and asset-directory expansion on HONOR ARM64 and Windows x86/x64.
The directory contains 37/1045 records. All 47 fixtures and 12 repeated allocation
cycles per ABI pass; source ends at 348/27245 bytes and cleanup restores counters.
No asset payloads, interned IDs, gameplay or game-owned graphics run yet.

The two recursive locks and four-byte atomics needed by the real string pool
also pass on all three ABIs: 80,000 atomic and 80,000 protected updates. A review
found a thread-lifetime error in the original identity probe; its evidence is
preserved. The corrected live-thread probe and expected fatal cases pass.

The next task is TASK-0017: execute the actual SL/MT pool, verify deterministic
IDs and bytes against Windows, exercise empty/duplicate/high-byte/collision and
length-boundary strings, and verify user4 shutdown/reinitialization restores live
allocation and debug counters. Then feed the 859 genuine strings in their exact
order. Keep unsupported VM callbacks explicit failures. Acceptance requires
HONOR and Windows results, independent evidence, no fake IDs and preserved ABI.

The following two tasks are the first genuine StringTable header in block0, then
bounded name/cell/index loading before fixups and registration. The first asset
in both directories is a StringTable; do not skip ahead to a convenient asset.

Exact commands, limitations and file inventory are in arm64_asset_directory.md,
arm64_script_string_services.md and arm64_asset_directory_changes.md. Previous
pointer/raw checkpoints are 583b976 and da7f496; use Git and .ledger/STATE.md for
the latest checkpoint and pending work. Local verified commits are authorized;
no push. Windows, DXVK-Native, root Ledger and older evidence are preserved.
All tests have finished. Scratch and failed diagnostics remain. No new timing or
RSS comparison is claimed; the separate TASK-0011 measurements remain valid.
