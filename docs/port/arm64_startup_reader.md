# Production decoded-zone reader prerequisite

Verified 12 October 2026, continuing from `db6c0e4`. TASK41's complete MP
asset-dispatch milestone remains active; this report covers its reusable reader
and allocation-budget prerequisite only. No full engine startup is claimed.

`src/platform/native_zone_file.h/.cpp` owns the complete validated decoded source
and real named XBlocks. It uses the existing checked zone policy and actual
Android named-owner/DB allocator or Windows PMem. Reads check finite source and
destination capacity before mutation. Close refuses actual or copied stream
bindings; callers must first remove registered headers/child references, detach
streams and then release the owner. The object is noncopyable/nonmovable because
engine descriptors retain its addresses. Required thread/allocator initialization
is explicit; it does not fabricate registry, Dvar or worker initialization.

The production decoder adds optional `maxWorkingBytes` accounting for fixed
64 KiB I/O buffers, codec allocation requests with aligned bookkeeping, and
old-plus-new output storage during growth. The validated XFile size permits one
bounded reservation after its header. Zero keeps historical uncapped output
growth. `peakWorkingBytes` measures accounted allocation bytes, not process RSS,
allocator bookkeeping or all engine memory. Exact vector reservation behavior is
checked for the tested MSVC/libc++ configurations; other implementations are not
certified. Zone ownership is a later phase with a separate decoded-plus-owner
capacity check and bounded name storage.

Windows preflight follows the actual current low/high allocator cursors and
alignment. Native Windows refuses creating a third same-primitive record because
the inherited PMem free-stride path remains uncertified. This containment does not
alter the allocator or Android named-owner backend; wider owner support is a
separate dependency.

## Verified evidence

| Test | Windows x86/x64 | HONOR ARM64 |
| --- | --- | --- |
| Final reader: four alternating low/high complete real-file owner cycles | Pass, `native_zone_file_host_final3` | Pass, final2 O0/O3 binaries, assertions/UBSan |
| Source/destination failure atomicity and occupied-owner refusal | Pass | Pass |
| Actual and copied attached-stream close refusal, detach/free baseline | Pass | Pass |
| Full historical codec controls plus independent working-budget boundaries | Pass | Pass, O0/O3 |
| Chunk sizes 1/7/32768: observed peak succeeds, peak-minus-one rejects with empty output | Pass | Pass |
| Complete genuine MP startup independent metadata oracle and 13 rejection controls | Host oracle pass | Runtime full-source checksum matches |

All final reader runs consume 150108 decoded bytes, retain 27 zero padding bytes
and yield Adler32 777693148 from genuine `code_pre_gfx_mp.ff`. Codec accounted
peaks are 290716/290740/290764 bytes for x86/x64/ARM64; planned owner capacities
are 159744/442368/442368. Four cycles restore actual allocator ownership before
each next cycle. These runs read the complete source; they do not deserialize or
register all 71 assets.

Retail SHA256: `75ffbf7a2ca0fe926e0f499abd182b5a1c3bb52e7c218c3956fa736dfda2a44f`.
Decoded SHA256: `42ff63a5586b5876e0b2a54a45aa66b18c4a2678645715265f2bff141f044e88`.
The independent oracle accounts for all 3 StringTables and 68 RawFiles, earlier
initialized string aliases, children/checksums and exact source/block4 endpoints.
It produces metadata only and never supplies production payloads.

`verify_startup_reader_evidence.py` checks final reader source manifests, all
19930 inherited protected hashes, exact stable Windows/DXVK Git state and 689
prior checkpoint archive records. All pass. Older failed/superseded builds are
retained; notably an initial ARM template type mismatch was corrected before the
final builds. No assets, decoded payloads or executable binaries are archived.
Logs/metadata are frozen losslessly under `evidence/arm64_startup_reader`, with
original/archive SHA256 and byte counts in its inventory.

## Reproduction

From the ARM worktree, use fresh output directories:

```powershell
./tools/port/build_native_zone_file_host.ps1 -OutputDirectory build-android-audit/reader-host-new
./tools/port/build_native_zone_file_arm64.ps1 -BuildDirectory build-android-audit/reader-arm-new -Optimization O0
```

Run each host binary with the retail MP FastFile path. For actual HONOR use
`tools/port/run_native_zone_file_gate.py --build <arm-build> --output <fresh-output>
--serial <detected-serial> --optimization O0` (repeat with a fresh O3 build).
The collector checks ARM64 ELF, pushed executable/retail SHA, zero exit and all
four rows against independent source metadata. It preserves its exact commands.

The full codec suite is `tools/port/check_inflate.ps1` with the bundled Python
runtime and both genuine MP/SP pre-gfx files. In the native build, build target
`bo1_inflate_tests`, then run `tools/port/run_startup_codec_regressions.py` with
the matching build, fresh evidence output, detected serial and optimization.

## Next integration

Connect this reusable source to canonical production read functions and actual
`Load_XAsset` / pointer loaders / registry wrappers, then validate all 71 assets
and real configuration/table lookup. Preserve caller initialization and explicit
unload/lifetime contracts. See `production_startup_dependency_map.md` for the
authentic `Com_Init`/`Com_Frame` sequence and immediate native service blockers.
The serial reader does not replace or certify the asynchronous database worker.
