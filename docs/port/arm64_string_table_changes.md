# TASK-0014 pre-checkpoint file inventory

Baseline/current HEAD ffc4241, android-arm64. Relative to baseline: 12 durable modified, 138 durable created, one ignored Ledger SQLite index modified, no removals. Local verified checkpoint authorized DEC-0026; no push. This is the exact pre-checkpoint snapshot; later commits do not rewrite it.

All gate tests/reviews completed; overnight goal active. [Full machine inventory](evidence/arm64_string_table/file_inventory.json) lists every path/current status/ignored artifact. [Preservation](evidence/arm64_string_table/preservation.json) verifies1280 old frozen files/protected repos/original game files/index/APK.

## Modified

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
- tools/port/tests/checked_zone_support.cpp

## Created sources/tools/docs

- docs/port/arm64_string_table.md
- docs/port/arm64_string_table_changes.md
- src/platform/fastfile_script_table.cpp
- src/platform/fastfile_script_table.h
- tools/port/build_string_table_host.ps1
- tools/port/collect_string_table_evidence.py
- tools/port/run_string_table_gate.py
- tools/port/string_table_oracle.py
- tools/port/tests/frontend_zone_fixture.h
- tools/port/tests/string_table_tests.cpp

## Evidence and outstanding scope

128 files under docs/port/evidence/arm64_string_table: all passing/failed compiler/orchestration logs, independent prefix/tags/digests, native rows/fixtures, Windows regressions/source/body proof, symbol/source/artifact manifest and Ledger/protection/inventory. Exact names in machine inventory.

No incomplete implementation inside TASK-0014. Raw strings, real SL/MT pool, asset directory/dispatch, full async/refill/APC, post-read bad_alloc/fatal recovery and engine-owned graphics remain unvalidated. Goal next3: raw strings -> directory -> real pool. Legacy multi-owner PMem/full VM/Z/backend/16KB/S25 gaps retained. No game-owned graphics achieved.

All ignored string_table_pass1/2, host_pass1/2, run1/2, full-header check and independent Windows outputs remain. Original game files and binary prefixes/probes stay local/device, never staged. SQLite local index excluded; JSONL is durable Ledger. Prior checkpoint committed prior gates; current canonical progress records and source changes are captured above.
