# Actual native cinematic job callback signature

2026-10-14. Scoped source owner: `src/gfx_d3d/r_cinematic.cpp` only; no shared declaration or job queue edits. Root owns integrated build/SDK device execution.

Actual SDK433 log `native_registry_closure/sdk-native-entry433-launch1-only.log` reports UBSan at `tl/jobqueue/jobqueue_all.cpp:1812`: calling `int (*)(jqBatch*)` through a pointer to an incompatible function. Root resolves the stage5 shared-engine target to canonical `_UpdateFrameCallback()` in r_cinematic.cpp. This happened on real JQWorker2 after actual graphics startup, not an isolated fixture.

The module producer used `.Code = (int(__cdecl *)(jqBatch *))_UpdateFrameCallback`. The actual callback is declared/defined `int _UpdateFrameCallback()` with no argument; the working BlackOps64 uses that same declaration/body and cast. Search finds no separate direct producer/caller requiring a declaration change. Canonical `jqModule::Code` in `jobqueue_all.h` is `int (*)(jqBatch*)`, and both job queue invocation sites call it with their actual current batch.

Android now assigns `_UpdateFrameModule.Code` directly to a local exact `int NativeCinematicUpdateFrameCallback(jqBatch*)` adapter without a function-pointer cast. The adapter calls the existing no-argument `_UpdateFrameCallback()` normally and returns its actual result. The callback remains unchanged: actual `R_Cinematic_UpdateFrame_Core(false)` executes first, `_InterlockedExchange(&cineThreadBlock,0)` follows it, then the callback returns0. Job queue LABEL21 interprets0 as completed/no requeue and retires execution counts. No synthetic success, weakened sanitizer, modified work ordering, premature flag release or independent cinematic implementation is introduced.

`R_Cinematic_UpdateFrame` still atomically claims cineThreadBlock, updates actual renderer images and submits `_UpdateFrameWorkerCmd`. Module limit1, command size, queue storage/lifetime and shutdown's real `Sys_WaitWorkerCmdInternal` stay unchanged. The batch argument is intentionally unused because the original canonical callback consumes no batch payload; the adapter creates no retained batch pointer. Native cinematic critical-section guard behavior is unchanged.

Whole native TU compilation with the current production flags (function instrumentation and UBSan/no recovery retained) passes. Normal Windows x86/x64 whole TU compilations each pass once using established `/permissive /DWIN32=1` options. All three retain final source hash. The final actual AArch64 object contains both the typed adapter and original callback. Saved `native-adapter-disassembly.txt` shows a direct CALL26 relocation to `_Z20_UpdateFrameCallbackv` and preserves its returned w0 through instrumentation to the adapter's return. The direct module assignment provides compiler type checking. No fake callback/core provider fixture was added; the next actual SDK execution is the meaningful worker-completion authority.

Evidence: `build-android-audit/native_cinematic_job_callback1` holds exact command/log/result files, before bytes, manifest, object and adapter disassembly. Removing the Android adapter and initializer branch restores exact captured prechange bytes; all new lines are ASCII-compatible CRLF, original CP1252 bytes are retained. Normal Windows original module registration/callback bodies and shared header are unchanged.

Before SHA256: `CF749F5B3FAE0687A6E1A67B4C58C6C4D705252BDFD70C86733D93DC8DB8EB98`

Final SHA256: `034CC9E3B706262D82CD5D8A2AE8A7E71C6DE4B2005AEF348033568FFB622E09`

No full build, device command, asset transfer, protected oracle/provider write, configuration/runtime change, ledger mutation or broad audit/test suite was performed. Actual successful cinematic worker execution/later rendering/lifecycle remain pending root's next real SDK run.

## Actual SDK integration result

SDK434/APK6 uses this frozen source and passes the previous cinematic queue
callback failure. It completes graphics startup, loads patch and UI fastfiles,
then reaches the real loading-screen text producer. A distinct runtime command
alignment issue aborts there; no later cinematic playback or rendered-gameplay
claim follows from this callback repair. Matching engine
BuildIdd934146ffe257d28dd82c1334f0a296489155ea0 and raw evidence are in
native_registry_closure/sdk-native-entry434-launch1-only.log and
sdk-native-launch434-crash1.txt.
