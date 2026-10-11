# TASK-0012 uncommitted file inventory

Android remains android-arm64 at 6f0787ab08e2576a342964f0297c310801ec9d11. No commit/push. Relative to the task baseline: 10 durable files modified, 153 durable files created, one ignored Ledger SQLite index modified, no files removed. Previous TASK-0011 uncommitted changes are retained.

Implementation and tests are complete. No active test processes or TASK-0013 implementation. [Machine inventory](evidence/arm64_stream_init/file_inventory.json) records every new/modified path, cumulative Git status and ignored local artifacts; [preservation](evidence/arm64_stream_init/preservation.json) verifies protected checkouts and847 previous archive/handoff files. No shared engine files changed in this gate.

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

## New source, tools and documentation

- docs/port/arm64_stream_init.md
- docs/port/arm64_stream_init_changes.md
- docs/port/stream_init_handoff.md
- tools/port/build_stream_init_host.ps1
- tools/port/collect_stream_init_evidence.py
- tools/port/run_stream_init_gate.py
- tools/port/tests/stream_init_support.cpp
- tools/port/tests/stream_init_tests.cpp

## Frozen evidence

145 new files under docs/port/evidence/arm64_stream_init. The machine inventory lists every path: all initial/final compiler and runtime logs/commands, original metadata/checksum/header provenance,12-cycle rows and summaries per ABI, expected fallback diagnostics, prior regression logs, artifact/source hashes, production symbol check, Ledger health, protection check and this inventory. Old archives remain byte-identical.

## Local artifacts retained

Ignored build-android-audit/stream_init_pass1/pass2, stream_init_host_pass1/pass2, stream_init_run1/run2, stream-init-baseline.json, fresh host zone/narrow directories, original TASK-0011 decoder binaries and all earlier scratch remain. Binary extracted36-byte headers stay local/device only. The ignored Ledger database is a rebuilt local index; durable history lives in ledger.jsonl. No assets, binary headers, executables or SQLite index staged. The full current cumulative status includes older TASK-0011 work and is in the machine inventory.
