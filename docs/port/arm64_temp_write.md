# TASK-0028: TEMP reuse and genuine next RawFile header

Production TEMP stream mappings now track successful writes rather than scope
exit. Before owned stores, the native loader snapshots live typed alias values
and reserves metadata. Commit replaces overlapping mappings and preserves
untouched byte ranges or complete expanded-array elements. An overlapping opaque
object loses its address mapping. Pop still retains readable mappings and values.

A pointer word can survive logically in Win32 while expansion overwrites its
native storage. Such aliases retain the current value captured before overwrite,
including null and raw-tag bits. Value resolution is separate from address
mapping. New registrations require a complete four-byte logical word and complete
native slot inside the actual commit. Invalid records remain tombstones; missing,
invalid or external TEMP alias routes fail explicitly instead of using drift
fallback. Other streams retain their existing policies.

This changes stream metadata, not zone allocation factors or FastFile layouts.
The reserve bound3N+1 follows from two overwrite intervals leaving at most three
fragments per old affine segment, plus the new segment. It is not an engine
allocation multiplier. Alias budgets use actual staged disk-word counts in the
audited single-read expanders. There are no copied engine implementations.

## Genuine boundary reached

After the first complete, pooled StringTable, code_pre_gfx's actual next asset is
RawFile type36. Its genuine numeric header at decoded10407..10419 has name=-1,
length140 and buffer=-1. The unchanged production expander writes12disk/24native
bytes at TEMP0; Win32 uses its original12-byte Load_Stream path. Padding, field
tags, cursors, retained old values, untouched old index pointer and block4 data
are checked. First-table queries run immediately after the real overwrite, then
again after the prior deliberate A5 workspace overwrite.

Zombie's next actual asset is TechniqueSet type7. Its stream stops at65678 without
skipping assets or searching ahead for a RawFile. RawFile name/buffer reads,
registration, full dispatcher, zone initialization and rendering do not run.
The constructed alias inputs13/17 use genuine live values as controlled checks;
they are not encoded-block0 references observed in either FastFile.

RawFile's canonical declaration and packed disk definition were moved once.
The original RawFile expander body and source order are unchanged. A checked
header planner validates tags, signed length, len+1 overflow for a present buffer,
decoded remaining extent, alignment and actual disk/native capacities before
body execution. The original Load_RawFile guards buffer loading with a nonzero
buffer tag: old oracle prose said unconditional, now corrected. Numeric genuine
metadata and old frozen evidence remain unchanged.

## Validation and failures

- Final controlled gate: temp_write_run4, fresh pass6 binaries with source
  manifests. Three processes per ABI each pass18cases/12shortreads; native78
  overlap cases. Both native platforms additionally pass six opaque-value cases,
  invalid staged-tombstone masking and two actual C-string hook cases. Three
  converter failures each are explicit, diagnostic-checked: Windows exit3,
  HONOR exit134. ARM uses-O3/assertions/UBSan.
- Genuine debug rawfile_header_run2 and optimized_run1: all three ABIs pass
  186cases/12cycles, both complete first payloads/all1994queries, original
  normalization/hash/query references and pool ownership/cleanup. Six code cycles
  execute the next genuine RawFile header; six zombie cycles defer actualtype7.
- Sourceproof_rawfile3: ten Win32 bodies/header unchanged after native guards are
  removed;161/163 expander functions unchanged. The scratch reader and exact
  external-envelope cancellation are recorded exceptions. Canonical RawFile moves
  and the original RawFile function body/order are verified. Normal Windows8TUs
  on x86/x64 compile, including gates disabled.
- Full ARM build: initial0errors/8640retained warnings, followed by the fixed
  prefix-producer rebuild; optimized compilation and passing paths have no UBSan
  failures. Sixteen existing HONOR regression executables also exit0, including
  native8/fixed4 Narrow32 slots, production VM/parse layouts, RNG, Hunk, zone and
  codec probes. Expected child-failure diagnostics remain in the original log.
