# TASK-0013 uncommitted file inventory

Android remains android-arm64 at 6f0787ab08e2576a342964f0297c310801ec9d11. No commit/push. Relative to task baseline: 12 durable files modified, 293 durable files created, one ignored Ledger SQLite index modified, no files removed. Earlier TASK-0011/0012 changes remain uncommitted.

Bounded implementation, all tests and two read-only reviews are complete. TASK-0014 has not started. [Machine inventory](evidence/arm64_assetlist/file_inventory.json) records every new/modified file, full cumulative status and retained ignored local artifacts. [Preservation](evidence/arm64_assetlist/preservation.json) verifies Windows/DXVK/root, Android index/HEAD/APK and997 prior archived report/handoff files.

## Modified in this task

- .gitignore
- .ledger/AGENT-INSTRUCTIONS.md
- .ledger/DECISIONS.md
- .ledger/GOAL.md
- .ledger/STATE.md
- .ledger/ledger.jsonl
- AGENTS.md
- android/cmake/arm64-audit/CMakeLists.txt
- docs/port/android_arm64.md
- docs/port/roadmap_headless.md
- src/database/db_expand.cpp
- src/database/db_registry.h

## New source, tools and documents

- docs/port/arm64_assetlist.md
- docs/port/arm64_assetlist_changes.md
- docs/port/assetlist_handoff.md
- src/database/db_assetlist_types.h
- src/platform/fastfile_assetlist.cpp
- src/platform/fastfile_assetlist.h
- tools/port/build_assetlist_host.ps1
- tools/port/check_assetlist_headers.ps1
- tools/port/collect_assetlist_evidence.py
- tools/port/run_assetlist_gate.py
- tools/port/tests/assetlist_layout_probe.cpp
- tools/port/tests/assetlist_tests.cpp
- tools/port/verify_assetlist_source.py

## Frozen evidence

280 new files under docs/port/evidence/arm64_assetlist; exact paths are in the machine inventory. All passing build/run attempts, optional failed header linkage/graphics diagnostics, corrected source proof, agent reviews, independent full-decode prefix oracle, native results/commands/metadata, prior regressions, symbol/source/artifact provenance, normal Windows compile, Ledger health and final inventory/protection checks are retained. No originals or binary prefixes archived.

## Incomplete/deferred and local artifacts

No incomplete work within TASK-0013 or active test processes. Full async/refill reader, post-read bad_alloc/fatal OOM recovery, strings/asset parsing/fixups/zone init, full Android expander, legacy multi-owner PMem and actual16KB/S25 remain deferred. The finite read adapter is test-only. The shared change moves exact existing declarations and adds test compilation selection; production helper/envelope bodies are unchanged.

Ignored assetlist_pass1..3, assetlist_host_pass1..3, assetlist_run1..3, assetlist_headers1..4, fresh stream/checked host and genuine regression output, assetlist-baseline.json and fresh host zone/narrow outputs remain local. All prior scratch and unique device directories remain; no deletion/reset. SQLite ledger.db is local rebuilt index; ledger.jsonl is durable history. No files were staged.
