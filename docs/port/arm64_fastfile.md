# ARM64 FastFile preamble reader gate

Completed 2026-10-11 under TASK-0008, the first headless-engine milestone
(see roadmap_headless.md). A shared, platform-neutral reader parses the first
48 bytes of a zone FastFile — magic `IWff0100` / `IWffu100`, version `473`,
and the fixed-size `XFile` header — through a minimal synchronous file-I/O
boundary. This mirrors production `DB_LoadXFileInternal` header order without
attempting zone loading. Exact changes: `src/platform/fastfile_reader.h`
(new), `tools/port/tests/fastfile_tests.cpp` (new),
`android/cmake/arm64-audit/CMakeLists.txt` (+fastfile target),
`tools/port/check_fastfile.ps1` (new), `tools/port/run_arm64_audit_tests.ps1`
(+fastfile block, +stderr-tolerant adb steps).

## Source changes

- `Bo1FastFilePreamble` is 48 bytes with static-asserted offsets matching
  production `XFile` in `src/database/db_file_load.h` (version@8, size@12,
  externalSize@16, blockSize@20), plus a 12-byte `Bo1FastFileRawHeader`
  (magic@0, version@8) that mirrors what production verifies before inflate
  init. Retail survey forced this split: past raw byte 12, genuine files
  carry deflate bytes (e.g. `code_pre_gfx.ff` block word `0x28a47e95`), so
  the `XFile` header only exists post-inflate (TASK-0010). No engine source
  was modified and no 32-bit disk layout was touched.
- `Bo1_ParseFastFilePreamble` returns explicit `Ok / ShortBuffer / BadMagic /
  BadVersion`; `Bo1_ReadFilePrefix` / `Bo1_WriteFileWhole` / `Bo1_FileSize`
  return explicit `Ok / NotFound / ShortRead / IoError`. No success stubs.
- The I/O boundary is synchronous stdio (`fopen_s` on MSVC to satisfy
  `/WX C4996`, plain `fopen` on Bionic/POSIX). Production's overlapped
  `CreateFileA` + `ReadFileEx`/`SleepEx` APC pump and the zlib inflate stage
  remain future work (TASK-0009/0010).
- The probe roundtrips a byte-exact 48-byte fixture (retail magic, version
  473, `size=0x12345678`, blocks `0x1000..0x7000`) through a temp file beside
  its own executable, and covers unsigned magic, bad magic, versions 472/474,
  47-byte and empty buffers, missing files and 10-byte short files, plus
  raw-head fixture/negative paths. With `argv[1]` it verifies a genuine
  retail `.ff` raw head through the same boundary (fixture default keeps
  the test retail-independent).

## Verification

Final NDK r30 / Clang 21 / API 28 / arm64-v8a / Debug C++20 build
`build-android-audit/fastfile-20261011-185715098`: **0 compiler errors, 1261
retained warnings** (unchanged baseline), successful link, UBSan
non-recovering probe with no findings on passing paths.

All **14 native probes exit 0** on HONOR NDL-L09 / API36 / page size 4096,
serial AKWJ9X5116G04618, device dir
`/data/local/tmp/blackopsarm-phase1-20261011-185755303`. Frozen log:
[device-tests.log](evidence/arm64_fastfile/device-tests.log).

Host checks pass from this worktree: `check_fastfile` x86/x64
(`host-fastfile-20261011-185705875`, then `host-fastfile-20261011-190258517`
with the raw-head parser), plus full regressions `check_debugger`,
`check_parsetree`, `check_script_headers`, `check_narrow32`
(`host-debugger-20261011-185805713`, `host-parsetree-20261011-185836895`,
`host-script-20261011-185859625`, `host-narrow-20261011-185927544`).
Genuine retail verification: `code_pre_gfx.ff` (13,280 bytes) and 48 MB
`zombie_pentagon.ff` both report `IWffu100` / 473 / exact size on Windows
x86/x64; `code_pre_gfx.ff` pushed to HONOR reports the same on device
(`/data/local/tmp/blackopsarm-phase1-ffgenuine-20261011-190400000`, frozen in
[device-genuine.log](evidence/arm64_fastfile/device-genuine.log)). A full
census of all 158 retail `.ff` files (common + English) shows uniform
`IWffu100` / 473: [genuine-headers.tsv](evidence/arm64_fastfile/genuine-headers.tsv).
Final Android build `fastfile-genuine-20261011-190307816` re-ran the whole
14-probe suite green (frozen
[device-tests.log](evidence/arm64_fastfile/device-tests.log)).
`git diff --check` clean; `ledger doctor` healthy.

## Limits and next work

Retail game files are available at `BO1_GAME_DIR` (158 zone FastFiles) and
the raw-head contract is now verified against genuine files on both Windows
and HONOR. What remains fixture-only is the decompressed `XFile` field
content: genuine `XFile` bytes require the inflate stage (TASK-0010). Async
file staging, block allocation, asset parsing, server init and gameplay
remain untested. Windows/DXVK/root Ledger are unchanged.

Next: TASK-0009 zone block allocation and stream init. Frozen summary:
[compiler_summary.json](evidence/arm64_fastfile/compiler_summary.json).
