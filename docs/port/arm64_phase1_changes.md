# Phase 1 exact change inventory — 2026-10-11

This inventory compares the final Android worktree with the snapshot captured at the start of Phase 1 (`build-android-audit/phase1-baseline.json`), including existing uncommitted setup files. It is not a diff against Git HEAD: the earlier infrastructure setup was already uncommitted and is preserved. All listed project edits are inside BlackOpsARM. No files were removed.

**76 modified files; 80 created files.** `.ledger/ledger.db` is the changed, ignored local SQLite index; durable Ledger history is `ledger.jsonl` and Markdown. The root Ledger and Windows/DXVK files are unchanged.

Code changes consist of the inventory-only CMake return, two Android include boundaries, reviewed per-site native-width guards, fixed-layout checks, the Android handle backend and scoped compatibility/test tooling. Engine function bodies are retained apart from conditional platform selection. The Windows handle implementation, root CMake, APK sources/configuration and existing compatibility-free Windows includes are unchanged.

## Modified files

| File | Purpose |
|---|---|
| `.gitignore` | Keep bounded compiler/test logs as durable audit evidence |
| `.ledger/AGENT-INSTRUCTIONS.md` | Android project memory/instructions and verification progress |
| `.ledger/DECISIONS.md` | Android project memory/instructions and verification progress |
| `.ledger/GOAL.md` | Android project memory/instructions and verification progress |
| `.ledger/STATE.md` | Android project memory/instructions and verification progress |
| `.ledger/ledger.db` | Android project memory/instructions and verification progress |
| `.ledger/ledger.jsonl` | Android project memory/instructions and verification progress |
| `AGENTS.md` | Android project memory/instructions and verification progress |
| `DemonWare/bdCore/bdContainers/bdLinkedList.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `DemonWare/bdCore/bdContainers/bdQueue.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `cmake_files.cmake` | Opt-in reuse of existing source inventory without Windows executable creation |
| `docs/port/android_arm64.md` | Development/build/semantics documentation |
| `src/EffectsCore/fx_draw.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/bgame/bg_local.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/cgame/cg_weapons.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/cgame_mp/cg_main_mp.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/client_mp/cl_main_mp.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/client_mp/cl_main_mp.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/clientscript/cscr_debugger.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/clientscript/cscr_debugger.h` | Native-width narrow parse slots and Android handle decoding; Windows macro retained |
| `src/clientscript/cscr_variable.h` | Keep four-byte VM assertion active on Android |
| `src/database/db_assetnames.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_expand.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_expand.hpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_file_load.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_load.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_memory.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_stream.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_stream.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_stream_load.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/database/db_stream_load.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game/sentient.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_mp/g_main_mp.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_sp/g_sp_save_actor.inl` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_sp/g_sp_save_client.inl` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_sp/g_sp_save_entity.inl` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_sp/g_sp_save_level.inl` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_sp/g_sp_save_vehicle.inl` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_sp/g_sp_savegame.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/game_sp/g_sp_savegame.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/gfx_d3d/r_font.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/gfx_d3d/r_gfx.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/gfx_d3d/r_material.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/gfx_d3d/r_model_skin.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/gfx_d3d/r_model_skin_sse.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/gfx_d3d/r_rendercmds.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/glass/glass_shard.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_broad_phase.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_broad_phase.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_colgeom.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_gjk.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_gjk_collision_detection.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_local.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_mem_new.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/phys_mem_new.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/physics_system.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/physics_system_internal.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/physics/rigid_body.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/platform/android/README.md` | Development/build/semantics documentation |
| `src/platform/win32_compat/README.md` | Development/build/semantics documentation |
| `src/qcommon/dobj_management.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/server/server.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/server_mp/sv_main_mp.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/sound/snd.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/universal/com_math_anglevectors.cpp` | Minimal Android include boundary; Windows includes/body retained |
| `src/universal/com_stringtable.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/universal/com_workercmds.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/universal/com_workercmds.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/universal/narrow32.h` | Native-width selector plus Android atomic publication reads; four-byte representation retained |
| `src/universal/physicalmemory.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/universal/q_shared.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `src/universal/timing.cpp` | Minimal Android include boundary; Windows includes/body retained |
| `src/vehicle/nitrous_vehicle.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `tl/jobqueue/jobqueue_all.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |
| `tl/jobqueue/jobqueue_all.h` | Reviewed native-width guard(s); fixed records retained as applicable |
| `tl/physics/rbc_def_contact.cpp` | Reviewed native-width guard(s); fixed records retained as applicable |

## Created files

