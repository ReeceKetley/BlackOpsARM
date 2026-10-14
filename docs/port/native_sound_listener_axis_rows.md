# Native sound listener and entity orientation rows

2026-10-14. Source scope: `src/sound/snd_public_async.cpp` and the explicitly authorized two related consumer blocks in `src/sound/snd_public_async_q.cpp`. Both sources frozen/released for root-owned SDK447 integration. No sound header, structure, command layout, lock, command pool, lifetime or device change.

## Actual failure

SDK446/stage18 engine BuildId `f1a7c2d69c88559d3ad9bda61dd04e9da169d9b1` passes the preceding resource callback repairs and enters the first active frontend snapshot. Existing nonrecovering UBSan stops at `snd_public_async.cpp:441:46`: `index 3 out of bounds for type 'const float[3]'`. The current crash segment shows `SND_SetListener` reached through `CG_SetInitialSnapshot`, `CG_ProcessSnapshots`, `CG_DrawActiveFrame`, and `Com_Frame`.

Evidence: [sdk-native-entry446-follow1-only.log](../../build-android-audit/native_registry_closure/sdk-native-entry446-follow1-only.log) and [sdk-native-launch446-follow1-crash.txt](../../build-android-audit/native_registry_closure/sdk-native-launch446-follow1-crash.txt). The crash capture also retains older launches; only the segment with the SDK446 BuildId establishes this failure.

## Repair and command contract

`SND_SetListener` receives the canonical `const float (*axis)[3]` for a 3-by-3 matrix. The old `(*axis)[3..8]` accesses exceed the first three-float row. Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, all nine values now use `axis[row][lane]` and the destination `cmd->context.set_listener.axis[row][lane]`. Listener/client/team and all three origin coordinates likewise use their canonical `set_listener` members. The listener queue consumer now passes the canonical fields and matrix directly to `SNDL_SetListener`.

The scoped same-family destination audit also found `SND_SetEntState` manufacturing a matrix pointer from `&cmd->context.play.position[2]`, with origin/velocity written through unrelated scalar union members. Its native producer now passes `set_ent_state.origin`, `.orientation`, and `.velocity` to the canonical `CG_GetSoundEntityOrientation(SndEntHandle, float*, float (*)[3], float*)` and assigns `.handle` directly. The related queue consumer now passes those canonical members to `SNDL_SetEntState`, preserving the consumer's origin/velocity/orientation argument order.

The header already declares actual `float axis[3][3]` and `float orientation[3][3]` objects in these payloads. Direct typed member access gives the compiler the correct matrix contract throughout production and consumption. Windows and other legacy paths retain the complete original operations.

## Measured layout proof

The [layout.cpp](../../build-android-audit/native_sound_listener_axis_rows1/layout.cpp) compile-only oracle includes the actual project header and asserts every replaced alias's offset, the two matrix extents, scalar widths, and payload sizes. It passes for production Android ARM64, MSVC Windows x86, and MSVC Windows x64. Compiler record-layout logs accompany each command. Native proof completed before editing the consumer.

Offsets below are relative to the union payload, identical on all three ABIs:

| Canonical field | Offset | Original overlapping expression |
| --- | ---: | --- |
| listener.listener | 0 | play.alias / stop_alias.ent.handle |
| listener.clientNum | 4 | play.fadeTimeMs |
| listener.team | 8 | set_game_state.cg_time |
| listener.origin | 12 | existing origin[0..1], play.position[0] at 20 for origin[2] |
| listener.axis | 24 | play.position[1] base; position[2] at 28; direction[0..2] at 32/36/40 |
| set_ent_state.handle | 0 | stop_alias.ent |
| set_ent_state.origin | 4 | set_playback_attenuation.attenuation |
| set_ent_state.velocity | 16 | set_listener.origin[1] |
| set_ent_state.orientation | 28 | play.position[2] |

`snd_set_listener_context` remains 60 bytes; `snd_set_ent_state_context` remains 64 bytes. Pointer width affects the trailing `snd_play_context.playback`: x86 offset 44/size 48, native and Windows x64 offset 48/size 56. It does not move any overlapping scalar/vector field used here. `snd_command_context` remains 64 bytes; native/x64 `snd_command` is 80 bytes with payload offset 8, while x86 is 76 bytes with payload offset 4. No layout declaration was changed.

All listener writes retain the same physical offsets and order. Matrix storage spans offsets 24 through 56 inclusive; each axis row stays within lanes 0 through 2. Entity orientation uses the actual nine-float object at offset 28. Queue allocation, command type assignment, single publication after payload fill, dispatcher order, commit/pop behavior and object lifetime remain unchanged. The original `SND_Active` gate, listener normalization assertion, entity eligibility checks and no-device listener fallback remain intact.

## Bounded audit and validation

The only active native flattened first-row read in the scoped listener/3D entity-orientation family was the repaired listener matrix. The paired entity producer/consumer had the confirmed matrix-pointer contract issue described above. `SND_UpdateEntState`, `SND_GetEntState` and `SNDL_SetEntState` already use three separate typed rows or the existing native-safe `AxisCopy`, and remain unchanged. Other sound command payloads and unrelated scalar casts are outside this change.

- Production Android whole translation units for both files: exit 0 using each unique command from `native_engine_entry1/compile_commands.json`, redirecting only output objects (producer also requests layout diagnostics). O0/debug/PIC/instrumentation/UBSan/nonrecovery unchanged.
- Windows x86/x64 whole translation units for both files: all four exit 0 using the established worktree MSVC comparison templates.
- Both native preprocessing runs: exit 0. Listener contains nine row/lane accesses and canonical scalars; entity producer and the two consumer blocks contain direct typed payload members; no active first-row flattening or matrix cast remains in those blocks.
- Twelve Windows preprocessing runs (before/after/read-only BlackOps64 oracle, both sources/architectures): all exit 0. Five producer-side bodies and the entire `SND_CommandProcess` body compare equal against both before and oracle after normalizing only macro-generated assertion source filenames/line numbers. Raw preprocessing files remain preserved; assertion conditions, messages and hand-authored source locations are not normalized.
- Actual-header layout/extent assertion compilation: all three ABIs exit 0.
- Original CP1252 high bytes and CRLF line endings preserved in both sources; no concurrent source changes during validation.

[Producer results](../../build-android-audit/native_sound_listener_axis_rows1/check-results.json), [consumer results](../../build-android-audit/native_sound_listener_axis_rows1/consumer/check-results.json), and all commands/logs/source snapshots/diffs/manifests are in [native_sound_listener_axis_rows1](../../build-android-audit/native_sound_listener_axis_rows1).

The first checker attempt is retained in `check-results-attempt1.json`. Its compiles/layout proofs already passed; comparison mismatches were generated assertion `__FILE__`/`__LINE__` metadata, and the flattening predicate also matched the required parameter declaration. The checker now normalizes only that generated diagnostic metadata and searches inside the body. The complete final check passes without further production-source edits.

Frozen source hashes:

| Source | SHA256 | Bytes |
| --- | --- | ---: |
| snd_public_async.cpp | `F5FEACC025BC94A6D9EA5C831FB5816FF167E208186E5FF933EEB5F5BA7E830C` | 50808 |
| snd_public_async_q.cpp | `615583EE919D64DF51671ED99AC3E15566EFF14C36CF33967E09DC2644DE36CE` | 31774 |

These are source/ABI and Windows-preservation checks, not hardware audio or a completed rendered-client proof. Root owns the next actual SDK447 launch.
