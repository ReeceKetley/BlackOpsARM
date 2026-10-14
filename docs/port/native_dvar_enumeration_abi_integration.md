# Native Dvar_ForEach callback ABI

Actual full414 map startup reached GameInitialization/G_SetupWeaponDef/G_InitGame and stopped when Dvar_ForEach invoked the differently typed Dvar_InfoStringSingle callback through its generic `(const dvar_s *, void *)` signature. Root owns production reproduction; this batch ran no device or production build.

`src/qcommon/dvar_cmds.cpp` now has six exact native adapters replacing seven casts:

| Canonical owner | Typed context | Invocation sites |
|---|---|---|
| Dvar_WriteSingleVariable | int file handle pointer | WriteVariables |
| Dvar_WriteSingleDefault | int file handle pointer | WriteDefaults |
| Dvar_ListSingle | const char filter, including null | List_f, both branches |
| SV_SetConfigDvar | actual three-int array | SetConfig |
| Dvar_InfoStringSingle | unsigned mask pointer | InfoString |
| Dvar_InfoStringSingle_Big | unsigned mask pointer | InfoString_Big |

Each adapter has the actual Dvar_ForEach signature and calls the canonical owner. SV's producer supplies DvarSetConfigInfo; its adapter reads the real start/max/bit fields and passes those values in a real int[3] because the owner indexes its input as an array. It consumes those values synchronously and does not retain a pointer. Signed/unsigned corresponding mask access retains original bit representation. File handles, filter strings/null, masks, info1/info2 buffers, enumeration order and original algorithms remain unchanged. Com_DvarDumpSingle already has the exact signature and was left alone.

Windows retains its original callback invocations. No Dvar_ForEach API/header, mask or string algorithm, sanitizer option, dependency selection or historical body gate was changed. CP1252 and original line endings preserved.

Evidence:

- Whole NDK native translation unit: `build-android-audit/native_registry_closure/dvar_cmds-enum-native.log`, exit0.
- Normal Windows x86/x64 once: `build-android-audit/takeover_dvar_enumeration_windows1/{x86,x64}/build-result.json`, exit0.
- No standalone fixture was added for this narrow adapter change; actual full415 integration is the next runtime proof. Compilation does not certify GameInitialization, Zombies load or gameplay.

Frozen source SHA256: 8C5259F379D69376ABBADD32F3364670746C7E8817A6B855737703B8CDFABC3E.

Integration: existing production dvar_cmds.cpp only; no new source/header/flag/CMake changes needed.
