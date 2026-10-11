# RNG/script lookup gate file inventory

Relative to the fresh rng-script-baseline.json starting snapshot, after the
completed actual-header gate. All paths below are inside BlackOpsARM.
37 durable files modified, 81 durable files created, zero removed.
The ignored Ledger SQLite index also changed. No staging, commit or push.

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
- `docs/port/arm64_script_headers.md`
- `src/DynEntity/DynEntity_pieces.cpp`
- `src/EffectsCore/fx_system.cpp`
- `src/EffectsCore/fx_update.cpp`
- `src/bgame/bg_fire.cpp`
- `src/bgame/bg_wind.cpp`
- `src/cgame/cg_main.cpp`
- `src/cgame/cg_scr_main.cpp`
- `src/cgame/cg_sp_servercmds.cpp`
- `src/cgame/cg_vehicle.cpp`
- `src/cgame/cg_weapons.cpp`
- `src/cgame_mp/cg_players_mp.cpp`
- `src/cgame_mp/cg_view_mp.cpp`
- `src/client_mp/cl_main_mp.cpp`
- `src/client_mp/sv_client_mp.cpp`
- `src/game_mp/g_main_mp.cpp`
- `src/gfx_d3d/r_buffers.cpp`
- `src/physics/rope.cpp`
- `src/server_mp/sv_bot_mp.cpp`
- `src/sound/snd.cpp`
- `src/ui/ui_playlists.cpp`
- `src/ui/ui_utils.cpp`
- `src/universal/com_files.cpp`
- `src/universal/com_math.cpp`
- `tools/port/check_script_headers.ps1`
- `tools/port/collect_script_header_evidence.py`
- `tools/port/generate_rng_probe.py`
- `tools/port/run_arm64_audit_tests.ps1`

## Created durable files

- `docs/port/arm64_rng_script.md`
- `docs/port/arm64_rng_script_changes.md`
- `docs/port/evidence/arm64_rng_script/aarch64_elf_headers.txt`
- `docs/port/evidence/arm64_rng_script/apk-recheck.log`
- `docs/port/evidence/arm64_rng_script/compiler_summary.json`
- `docs/port/evidence/arm64_rng_script/device-tests.log`
- `docs/port/evidence/arm64_rng_script/diff-check.log`
- `docs/port/evidence/arm64_rng_script/host_x64.engine_rng.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x64.engine_rng.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x64.engine_rng.test.log`
- `docs/port/evidence/arm64_rng_script/host_x64.math_axis.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x64.math_axis.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x64.math_axis.test.log`
- `docs/port/evidence/arm64_rng_script/host_x64.profile.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x64.profile.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x64.profile.test.log`
- `docs/port/evidence/arm64_rng_script/host_x64.rng.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x64.rng.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x64.rng.test.log`
- `docs/port/evidence/arm64_rng_script/host_x64.script_headers.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x64.script_headers.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x64.script_headers.test.log`
- `docs/port/evidence/arm64_rng_script/host_x64.script_lookup.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x64.script_lookup.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x64.script_lookup.test.log`
- `docs/port/evidence/arm64_rng_script/host_x86.engine_rng.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x86.engine_rng.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x86.engine_rng.test.log`
- `docs/port/evidence/arm64_rng_script/host_x86.math_axis.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x86.math_axis.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x86.math_axis.test.log`
- `docs/port/evidence/arm64_rng_script/host_x86.profile.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x86.profile.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x86.profile.test.log`
- `docs/port/evidence/arm64_rng_script/host_x86.rng.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x86.rng.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x86.rng.test.log`
- `docs/port/evidence/arm64_rng_script/host_x86.script_headers.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x86.script_headers.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x86.script_headers.test.log`
- `docs/port/evidence/arm64_rng_script/host_x86.script_lookup.command.txt`
- `docs/port/evidence/arm64_rng_script/host_x86.script_lookup.compile.log`
- `docs/port/evidence/arm64_rng_script/host_x86.script_lookup.test.log`
- `docs/port/evidence/arm64_rng_script/local_artifacts.json`
- `docs/port/evidence/arm64_rng_script/narrow_x64.compile.log`
- `docs/port/evidence/arm64_rng_script/narrow_x64.guard-compile.log`
- `docs/port/evidence/arm64_rng_script/narrow_x64.guard-off-compile.log`
- `docs/port/evidence/arm64_rng_script/narrow_x64.slot-compile.log`
- `docs/port/evidence/arm64_rng_script/narrow_x64.slot-test.log`
- `docs/port/evidence/arm64_rng_script/narrow_x64.test.log`
- `docs/port/evidence/arm64_rng_script/narrow_x86.compile.log`
- `docs/port/evidence/arm64_rng_script/narrow_x86.guard-compile.log`
- `docs/port/evidence/arm64_rng_script/narrow_x86.slot-compile.log`
- `docs/port/evidence/arm64_rng_script/narrow_x86.slot-test.log`
- `docs/port/evidence/arm64_rng_script/narrow_x86.test.log`
- `docs/port/evidence/arm64_rng_script/preservation.json`
- `docs/port/evidence/arm64_rng_script/rng_call_migration.json`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x64.command.txt`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x64.compile.log`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x64.test.log`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x64.windows_crt_rng_vectors.h`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x86.command.txt`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x86.compile.log`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x86.test.log`
- `docs/port/evidence/arm64_rng_script/rng_oracle_x86.windows_crt_rng_vectors.h`
- `docs/port/evidence/arm64_rng_script/rng_script_pass1.build.log`
- `docs/port/evidence/arm64_rng_script/rng_script_pass1.compile_commands.json`
- `docs/port/evidence/arm64_rng_script/rng_script_pass1.configure.log`
- `docs/port/evidence/arm64_rng_script/rng_script_pass2.build.log`
- `docs/port/evidence/arm64_rng_script/rng_script_pass2.compile_commands.json`
- `docs/port/evidence/arm64_rng_script/rng_script_pass2.configure.log`
- `docs/port/evidence/arm64_rng_script/vendor_rng_boundary.json`
- `docs/port/evidence/arm64_rng_script/windows_crt_reference.json`
- `src/platform/android/engine_random.cpp`
- `src/platform/android/engine_random.h`
- `src/platform/engine_random.h`
- `tools/port/check_engine_rng.ps1`
- `tools/port/tests/engine_rng_oracle.cpp`
- `tools/port/tests/engine_rng_tests.cpp`
- `tools/port/tests/script_lookup_tests.cpp`
- `tools/port/tests/windows_crt_rng_vectors.h`

## Local outputs and preservation

local_artifacts.json lists each file in this stage's two NDK output directories,
Windows CRT oracle, failed/passing script harness attempts and fresh ABI host
run, plus its baseline and static call audit. These ignored outputs include
only test-only generated production-body/declaration fixtures; no engine tree
is duplicated. APK/Gradle caches are not cataloged; successful assemble/lint and
the unchanged APK hash are retained. SQLite is Ledger's disposable local index.

All 23 T5 caller edits are byte-identical after reversing only 48 service call
names and their added header include. cscr_readwrite.cpp is unchanged; the
existing inventory selects it directly. Vendor libtommath/Speex files remain
unchanged. Windows, DXVK, root Ledger and 54/61 prior evidence files match the
fresh snapshot. Worktree history is shared; directories/indexes and DXVK's
repository remain independent. See preservation.json for exact checks.
