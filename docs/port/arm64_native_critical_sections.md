# TASK43 general native recursive critical sections

The existing native bridge now provides all 75 production enum critical sections.
The 72 additional identifiers use real process-lifetime recursive mutexes with
thread-safe once initialization. SL27, memory-tree28 and DBHASH74 continue to
route directly to their existing PoolLock objects. Repeated initialization cannot
replace a live lock; server-role publication remains unchanged.

This closes the enum mutex prerequisite for CBUF57, COM_ERROR12 and ASSERT29.
It does not implement engine TLS, Dvar's separate FastCriticalSection protocol,
actual `Com_Error` recovery, `Com_Init`, DB workers or gameplay. Normal Windows
`win_common.cpp` is untouched; Windows probes select the native bridge explicitly.

## Evidence

Windows x86/x64 and exact HONOR ARM64 O0/O3 binaries pass:

- Twelve simultaneous first initializers and mutual exclusion.
- All75 identifiers: same-thread recursion and try-lock, other-thread refusal at
  each of three depths, final-release acquisition and neighboring-lock independence.
- Four workers per section, 30000 protected increments per run.
- Cross-interface same-thread recursion and both directions of other-thread
  contention between PoolLock and the matching enum section, for all three pools.
- Unset server identity remains false. Invalid -1/75 identifiers fail for enter,
  try and leave: six controls per binary, exact exit3 Windows/134 Android.
- Fresh complete MP71 loader regressions pass Windows x86/x64 and HONOR O0/O3:
  all284 asset records/run, queries, child-lifetime and removal baselines.
- Genuine retained-provider default resolution and actual SL/MT name ownership
  pass all three ABIs at both ARM optimizations, including ref/pool/owner cleanup.

The test executable bounds itself with a20-second watchdog. The first runner
exposed Windows CRT abort reporting delaying invalid controls until watchdog
exit2. That run was interrupted, retained and excluded from final passing
evidence. Test-only abort policy was corrected, and the runner now requires the
actual abort exit codes. No production checks were disabled.

Read-only independent review found no correctness blocker. Invalid enum tests
do not certify caller misuse such as wrong-thread or unbalanced valid unlock.
Process-lifetime locks have no shutdown/reset service; future engine lifecycle
work must preserve live synchronization identities.

Reproduce from the Android worktree using fresh output directories:

```powershell
./tools/port/build_native_critical_sections_host.ps1 -OutputDirectory build-android-audit/native_critical_sections_host2
python -B tools/port/run_native_critical_sections.py --host-build build-android-audit/native_critical_sections_host2 --output build-android-audit/native_critical_sections_run2 --serial <detected-HONOR-serial>
```

The runner builds native ARM binaries with assertions/UBSan, records exact
commands, binary hashes and typed results, and verifies transferred ELF/SHA.
The existing startup-loader and default-resolution scripts produced fresh
`native_startup_zone_critical*` and `default_resolution_critical*` evidence.
`verify_native_critical_sections_evidence.py --output <fresh.json> --archive`
checks current source manifests, protected references and prior frozen archives,
then freezes log/JSON/TXT metadata under `evidence/arm64_native_critical_sections`.
Original retail/decoded payloads and binaries are excluded.

Next authentic boundary is native thread roles/TLS executing the original
`Com_InitThreadData` against canonical va/error/trace/CmdArgs storage. Keep
main/backend/database binding policy and hunk owner identity; native libc jumps
prove only binding/recovery mechanics, not production `Com_Error`. Then close
real Dvar/Cmd/Cbuf and DB-worker ownership/publication before full headless boot.
