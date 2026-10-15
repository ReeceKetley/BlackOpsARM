# Automatic Kino god mode — 2026-10-15

Automatic rendered Kino launches append `+set bo1_testclient_god 1` before Com_Init. This reuses the existing pre/post server-frame hook, which applies the canonical god flag to active living Zombies players. It survives spawn flag resets without timing or toggling a console command. Ordinary menu launches do not request it. The existing test dvar can be disabled with `/bo1_testclient_god 0`; an already-set god flag can then be toggled off with `/god`.

Added a diagnostic on actual off-to-on flag application. Full ARM64 engine and APK packaging passed: SDK663/stage230, BuildId db6a265a114e3bca25dd1f4957581bfa10b57393, observed PID26672. Actual launch log records startup request, automatic zombie_theater load, and `bo1_testclient: god mode enabled client 0 time 2400`. No console command or UI interaction after launch was needed.

Evidence: `build-android-audit/fresh-perf-20261015/deploy663.log`, `god663-entry.log`, `god663-spawn.png`; package hashes in `build-android-audit/native_registry_closure/sdk-package-verification230.json`. Initial build invocation used a nonexistent target (god-build.log); corrected production helper built blackopsarm_engine successfully. God-mode application is verified; this is not performance-goal completion.
