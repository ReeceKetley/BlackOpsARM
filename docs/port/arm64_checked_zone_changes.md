# TASK-0011 uncommitted file inventory

Android remains android-arm64 at 6f0787ab08e2576a342964f0297c310801ec9d11. No commit/push. 14 tracked files modified; 93 durable files created, including frozen evidence. No files removed. Ignored Ledger index/build/binary/header artifacts remain local; no original game bytes archived.

Implementation and all tests are complete. TASK-0012 is recommended only. [Full machine inventory](evidence/arm64_checked_zone/file_inventory.json) records every untracked evidence path, ignored artifacts and exact Git status. [Protected-state check](evidence/arm64_checked_zone/preservation.json) confirms Windows/DXVK/root/index/old archives.

## Modified tracked files

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
- src/platform/android/virtual_memory.cpp
- src/platform/android/zone_memory.cpp
- src/platform/android/zone_memory.h
- tools/port/tests/hunk_test_support.cpp

## New source, tools and documentation

- docs/port/arm64_checked_zone.md
- docs/port/arm64_checked_zone_changes.md
- src/platform/fastfile_zone.cpp
- src/platform/fastfile_zone.h
- tools/port/build_checked_zone_host.ps1
- tools/port/collect_checked_zone_evidence.py
- tools/port/measure_checked_zone.py
- tools/port/tests/checked_zone_support.cpp
- tools/port/tests/checked_zone_tests.cpp

## Evidence and local artifacts

84 new evidence files under docs/port/evidence/arm64_checked_zone: full raw measurements/commands, initial superseded metrics, all compiler attempts, failure/fixture/regression/provenance logs, source/artifact hashes and ELF/PE/symbol verification, independent audits, preservation and this inventory.

Controlled failures are labeled separately from actual terminal backend exhaustion. All previous763 archive hashes remain intact. Ignored source inputs/binaries/derived36-byte header files and per-process measurement logs remain in the listed local build/device paths. Auto-updated .ledger/ledger.db is an ignored cache; .ledger/ledger.jsonl is durable memory. All gate source/tool/docs/Ledger/evidence changes listed here remain uncommitted.
