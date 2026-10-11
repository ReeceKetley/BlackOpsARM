# Actual script-header gate file inventory

Relative to the fresh script-headers-baseline.json snapshot taken at the start
of this milestone, after completed Phase 1 work. All paths below are inside
BlackOpsARM. 26 durable files modified, 73 durable files created,
zero removed. The ignored local Ledger SQLite index also changed; it is not a
source-of-truth file. No staging, commit or push. The preservation JSON lists
all exact paths and protected-checkout results.

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
- `docs/port/arm64_compilation_audit.md`
- `src/bgame/bg_wind.cpp`
- `src/cgame/cg_main.cpp`
- `src/cgame/cg_playerstate.cpp`
- `src/cgame/cg_weapons.cpp`
- `src/cgame_mp/cg_predict_mp.cpp`
- `src/gfx_d3d/r_staticmodelcache_load_obj.cpp`
- `src/glass/glass_shard.cpp`
- `src/server_mp/sv_bot_mp.cpp`
- `src/sound/snd.cpp`
- `src/sound/snd_public_async.cpp`
- `src/universal/com_math.cpp`
- `src/universal/com_math.h`
- `src/universal/com_math_anglevectors.cpp`
- `src/universal/profile.cpp`
- `src/universal/profile.h`
- `tools/port/run_arm64_audit_tests.ps1`

## Created durable files

- `docs/port/arm64_script_header_changes.md`
- `docs/port/arm64_script_headers.md`
- `docs/port/evidence/arm64_script_headers/aarch64_elf_headers.txt`
- `docs/port/evidence/arm64_script_headers/apk-recheck.log`
- `docs/port/evidence/arm64_script_headers/compiler_summary.json`
- `docs/port/evidence/arm64_script_headers/device-tests.log`
- `docs/port/evidence/arm64_script_headers/diff-check.log`
- `docs/port/evidence/arm64_script_headers/host_x64.math_axis.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x64.math_axis.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x64.math_axis.test.log`
- `docs/port/evidence/arm64_script_headers/host_x64.profile.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x64.profile.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x64.profile.test.log`
- `docs/port/evidence/arm64_script_headers/host_x64.rng.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x64.rng.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x64.rng.test.log`
- `docs/port/evidence/arm64_script_headers/host_x64.script_headers.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x64.script_headers.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x64.script_headers.test.log`
- `docs/port/evidence/arm64_script_headers/host_x86.math_axis.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x86.math_axis.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x86.math_axis.test.log`
- `docs/port/evidence/arm64_script_headers/host_x86.profile.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x86.profile.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x86.profile.test.log`
- `docs/port/evidence/arm64_script_headers/host_x86.rng.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x86.rng.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x86.rng.test.log`
- `docs/port/evidence/arm64_script_headers/host_x86.script_headers.command.txt`
- `docs/port/evidence/arm64_script_headers/host_x86.script_headers.compile.log`
- `docs/port/evidence/arm64_script_headers/host_x86.script_headers.test.log`
- `docs/port/evidence/arm64_script_headers/local_artifacts.json`
- `docs/port/evidence/arm64_script_headers/narrow_x64.compile.log`
- `docs/port/evidence/arm64_script_headers/narrow_x64.guard-compile.log`
- `docs/port/evidence/arm64_script_headers/narrow_x64.guard-off-compile.log`
- `docs/port/evidence/arm64_script_headers/narrow_x64.slot-compile.log`
- `docs/port/evidence/arm64_script_headers/narrow_x64.slot-test.log`
- `docs/port/evidence/arm64_script_headers/narrow_x64.test.log`
- `docs/port/evidence/arm64_script_headers/narrow_x86.compile.log`
- `docs/port/evidence/arm64_script_headers/narrow_x86.guard-compile.log`
- `docs/port/evidence/arm64_script_headers/narrow_x86.slot-compile.log`
- `docs/port/evidence/arm64_script_headers/narrow_x86.slot-test.log`
- `docs/port/evidence/arm64_script_headers/narrow_x86.test.log`
- `docs/port/evidence/arm64_script_headers/preservation.json`
- `docs/port/evidence/arm64_script_headers/rng_symbol_audit.json`
- `docs/port/evidence/arm64_script_headers/script_headers_final.build.log`
- `docs/port/evidence/arm64_script_headers/script_headers_final.compile_commands.json`
- `docs/port/evidence/arm64_script_headers/script_headers_final.configure.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass1.build.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass1.compile_commands.json`
- `docs/port/evidence/arm64_script_headers/script_headers_pass1.configure.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass2.build.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass2.compile_commands.json`
- `docs/port/evidence/arm64_script_headers/script_headers_pass2.configure.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass3.build.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass3.compile_commands.json`
- `docs/port/evidence/arm64_script_headers/script_headers_pass3.configure.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass4.build.log`
- `docs/port/evidence/arm64_script_headers/script_headers_pass4.compile_commands.json`
- `docs/port/evidence/arm64_script_headers/script_headers_pass4.configure.log`
- `docs/port/evidence/arm64_script_headers/script_headers_validated.build.log`
- `docs/port/evidence/arm64_script_headers/script_headers_validated.compile_commands.json`
- `docs/port/evidence/arm64_script_headers/script_headers_validated.configure.log`
- `src/platform/android/profile_services.cpp`
- `src/platform/android/profile_services.h`
- `src/platform/profile_services.h`
- `tools/port/check_script_headers.ps1`
- `tools/port/collect_script_header_evidence.py`
- `tools/port/generate_math_oracle.py`
- `tools/port/generate_rng_probe.py`
- `tools/port/tests/math_axis_tests.cpp`
- `tools/port/tests/profile_tests.cpp`
- `tools/port/tests/script_headers_tests.cpp`

## Local outputs and boundaries

`evidence/arm64_script_headers/local_artifacts.json` lists every file in this
milestone's six NDK output directories, five Windows harness attempts and the
fresh Narrow32 host run, plus its ignored baseline snapshot, with hashes.
Generated function/declaration probes exist only in these ignored test outputs.
APK/Gradle output and caches are machine-local; the unchanged APK hash and
successful assemble/lint log are retained. SQLite is only Ledger's local index.

No Windows checkout, DXVK repository or root Ledger file changed. Both linked
worktrees retain their branch/HEAD and independent working directories/indexes;
DXVK retains a separate object store. All 54 frozen Phase 1 evidence files match
the fresh baseline. The original cscr_instance.cpp source is selected by CMake
without modification. RNG's ten caller files have name-only changes, verified
by rng_symbol_audit.json; the actual RNG calculation is preserved.
