# Real script-string platform prerequisites

FACT-0048: Windowsx86/x64 and HONOR execute actual scoped four-byte seq_cst
atomics, two independent recursive locks, contention and server identity.80000
atomic and80000 protected updates pass. Invalid lock/conflictingserver terminate
explicitly (Windows3/Android134) with case-specific diagnostics. Actual SL/MT pool
is not integrated or certified yet. No fake Windows intrinsics/global no-opservices.

Counters must be live writable naturally4-byte-aligned uint32 storage, with
consistent atomic access during concurrency. Windows load uses CompareExchange;
never pass readonly diskmetadata. Role is immutable/processlifetime; registered
server thread must remain alive whenever queried. Locks are processlifetime and
separate fromfirstfit. No reset/unregister/restartedserver contract is certified.

Firstprobe queried a saved pthread afterworkerexit; FAIL-0011 and original
arm64_script_string_services archive preserve the review failure. Corrected
probe holds server alive duringallqueries thenjoinswithoutfurtherrolelookup.
Current authoritative corrected evidence is arm64_script_string_services_lifetime.
Host /W4 /WX /O2 and service-only Android /Werror UBSan build0errors0warnings pass.

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
./tools/port/build_script_string_services_host.ps1 -OutputDirectory build-android-audit/script_services_host_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_script_string_services_host.py --host-build build-android-audit/script_services_host_repro1
./tools/port/build_script_string_services_arm64.ps1 -BuildDirectory build-android-audit/script_services_native_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_script_string_services.py --android-build build-android-audit/script_services_native_repro1 --host-results build-android-audit/script_services_host_repro1/results.json --output build-android-audit/script_services_device_repro1 --serial AKWJ9X5116G04618
```

Use fresh directories. The host runner writes all six results beside the built
binaries, with exact commands, raw output and confirmed exits. Its fresh host
pass3 test passed all cases; separate reproduction evidence is preserved in the
corrected lifetime archive. The native runner verifies ELF/PE/pushed SHA and
case-specific exits, creates fresh scratch and leaves no native process running.
