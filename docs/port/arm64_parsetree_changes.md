# Parse-tree/HunkUser gate file inventory

Relative to the fresh parsetree-baseline.json snapshot, after the completed
RNG/script lookup gate. All paths below are inside BlackOpsARM.
18 durable files modified, 106 durable files created, zero removed.
The ignored local Ledger SQLite index also changed. Android index/HEAD remain
unchanged; no staging, commit or push. Local ignored build/host outputs are
listed with sizes/hashes in evidence/arm64_parsetree/ignored_artifacts.json;
Gradle caches are excluded from that catalog. The scaffold APK hash is unchanged.

## Modified durable files

- `.gitignore`
- `.ledger/AGENT-INSTRUCTIONS.md`
- `.ledger/DECISIONS.md`
- `.ledger/GOAL.md`
- `.ledger/STATE.md`
- `.ledger/ledger.jsonl`
- `AGENTS.md`
- `android/cmake/arm64-audit/CMakeLists.txt`
- `docs/port/android_arm64.md`
- `docs/port/arm64_rng_script.md`
- `src/clientscript/cscr_parsetree.cpp`
- `src/clientscript/cscr_parsetree.h`
- `src/clientscript/cscr_vm.h`
- `src/qcommon/common.h`
- `src/universal/com_memory.h`
- `src/universal/mem_userhunk.cpp`
- `tools/port/collect_script_header_evidence.py`
- `tools/port/run_arm64_audit_tests.ps1`

## Created durable files

- `docs/port/arm64_parsetree.md`
- `docs/port/arm64_parsetree_changes.md`
- `docs/port/evidence/arm64_parsetree/aarch64_elf_headers.txt`
- `docs/port/evidence/arm64_parsetree/additional_log_sources.json`
- `docs/port/evidence/arm64_parsetree/apk-recheck.log`
- `docs/port/evidence/arm64_parsetree/compiler_summary.json`
- `docs/port/evidence/arm64_parsetree/device-tests.log`
- `docs/port/evidence/arm64_parsetree/diff-check.log`
- `docs/port/evidence/arm64_parsetree/host-parsetree-20261011-173414324.x86.parsetree-command.txt`
- `docs/port/evidence/arm64_parsetree/host-parsetree-20261011-173414324.x86.parsetree-compile.log`
- `docs/port/evidence/arm64_parsetree/host-parsetree-20261011-173445035.x86.parsetree-command.txt`
- `docs/port/evidence/arm64_parsetree/host-parsetree-20261011-173445035.x86.parsetree-compile.log`
- `docs/port/evidence/arm64_parsetree/host_x64.engine_rng.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x64.engine_rng.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x64.engine_rng.test.log`
- `docs/port/evidence/arm64_parsetree/host_x64.math_axis.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x64.math_axis.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x64.math_axis.test.log`
- `docs/port/evidence/arm64_parsetree/host_x64.profile.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x64.profile.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x64.profile.test.log`
- `docs/port/evidence/arm64_parsetree/host_x64.rng.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x64.rng.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x64.rng.test.log`
- `docs/port/evidence/arm64_parsetree/host_x64.script_headers.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x64.script_headers.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x64.script_headers.test.log`
- `docs/port/evidence/arm64_parsetree/host_x64.script_lookup.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x64.script_lookup.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x64.script_lookup.test.log`
- `docs/port/evidence/arm64_parsetree/host_x86.engine_rng.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x86.engine_rng.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x86.engine_rng.test.log`
- `docs/port/evidence/arm64_parsetree/host_x86.math_axis.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x86.math_axis.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x86.math_axis.test.log`
- `docs/port/evidence/arm64_parsetree/host_x86.profile.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x86.profile.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x86.profile.test.log`
- `docs/port/evidence/arm64_parsetree/host_x86.rng.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x86.rng.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x86.rng.test.log`
- `docs/port/evidence/arm64_parsetree/host_x86.script_headers.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x86.script_headers.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x86.script_headers.test.log`
- `docs/port/evidence/arm64_parsetree/host_x86.script_lookup.command.txt`
- `docs/port/evidence/arm64_parsetree/host_x86.script_lookup.compile.log`
- `docs/port/evidence/arm64_parsetree/host_x86.script_lookup.test.log`
- `docs/port/evidence/arm64_parsetree/ignored_artifacts.json`
- `docs/port/evidence/arm64_parsetree/narrow_x64.compile.log`
- `docs/port/evidence/arm64_parsetree/narrow_x64.guard-compile.log`
- `docs/port/evidence/arm64_parsetree/narrow_x64.guard-off-compile.log`
- `docs/port/evidence/arm64_parsetree/narrow_x64.slot-compile.log`
- `docs/port/evidence/arm64_parsetree/narrow_x64.slot-test.log`
- `docs/port/evidence/arm64_parsetree/narrow_x64.test.log`
- `docs/port/evidence/arm64_parsetree/narrow_x86.compile.log`
- `docs/port/evidence/arm64_parsetree/narrow_x86.guard-compile.log`
- `docs/port/evidence/arm64_parsetree/narrow_x86.slot-compile.log`
- `docs/port/evidence/arm64_parsetree/narrow_x86.slot-test.log`
- `docs/port/evidence/arm64_parsetree/narrow_x86.test.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x64.hunk.command.txt`
- `docs/port/evidence/arm64_parsetree/parse_host_x64.hunk.compile.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x64.hunk.test.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x64.parsetree.command.txt`
- `docs/port/evidence/arm64_parsetree/parse_host_x64.parsetree.compile.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x64.parsetree.test.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x86.hunk.command.txt`
- `docs/port/evidence/arm64_parsetree/parse_host_x86.hunk.compile.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x86.hunk.test.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x86.parsetree.command.txt`
- `docs/port/evidence/arm64_parsetree/parse_host_x86.parsetree.compile.log`
- `docs/port/evidence/arm64_parsetree/parse_host_x86.parsetree.test.log`
- `docs/port/evidence/arm64_parsetree/parsetree-host-run2.log`
- `docs/port/evidence/arm64_parsetree/parsetree-host-run3.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass1.build.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass1.compile_commands.json`
- `docs/port/evidence/arm64_parsetree/parsetree_pass1.configure.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass2.build.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass2.compile_commands.json`
- `docs/port/evidence/arm64_parsetree/parsetree_pass2.configure.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass3.build.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass3.compile_commands.json`
- `docs/port/evidence/arm64_parsetree/parsetree_pass3.configure.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass4.build.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass4.compile_commands.json`
- `docs/port/evidence/arm64_parsetree/parsetree_pass4.configure.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass4.device-tests.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass5.build.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass5.compile_commands.json`
- `docs/port/evidence/arm64_parsetree/parsetree_pass5.configure.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass5.device-tests.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass6.build.log`
- `docs/port/evidence/arm64_parsetree/parsetree_pass6.compile_commands.json`
- `docs/port/evidence/arm64_parsetree/parsetree_pass6.configure.log`
- `docs/port/evidence/arm64_parsetree/preservation.json`
- `docs/port/evidence/arm64_parsetree/source_contract.json`
- `src/platform/android/virtual_memory.cpp`
- `src/platform/android/virtual_memory.h`
- `src/universal/com_errors.h`
- `src/universal/memory_services.h`
- `tools/port/check_parsetree.ps1`
- `tools/port/tests/hunk_test_checks.h`
- `tools/port/tests/hunk_test_memory_windows.cpp`
- `tools/port/tests/hunk_test_support.cpp`
- `tools/port/tests/hunk_tests.cpp`
- `tools/port/tests/parsetree_tests.cpp`
