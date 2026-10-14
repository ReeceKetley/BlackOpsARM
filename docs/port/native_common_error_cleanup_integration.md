# Original common error classification and cleanup integration

2026-10-13. Genuine Com_Init/Com_Frame and recoverable execution remain incomplete.

Eight original common owners now selected in existing full-only lifecycle scope:
Com_Error, Com_CheckError, Com_ErrorCleanup, Com_SetLocalizedErrorMessage,
Com_SetErrorMessage, Com_ErrorIsNotice, Com_PrintStackTrace, Com_ClearTempMemory.
FULL excludes inherited Com_Error/Com_ErrorCleanup diagnostics only where these
real bodies replace them; OFF diagnostics retained. Original classification,
critical sections, script/localization notices, tool/quit policy, cleanup ordering,
thread sync, DB/FS/hunk/script cleanup, escalation and TLS longjmp calls retained.
No fake recovery, skipped required callbacks or replacement startup sequence.

Native adaptations: bounded formatting before publication, explicit va_end before
potential nonlocal jump, literal-safe percent logging, actual typed32 seqcst atomic
scriptError exchange, original tool terminal status after real normal-exit callback,
and com_errorEntered cleanup reset under existing COM_ERROR critical section.
Windows retains original paths. Native formatting failure reports explicit text.

Com_PrintStackTrace uses actual Android bounded _Unwind_Backtrace capture without
narrowing program counters into legacy32 AddressInfo records. Canonical assertive
source selects original address-cache globals/reset body only through source-scoped
BO1_NATIVE_ASSERT_ADDRESS_STATE; remaining Windows map/symbol/dialog/assert bodies
excluded, not replaced with successful dummy implementations. Legacy cache count
reset is original; native trace does not pretend to populate Windows symbols/cache.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link316..319 record genuine source/header
  closure failures; an initial PowerShell alias collision left partial edits and
  was corrected before final checks. Logs retained.
- link320 compiles final common classification/cleanup; link321 test lacked
  initializer_list include, repaired; link322 compiles final stack fixture/service
  and still fails genuine full-engine dependencies.
- common-error-cleanup-definitions1.log confirms eight actual common owners,
  native capture and original Assert_ResetAddressInfo in archives.
- takeover_common_error_classification_assert_state_windows1 and final
  takeover_common_error_cleanup_final_windows1: two normal whole common/assertive
  TUs pass x86/x64 exit0 with final engine-source manifests.
- stack-trace-run1: HONOR UBSan actual provider verifies real frames/returned length,
  bounded1/2/9-byte outputs with untouched sentinel, null/zero-capacity EINVAL and
  all-frame skipping. Only changed68848-byte fixture transferred; no assets.

Real common error/cleanup caller paths have not executed. Full link still needs
Monkey_GrabComPrints/Monkey_Error, Sys_NormalExit, TaskManager2_ComErrorCleanup,
R_ComErrorCleanup, QuitOnError, NET_RestartDebug and other actual owners. No claim
that callback/runtime dependencies resolved merely from source selection.

Before recovery execution, verify initialized lock/worker/server/render ownership,
all scriptError readers use atomic accesses, TLS checkpoint thread/lifetime validity,
C++ lifetime/lock behavior across longjmp, error message/code publication and cleanup
reentrancy. Native stack capture is normal-context address logging; no signal-safe,
corrupt-stack or symbolic source resolver claim. Fixture proves buffer provider only,
not initialized Com_Error, Com_ErrorCleanup, failure escalation or recovered state.
Full Com_Init return/Com_Frame remains unlinked and unexecuted.