| File | Purpose |
|---|---|
| `android/cmake/arm64-audit/CMakeLists.txt` | Standalone bounded NDK AArch64 compilation target |
| `docs/port/arm64_compilation_audit.md` | Compilation audit or this exact inventory |
| `docs/port/arm64_phase1_changes.md` | Compilation audit or this exact inventory |
| `docs/port/evidence/arm64_phase1/aarch64_elf_headers.txt` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/apk_recheck.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/before_narrow_slot_failure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/compiler_summary.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/dependency_inventory.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/final_validated.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/final_validated.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/final_validated.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/flag_guard_audit.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/honor_device_tests.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_tls.compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_tls.test.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x64.compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x64.guard-compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x64.guard-off-compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x64.slot-compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x64.slot-test.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x64.test.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x86.compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x86.guard-compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x86.slot-compile.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x86.slot-test.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/host_x86.test.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/legacy_flag_references.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/local_artifacts.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass0_raw.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass0_raw.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass0_raw.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass10_final.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass10_final.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass10_final.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass1_spelling.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass1_spelling.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass1_spelling.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass2_foundation.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass2_foundation.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass2_foundation.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass4_signed_char.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass4_signed_char.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass4_signed_char.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass5_native_width.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass5_native_width.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass5_native_width.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass6_script_slots.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass6_script_slots.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass6_script_slots.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass7_device.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass7_device.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass7_device.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass9_guard_matrix.build.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass9_guard_matrix.compile_commands.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/pass9_guard_matrix.configure.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/preservation.json` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `docs/port/evidence/arm64_phase1/rejected_low_address_mode.log` | Retained compiler/audit/test/preservation evidence (see audit report) |
| `src/platform/android/narrow32_android.cpp` | Android atomic region-tagged handle backend |
| `src/platform/bo1_native_width.h` | Shared native-width selection independent of Windows bring-up policies |
| `src/platform/win32_compat/msvc_compat.h` | Bounded Android compiler/Win32 timing/error compatibility foundation |
| `src/platform/win32_compat/win32_errors.cpp` | Bounded Android compiler/Win32 timing/error compatibility foundation |
| `src/platform/win32_compat/win32_errors.h` | Bounded Android compiler/Win32 timing/error compatibility foundation |
| `src/platform/win32_compat/win32_time.cpp` | Bounded Android compiler/Win32 timing/error compatibility foundation |
| `src/platform/win32_compat/win32_time.h` | Bounded Android compiler/Win32 timing/error compatibility foundation |
| `src/platform/win32_compat/win32_types.h` | Bounded Android compiler/Win32 timing/error compatibility foundation |
| `tools/port/arm64_dependency_audit.py` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/audit_native_width_flags.py` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/build_arm64_audit.ps1` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/check_narrow32.ps1` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/check_win32_errors.ps1` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/collect_arm64_evidence.py` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/generate_script_slot_probe.py` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/run_arm64_audit_tests.ps1` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/tests/arm64_abi_contract.cpp` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/tests/arm64_narrow_slot_probe.cpp` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/tests/narrow32_tests.cpp` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/tests/narrow32_windows_test_support.cpp` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/tests/win32_compat_tests.cpp` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/tests/win32_error_tests.cpp` | Reproducible inventory/build/ABI/compatibility/device verification tooling |
| `tools/port/verify_arm64_audit.py` | Reproducible inventory/build/ABI/compatibility/device verification tooling |

## Generated/local artifacts

`evidence/arm64_phase1/local_artifacts.json` individually lists the 390 ignored local Phase 1 outputs present when the inventory was captured, with paths, lengths and SHA256 hashes: CMake/Ninja caches, actual AArch64 objects/executables/libraries, generated declaration/guard probes, host MSVC probes, full logs and transient snapshots. Production declarations are extracted only into ignored test builds; no persistent engine source copy is created. The subsequent documentation-only inventory/preservation refresh does not regenerate binaries.

The APK recheck uses existing ignored Gradle/app outputs; the APK SHA256 is identical to setup verification. No APK was installed. On the explicitly authorized connected HONOR, the final test run leaves only its three native test executables in the unique directory recorded by `honor_device_tests.log`; earlier probe runs have their own unique directories and are retained. No device settings, assets, DXVK checkout or other project files were changed.

Protected checks: Windows main at 76edfd5, 2,528 files including nine scratch files plus index/status; DXVK master at 0e271c6, 2,552 files plus index/status and independent repository; all 15 root Ledger files. Android remains android-arm64 at the same committed base, with shared history and independent index/directory. No commit/push. Ledger doctor is healthy.
