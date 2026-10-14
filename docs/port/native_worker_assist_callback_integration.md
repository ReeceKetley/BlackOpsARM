# Native worker assist callback ABI

Actual full426 completed G_InitGame and SV_SpawnServer for Kino, then the main thread failed at jqTempWorkerLoop1875 calling bool(void*) through a pointer whose actual provider was Sys_AssistSingleCallback(unsigned int*). Root's matching stack passes SV_WaitServer1464 -> SV_FrameInternal1656 -> Com_Frame. Full426 also has an independent concurrent snapshot-writer failure owned by another agent. This batch repairs only the demonstrated assist callback family; root owns production full427 integration. Matching full426 BuildId: 2220133ebe05f79fa25cce39b32dffce829187b0.

Only src/universal/com_workercmds.cpp changed. Two Android native64 TU-local adapters have the exact queue-required bool __cdecl(void*) type. Each casts the borrowed object pointer with static_cast and calls the existing provider through its actual declaration:

- BO1_AssistSingleCallback forwards to canonical bool Sys_AssistSingleCallback(unsigned int*).
- BO1_AssistAndWaitWorkerCmdCallback forwards to canonical bool Sys_AssistAndWaitWorkerCmdInternalCallback(jqBatchGroup*).

All three owned jqAssistWithBatches callback-erasing casts are replaced on Android: the single-assist call and the named/null-group assist-and-wait branches. Windows keeps all original expressions. No canonical provider signature/body, public header or queue engine was edited. The adapters return the exact provider bool; they do not convert an int/uint callback result, synthesize completion, remove a wait or bypass worker execution. Same-owner audit found no other jqAssist callbacks/casts. jqPoll and jqAreJobsQueued are actual bool APIs; canonical single provider's temporary v1 remains int, fed by its original unsigned decrement, and the provider still returns v1==0.

Native Sys_AssistSingle owns an unsigned int stack count to match the provider's actual pointee type, with the original four-byte width, initialization1 and address publication. Its first callback reads1/decrements to0/returnsfalse; its second reads0/decrements modulo32 toUINT_MAX/returnstrue. Windows retains its original int declaration. No callback retains or transfers this pointer: jqAssistWithBatches invokes jqTempWorkerLoop synchronously and returns only after that loop ends. Count is owned and accessed on the assisting caller's thread, so it does not acquire new atomic/concurrent ownership requirements.

The wait context remains exactly &name->module->Group for a named command, or null for the original global-group poll. The canonical provider remains return !jqPoll(context). Actual jqPoll selects the real module/global group and atomically loads its aligned64-bit BatchCount; the queued/executing32-bit components must both be zero for completion. Existing Android sequentially consistent BO1_JQ_LOAD and queue worker/counter publication are untouched. Original group filter, lifetime, queue processing, provider order and temporary worker retirement remain unchanged. Sys_AssistNeeded and all frontend worker polling/waits remain unchanged. No jobqueue engine patch is required.

Verification under build-android-audit/native_worker_assist_callbacks:

- Actual Android ARM64/API28 whole TU compiled once at final source, exit0, using production flags and UBSan/non-recovery instrumentation: com-workercmds-native.rsp and native-tu-final.log. Existing37 warnings retained.
- Normal Windows x86 and x64 whole TUs each compiled once at final source, exit0: {x86,x64}/build-result.json, command.txt and compile.log. Native-selection is false for these checks.
- windows-source-proof.json confirms that final selected Windows source matches the captured prior Windows source with whitespace normalized. Prior Android atomic32 include/fence adaptation selects original Windows behavior. Restoring only the inherited native-width include/guard spelling then gives the complete read-only BlackOps64 source with whitespace normalized. The prior baseline, this callback change and oracle comparison are therefore distinguished.
- Verbatim prior source is com_workercmds-before.cpp, SHA256 C3488DDAA6ACD15C580310C077071821D2BAB48906C9FCAD2980C9D94DC74DE3. Its9 bareLF line terminators in the existing atomic adaptation were normalized to CRLF without token changes. Final source is CP1252/CRLF with exact encoded-byte readback.
- No fixture was added: this narrow signature adapter introduces no new queue implementation, asynchronous context escape, atomic state or ownership regime. Actual production full427 is the authoritative completion/integration check; compilation does not claim the callback or full server frame has executed successfully after this repair.

Reproduce native whole TU:

```powershell
& D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe '@build-android-audit/native_worker_assist_callbacks/com-workercmds-native.rsp'
```

Frozen com_workercmds.cpp SHA256: F087B4E985169AA9E67ED083B715DB511BD9740151F0D7FA1F08D96996BD36BF. Public header remained SHA25669B49D69401781FD5EF340EAE9B09725E02027F8A1F2747B5158253129745B2C; inspected jobqueue source/header remained SHA256449530666E520BBC87F3C3E3309EF468AB29495ED0C234EEA7548B68C59AF385 / FD054473D627ECD818E592AF337AD7F2CC9041F87F3715E587309B982BB5C652. This report is explicit UTF8, temporary-write/readback verified before publication. No ledger, CMake, shared math, runtime, device, asset or library mutation occurred; no commit/reset/clean or protected reference writes.
