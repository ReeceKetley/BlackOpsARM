# TASK-0015 — Genuine bounded raw string loading

Completed 2026-10-12 local time. Actual `Load_XStringCustom` runs on HONOR ARM64
and Windows x86/x64, using actual stream allocation/commit and production zone
policies. Native64 uses the actual pointer-table expander; Win32 now executes
actual `Load_Stream` for its packed table. All6 existing stream-loading function
bodies are byte-preserved. Only includes/standalone compile selection changed.

| Genuine input | Inline strings | Bytes including NUL | Decoded source end | Block4 disk/native64 end |
|---|---:|---:|---:|---:|
| code_pre_gfx.ff | 0 | 0 | 52 | 0 / 0 |
| zombie_pentagon.ff | 859 | 15393 | 18885 | 18833 / 25639 |

Zombie has860 table slots: null0 consumes nothing; empty slot12 consumes one NUL.
Maximum length including NUL38. Native raw allocations add3366 alignment bytes;
Win32 byte alignment adds0. Memory padding never consumes file bytes. Every
loaded byte, per-slot length/Adler, character exact mapping and table alias is
verified. Raw pointers are deliberately distinct from interned script IDs.

24 controlled fixtures and12 alternating genuine low/high allocation cycles
pass on each ABI. Missing NUL, null source, exact bounds, capacity/padding
exhaustion, invalid tags, cursor overflow, short/negative reads, overlapping
source/destination and injected pre-copy failure are checked. Entire genuine
frame is preflighted before allocation/writes. Cleanup detaches persistent
stream descriptors before freeing blocks/named owner, restoring logical counts.
Offsets including -2 are explicitly unsupported; untouched allocated pages are
never treated as validated strings. No per-array rollback claim after allocation
failure in private expander vectors/segments.

Independent Python streams full original-game decode, verifies raw/full decoded
SHA/Adler against prior references and derives every string length/offset/digest.
Each native decoder runs separately, releases its one decoded vector before zone
tests; allocation retains only52/18885-byte prefixes. Device originals are reused
from the SHA-verified TASK14 directory. No original strings/prefix buffers in Git.
Python1MiB prefix/64KiB string extraction budgets are oracle resource budgets,
not imposed production limits. No new timing/RSS performance claim; TASK11's
frozen decoder/allocator measurements remain separate.

NDK30/Clang21/API28 Debug C99/C++20 final raw_strings_pass3:0 compiler/linker
errors,3275 retained warnings,128 AArch64 artifacts/21 test executables. Passing
instrumented paths have no UBSan diagnostics. Previous16 HONOR probes pass.
Full normal Windows stream-load TU with gate disabled compiles x86/x64. Independent
review found and corrected Win32 alignment, native table scratch extent, current
block0 array convention and source-bound/overlap checks before final verification.
Initial MSVC std::max macro failure and all superseded builds are preserved.

`DecodedSource` is real reusable borrowed decoded-byte reading with checked,
all-or-nothing copy. The bridge callbacks in this executable remain finite test
adapters, separate from the full engine async/refill/APC loader. No interning,
asset-directory/payload dispatch, pointer fixups, registry, zone initialization,
rendering or game-owned graphical output is claimed. Full original custom codec
is used; system libz is never mixed with its ABI. Frozen metadata/logs are under
`docs/port/evidence/arm64_raw_strings`.

## Reproduce in BlackOpsARM PowerShell

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/raw_strings_repro1
./tools/port/build_raw_strings_host.ps1 -OutputDirectory build-android-audit/raw_strings_host_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_raw_strings_gate.py --android-build build-android-audit/raw_strings_repro1 --host-build build-android-audit/raw_strings_host_repro1 --output build-android-audit/raw_strings_repro_run1 --serial AKWJ9X5116G04618
./tools/port/run_arm64_audit_tests.ps1 -BuildDirectory build-android-audit/raw_strings_repro1 -Serial AKWJ9X5116G04618
```

Use fresh output names; keep old logs/scratch. Runner defaults to read-only
D:/Games/Call of Duty - Black Ops and reuses raw files whose hashes are verified
from string_table_run2/results.json. If that device directory disappears, an
explicit small asset transfer/reuse manifest is needed; do not silently use an
unverified replacement. Both original raw assets total about48.2MB. Device probe
and small prefixes are new unique scratch files. Commands/results/digests frozen.

DEC-0030, FACT-0043, FAIL-0010 and WARN-0015 record this gate. Verified local
milestone commits authorized by DEC-0026; no push. Protected Windows2528 files,
DXVK2552, root Ledger15 and1426 prior report/archive/handoff files preserved.
See preservation.json/file_inventory.json for the exact pre-checkpoint inventory.
Parallel pool-service prerequisite files are independent uncommitted work, not
part of this raw-string checkpoint until their own verification.

## Next three milestones

1. TASK-0016 actual asset-directory expansion, genuine37/1045 packed8-byte entries;
   every type/header/tag/padding/alias/stride/source/cursor, malformed requests and
   repeated cleanup. Windows real Load_Stream, native real XAssetArray. Stop
   before dispatch; expected genuine file end348/27245, block4 shadow296/27196,
   native592/42360, verified rather than inferred payload descriptors.
2. TASK-0017 real SL/MT pool init/intern/deduplicate/user4 release, recursive locks
   and fixed unsigned32 ARM64 atomics; match Windows IDs/bytes/lifetimes. Then
   feed genuine strings in order. Stop if full VM initialization or fake services
   would be required. Native platform prerequisite investigation is in progress.
3. First real StringTable packed20/native32 header in block0: first directory
   entry is type37 in both originals. Verify checked header expansion before
   name/cell/index/fixups/registration. Continue critical-path engine startup.
