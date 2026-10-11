# TASK-0009 exact file inventory

Against the captured start-of-turn zone-baseline.json, after the user's TASK-0007/0008 work. No files removed. No commit or push; all durable changes remain unstaged.

Modified durable files (18):

- `.gitignore`
- `.ledger/AGENT-INSTRUCTIONS.md`
- `.ledger/DECISIONS.md`
- `.ledger/GOAL.md`
- `.ledger/STATE.md`
- `.ledger/ledger.jsonl`
- `AGENTS.md`
- `android/cmake/arm64-audit/CMakeLists.txt`
- `docs/port/android_arm64.md`
- `docs/port/roadmap_headless.md`
- `src/database/db_file_load.h`
- `src/database/db_memory.cpp`
- `src/database/db_registry.h`
- `src/database/db_stream.cpp`
- `src/database/db_stream.h`
- `tools/port/collect_script_header_evidence.py`
- `tools/port/run_arm64_audit_tests.ps1`
- `tools/port/tests/hunk_test_support.cpp`

Created durable files (153):

- `docs/port/arm64_zone.md`
- `docs/port/arm64_zone_changes.md`
- `docs/port/evidence/arm64_zone/aarch64_elf_headers.txt`
- `docs/port/evidence/arm64_zone/android_evaluator_test_services.cpp`
- `docs/port/evidence/arm64_zone/apk-recheck.log`
- `docs/port/evidence/arm64_zone/compiler_summary.json`
- `docs/port/evidence/arm64_zone/debugger_host_x64.debugger.command.txt`
- `docs/port/evidence/arm64_zone/debugger_host_x64.debugger.compile.log`
- `docs/port/evidence/arm64_zone/debugger_host_x64.debugger.test.log`
- `docs/port/evidence/arm64_zone/debugger_host_x64.firstfit.command.txt`
- `docs/port/evidence/arm64_zone/debugger_host_x64.firstfit.compile.log`
- `docs/port/evidence/arm64_zone/debugger_host_x64.firstfit.test.log`
- `docs/port/evidence/arm64_zone/debugger_host_x86.debugger.command.txt`
- `docs/port/evidence/arm64_zone/debugger_host_x86.debugger.compile.log`
- `docs/port/evidence/arm64_zone/debugger_host_x86.debugger.test.log`
- `docs/port/evidence/arm64_zone/debugger_host_x86.firstfit.command.txt`
- `docs/port/evidence/arm64_zone/debugger_host_x86.firstfit.compile.log`
- `docs/port/evidence/arm64_zone/debugger_host_x86.firstfit.test.log`
- `docs/port/evidence/arm64_zone/device-tests.log`
- `docs/port/evidence/arm64_zone/diff-check.log`
- `docs/port/evidence/arm64_zone/fastfile_host_x64.command.txt`
- `docs/port/evidence/arm64_zone/fastfile_host_x64.compile.log`
- `docs/port/evidence/arm64_zone/fastfile_host_x64.test.log`
- `docs/port/evidence/arm64_zone/fastfile_host_x86.command.txt`
- `docs/port/evidence/arm64_zone/fastfile_host_x86.compile.log`
- `docs/port/evidence/arm64_zone/fastfile_host_x86.test.log`
- `docs/port/evidence/arm64_zone/final-diff-check.log`
- `docs/port/evidence/arm64_zone/host_evaluator_test_services.cpp`
- `docs/port/evidence/arm64_zone/host_x64.engine_rng.command.txt`
- `docs/port/evidence/arm64_zone/host_x64.engine_rng.compile.log`
- `docs/port/evidence/arm64_zone/host_x64.engine_rng.test.log`
- `docs/port/evidence/arm64_zone/host_x64.math_axis.command.txt`
- `docs/port/evidence/arm64_zone/host_x64.math_axis.compile.log`
- `docs/port/evidence/arm64_zone/host_x64.math_axis.test.log`
- `docs/port/evidence/arm64_zone/host_x64.profile.command.txt`
- `docs/port/evidence/arm64_zone/host_x64.profile.compile.log`
- `docs/port/evidence/arm64_zone/host_x64.profile.test.log`
- `docs/port/evidence/arm64_zone/host_x64.rng.command.txt`
- `docs/port/evidence/arm64_zone/host_x64.rng.compile.log`
- `docs/port/evidence/arm64_zone/host_x64.rng.test.log`
- `docs/port/evidence/arm64_zone/host_x64.script_headers.command.txt`
- `docs/port/evidence/arm64_zone/host_x64.script_headers.compile.log`
- `docs/port/evidence/arm64_zone/host_x64.script_headers.test.log`
- `docs/port/evidence/arm64_zone/host_x64.script_lookup.command.txt`
- `docs/port/evidence/arm64_zone/host_x64.script_lookup.compile.log`
- `docs/port/evidence/arm64_zone/host_x64.script_lookup.test.log`
- `docs/port/evidence/arm64_zone/host_x86.engine_rng.command.txt`
- `docs/port/evidence/arm64_zone/host_x86.engine_rng.compile.log`
- `docs/port/evidence/arm64_zone/host_x86.engine_rng.test.log`
- `docs/port/evidence/arm64_zone/host_x86.math_axis.command.txt`
- `docs/port/evidence/arm64_zone/host_x86.math_axis.compile.log`
- `docs/port/evidence/arm64_zone/host_x86.math_axis.test.log`
- `docs/port/evidence/arm64_zone/host_x86.profile.command.txt`
- `docs/port/evidence/arm64_zone/host_x86.profile.compile.log`
- `docs/port/evidence/arm64_zone/host_x86.profile.test.log`
- `docs/port/evidence/arm64_zone/host_x86.rng.command.txt`
- `docs/port/evidence/arm64_zone/host_x86.rng.compile.log`
- `docs/port/evidence/arm64_zone/host_x86.rng.test.log`
- `docs/port/evidence/arm64_zone/host_x86.script_headers.command.txt`
- `docs/port/evidence/arm64_zone/host_x86.script_headers.compile.log`
- `docs/port/evidence/arm64_zone/host_x86.script_headers.test.log`
- `docs/port/evidence/arm64_zone/host_x86.script_lookup.command.txt`
- `docs/port/evidence/arm64_zone/host_x86.script_lookup.compile.log`
- `docs/port/evidence/arm64_zone/host_x86.script_lookup.test.log`
- `docs/port/evidence/arm64_zone/ledger-doctor.log`
- `docs/port/evidence/arm64_zone/ledger-status.log`
- `docs/port/evidence/arm64_zone/narrow_x64.compile.log`
- `docs/port/evidence/arm64_zone/narrow_x64.guard-compile.log`
- `docs/port/evidence/arm64_zone/narrow_x64.guard-off-compile.log`
- `docs/port/evidence/arm64_zone/narrow_x64.slot-compile.log`
- `docs/port/evidence/arm64_zone/narrow_x64.slot-test.log`
- `docs/port/evidence/arm64_zone/narrow_x64.test.log`
- `docs/port/evidence/arm64_zone/narrow_x86.compile.log`
- `docs/port/evidence/arm64_zone/narrow_x86.guard-compile.log`
- `docs/port/evidence/arm64_zone/narrow_x86.slot-compile.log`
- `docs/port/evidence/arm64_zone/narrow_x86.slot-test.log`
- `docs/port/evidence/arm64_zone/narrow_x86.test.log`
- `docs/port/evidence/arm64_zone/parse_host_x64.hunk.command.txt`
- `docs/port/evidence/arm64_zone/parse_host_x64.hunk.compile.log`
- `docs/port/evidence/arm64_zone/parse_host_x64.hunk.test.log`
- `docs/port/evidence/arm64_zone/parse_host_x64.parsetree.command.txt`
- `docs/port/evidence/arm64_zone/parse_host_x64.parsetree.compile.log`
- `docs/port/evidence/arm64_zone/parse_host_x64.parsetree.test.log`
- `docs/port/evidence/arm64_zone/parse_host_x86.hunk.command.txt`
- `docs/port/evidence/arm64_zone/parse_host_x86.hunk.compile.log`
- `docs/port/evidence/arm64_zone/parse_host_x86.hunk.test.log`
- `docs/port/evidence/arm64_zone/parse_host_x86.parsetree.command.txt`
- `docs/port/evidence/arm64_zone/parse_host_x86.parsetree.compile.log`
- `docs/port/evidence/arm64_zone/parse_host_x86.parsetree.test.log`
- `docs/port/evidence/arm64_zone/preservation.json`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x64.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x64.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x64.test.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x86.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x86.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x86.test.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x64.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x64.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x64.test.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x86.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x86.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x86.test.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x64.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x64.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x64.test.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x86.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x86.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x86.test.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x64.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x64.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x64.test.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x86.command.txt`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x86.compile.log`
- `docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x86.test.log`
- `docs/port/evidence/arm64_zone/retained.zone-baseline.json`
- `docs/port/evidence/arm64_zone/retained.zone-device-reconnected2.log`
- `docs/port/evidence/arm64_zone/retained.zone-device-run2.log`
- `docs/port/evidence/arm64_zone/retained.zone-device-run3.log`
- `docs/port/evidence/arm64_zone/retained.zone-host-run1.log`
- `docs/port/evidence/arm64_zone/retained.zone-host-run2.log`
- `docs/port/evidence/arm64_zone/retained.zone-host-run3.log`
- `docs/port/evidence/arm64_zone/retained.zone-regression-debugger.log`
- `docs/port/evidence/arm64_zone/retained.zone-regression-fastfile.log`
- `docs/port/evidence/arm64_zone/retained.zone-regression-headers.log`
- `docs/port/evidence/arm64_zone/retained.zone-regression-narrow.log`
- `docs/port/evidence/arm64_zone/retained.zone-regression-parsetree.log`
- `docs/port/evidence/arm64_zone/retained.zone-reset-control.log`
- `docs/port/evidence/arm64_zone/retained.zone_pass2.device-tests.log`
- `docs/port/evidence/arm64_zone/retained_local_artifacts.json`
- `docs/port/evidence/arm64_zone/source_contract.json`
- `docs/port/evidence/arm64_zone/zone_host_x64.command.txt`
- `docs/port/evidence/arm64_zone/zone_host_x64.compile.log`
- `docs/port/evidence/arm64_zone/zone_host_x64.test.log`
- `docs/port/evidence/arm64_zone/zone_host_x86.command.txt`
- `docs/port/evidence/arm64_zone/zone_host_x86.compile.log`
- `docs/port/evidence/arm64_zone/zone_host_x86.test.log`
- `docs/port/evidence/arm64_zone/zone_pass1.build.log`
- `docs/port/evidence/arm64_zone/zone_pass1.compile_commands.json`
- `docs/port/evidence/arm64_zone/zone_pass1.configure.log`
- `docs/port/evidence/arm64_zone/zone_pass2.build.log`
- `docs/port/evidence/arm64_zone/zone_pass2.compile_commands.json`
- `docs/port/evidence/arm64_zone/zone_pass2.configure.log`
- `docs/port/evidence/arm64_zone/zone_pass3.build.log`
- `docs/port/evidence/arm64_zone/zone_pass3.compile_commands.json`
- `docs/port/evidence/arm64_zone/zone_pass3.configure.log`
- `src/database/db_zone_types.h`
- `src/platform/android/database_diagnostics.cpp`
- `src/platform/android/database_diagnostics.h`
- `src/platform/android/zone_memory.cpp`
- `src/platform/android/zone_memory.h`
- `tools/port/check_zone.ps1`
- `tools/port/tests/zone_test_support.cpp`
- `tools/port/tests/zone_tests.cpp`

