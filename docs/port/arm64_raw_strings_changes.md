# TASK-0015 exact file inventory

Raw-string milestone complete; local verified checkpoint approval DEC-0026.
Unverified ARM64 pool prerequisite files excluded from this checkpoint. All raw
compiler/test scratch and original asset prefixes remain ignored and preserved.

Modified durable paths:

- `.gitignore`
- `.ledger/AGENT-INSTRUCTIONS.md`
- `.ledger/STATE.md`
- `.ledger/ledger.jsonl`
- `AGENTS.md`
- `android/cmake/arm64-audit/CMakeLists.txt`
- `docs/port/android_arm64.md`
- `docs/port/roadmap_headless.md`
- `src/database/db_stream_load.cpp`

Created paths (including separately pending pool prerequisites):

- `docs/port/arm64_raw_strings.md`
- `docs/port/evidence/arm64_raw_strings/manifest.json`
- `docs/port/evidence/arm64_raw_strings/preservation.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass1.x86.command.txt`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass1.x86.compile.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass2.x64.command.txt`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass2.x64.compile.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass2.x64.raw_strings.map`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass2.x86.command.txt`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass2.x86.compile.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass2.x86.raw_strings.map`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass3.x64.command.txt`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass3.x64.compile.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass3.x64.raw_strings.map`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass3.x86.command.txt`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass3.x86.compile.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_host_pass3.x86.raw_strings.map`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass1.build.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass1.compile_commands.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass1.configure.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass2.build.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass2.compile_commands.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass2.configure.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass3.build.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass3.compile_commands.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass3.configure.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass3.device-tests.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass3.review-evidence.md`
- `docs/port/evidence/arm64_raw_strings/raw_strings_pass3.source-preservation.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.code_pre_gfx-reused-device-sha.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.code_pre_gfx-reused-device-sha.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-create.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-create.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-metadata.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-metadata.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-probe-chmod.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-probe-chmod.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-probe-push.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-probe-push.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-probe-sha.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.device-probe-sha.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-code_pre_gfx-decode.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-code_pre_gfx-decode.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-code_pre_gfx-prefix-sha.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-code_pre_gfx-prefix-sha.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-raw-strings.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-raw-strings.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-zombie_pentagon-decode.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-zombie_pentagon-decode.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-zombie_pentagon-prefix-sha.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.honor_arm64-zombie_pentagon-prefix-sha.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.independent-oracles.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.invocations.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.results.json`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x64-code_pre_gfx-decode.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x64-code_pre_gfx-decode.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x64-raw-strings.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x64-raw-strings.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x64-zombie_pentagon-decode.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x64-zombie_pentagon-decode.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x86-code_pre_gfx-decode.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x86-code_pre_gfx-decode.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x86-raw-strings.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x86-raw-strings.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x86-zombie_pentagon-decode.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.windows_x86-zombie_pentagon-decode.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.zombie_pentagon-reused-device-sha.stderr.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_run1.zombie_pentagon-reused-device-sha.stdout.log`
- `docs/port/evidence/arm64_raw_strings/raw_strings_symbols.txt`
- `docs/port/evidence/arm64_raw_strings/windows.object-evidence.json`
- `docs/port/evidence/arm64_raw_strings/windows.results.json`
- `docs/port/evidence/arm64_raw_strings/windows.x64.command.txt`
- `docs/port/evidence/arm64_raw_strings/windows.x64.compile.log`
- `docs/port/evidence/arm64_raw_strings/windows.x86.command.txt`
- `docs/port/evidence/arm64_raw_strings/windows.x86.compile.log`
- `src/platform/android/script_string_services.cpp` — pending ARM64 pool prerequisite
- `src/platform/android/script_string_services.h` — pending ARM64 pool prerequisite
- `src/platform/atomic32.h` — pending ARM64 pool prerequisite
- `src/platform/fastfile_raw_string.cpp`
- `src/platform/fastfile_raw_string.h`
- `tools/port/build_raw_strings_host.ps1`
- `tools/port/build_script_string_services_host.ps1` — pending ARM64 pool prerequisite
- `tools/port/collect_raw_strings_evidence.py`
- `tools/port/raw_strings_oracle.py`
- `tools/port/run_raw_strings_gate.py`
- `tools/port/tests/raw_strings_tests.cpp`
- `tools/port/tests/script_string_services_tests.cpp` — pending ARM64 pool prerequisite
- `tools/port/verify_raw_strings_source.py`

Inventory and this report are also created. No removed paths; no push.
