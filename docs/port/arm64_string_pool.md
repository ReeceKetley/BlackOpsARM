# TASK-0017 — Real script-string pool and genuine interning

Completed 12 October 2026. The approved FastFile structural core and real
interning stretch are verified. Actual `cscr_stringlist`, memory-tree and instance
code runs on HONOR ARM64 and Windows x86/x64. Actual `Load_TempStringCustom`
loads and interns all 859 genuine inline strings through `SL_GetString`, producing
identical real IDs on all three ABIs. Null slot0 stays0; empty slot12 is nonzero.
This remains an isolated production-leaf gate with finite reader providers;
no whole-engine startup, asset payload dispatch or graphics is claimed.

| Genuine file | Inline / unique strings | Pool buckets / bytes | Live user4 refs | Source end |
|---|---:|---:|---:|---:|
| code_pre_gfx.ff | 0 / 0 | 0 / 0 | 0 | 52 |
| zombie_pentagon.ff | 859 / 859 | 1665 / 26640 | 859 | 18885 |

Pool allocation granularity is the original16 bytes. The26640 figure is logical
bucket occupancy, not RSS, virtual reservation or an independent heap release.
Static pool/debug storage remains process-owned. The genuine input bytes and
prefixes have prior complete native decode and independent full-stream SHA/Adler
provenance. Cached52/18885-byte prefixes and original device files are SHA-verified;
no duplicate108MB decoded buffer or new full asset copies are needed. Only IDs,
lengths, checksums, structural metadata and test evidence are archived in Git.

Six serial lifecycle cycles cover server/client instances, empty strings,
same-user duplicates, signed high bytes, equal-length hash collisions, lengths
255/511/767/1023 and the actual65530-character accepted maximum. The next length
65531 is rejected through actual MT_GetSize; the unsupported standalone VM
recovery callback explicitly aborts (Windows3/Android134), rather than pretending
recovery. Distinct user4/user1 references, sole-user transfer and user0 duplicate
references pass. User4 release removes one reference per unique string, not per
appearance. MT allocations/buckets, every debug reference and the entire hash
free list/backlinks return to baseline BEFORE shutdown or reinitialization.
Genuine inputs additionally pass24 raw-reader fixtures and12 alternating low/high
zone allocation/release cycles per ABI, every ID/byte/length/alias/map/cursor and
both pool/zone ownership checks. Pool is released while source/zone bytes remain
live; then streams detach and owners release. This is gate cleanup, not full
FastFile-to-game ownership transfer certification.

`stringsUsed` is int[32] script-usage diagnostics, not an ownership bitmap.
Production increments every new string but decrements only hash-head removal;
collision removal retains values. Actual retention is1 in the collision fixture
and13 in genuine loading on all ABIs. Initial wrong zero assertion failed and
is preserved (FAIL-0014). No engine counter reset or behavior change was added.
Actual owned counters/hash entries are zero. Cumulative avgAlloc and retained
static storage likewise are not evidence of a leak. No new timing/RSS claim;
historical TASK-0011 decoder/allocator measurements remain separate.

## Shared source changes and scope

CriticalSection's full production enum moved once, byte-for-byte, into
universal/critical_sections.h; normal win_common.h imports it. Actual native
bridge supports only SCRIPT_STRING and MEMORY_TREE through separate recursive
locks; other sections fail explicitly. Real thread identity requires a live,
stable registered server thread. No automatic fake server role or general
server-restart contract. Fixed unsigned32 atomics keep Windows real Interlocked
semantics and Android seq_cst operations. Both CAS comparands/loads use uint32;
unsigned decrement preserves low16 refcount checks without signed-shift UB.
Two BYTE value casts become explicit uint8. RefString remains8 bytes/data4/
str offset4/alignment4; nodes16 and hash entries8, with native-width publishers.

Include narrowing and standalone compile selection exclude unrelated live-update/
integer-string utilities and variable-debug globals; normal Windows retains their
original bodies. No fake utilities or VM globals were supplied. Real memset,
formatted printing and bounded TLS formatting serve the standalone gate; full
engine console/TLS/error recovery is unvalidated and unavailable VM operations
abort explicitly. Exact enum move and67 production bodies are checked against
checkpoint7cfc24c: only documented fixed32 transformations, otherwise unchanged.
All6 production stream-loading bodies, including TempStringCustom, are unchanged.

Serial/quiescent lifecycle is certified. Remaining plain bitfield/data accesses
prevent a broad concurrent-pool claim; no allocator or pool rewrite was made.
Original MT typed DWORD casts also pass actual optimized ARM64 -O3 execution
with assertions and UBSan enabled, without an optimizer workaround.

## Verification

