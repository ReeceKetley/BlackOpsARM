# Native weapon option color rows

Date: 2026-10-14. Scope: `src/cgame/cg_weapon_options.cpp` only. Source frozen after the checks below; Android SDK445 execution belongs to the root coordinator.

## Actual trigger

SDK444/stage16 engine BuildId `28df671011f1cc3bd9e4668cad970d3aac8dc77c` passes the preceding loading callbacks and returns canonical rendered `Com_Init`. Its same process loads `frontend.ff`, enters client initialization, then exits under the existing nonrecovering UBSan instrumentation:

```text
src/cgame/cg_weapon_options.cpp:121:15: runtime error: index 8 out of bounds for type 'float[4]'
```

The exact raw record is [sdk-native-entry444-follow1-only.log](../../build-android-audit/native_registry_closure/sdk-native-entry444-follow1-only.log); [sdk-native-launch444-follow1-crash.txt](../../build-android-audit/native_registry_closure/sdk-native-launch444-follow1-crash.txt) is the associated crash capture. Existing image warnings precede the abort but are outside this repair.

## Cause and repair

The canonical declaration is `float (*colors)[4]`. `WeaponOptions` owns `reticleColors[16][4]`, `lensColors[16][4]`, and `facepaintColors[16][4]`, and the three initialization callers pass those arrays directly. The decompiled expression `&(*colors)[4 * i]` indexes the first four-float row as if it were the complete 64-float object. The matching `sscanf` output addresses and normalization base repeat the same invalid row flattening.

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, initialization now accesses `colors[i][0..3]` and parsing/normalization accesses `colors[j][0..3]`. The existing `platform/bo1_native_width.h` supplies the ABI guard. The Windows and other legacy branch retains the exact original declarations and operations.

The reset still writes -1.0f to all four lanes of each of 16 rows before the table lookup. `StringTable_Find` still receives capacity 16 and the original label/column/filter arguments. The match loop still indexes color destination by match ordinal `j`, obtains table data through `rows[j]`, and only parses/scales a nonnull, nonempty string. `sscanf` still requests precisely three floats and its return value remains unused. Each lane still uses the original double literal `0.0039215689` in the original order, including alpha: alpha starts at -1.0f and is scaled for each nonempty selected row. Missing or partially parsed RGB values retain the original reset/scale behavior. No change to table selection, material validation, assertions, image lookup, asset data, fixed-width formats, renderer APIs, or instrumentation.

Typed row arithmetic selects a four-float array within the caller's 16-row object, then a lane in `[0,3]`. The largest reset destination is row 15/lane 3 (byte offset 252 in the 256-byte array). Parsing uses lanes 0 through 2 in the selected row; scaling uses all four lanes. This preserves physical offsets while expressing the correct array objects to the compiler.

## Same-file family audit

| Site | Finding |
| --- | --- |
| `InitWeaponOptionColors` reset | First-row flattening repaired for all 16 rows. |
| `InitWeaponOptionColors` parser outputs | Three same-family addresses repaired. |
| `InitWeaponOptionColors` normalization | Four lanes repaired using typed selected row; original scale and alpha retained. |
| Three `InitWeaponOptions` callers | Canonical 16-by-4 arrays; no caller cast or allocation repair needed. |
| `SetupWeaponOptionsRender` reticle color reads | Already indexes `reticleColors[v10][lane]`; unchanged. |
| `IsValidRenderOption` color validation | Already indexes the selected reticle row/lane zero; unchanged. |
| Reticle and lens color getters | Existing bounds/validity checks followed by typed row/lane reads and four `outColor` stores; unchanged. |
| Historical commented validation body | Inactive and already a selected row/lane-zero read; unchanged. |

Searching all live color references, `*colors`, flattened `4 *` indices and float casts in this file found no additional runtime color-row flattening consumer. The guarded legacy expressions remain in source and are absent from the active native initialization body.

## Validation and freeze

[check-results.json](../../build-android-audit/native_weapon_option_color_rows1/check-results.json) records the complete results. Commands, full logs, preprocessed bodies, original source snapshot, exact diff and source manifest are preserved in [native_weapon_option_color_rows1](../../build-android-audit/native_weapon_option_color_rows1).

- Production Android whole translation unit: exit 0, using the unique `cg_weapon_options.cpp` command from `native_engine_entry1/compile_commands.json`, with only the output object redirected. NDK30/API28, ARM64, O0/debug/PIC, function instrumentation, undefined-behavior sanitization and nonrecovery remain enabled.
- Native preprocessing: exit 0. Active body contains four typed reset lanes, three typed parser addresses, four original-scale normalization lanes, the original reset/match capacities and the original selection/conditional order. No active `&(*colors)` expression remains.
- Windows x86 and x64 whole translation units: both exit 0 using the established MSVC comparison command templates in the Android worktree.
- Before/after/read-only BlackOps64 oracle preprocessing: all six runs exit 0. For both architectures, `InitWeaponOptionColors`, `SetupWeaponOptionsRender`, `IsValidRenderOption`, `GetWeaponOptionReticleColor`, and `GetWeaponOptionLensColor` active bodies compare exactly equal against both the saved before source and the Windows oracle.
- All original CP1252 high bytes and CRLF line endings preserved. No concurrent source changes during validation.

The first checker attempt falsely treated the required `float (*colors)[4]` parameter declaration as remaining flattening. Its results are preserved in `check-results-attempt1.json`; all three compile runs had already exited 0. The predicate was corrected to search for `&(*colors)`, and the complete final check passes. This checker correction did not change the production source.

Frozen source SHA256: `7645721E4DADC92506BBF06A829980DF9841DD20043DDA439B78D42B9B292244` (42604 bytes). Before SHA256: `4D2A3259438423A4F3DB788DF10D49A9F0E73E93A86F5A49AAFDABE9B0C4E5FA` (41939 bytes).

These checks prove the source/ABI repair and Windows preservation. They do not establish a completed Android client frame or playable gameplay; the root coordinator owns the next actual APK execution.
