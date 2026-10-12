# TASK44 genuine native thread data with MP startup assets

Actual `Com_InitThreadData` now executes through native main/backend/database
role and TLS services using the original va, trace, error and CmdArgs storage.
The original initializer and `Cmd_ResetArgs` bodies are unchanged. Canonical
types were moved once to lightweight headers; normal Windows source paths remain
available. Native main registration agrees with the real Android hunk owner.

The original signed32 invalid-role literal `0xFFFFFFFF` required an explicit
`-1` spelling for Clang. Original MSVC controls prove its value is already -1;
size, representation and all other role values are preserved. No narrowing check
was disabled and no LP64 pointer/counter casts were added.

## Genuine execution and verification

Windows x86/x64 and exact HONOR O0/O3 execute the actual canonical initializer,
reset and native bindings. Simultaneously live main/backend/database roles verify
all12 original TLS slots, distinct full-width slot0 pointers, va/trace storage,
same-thread rebind preservation, original shared main/database CmdArgs identity,
three own-thread native libc jumps with exact return7, and two worker retirements.
Jump extents contain no owning C++ locals. This proves bindings/libc mechanics,
not production Com_Error recovery.

All72 root-collected negative controls pass: unregistered access, invalid and
unsupported roles, invalid slots, role switching, main retirement, duplicate live
roles, exit without retirement, retired access/rebind and retired role reuse.
Ordinary aborts are exit3 Windows/134 Android. Windows abort during CRT TLS
destruction uniquely produces0xC0000409, with its exact expected diagnostic.
This case-specific failure is recorded; watchdog or unexplained exits do not pass.

An independent oracle extracts the original declarations at `f117f5d`, computes
field expectations separately, and compiles/runs on x86/x64/HONOR. CmdArgs sizes
are10476/12592; TraceThreadInfo20/40; TraceCheckCount8/16; va_info4100 with index
offset4096. Actual canonical declarations, globals and both function bodies are
independently proven. Four complete normal Windows translation units per ABI
(q_shared, cmd, threads, cm_trace) compile with the new native selection off.

**Same-process integration passes:** actual native main/TLS initialization and
original Cmd_ResetArgs run before all71 genuine MP pre-gfx roots. All284 asset
records/run,1792 queries/cycle,357 cross-table references, full150108-byte source,
child lifetime and scoped removal/pool/owner baselines pass Windows x86/x64 and
HONOR O0/O3. Android integration uses one actual hunk-owner target. Host borrowed
thread objects are source-checked and their SHA/provenance is recorded.

The optional prefix adds one strict typed record to the existing288 MP records.
Seven altered scope/type controls are rejected by the independent collector.
Existing default test output remains288 records. Controlled Dvar caller inputs
remain test-only; reusable production libraries contain no test source.

Ten final source manifests,19930 inherited protected hashes, exact reference Git
and1207 prior frozen metadata records pass. 138 new logs/JSON/TXT records are
frozen losslessly under `evidence/arm64_native_thread_data`, including failed and
superseded probes. No retail/decoded payloads or build binaries are committed.

## Reproduce

Use fresh output paths inside the Android worktree. Standalone builders:

```powershell
./tools/port/build_native_thread_data_host.ps1 -OutputDirectory build-android-audit/native_thread_data_impl_host3
./tools/port/build_native_thread_data_arm64.ps1 -BuildDirectory build-android-audit/native_thread_data_impl_O0_3 -Optimization O0
./tools/port/build_native_thread_data_arm64.ps1 -BuildDirectory build-android-audit/native_thread_data_impl_O3_3 -Optimization O3
./tools/port/build_native_thread_data_windows_compile.ps1 -OutputDirectory build-android-audit/native_thread_data_normal1
```

`run_native_thread_data.py` accepts those three build paths, fresh output and
detected HONOR serial. Historical-layout generators/runners are
`thread_data_layout_oracle.py` and `run_thread_data_layout_oracle.py`.

For integrated MP execution:

```powershell
./tools/port/build_native_startup_zone_host.ps1 -OutputDirectory build-android-audit/native_thread_zone_impl_host1 -ThreadDataBuildDirectory build-android-audit/native_thread_data_impl_host3
./tools/port/build_native_startup_zone_arm64.ps1 -BuildDirectory build-android-audit/native_thread_zone_impl_O0_1 -Optimization O0 -ThreadIntegration
./tools/port/build_native_startup_zone_arm64.ps1 -BuildDirectory build-android-audit/native_thread_zone_impl_O3_1 -Optimization O3 -ThreadIntegration
```

Run host executables on the original MP code_pre_gfx file; collect using
`check_startup_zone_runtime.py --thread-integration`. Device runner
`run_native_startup_zone_gate.py` also takes `--thread-integration`.
Final source/evidence verifiers are `verify_native_thread_data_source.py` and
`verify_native_thread_data_evidence.py`; the latter optionally freezes archives.

## Remaining boundaries

Role claims are intentionally one-shot: main is process lifetime; workers must
retire, and roles cannot be recycled. Server/general workers/events/spawning and
Android app restart are unsupported. Original Com_InitThreadData has its original
unchecked caller body; use validated Sys_Init services for native binding.
Collision thread initialization is a different operation and is not certified.

Actual Dvar/Cmd/Cbuf initialization and their separate FastCriticalSection
protocol, DB worker/files/publication, full Com_Init/Com_Frame and Zombies
simulation remain unexecuted. Android zone owners still require main-thread
allocation. This checkpoint increases authentic production code executing
together; it does not establish integrated application startup or gameplay.
Next: `native_dvar_lock_plan.md`, real Dvar/command services, then DB orchestration
and genuine headless startup before rendering.
