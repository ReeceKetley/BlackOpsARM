# Real Android fatal error boundary and original common abort

2026-10-13. Genuine Com_Init/Com_Frame and recoverable error handling incomplete.

Original Com_ErrorAbort now selected in existing common owner only full target;
body unchanged, actual Sys_Error("%s",com_errorMessage). Native Android fatal_error
service replaces inherited unavailable Sys_Error only FULL_DATABASE=ON. OFF keeps
diagnostic endpoint. No fake success/recovery or substitute common error cleanup.

Native Sys_Error formats into bounded4096-byte local buffer, va_end before terminal
exit, emits original error text to stderr and real bounded32-frame _Unwind_Backtrace
program counters, flushes stderr, terminates all process threads with _Exit(2).
Null/failed format emits explicit failure text. Headless Windows reference also
terminates fatally with status2; native does not implement Windows console/dialog,
suspend-thread UI loop, symbolic address resolution or separate durable emergency
file logging. Native process/application stderr routing still matters.

This is terminal failure handling, not orderly shutdown or recovered engine state.
Com_Error and Com_ErrorCleanup remain diagnostic endpoints; real classification,
cleanup, shared error publication and TLS longjmp checkpoint lifetime still need
integration/audit. Native service does not fabricate those behaviors.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link314 compiles actual fatal service/common
  abort and links scoped fatal test; link315 compiles final real unwind addition.
  Full executable still fails genuine remaining engine/native links.
- native-fatal-abort-definitions1.log confirms actual Sys_Error and Com_ErrorAbort
  archive owners. No duplicate/unavailable replacement owner in FULL.
- takeover_common_fatal_abort_windows1: final normal whole common TU x86/x64
  exit0 with source manifests. Windows source fatal path unchanged.
- fatal-error-run1: HONOR UBSan actual service formatting/literal-percent/long
  bounded-output/null-format and child terminal-exit2 pass.
- fatal-error-run2: final service adds real unwind; all preceding cases and actual
  first nonzero program-counter frame pass. Child fixtures fork/pipe capture actual
  Sys_Error, no replacement terminal service. Only changed ~1MiB test executable
  transferred; no game assets or production entry run.

Fixture does not execute original Com_ErrorAbort caller, initialized Com_Init error,
all stack depths/corrupt stack/signal context or competing-thread fatal/error IO
failure. Stack frames are addresses, not validated symbol/source names. Fatal service
is normal-context code; no async-signal-safe crash handler claim. Process kill is
verified; orderly engine resource cleanup remains separate unfinished work.

Previous expanded current handoff preserved byte-for-byte with verified SHA256/size
in .ledger/archive/native-fatal-handoff-20261013. Reports and all ledger history
remain. Current STATE compacted to latest gates and explicit recovery limitations.
