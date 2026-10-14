# Native cgame progress callback return type

Frozen source: `src/cgame/cg_info.cpp`, SHA256
`C5FD74454B20153787C602A5EF45D3DF739D3369C34229495E814E44940B5612`.
Source is released to the root agent for SDK444/stage16 integration. This owner
did not build or launch an APK; SDK444 actual runtime authority is pending.

## Actual trigger and repair

Actual SDK443/stage15 (BuildId
`a95a32d2819a96b172a5303a8374dc4a482a7ccd`) passes the preceding UI load-bar
direct-call repair. Its isolated launch log records actual Com_Init return at
line 19211, enters the genuine input/frame loop and reaches CG_DrawInformation
after UI_DrawMapLevelshot. It aborts at `cg_info.cpp:59:11`, calling a canonical
bool-return function through `unsigned char (*)()`. Diagnostic lines
19229..19231 and matching BuildId crash frame `CG_DrawInformation(int)+580`
retain this evidence. Engine PID 11305 is absent and its guard is dirty; this
does not prove visible gameplay or complete normal client frames.

CG_IsShowingProgress_FastFile and CG_IsShowingProgress_LoadObj are both declared
and defined as `bool __cdecl ()`. The native64 non-Windows branch now declares
the selected variable as `bool (__cdecl *v2)(void)` and assigns those canonical
functions directly. The Windows branch retains the original unsigned-byte
pointer and both casts. The explicit native-width header and guard
`BO1_NATIVE_64BIT && !defined(_WIN32)` preserve both Windows ABI paths.

Function selection still follows useFastFile->current.enabled after the same
UI_DrawMapLevelshot call. The original single `!v2()` invocation and
`&& serverLoading && g_waitingForServer` condition order remain unchanged.
FastFile still evaluates DB_GetLoadedFraction() > 0.0; LoadObj still tests
com_expectedHunkUsage > 0. Waiting-state updates, loading strings, translated
waiting message, timing/dots, text position and draw calls are untouched.
The existing Android Com_Printf format guard remains intact. No adapter,
alternate progress provider or substitute load-screen implementation was added.

The complete src search finds only this selected callback pair, their canonical
declarations/definitions and the actual CG_DrawInformation consumer in
ui_main_mp.cpp. There are no other wrong-return casts in this narrow cgame
progress family. The already frozen ui_atoms.cpp source is unchanged.

## Verification and evidence

Evidence directory: `build-android-audit/native_cgame_progress_callback1`.

- `cg_info.before.cpp` preserves all 2891 original bytes. `source.diff` and
  `source-manifest.json` record original/final hashes. CP1252 high bytes and
  original CRLF endings are preserved.
- Actual production compile_commands whole cg_info.cpp Android ARM64 TU exits 0.
  Only output-object destination differs; production O0/debug/function
  instrumentation/undefined sanitizer/no-recovery flags remain unchanged.
  Exact command, log and `cg_info-native.o` are retained.
- Native preprocessing exits 0. It proves one exact bool callback declaration,
  one direct typed assignment for each branch, no active wrong-return casts,
  the unchanged single callback/condition order and existing Com_Printf guard.
  `native.i` and four extracted function bodies retain the proof.
- Windows MSVC x86/x64 full translation units compile with exit 0 inside ARM.
  Before/final/read-only BlackOps64 preprocessing exits 0 on both ABIs. Every
  cg_info.cpp function body (CG_DrawInformation, both progress functions and
  CG_LoadingString) is byte-identical before/final/oracle on both ABIs.
  Exact commands, logs, preprocessed files and body extracts are retained.
- `check-results.json` records these passes and confirms source bytes stayed
  unchanged during checks. The source was frozen before root's SDK444 build.

No renderer mock fixture was added. SDK443 evidence remains in
`build-android-audit/native_registry_closure/sdk-native-entry443-launch1-only.log`
and `sdk-native-launch443-crash1.txt`; only the matching SDK443 BuildId is used
from the cumulative crash output. No assets, providers, config, assertions,
sanitizer options, device state, shared ledger/STATE or Windows oracle bytes
were changed by this owner. Root owns actual SDK444 integration and handoff.
