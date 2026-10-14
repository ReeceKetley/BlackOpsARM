# Native IWD clone metadata prefix

SDK461's real engine16859 (BuildId6f56e48691d11a65ac309de2b8558c2ad197de51) reaches CA_ACTIVE at81589ms, then logs `Invalid file (incorrect length)` for `sound\amb\electronics\computer\loops\mainframe\mainframe_loop_02_l.wav`. The subsequent physics null-pointer UBSan is a separate owner. This report establishes a concrete native filesystem defect and a matching warning mechanism; SDK461 does not expose the actual clone flag or mismatched size value. Revised device execution remains pending.

## Exact cause and bounded repair

`Snd_StreamLoadHeader` already decodes the original56-byte disk header field by field. Its version, loop and2096-byte header checks precede the length warning, and its length arithmetic matches read-only BlackOps64. `Snd_FileOpen` stores `FS_filelength` before `Snd_FileRead` performs `FS_Seek(handle,0,2)`. The file is therefore present and readable in the observed warning path; missing-file handling uses a different error.

In `FS_FOpenFileReadForThread`, a concurrently occupied IWD handle uses `unzReOpen`, which initially copies the previous current-entry metadata. The engine selects the requested entry on the shared archive, then copies exactly128 bytes into the clone. On ARM64 LP64 that omits compressed/uncompressed lengths, the remaining central metadata and the local-entry offset. `FS_filelength` can consequently return the prior entry's length. The subsequent absolute-zero ZIP seek calls `unzSetCurrentFileInfoPosition` on the clone, refreshing its metadata before reading the requested header, so a valid current header can be compared against the stale captured length.

| Canonical field/boundary | ARM64 LP64 offset | Windows x64 LLP64 boundary |
| --- | ---: | ---: |
| cur_file_info |80|44|
| compressed_size |128|68|
| uncompressed_size |136|72|
| cur_file_info_internal |216|124|
| pfile_in_zip_read |224|128|

The one native copy now uses `offsetof(unz_s,pfile_in_zip_read)` (224 bytes), guarded by `BO1_NATIVE_64BIT && !defined(_WIN32)`. Windows retains its literal128-byte copy. The existing file/pfile save and restore, archive selection, open/read calls, return value and all error/assertion behavior remain in their original order. The boundary excludes active read state and scratch members. The immediate family audit finds one such128-byte metadata copy; `unzReOpen` already uses `sizeof(unz_s)` correctly and is unchanged. There are no stream/header/asset/provider changes.

## Proportionate verification

- Final whole `com_files.cpp` native ARM64 and Windows x86/x64 translation units pass, with the existing instrumentation/UBSan native flags retained. Three layout compilation checks assert the canonical native offsets above and Windows metadata boundaries128(x64)/124(x86).
- Eight preprocess checks pass: native before/after and each Windows ABI before/after/read-only oracle. Native function reversal proves precisely the copy-size substitution. Windows function bodies equal their before snapshots; the oracle comparison explicitly normalizes only inherited `_iobuf`→`FILE` and `rand`→`BO1_CrtRand` changes. Exact source reversal also passes, preserving original encoding and surrounding line endings.
- Three tiny host byte-copy controls use the compiler-asserted ARM64 layout: distinct clone, same handle, and the old128-byte negative control. Metadata copies exactly; I/O pointer, active read pointer and full scratch tail remain unchanged. The old copy demonstrably retains stale149000 length and74565 local offset. These are host ownership simulations, not ARM execution.
- Only the named original `iw_15.iwd` entry was opened for final asset validation. Archive length149340, stored entry149340, header data_size147244 plus2096 equals149340. Version1/header2096/loop flag all pass; no padding applies at this size. Read only56 header bytes, SHA256 `0cea276e7427fff0d52d694ed91845e9dd4783fc6346c6d735d334318660c073`. Initial discovery consulted archive central indexes; no assets were copied or changed.

The initial comparison harness result is preserved. Compilation/layout checks passed immediately, but the comparison expected unexpanded `offsetof` and unnormalized inherited Windows FILE/RNG spelling. The harness was corrected to compare actual `__builtin_offsetof` output and name those two existing Windows differences; production behavior was not changed to satisfy it. Final verification recompiles the frozen source.

## Handoff

Frozen/released `src/universal/com_files.cpp` SHA256 67E6FA8976FA213DD1152DC2456529600C3E64B55F67E49204B11B8CD7C7B0BB. Root owns combined SDK462stage34 build/package/device observation and Ledger/STATE. Actual sound-warning removal, native cloned-entry playback and gameplay progression remain unclaimed until that run.

Evidence: `build-android-audit/native_iwd_clone_metadata1/check-results.json`, `ownership-and-retail.json`, per-command logs/preprocessed bodies, original source snapshot/diff, and frozen `source-manifest.json`. Actual SDK461 warning is preserved in `build-android-audit/native_registry_closure/sdk-native-entry461-launch1-only.log:11326`.