- Preserved failures: Windows min/max macro compile collision; Windows abort
  reporting timeout; two incorrect fixture expectations; an actual unchecked
  newly staged alias-publication gap; and a mutable-front-cursor prefix-producer
  error caught before genuine header execution. All corrected, with earlier
  compiler logs, failed runs and scratch retained separately.

All tests are complete. No new FPS, latency, RSS or decoder-peak benchmark is
claimed. TASK11's decoder and allocator measurements remain separate.

## Limits

Exception cleanup is certified for the finite C++ throw adapter. Real
Com_Error/Com_CheckError use longjmp; the cleanup catches do not certify production
async decoder recovery. Scratch-read failure cancels unpublished metadata; an
uncertain direct owned-byte write poisons mappings without restoring bytes.
A later validated replacement or owner reset can create valid mappings again.
Post-read unsupported caller contracts, multi-read/interleaved expanders,
nonlocal-exit/SEH recovery, broader concurrency and retained TEMP pointees remain
unverified. Capturing a pointer value does not preserve an overwritten pointee.

Full registry/DBHASH services, async/refill, PMem multiple owners/retained pages,
S25/16KB pages, non-ASCII hash locale and engine/backend startup remain separate.
Windows, DXVK-Native, the root Ledger, original game files, APK and3652prior frozen
paths are protected. Only the Android worktree is changed; no push.

## Reproduce

Run from D:/Desktop/Black Ops 2 Android/BlackOpsARM in PowerShell with fresh output
directory names and HONOR serial AKWJ9X5116G04618 online:

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_temp_write_host.ps1 -OutputDirectory build-android-audit/temp_write_host_repro
./tools/port/build_temp_write_arm64.ps1 -BuildDirectory build-android-audit/temp_write_arm64_repro
& $taskPython -B tools/port/run_temp_write_gate.py --host-build build-android-audit/temp_write_host_repro --android-build build-android-audit/temp_write_arm64_repro --output build-android-audit/temp_write_repro --serial AKWJ9X5116G04618
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/rawfile_header_full_repro
./tools/port/build_rawfile_header_host.ps1 -OutputDirectory build-android-audit/rawfile_header_host_repro
& $taskPython -B tools/port/run_rawfile_header_gate.py --android-build build-android-audit/rawfile_header_full_repro --host-build build-android-audit/rawfile_header_host_repro --output build-android-audit/rawfile_header_repro --serial AKWJ9X5116G04618
./tools/port/build_rawfile_header_optimized_arm64.ps1 -BuildDirectory build-android-audit/rawfile_header_opt_repro
& $taskPython -B tools/port/run_rawfile_header_gate.py --android-build build-android-audit/rawfile_header_opt_repro --host-build build-android-audit/rawfile_header_host_repro --output build-android-audit/rawfile_header_opt_run_repro --serial AKWJ9X5116G04618
& $taskPython -B tools/port/verify_temp_write_source.py --output build-android-audit/temp_write_source_repro.json
```

The genuine runner SHA-verifies/reuses original FastFiles cached in
/data/local/tmp/blackopsarm-phase1-string-table-20261011-225356426113 via
asset_directory_run1/results.json. Override --reuse-results if that cache changes.
Complete native decoding runs in separate processes before zone allocation;
only10419/65678-byte prefixes are retained for execution. No originals, full
decoded buffers, prefixes or executable binaries enter Git. Exact compiler/ADB
commands, hashes and raw outputs are frozen in evidence/arm64_temp_write/.

Next TASK29: actual RawFile name and guarded len+1 buffer loading through the
original wrapper/leaves, with full byte/digest/cursor/alias/ownership checks and
first-table queries preserved. Stop before registration and later asset dispatch;
defer zombie's TechniqueSet without skipping. Then real registry ownership/init
services and the next actual registration boundary.
