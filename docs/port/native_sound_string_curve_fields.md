# Native sound filename and curve member repair

2026-10-14. Bounded SDK460 repair; source owners frozen and released for root's SDK461/stage33 integration.

## Actual failure evidence

SDK460/stage32 BuildId `8fd2538cc68423f78dbb4150812dc6525d67fee6`, actual engine PID13546 (now dead), reached actual CA_ACTIVE at 86580 ms. `build-android-audit/native_registry_closure/sdk-native-entry460-launch1-only.log` reports `snd_stream.cpp:1570:29`: member access through misaligned `snd_stream_file` at `0x006f199dcf79`, requiring 8-byte alignment. The same run reports `snd_globals.cpp:289:58`: index 84 outside `const char[32]`. `sdk-native-launch460-crash1.txt` identifies the matching BuildId and `Snd_CurveEval`/`Snd_CurveEvalOverRange` stack. These are production APK observations, separate from the static and host proofs below.

Prior packed snapshot field report was absent in this launch and the Player 0 second-snapshot message repeated. Actual video shows post-loading black HUD/compass only; neither visible 3D geometry nor playable gameplay is proved. The recording is a visual witness, not a performance benchmark.

## Ownership and repair

`snd_stream_file g_snd_stream_files[10]` is the existing aligned typed owner. In `Snd_ReadBuffer`, `file`/`free_file` are selected by array indexing; all selection, close/open, cache, age, read and failure behavior is retained. `I_strncpyz(free_file->filename, filename,260)` creates the bounded terminated filename. The decompiled lowercase loop advances the whole `snd_stream_file*` one byte and then accesses its first member; its second iteration violates struct alignment although the desired character itself needs only 1-byte alignment. The native branch instead initializes a `char*` to `free_file->filename`, increments that cursor and passes the same character expression to `tolower`. Its input conversion, result assignment and terminator condition remain the original semantics. The struct/global array, original source filename and filesystem path construction remain unchanged.

`snd_curve` owns `name[32]` at 0, `id` at 32 and `float points[8][2]` at 36; actual native layout is 100 bytes/alignment 4. For each loop index i=1..7, `8*i+28 =36+8*(i-1)` and `8*i+32 =36+8*(i-1)+4`. Therefore the four decompiled cross-name float reads are exactly the preceding point's x/y coordinates. Native code reads `curve->points[i-1][0/1]` directly. The actual failing index 84 is point 6 x, not a name character. Array shape, assets and all interpolation comparisons, expression order, float casts, double return, `(1.0-t)` and `(double)t` products, endpoints, invalid-curve fallback and assertions remain intact. The zombies L16 x87 precision comment is byte-identical; no interpolation precision change is introduced.

Both repairs use `#if BO1_NATIVE_64BIT && !defined(_WIN32)`; `snd.h` already imports the canonical `platform/bo1_native_width.h`. Windows x86/x64 retain their exact prior executable function bodies, also matching the read-only BlackOps64 oracle for both edited functions and immediate reviewed callees/callers.

## Bounded family closure

Reviewed every occurrence of character-member walking/casts and cross-name point reads in the two owned files. The reported filename loop is the only whole-struct byte cursor there; all four cross-name curve reads belong to the one interpolation interval. Remaining same-family expressions exist only in the preserved Windows branches. `Snd_LoadBuffer`, `Snd_FileClose/Open/Read`, `Snd_CurveEvalOverRange`, `SND_GetCurveById/Index`, the curve declaration/global constants owner, `SND_ParseInit` curve parameter/CSV fields and direct `snd.cpp` curve callers were inspected read-only. Those immediate paths publish/consume actual typed curve pointers and use ordinary array indexing. No adjacent ownership is required. This is not an all-sound audit; no other source was changed.

## Verification

Evidence: `build-android-audit/native_sound_string_curve_fields1/`.

- 6 translation units pass: both sources under the actual native ARM64 compile database and Windows x86/x64 comparison commands. Native instrumentation/UBSan flags remain unchanged.
- 16 preprocess runs pass: native before/after for each source, plus Windows before/after/read-only oracle for each source and architecture.
- All 44 native function bodies compare equal after only the exact filename cursor and four point-access substitutions: 2 changed, 42 unchanged. This covers all functions in both owned sources, including unchanged immediate callees/callers. Arithmetic, assertions, loops and all other operations are identical.
- 9 selected Windows function bodies across 2 architectures are identical to both before and oracle: 36 body comparisons pass.
- Actual compiler owner layouts prove `snd_stream_file` size 288/alignment 8/filename offset 0 and `snd_curve` size 100/alignment 4/name 0/id 32/points 36.
- Full-source reversal of the new guarded branches reproduces the byte-identical pre-edit files. CP1252 high bytes, BOM status and existing mixed line-ending counts are preserved.
- Host byte-offset proof covers all 14 preceding-point coordinate offsets and 112 raw bit-pattern checks, including signed zeros, infinities and NaN payloads. It also covers all 260 legal terminated filename lengths/33930 byte positions. This proves address/byte equivalence; it is not Android execution, filesystem integration or a new numeric benchmark. Existing exact native body comparison proves all interpolation operations/casts remain unchanged.

`check.py`, `check-results.json`, `source-manifest.json`, `*.before`, `*.diff`, native preprocessed bodies, native owner layouts, Windows comparison bodies, command/log artifacts and `offset_proof.py`/`offset-proof-results.json` retain the reproducible evidence. No engine build/package/device launch, provider/assets/configuration, Ledger/STATE change, commit or Windows-reference mutation was performed by this source owner. Actual SDK461 confirmation belongs to root's combined staged APK.

## Frozen source hashes

|Source|Before SHA256|Frozen after SHA256|
|---|---|---|
|`src/sound/snd_stream.cpp`|`C2533E635425E6C71F062D02D418744A596C616C02B26539EAECA85BF2B3050E`|`1F854DD69AE018F4E84A1B9F91718385D116B7F10DA3987868EE4C414E87A62B`|
|`src/sound/snd_globals.cpp`|`366772770F4E3F7F2A6AD5A78EB790E32A86B09747B487563447CA1E36A054DF`|`B42CF79A1186F0323F169A24B1807DA07FB0F80BBD5B8440D048C63A12B53425`|
