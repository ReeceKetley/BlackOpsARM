# Native UI load-bar progress call signatures

Frozen source: `src/ui/ui_atoms.cpp`, SHA256
`54EAF41BDEE71A2BB599E0AF60B4EE42CED1B93E4CF65AA0D9ED8C770869A73B`.
Source is released to the root agent for actual SDK443/stage15 integration.
This owner did not build or launch an APK; SDK443 runtime authority is pending.

## Actual trigger and repair

Actual SDK442/stage14 (BuildId
`bafb25441f54ec530624dd40f5abf90e296c3e6c`) passes the prior visibility/copy
failure and genuinely returns Com_Init. Loading-menu rendering then aborts
at `ui_atoms.cpp:147:14`: a call through `double (*)(double (*)())` does not
match the actual `double()` progress function. The isolated launch log records
the diagnostic at lines 19229..19231. The SDK442 crash stack identifies
UI_DrawLoadBar -> Window_Paint -> Item_Paint -> Menu_Paint ->
UI_DrawMapLevelshot. Actual engine PID 7976 is absent and its guard is dirty;
this repair does not claim visible gameplay or complete client frames.

Both canonical declarations in ui_atoms.h and definitions in ui_atoms.cpp
take no arguments. Four native calls now invoke them directly:
FastFile and LoadObj branches in UI_DrawLoadBar and UI_DrawSpinnerLoadbar.
The spinner has the same concrete incompatible casts and remains an active
Window_Paint consumer, so it is included in this narrow family repair.
No adapters, new progress providers or substitute renderer were added.

Selection still follows useFastFile->current.enabled. FastFile still returns
DB_GetLoadedFraction(); LoadObj retains its expected-hunk-usage zero handling,
Hunk_Used division and original upper clamp. The local double-to-float
conversion, bar width/UV fraction and spinner progress argument are unchanged.
The native guard is `BO1_NATIVE_64BIT && !defined(_WIN32)`; Windows keeps its
original call expressions on both ABIs. The native-width header is explicitly
included. CP1252 high bytes and original CRLF endings are preserved.

The complete src search finds these two draw consumers, their Window_Paint
call sites, canonical declarations/definitions and these four call expressions;
there is no other wrong-signature call in this progress family. No broad
unrelated callback audit or mock full-renderer fixture was introduced.

## Verification and evidence

Evidence directory: `build-android-audit/native_ui_loadbar_callback1`.

- `ui_atoms.before.cpp` retains all 6809 original bytes. `source.diff` and
  `source-manifest.json` retain before/final hashes and encoding checks.
- Actual production compile_commands whole-TU Android ARM64 compilation exits 0.
  Only output-object destination differs from the production command;
  O0/debug/function instrumentation/undefined sanitizer/no-recovery flags remain.
  `native-command.txt`, `native.log` and `ui_atoms-native.o` retain the check.
- Native preprocessing exits 0; the two draw bodies each contain one direct
  FastFile call and one direct LoadObj call, with zero active incorrect casts.
  `native.i` and four extracted native family bodies retain the proof.
- Normal Windows MSVC x86 and x64 whole-TU compilation exits 0 in BlackOpsARM.
  Both ABIs preprocess before/final/read-only BlackOps64 sources successfully.
  All four family function bodies are byte-identical before/final/oracle.
  Exact commands, logs, preprocessed files and extracted bodies are retained.
- Final `check-results.json` records all passes and verifies source bytes did
  not change during checks. Source was frozen before the root's SDK443 build.

Two initial extraction-only harness failures are retained in
`check-results-harness-failure{1,2}.json`: native preprocessing erases __cdecl,
then an overly permissive matcher selected a declaration. Corrected extraction
requires a function definition. A separate genuine comparison failure in
`check-results-windows-guard-failure1.json` showed BO1_NATIVE_64BIT also selects
the Windows x64 comparison path; the final guard explicitly excludes _WIN32.
The earlier intermediate source is retained in `ui_atoms.guard-before.cpp`.
All final production/Windows checks pass after those corrections.

Actual SDK442 evidence remains in
`build-android-audit/native_registry_closure/sdk-native-entry442-launch1-only.log`
and `sdk-native-launch442-crash1.txt`. No assets, providers, config, assertions,
sanitizer options, device state, shared ledger/STATE or Windows oracle bytes were
changed by this owner. Root owns the next genuine APK launch and handoff state.
