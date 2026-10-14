# Native menu script callback signature

Frozen for root integration in SDK468/stage40 on 2026-10-14. Only production source edited: `src/ui/ui_shared.cpp`, SHA256 `81518371991AB9D49C714BA850BA53CF145C57F2A5993C75120A06A94B6605B5`. Actual Android validation remains pending.

## Actual failure and exact callee

SDK467/stage39 engine BuildId `2bf26210860377eb187dafd0e66865967a473a2b`, PID5746, SDLThread5839, died after root's physical ADB Enter at 07:11:23.764. The fresh matching log reports `ui_shared.cpp:2370:9`: call through `void (*)(int, int, itemDef_s *, const char *)`. Its callee offset `0x64f89c` resolves in the preserved stage39 ELF to `Cbuf_ExecuteBufferUI(int, int, itemDef_s*, char*)`, `cmd.cpp:400`. Every engine frame in the matched crash segment has the same BuildId. Stack: Com_EventLoop -> CL_KeyEvent -> UI_KeyEvent -> Menu_HandleKey -> Menu_CheckOnKey -> Item_RunSingleEventScript -> Item_RunScript -> Script_ExecNow -> Script_ExecHandler.

The canonical `cmd.h` declaration and `cmd.cpp` definition agree on mutable `char *`. The UI callback declaration and both consumers agree on `const char *`. The previous producer cast converted one function pointer type to the other, which UBSan correctly rejected. `execnow` and the three conditional `execNowOnDvar*Value` producers share this mismatch. The queued producers already use the exact `Script_AddTextWrapper` signature; all three predicate declarations/definitions match their consumer type. The command table's 50 registrations use their exact canonical script-handler type.

## Repair and ownership

A local `Script_ExecuteBufferUIWrapper(int,int,itemDef_s*,const char*)`, guarded by `defined(__ANDROID__) && BO1_NATIVE_64BIT`, receives the callback with its exact type and calls the real `Cbuf_ExecuteBufferUI` directly. Only the four immediate producers select that wrapper in the native branch. Their Windows branches retain the original cast. No callback/header/table declaration, executor implementation, parser, predicate, assertion, reference count, buffer size, command order or UI behavior changes.

The wrapper's `const_cast<char *>` restores the type of the owned mutable stack buffer that each existing consumer supplies: `Script_ExecHandler` owns `char val[1028]`; `Script_ConditionalExecHandler` owns `char command[1024]`. Both parse then append the same newline before calling the callback synchronously. No string literal or const object is supplied to this adapter and no buffer outlives its handler. The canonical executor forwards the same client, controller, item and pointer to `Cbuf_ExecuteBufferInternal`; that implementation reads the input and copies individual commands to its local destination before canonical dispatch. The adapter creates no independent command implementation and claims no fabricated success.

## Verification

- Four production TU compiles pass: native `ui_shared.cpp` and canonical `cmd.cpp`, Windows x86/x64 `ui_shared.cpp`. Native canonical O0/debug/PIC/instrumentation/UBSan/no-recovery flags retained.
- A native compiler proof includes the full production TU and statically verifies the adapter/add-text callback signatures, actual mutable executor signature, command-table handler type, direct and conditional consumer signatures, and all three predicate signatures. Full TU compilation proves all 50 actual table registrations and all four native adapter producers type-check without a function-pointer cast.
- Eight preprocess passes compare native before/after and Windows x86/x64 before/after/read-only oracle. 62 native Script bodies have only the four expected adapter substitutions (plus the new wrapper); all other extracted bodies are identical. All 62 Windows x86 and 62 Windows x64 Script bodies are before-identical; the 15-function immediate/queued callback family is also oracle-identical on each Windows ABI.
- Exact source reversal reproduces all before bytes, including prior resource/drawing guards. CP1252 high bytes, BOM status and existing mixed line-ending count are preserved. `ui_shared.h`, `cmd.h` and `cmd.cpp` remained read-only and their hashes are retained in `freeze-proof.json`.
- 633 source-extracted Windows x64 host checks pass. Fixture uses the native preprocessed script handlers/adapters, original predicate bodies and the canonical UI executor/command splitter, with bounded stand-ins only at parser/dvar/client-controller/downstream dispatch boundaries. Checks cover all four immediate and corresponding queued variants, three client IDs, exact item/controller forwarding, cursor consumption, newline preservation, quoted semicolons, multiple-command splitting, empty/incomplete parse, false conditions, synchronous indirect adapter dispatch and unchanged input bytes. Host fixture is not APK execution and does not claim Android menu success.

The first fixture compile failed from harness extraction selecting the header's inline overload instead of the core definition and MSVC rejecting a duplicate intrinsic stub. Its raw log is preserved as `fixture-first-harness-failure.log`. Harness now extracts the core definition, preserves the exact header shim, and maps the native debugtrap spelling only within the host fixture. Production source did not change during fixture repair.

## Evidence and next action

Evidence directory: `build-android-audit/native_menu_script_callback_signature1`. `source-manifest.json`, `freeze-proof.json`, `check-results.json`, `signature_proof.cpp`, `fixture-results.json`, compile/preprocess logs, before source/diff and fresh matched SDK467 crash/UBSan/callee resolution are retained. Root owns device/build/Ledger/STATE; this agent performed no masterbuild, package/install, device operation or asset/provider/config edit.

Root can integrate frozen `ui_shared.cpp` into SDK468/stage40, then reproduce title Enter/menu navigation through the genuine input/frame loop. Absence of the old type-mismatch report and successful actual immediate command execution need fresh matching APK evidence; conditional paths are not individually runtime-claimed by these fixtures.
