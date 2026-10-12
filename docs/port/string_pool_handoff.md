# Interning milestone handoff — 12 October 2026

The approved structural front end and real interning stretch are verified.
Production SL/MT and Load_TempStringCustom run on HONOR and Windows x86/x64,
producing the same859 genuine IDs. Owned counters/hash state clear before reset;
retained diagnostics match Windows. Empty/null, collisions, long strings,
reference transfer, fatal length rejection and optimized ARM paths pass.
No asset payloads, full async loader, VM/gameplay or game-owned graphics run yet.

Read arm64_string_pool.md for exact reproduction, source changes, failures and
limits; arm64_string_pool_changes.md contains every modified/untracked path.
Prior checkpoint7cfc24c; current Git log/STATE supersede this frozen historical
pointer after the verified local checkpoint. No push. Windows/DXVK/root and
older evidence preserved, all tests finished, ignored scratch kept.

Next TASK18 acceptance: first genuine StringTable20-byte disk/32-byte native64
header on HONOR/x86/x64 using actual leaf and valid original metadata; dimensions
1x400/2x797, every tag/field/alias/cursor and block0 Pop behavior, adversarial
bounds/count/short-read and repeated detach/release. Stop before name/cells/index/
fixups/registry. Then name/cell-array expansion; actual encoded cell-string
conversion with initialized-range proof before index/registration. No all-inline
shortcut: the real first cell arrays contain2/49 ordinary offsets.