Earlier existing evidence newly included by .gitignore exceptions (3); these were not created in this gate:

- `docs/port/evidence/arm64_expr_compile/device-tests.log`
- `docs/port/evidence/arm64_fastfile/device-genuine.log`
- `docs/port/evidence/arm64_fastfile/device-tests.log`

Full currently modified/untracked status, including preexisting user work:

```text
# branch.oid 64c5d3c5f8126a2e3a7b09dffee3a0f1f957d939
# branch.head android-arm64
1 .M N... 100644 100644 100644 975c5e9feae8cf64b7eb971ae3f90415111fee51 975c5e9feae8cf64b7eb971ae3f90415111fee51 .gitignore
1 .M N... 100644 100644 100644 43b81813fbcadf18e9746fbeafd7ad80c91c3865 43b81813fbcadf18e9746fbeafd7ad80c91c3865 .ledger/AGENT-INSTRUCTIONS.md
1 .M N... 100644 100644 100644 5cf2b981e77f5a554f1337b5f2bb1fa0a85756b5 5cf2b981e77f5a554f1337b5f2bb1fa0a85756b5 .ledger/DECISIONS.md
1 .M N... 100644 100644 100644 8b64a7a4265da403e6855b1f16920db935300053 8b64a7a4265da403e6855b1f16920db935300053 .ledger/GOAL.md
1 .M N... 100644 100644 100644 71413790f56e03c60bf9843f2a741771180c6753 71413790f56e03c60bf9843f2a741771180c6753 .ledger/STATE.md
1 .M N... 100644 100644 100644 1a2734ecedff9c668c1f0c1dd4fcae709336c288 1a2734ecedff9c668c1f0c1dd4fcae709336c288 .ledger/ledger.jsonl
1 .M N... 100644 100644 100644 195d8aa45761c1408c33233ef18478fc9598a661 195d8aa45761c1408c33233ef18478fc9598a661 AGENTS.md
1 .M N... 100644 100644 100644 27e0e3ee7c4965534fa20dbbf532c0b70a129c29 27e0e3ee7c4965534fa20dbbf532c0b70a129c29 android/cmake/arm64-audit/CMakeLists.txt
1 .M N... 100644 100644 100644 7cec0a298d8393ac0d2e5ac6d0d3c3e680f3028a 7cec0a298d8393ac0d2e5ac6d0d3c3e680f3028a docs/port/android_arm64.md
1 .M N... 100644 100644 100644 8c548e4606698b34d89a0a92d4c2de778963f0cb 8c548e4606698b34d89a0a92d4c2de778963f0cb src/database/db_file_load.h
1 .M N... 100644 100644 100644 3735ec7e34b486727fb9fbd064de3d55b77f0959 3735ec7e34b486727fb9fbd064de3d55b77f0959 src/database/db_memory.cpp
1 .M N... 100644 100644 100644 316f1158cc25c82922928607a9ff47ff4eba0ed9 316f1158cc25c82922928607a9ff47ff4eba0ed9 src/database/db_registry.h
1 .M N... 100644 100644 100644 92d54259d228289f1afe146500aec3a9c4caa119 92d54259d228289f1afe146500aec3a9c4caa119 src/database/db_stream.cpp
1 .M N... 100644 100644 100644 68f277b66a1a57019aebe13c57f15fe2dd13debb 68f277b66a1a57019aebe13c57f15fe2dd13debb src/database/db_stream.h
1 .M N... 100644 100644 100644 3e3aa561353faa5cb490c2247787c8768eff3f0f 3e3aa561353faa5cb490c2247787c8768eff3f0f tools/port/collect_script_header_evidence.py
1 .M N... 100644 100644 100644 5c01bb5cce054c73186eca7e710281375e66c19d 5c01bb5cce054c73186eca7e710281375e66c19d tools/port/run_arm64_audit_tests.ps1
1 .M N... 100644 100644 100644 dc4fecf4b3a4f3922b4283660c0e7aa903b4393f dc4fecf4b3a4f3922b4283660c0e7aa903b4393f tools/port/tests/debugger_tests.cpp
1 .M N... 100644 100644 100644 6030bd71c3e079ce0e00818a4dc9a77269d08d49 6030bd71c3e079ce0e00818a4dc9a77269d08d49 tools/port/tests/hunk_test_support.cpp
? docs/port/arm64_expr_compile.md
? docs/port/arm64_fastfile.md
? docs/port/arm64_zone.md
? docs/port/arm64_zone_changes.md
? docs/port/evidence/arm64_expr_compile/compiler_summary.json
? docs/port/evidence/arm64_expr_compile/device-tests.log
? docs/port/evidence/arm64_fastfile/compiler_summary.json
? docs/port/evidence/arm64_fastfile/device-genuine.log
? docs/port/evidence/arm64_fastfile/device-tests.log
? docs/port/evidence/arm64_fastfile/genuine-headers.tsv
? docs/port/evidence/arm64_zone/aarch64_elf_headers.txt
? docs/port/evidence/arm64_zone/android_evaluator_test_services.cpp
? docs/port/evidence/arm64_zone/apk-recheck.log
? docs/port/evidence/arm64_zone/compiler_summary.json
? docs/port/evidence/arm64_zone/debugger_host_x64.debugger.command.txt
? docs/port/evidence/arm64_zone/debugger_host_x64.debugger.compile.log
? docs/port/evidence/arm64_zone/debugger_host_x64.debugger.test.log
? docs/port/evidence/arm64_zone/debugger_host_x64.firstfit.command.txt
? docs/port/evidence/arm64_zone/debugger_host_x64.firstfit.compile.log
? docs/port/evidence/arm64_zone/debugger_host_x64.firstfit.test.log
? docs/port/evidence/arm64_zone/debugger_host_x86.debugger.command.txt
? docs/port/evidence/arm64_zone/debugger_host_x86.debugger.compile.log
? docs/port/evidence/arm64_zone/debugger_host_x86.debugger.test.log
? docs/port/evidence/arm64_zone/debugger_host_x86.firstfit.command.txt
? docs/port/evidence/arm64_zone/debugger_host_x86.firstfit.compile.log
? docs/port/evidence/arm64_zone/debugger_host_x86.firstfit.test.log
? docs/port/evidence/arm64_zone/device-tests.log
? docs/port/evidence/arm64_zone/diff-check.log
? docs/port/evidence/arm64_zone/fastfile_host_x64.command.txt
? docs/port/evidence/arm64_zone/fastfile_host_x64.compile.log
? docs/port/evidence/arm64_zone/fastfile_host_x64.test.log
? docs/port/evidence/arm64_zone/fastfile_host_x86.command.txt
? docs/port/evidence/arm64_zone/fastfile_host_x86.compile.log
? docs/port/evidence/arm64_zone/fastfile_host_x86.test.log
? docs/port/evidence/arm64_zone/final-diff-check.log
? docs/port/evidence/arm64_zone/host_evaluator_test_services.cpp
? docs/port/evidence/arm64_zone/host_x64.engine_rng.command.txt
? docs/port/evidence/arm64_zone/host_x64.engine_rng.compile.log
? docs/port/evidence/arm64_zone/host_x64.engine_rng.test.log
? docs/port/evidence/arm64_zone/host_x64.math_axis.command.txt
? docs/port/evidence/arm64_zone/host_x64.math_axis.compile.log
? docs/port/evidence/arm64_zone/host_x64.math_axis.test.log
? docs/port/evidence/arm64_zone/host_x64.profile.command.txt
? docs/port/evidence/arm64_zone/host_x64.profile.compile.log
? docs/port/evidence/arm64_zone/host_x64.profile.test.log
? docs/port/evidence/arm64_zone/host_x64.rng.command.txt
? docs/port/evidence/arm64_zone/host_x64.rng.compile.log
? docs/port/evidence/arm64_zone/host_x64.rng.test.log
? docs/port/evidence/arm64_zone/host_x64.script_headers.command.txt
? docs/port/evidence/arm64_zone/host_x64.script_headers.compile.log
? docs/port/evidence/arm64_zone/host_x64.script_headers.test.log
? docs/port/evidence/arm64_zone/host_x64.script_lookup.command.txt
? docs/port/evidence/arm64_zone/host_x64.script_lookup.compile.log
? docs/port/evidence/arm64_zone/host_x64.script_lookup.test.log
? docs/port/evidence/arm64_zone/host_x86.engine_rng.command.txt
? docs/port/evidence/arm64_zone/host_x86.engine_rng.compile.log
? docs/port/evidence/arm64_zone/host_x86.engine_rng.test.log
? docs/port/evidence/arm64_zone/host_x86.math_axis.command.txt
? docs/port/evidence/arm64_zone/host_x86.math_axis.compile.log
? docs/port/evidence/arm64_zone/host_x86.math_axis.test.log
? docs/port/evidence/arm64_zone/host_x86.profile.command.txt
? docs/port/evidence/arm64_zone/host_x86.profile.compile.log
? docs/port/evidence/arm64_zone/host_x86.profile.test.log
? docs/port/evidence/arm64_zone/host_x86.rng.command.txt
? docs/port/evidence/arm64_zone/host_x86.rng.compile.log
? docs/port/evidence/arm64_zone/host_x86.rng.test.log
? docs/port/evidence/arm64_zone/host_x86.script_headers.command.txt
? docs/port/evidence/arm64_zone/host_x86.script_headers.compile.log
? docs/port/evidence/arm64_zone/host_x86.script_headers.test.log
? docs/port/evidence/arm64_zone/host_x86.script_lookup.command.txt
? docs/port/evidence/arm64_zone/host_x86.script_lookup.compile.log
? docs/port/evidence/arm64_zone/host_x86.script_lookup.test.log
? docs/port/evidence/arm64_zone/ledger-doctor.log
? docs/port/evidence/arm64_zone/ledger-status.log
? docs/port/evidence/arm64_zone/narrow_x64.compile.log
? docs/port/evidence/arm64_zone/narrow_x64.guard-compile.log
? docs/port/evidence/arm64_zone/narrow_x64.guard-off-compile.log
? docs/port/evidence/arm64_zone/narrow_x64.slot-compile.log
? docs/port/evidence/arm64_zone/narrow_x64.slot-test.log
? docs/port/evidence/arm64_zone/narrow_x64.test.log
? docs/port/evidence/arm64_zone/narrow_x86.compile.log
? docs/port/evidence/arm64_zone/narrow_x86.guard-compile.log
? docs/port/evidence/arm64_zone/narrow_x86.slot-compile.log
? docs/port/evidence/arm64_zone/narrow_x86.slot-test.log
? docs/port/evidence/arm64_zone/narrow_x86.test.log
? docs/port/evidence/arm64_zone/parse_host_x64.hunk.command.txt
? docs/port/evidence/arm64_zone/parse_host_x64.hunk.compile.log
? docs/port/evidence/arm64_zone/parse_host_x64.hunk.test.log
? docs/port/evidence/arm64_zone/parse_host_x64.parsetree.command.txt
? docs/port/evidence/arm64_zone/parse_host_x64.parsetree.compile.log
? docs/port/evidence/arm64_zone/parse_host_x64.parsetree.test.log
? docs/port/evidence/arm64_zone/parse_host_x86.hunk.command.txt
? docs/port/evidence/arm64_zone/parse_host_x86.hunk.compile.log
? docs/port/evidence/arm64_zone/parse_host_x86.hunk.test.log
? docs/port/evidence/arm64_zone/parse_host_x86.parsetree.command.txt
? docs/port/evidence/arm64_zone/parse_host_x86.parsetree.compile.log
? docs/port/evidence/arm64_zone/parse_host_x86.parsetree.test.log
? docs/port/evidence/arm64_zone/preservation.json
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x64.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x64.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x64.test.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x86.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x86.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202340390.x86.test.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x64.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x64.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x64.test.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x86.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x86.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202457435.x86.test.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x64.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x64.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x64.test.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x86.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x86.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-202832996.x86.test.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x64.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x64.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x64.test.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x86.command.txt
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x86.compile.log
? docs/port/evidence/arm64_zone/retained.host-zone-20261011-203011638.x86.test.log
? docs/port/evidence/arm64_zone/retained.zone-baseline.json
? docs/port/evidence/arm64_zone/retained.zone-device-reconnected2.log
? docs/port/evidence/arm64_zone/retained.zone-device-run2.log
? docs/port/evidence/arm64_zone/retained.zone-device-run3.log
? docs/port/evidence/arm64_zone/retained.zone-host-run1.log
? docs/port/evidence/arm64_zone/retained.zone-host-run2.log
? docs/port/evidence/arm64_zone/retained.zone-host-run3.log
? docs/port/evidence/arm64_zone/retained.zone-regression-debugger.log
? docs/port/evidence/arm64_zone/retained.zone-regression-fastfile.log
? docs/port/evidence/arm64_zone/retained.zone-regression-headers.log
? docs/port/evidence/arm64_zone/retained.zone-regression-narrow.log
? docs/port/evidence/arm64_zone/retained.zone-regression-parsetree.log
? docs/port/evidence/arm64_zone/retained.zone-reset-control.log
? docs/port/evidence/arm64_zone/retained.zone_pass2.device-tests.log
? docs/port/evidence/arm64_zone/retained_local_artifacts.json
? docs/port/evidence/arm64_zone/source_contract.json
? docs/port/evidence/arm64_zone/zone_host_x64.command.txt
? docs/port/evidence/arm64_zone/zone_host_x64.compile.log
? docs/port/evidence/arm64_zone/zone_host_x64.test.log
? docs/port/evidence/arm64_zone/zone_host_x86.command.txt
? docs/port/evidence/arm64_zone/zone_host_x86.compile.log
? docs/port/evidence/arm64_zone/zone_host_x86.test.log
? docs/port/evidence/arm64_zone/zone_pass1.build.log
? docs/port/evidence/arm64_zone/zone_pass1.compile_commands.json
? docs/port/evidence/arm64_zone/zone_pass1.configure.log
? docs/port/evidence/arm64_zone/zone_pass2.build.log
? docs/port/evidence/arm64_zone/zone_pass2.compile_commands.json
? docs/port/evidence/arm64_zone/zone_pass2.configure.log
? docs/port/evidence/arm64_zone/zone_pass3.build.log
? docs/port/evidence/arm64_zone/zone_pass3.compile_commands.json
? docs/port/evidence/arm64_zone/zone_pass3.configure.log
? docs/port/roadmap_headless.md
? src/database/db_zone_types.h
? src/platform/android/database_diagnostics.cpp
? src/platform/android/database_diagnostics.h
? src/platform/android/zone_memory.cpp
? src/platform/android/zone_memory.h
? src/platform/fastfile_reader.h
? tools/port/check_fastfile.ps1
? tools/port/check_zone.ps1
? tools/port/tests/fastfile_tests.cpp
? tools/port/tests/zone_test_support.cpp
? tools/port/tests/zone_tests.cpp
```

The baseline already contained five tracked modifications and nine durable untracked files from TASK-0007/0008. Their original status is retained in preservation.json; the debugger fixture and all earlier evidence remain byte-identical. CMake/runner/Ledger changes build on the user's work.

Local SQLite changed as the Ledger index was maintained; .ledger/.lock and android/local.properties stay local. Ignored builds/binaries/scratch are retained, not committed: 609 latest build/test artifacts and helper scripts are catalogued in retained_local_artifacts.json. The archive/update/inventory scripts remain under build-android-audit.

TASK-0009 is complete; no running compiler/test remains. The bounded named owner is intentionally not a complete PMem implementation. Deflate, real decoded XFile, full remapping/asset parsing and other WARN-0009 limits remain future work. Exact next-task acceptance and device commands are in arm64_zone.md.
