# Native console name callback signature

2026-10-14. Scoped producer repair for SDK470/stage42. Frozen source:
`src/client/cl_console.cpp`, SHA256
`9A730E4CE818484DE2CE3E3775C590506E091A75285CC223799A8BDADFE44774`.

## Actual failure and exact target

SDK469/stage41 BuildId `fdb9bc2173bccb513d8a94f4edb158e435cc899a`,
PID19207/SDLThread19280, died while typing `map zombie_theater` into the console.
Enter had not been pressed; this is autocomplete drawing, not map execution.
The matching log reports dvar.cpp116 calling a function through the wrong
`void (*)(const char *)` type, with target engine offset `0x2946884`.

The immutable stage41 ELF resolves that offset to
`ConDrawInput_DetailedDvarMatch(char*)`, cl_console.cpp3322. The staged manifest
records engine SHA256
`6E455D9F972991AB12E21F042DF2505482F3F89096656C963AF48283F8104262`.
Symbol command/output is preserved in `stage41-exact-target.txt`.

Actual evidence, retained unchanged in `build-android-audit/native_registry_closure`:
`sdk-native-entry469-console-map-text1-only.log` and `sdk-native-launch469-console-map-text1-crash.txt`.
The successful rendering/counter gate is distinct from this console failure.
No claim of successful Zombies execution follows from this repair.

## Ownership, types and bounded change

`Con_DrawInput` owns two registration pairs, detailed and compact matches.
Both use `Con_ForEach(bool, void(const char*), void(const char*))`, which
forwards through `Con_Restricted_ForEach`. Dvar enumeration reaches
`Dvar_ForEachConsoleAccessName` and, in the unrestricted branch observed here,
`Dvar_ForEachName`; command enumeration has the same const-name callback ABI.

The producer previously cast four `void(char*)` canonical drawing functions
into that ABI. The native path now registers four static `void(const char*)`
adapters: `ConDrawInput_DetailedDvarMatchEnumerated`,
`ConDrawInput_DetailedCmdMatchEnumerated`, `ConDrawInput_DvarMatchEnumerated`,
and `ConDrawInput_CmdMatchEnumerated`. Each invokes its own canonical function
once with the identical pointer via an explicit `const_cast<char*>`.

All four canonical functions return void. Their declarations, bodies, drawing
side effects and return behavior remain unchanged. The adapters do not copy,
replace or queue names. Scope is solely this confirmed producer family in
cl_console.cpp; no enumerator, header, input/backend or unrelated callback edit.

The cast is justified by the canonical functions' read-only name use: they
compare names through `Con_IsAutoCompleteMatch`, read the character used to
recognize exact matches, find dvars or command file completion metadata, and
submit text drawing. None assigns the pointer or a pointed-to name byte.
`ConDrawInput_Text` and `ConDrawInput_TextLimitChars` forward to
`R_AddCmdDrawText`; its cursor command builder copies text into the command's
own allocation using memcpy and terminates that owned copy, leaving the input
untouched. Dvar and command lookups accept const char*. The canonical source
bodies used for this inspection are preserved alongside the checks.

The additions use `#if defined(__ANDROID__) && BO1_NATIVE_64BIT`; Windows
retains its previous registration text and canonical callback functions.

## Verification and limitations

- Production Android ARM64 cl_console.cpp command, copied from the current
  compile database, compiles successfully to a separate evidence object.
  O0/debug/instrumentation/UBSan/no-recovery flags remain intact.
- Native before/after preprocessing shows all four canonical bodies identical.
  Con_DrawInput differs only by the four adapter registrations; no cast to an
  incompatible callback remains in that native producer body.
- Windows x86 and x64 preprocess successfully for prior/edited/reference
  sources. Con_DrawInput and all four canonical bodies are identical to both
  the prior source and read-only BlackOps64 oracle; adapters are absent.
- A bounded C++ forwarding fixture extracts the actual four adapter bodies.
  Four static_assert checks prove exact NameCallback types; four executed
  cases prove one call, identical pointer and unchanged input bytes. This is
  adapter forwarding proof with test callees, not actual game execution.
- Reversing only the three recorded replacements reproduces the prior source
  bytes exactly. Original CP1252 high bytes and CRLF endings are preserved;
  zero bare LF before and after. Source remained unchanged through checks.

Evidence: `build-android-audit/native_console_name_callback_signature1/`.
`check-results.json` records passes; command/log files retain compiler evidence,
`source-manifest.json` records before/after hashes and encoding/endings, and
`replacement-blocks.json` records the inverse proof. Root owns the engine build,
packaging, device and Ledger; none was performed by this scoped agent.
Actual console typing and subsequent command execution remain root runtime gates.


Root actual SDK470/stage42 validation passes the former autocomplete crash.
BuildId b1a729750397ae1b463e0dfab606091d97232638/PID28098 accepts real console
typing and slash-prefixed dvar/asset-pool commands, returns their results and
continues rendering. Canonical active-level console requires a slash prefix;
the first unprefixed queries were local chat and are not command proof. Current
query-save2/query-menulist2/query-menu2 logs preserve the actual successful
execution. No current470 runtime error observed; playable Kino remains pending.
