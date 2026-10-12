# TASK-0029: genuine RawFile name and buffer

The second genuine `code_pre_gfx.ff` asset now loads its complete name and buffer
on HONOR ARM64 and Windows x86/x64. After the original numeric header expander,
the unchanged `Load_RawFile(false)` follow-up invokes the original `Load_XString`,
byte allocator, raw-string loader and `Load_ConstCharArray`. Controlled cases also
execute the wrapper's `true` header-read path. No engine implementation is copied.

A narrow compile selector exposes the canonical bodies/globals from `db_load.cpp`.
The new checked child planner validates complete source and actual disk/native
capacities before writes. It reuses the existing inline-name planner. Encoded
names remain unsupported by this preflight until initialized-target evidence is
available. Buffer tags retain production semantics: any nonzero value means a
new allocation and an exact signed `len+1` byte read. No generic NUL validation,
payload multiplier, allocation policy or FastFile layout is added.

| Genuine code child | Decoded extent | Bytes | Adler32 | SHA256 |
| --- | --- | ---: | ---: | --- |
| Name, including NUL | 10419..10436 | 17 | 1037043293 | 3cb164b71986aeac3e8ce7776fd24261ebd6c485815ba1692d85ea9c43a29323 |
| Buffer | 10436..10577 | 141 | 804990665 | 09da1831288bb715838dcd7fad70333fc4865d2da02038da2de9d955232379b8 |

The genuine buffer's extra byte is NUL; that is an observation about this asset.
Controlled binary data and a nonzero extra byte verify the original count-based
behavior. The 10577-byte execution prefix has SHA256
`57a58a3d13b51726440eda5ec987366aaadbf720cf613cf14d0504ab71a9e834`.
Independent streaming decoding verifies complete original raw/decoded SHA256,
Adler32 and trailing padding while retaining only small prefixes. Metadata comes
from genuine directory entry1/type36 and its original header, not payload-derived
allocation descriptors. Original bytes and executable binaries remain ignored.

The disk block4 cursor advances 10338→10355→10496. On native64 the name spans
15192..15209, the buffer aligns to15216 and ends15357; all seven padding bytes
retain their previous values. Win32 uses mask0 and ends10496. Native pointer
slots remain native width, while serialized headers retain their12-byte layout.
The new current name/buffer values, retained first-table values and untouched
index pointer are checked after children through both exact resolution and actual
alias converters. Those constructed aliases are controlled inputs using genuine
values, not encoded block0 offsets observed in the FastFile.

Both genuine first StringTables retain every child byte and pass all400/1594
queries after loading the RawFile, then again after deliberate TEMP overwrite.
The RawFile header is still transient: this gate does not register or retain it
through the registry. Zombie's actual next asset remains TechniqueSet/type7;
its source stops at65678 without searching ahead or skipping assets.

Validation completed on all three ABIs in debug and optimized runs:215 cases
(186 preceding cases,24 new child preflights,5 actual wrapper controls) and12
genuine loading cycles per platform. ARM uses assertions/UBSan at `-O0` and `-O3`.
Strict collectors validate183 ordered JSON records and reject268 malformed
measurement controls. Current build manifests hash source/APIs and every fixture
header before/after compilation. Exact native x64/HONOR measurements agree;
Win32 comparisons allow only the individually checked native cursor fields.

The source proof reverses the documented selectors/includes to restore the whole
loader source to checkpoint7f7f005, preserving all542 collected bodies/order,
canonical globals and BOM. Earlier stream/expander/type sources remain unchanged.
Normal Windows gates-disabled loader compilation passes on x86/x64. The first
full ARM build completed with0errors/9135retained warnings; final fresh manifest
builds execute the post-review fixtures. Initial successful compilation and
manifest iterations are retained, along with independent review evidence.
TASK28's frozen proof remains valid at its checkpoint; the new proof bridges the
new selectors to it. Its old current-tree whitelist is not broadened.

A controlled truncated buffer fails after the header/name have loaded and the
child stream scope has been entered. The fixture unwinds that scope, clears loader
globals and detaches streams before release. This demonstrates partial progress,
not atomic rollback or production error recovery. Async/refill, longjmp/SEH,
OOM/concurrency, arbitrary TEMP pointees, RawFile persistent ownership, full
registry, startup and rendering remain unverified. No new latency/RSS benchmark
is claimed. Logical allocation counters regain baseline; TASK11's decoder and
allocator measurements remain separate.

All tests have finished. Windows2528 files, DXVK2552, root Ledger15, original
executable/assets, APK and4014 earlier frozen paths remain unchanged. Local
verified checkpoint commits are authorized; no push. Exact changed paths and
hashes are in `arm64_rawfile_payload_changes.md` and the new archive inventory.

Reproduce from `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, with HONOR serial
`AKWJ9X5116G04618` online and fresh output directories:

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_rawfile_payload_host.ps1 -OutputDirectory build-android-audit/rawfile_payload_host_repro
./tools/port/build_rawfile_payload_optimized_arm64.ps1 -Optimization O0 -BuildDirectory build-android-audit/rawfile_payload_debug_repro
./tools/port/build_rawfile_payload_optimized_arm64.ps1 -Optimization O3 -BuildDirectory build-android-audit/rawfile_payload_optimized_repro
& $taskPython -B tools/port/run_rawfile_payload_gate.py --host-build build-android-audit/rawfile_payload_host_repro --android-build build-android-audit/rawfile_payload_debug_repro --output build-android-audit/rawfile_payload_debug_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/run_rawfile_payload_gate.py --host-build build-android-audit/rawfile_payload_host_repro --android-build build-android-audit/rawfile_payload_optimized_repro --output build-android-audit/rawfile_payload_optimized_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/verify_rawfile_payload_source.py --output build-android-audit/rawfile_payload_source_repro.json
```

The runner reuses SHA-verified original files cached on HONOR via the existing
asset-directory results. Complete native decoding finishes in separate processes
before zone allocation; only10577/65678-byte execution prefixes are retained.
Exact commands, raw stdout/stderr, manifests, source proof and independent oracle
are frozen in `evidence/arm64_rawfile_payload/`. Scratch is preserved.

Next TASK30: actual production RawFile pool initialization/allocation/clone/free
with the existing1024 capacity, preserving genuine child ownership and queries
across TEMP reuse on Windows/HONOR/O3. Reject invalid ownership without claiming
unchecked leaf recovery. Then establish registry-entry/DBHASH initialization
services and the smallest actual registration/lookup boundary, before broad
dispatch or graphics.
