# TASK45 genuine FastCriticalSection native admission protocol

The original writer and reader/writer release functions now execute with native
fixed32 counters, consistently atomic polls/updates, and real scheduler yields.
The original Windows volatile-long type and function paths remain unchanged.
All seven Dvar reader sites, savegame admission and profile-cache polling have
conditional native atomic adaptations. This closes a synchronization prerequisite;
actual Dvar initialization and complete caller operations are not yet executed.

## Behavior preserved and tested

Readers increment before waiting for writers. Writers retain the original initial
reader check, writer increment, second reader check, failed-admission rollback
and retry/yield. Release assertions retain signed-positive meaning, including
rejection of zero and negative bit patterns. Native structure size8/alignment4
and counter offsets0/4 match Windows; no LP64 long pointer cast is introduced.

An independent historical oracle extracts actual Sys_LockWrite, Sys_UnlockWrite,
Sys_UnlockRead and NET_Sleep definitions at `cfbd591`. Windows x86/x64 pass3000
writer updates and3000 reader checks, queued/nested readers, independent locks,
canaries and zero counters. Reader admission is a contract caller; there is no
original standalone Sys_LockRead function.

Fresh native host x86/x64 and HONOR O0/O3 pass10000 writes and12000 reader checks
per run. Tests explicitly assert exclusive writer activity, yield between
generation/complement payload writes, verify coherent reader snapshots, blocked
writers, queued readers behind an active writer, nested reads, independent
objects, adjacent canaries and final zero counters. All24 zero/negative release
controls abort with actual assertion diagnostics (exit3 Windows/134 Android).
The driver contains a20-second process watchdog.

Sixteen original normal Windows function bodies and the canonical Windows type
are independently proven. Five complete normal Windows translation units per
ABI compile (database registry, Dvar, SP savegame, profile cache, win_common).
All non-ASCII lines are preserved. Root caught and restored incidental UTF-8
comment drift before final builds; the earlier source/compile failures remain.

The same-process native main/TLS/CmdReset plus genuine MP71 loader still passes
Windows x86/x64 and HONOR O0/O3 with all284 asset records, table queries, lifetime
and cleanup baselines. Twelve current source manifests,19930 protected hashes,
exact reference Git and1345 previous frozen records pass. 169 new metadata/log
records are frozen under `evidence/arm64_fast_critical_section`.

## ThreadSanitizer limitation

The installed NDK30/Clang21 ARM sanitizer runtime failed with exit139 and internal
`tsan_rtl.cpp:1043` ignore_reads_and_writes CHECK. Independent programs containing
no engine code, only a native atomic counter/std::thread/watchdog, reproduce the
same CHECK with ThreadSanitizer alone and with TSan+UBSan, after entering main.
They use the exact same runtime SHA. These diagnostics are retained as failed
sanitizer evidence, not passing race coverage. No runtime was patched and no
sanitizer check was disabled. Ordinary UBSan and protocol tests passed; they do
not establish exhaustive absence of data races.

## Reproduction

Use fresh output paths in the Android worktree:

```powershell
./tools/port/build_native_fast_critical_section_host.ps1 -OutputDirectory build-android-audit/native_fcs_impl_host4
./tools/port/build_native_fast_critical_section_arm64.ps1 -BuildDirectory build-android-audit/native_fcs_impl_O0_3 -Optimization O0
./tools/port/build_native_fast_critical_section_arm64.ps1 -BuildDirectory build-android-audit/native_fcs_impl_O3_3 -Optimization O3
./tools/port/build_fast_critical_section_windows_compile.ps1 -OutputDirectory build-android-audit/native_fcs_normal1
```

`generate_fast_critical_section_oracle.py` and
`build_fast_critical_section_oracle.ps1` produce the independent Windows control.
`run_native_fast_critical_section.py` collects both original/native evidence and
exact transferred ARM binaries using the detected HONOR serial.
`verify_fast_critical_section_source.py` proves normal source/type preservation;
`verify_fast_critical_section_evidence.py` verifies current builds, regressions,
protected references and prior archives, and optionally freezes metadata.
The TSan attempt and independent controls are reproducible through
`run_fast_critical_section_tsan.py` and `run_fcs_tsan_control.py`.

## Scope and next step

Writer locks remain nonrecursive; upgrades and fairness are not promised.
This protocol protects registry admission, not unrestricted concurrent Dvar
value mutation. Native error recovery while holding a writer is unverified.
The complete Dvar/savegame/profile operations are source-adapted, not runtime
certified by primitive tests. GPU services, DB workers, full Com_Init/Com_Frame
and gameplay remain unexecuted.

Next actual production boundary: Dvar/command/Cbuf startup closure with real
registration and SL-owned strings, followed by genuine database publication
and integrated headless startup. Preserve the original entry order from
`native_startup_services_plan.md`; do not manufacture a replacement startup.