Final Debug NDK30/Clang21/API28 C99/C++20:0 errors,4764 retained warnings,
174 AArch64 artifacts/25 test executables. Optimized two-target build:44 AArch64
artifacts, same genuine IDs/counters/diagnostics, no passing-path UBSan findings.
Full NORMAL pool/memory-tree/instance TUs compile on Windows x86/x64 with test
selectors disabled (six verified COFF objects). Windows Narrow32, actual four-byte
production declarations and all483 guards pass; previous16 HONOR probes pass.
All tests have completed. Initial compile failures and the diagnostic-counter
failure remain frozen under FAIL-0012/0013/0014, with exact commands/full output.

Full genuine run1 used Windows fixture build4 before test-only baseline extraction
into a shared header. Current-source Windows5 rebuild reproduces every fixture,
release and summary record; optimized full run uses Windows5 directly. No current
test hash is falsely attributed to old Windows4. See rebuild-provenance.json.
Independent source, adversarial and runner-result reviews found no remaining
blocker in claimed scope. Current evidence: docs/port/evidence/arm64_string_pool.
FACT-0051/0052 and WARN-0017 are authoritative; previous checkpoint7cfc24c.
Local verified milestone commits remain approved, no push.

## Reproduce on HONOR and Windows

From BlackOpsARM PowerShell, use fresh output names:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/string_pool_repro1
./tools/port/build_string_pool_host.ps1 -OutputDirectory build-android-audit/string_pool_host_repro1
./tools/port/build_intern_strings_host.ps1 -OutputDirectory build-android-audit/intern_strings_host_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_string_pool_gate.py --android-build build-android-audit/string_pool_repro1 --host-build build-android-audit/string_pool_host_repro1 --intern-host-build build-android-audit/intern_strings_host_repro1 --output build-android-audit/string_pool_repro_run1 --serial AKWJ9X5116G04618
./tools/port/run_arm64_audit_tests.ps1 -BuildDirectory build-android-audit/string_pool_repro1 -Serial AKWJ9X5116G04618
```

For optimized validation retaining all engine assertions:

```powershell
& 'D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe' -S android/cmake/arm64-audit -B build-android-audit/string_pool_opt_repro1 -G Ninja '-DCMAKE_MAKE_PROGRAM=D:/Android/Sdk/cmake/3.22.1/bin/ninja.exe' '-DCMAKE_TOOLCHAIN_FILE=D:/Android/Sdk/ndk/30.0.16248370/build/cmake/android.toolchain.cmake' '-DANDROID_ABI=arm64-v8a' '-DANDROID_PLATFORM=android-28' '-DCMAKE_BUILD_TYPE=Debug' '-DCMAKE_CXX_FLAGS_DEBUG=-O3 -g' '-DCMAKE_C_FLAGS_DEBUG=-O3 -g' '-DBO1_ARM64_COMPAT=ON'
& 'D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe' --build build-android-audit/string_pool_opt_repro1 --target bo1_string_pool_tests bo1_intern_strings_tests --verbose
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_string_pool_gate.py --android-build build-android-audit/string_pool_opt_repro1 --host-build build-android-audit/string_pool_host_repro1 --intern-host-build build-android-audit/intern_strings_host_repro1 --output build-android-audit/string_pool_opt_repro_run1 --serial AKWJ9X5116G04618
```

Preserve old directories. Runner validates existing cache provenance from
raw_strings_run1/results.json; if original device cache is absent, perform an
explicit verified transfer rather than substituting unverified bytes. Both original
game files stay read-only. No commit includes FF files, prefixes or binaries.

## Next three real production boundaries

1. TASK-0018 first genuine StringTable packed20/native32 header in block0. First
   directory entry is type37 in both files; source offsets348/27245, dimensions
   1x400 /2x797 and all three pointer tagsFFFFFFFF. Verify actual leaf/Win32 read,
   fields, capacities, aliases and temporary-block0 Pop rewind/shadow semantics.
   Stop before name, cells, index, fixups or registration.
2. TASK-0019 genuine name and packed cell-array expansion before per-cell strings.
   Names34/58 bytes; cells400/1594 at8 disk/16 native bytes. Preserve ordinary
   offsets unresolved; do not treat every cell pointer as inline.
3. Checked real cell-string offset resolution and loading, then signed16 index
   bytes before registry/first-asset completion. Genuine cells already contain
   two/49 ordinary block4 offsets. Prove initialized character-range provenance
   before actual conversion; no uninitialized zero-page or all-inline shortcuts.

Continue autonomously toward native engine startup. Full async loader, arbitrary
pool concurrency, VM/registry/zone initialization/rendering, Windows PMem multiple
owner gaps and16KB/S25 remain unvalidated. No game-owned graphical output yet.
