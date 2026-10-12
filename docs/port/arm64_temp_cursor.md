# TASK-0027 — TEMP cursor and alias prerequisite

The real stream Push/Pop now restores block0's physical and disk-coordinate
cursors together. It saves the TARGET entry position, including nested0→0 and
4→0→4 scopes. Nonzero streams retain their advancement. A private native-only
64-entry shadow stack preserves the existing StreamPosInfo declaration and Win32
bodies. Alias tails use stable sorting; stable merging retains chronological
equal-key order so the latest registered slot wins.

Pop leaves maps and bytes readable. Original SHA-pinned Push0x43ca60..0x43cac3,
Pop0x460ed0..0x460f28 and alias0x59bf20..0x59bf42 instructions establish that
contract; whole-map rollback would be incorrect. Original executable SHA256:
2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759.

## Verified evidence

- Dedicated probe: Windows x86/x64 and HONOR NDL-L09/API36, three fresh processes
  each. Every process passes40cases/8cycles; native112aliaschecks with150duplicate
  registrations across merged prefixes/tails. Actual DB_InsertPointer reserves
  persistent block4 slots while retaining four-byte disk footprint. Owned aligned
  scratch arrays isolate cursor semantics; they are not zone allocation tests.
- ARM probe uses NDK30/Clang21/API28, -O3, assertions and UBSan. Historical9a8534d
  source with the same probe fails exactly at paired-shadow restoration on HONOR
  (expected exit1). Current binaries exit0, no timeouts or sanitizer failures.
- Genuine owner regression debug/optimized: all3ABIs,162prior cases,
  86normalization cases,12cycles, all400/1594 StringTable lookups after TEMP
  overwrite remain correct. Actual post-Pop coordinate0 is emitted by a runtime
  getter; eight current runners require0. Pre-Pop20/aligned-end assertions remain.
- Sourceproof1 confirms exact six Win32 bodies/header after native guard removal,
  and whole expander/stream-load sources unchanged. Windows normal8TUs×2 and
  dedicated full-stream host builds pass. Full ARM pass3:0errors/8192warnings,
  followed by final collector rebuild; all logs retained. Read build-summary.json
  for initial/final warning counts and final ELF hashes.
- Root collector editing attempts failed on occurrence counts and quoting before
  correction. These tooling diagnostics, preliminary builds/runs and historical
  negative control are preserved separately; current runtime tests all pass.

No new latency, RSS, decoder peak or graphics measurements were made. TASK11's
allocator/decoder timing and memory evidence remain their own measurements.

## Deliberate boundary and next gate

This is a prerequisite, not complete TEMP overwrite support. Shadow rewind permits
out-of-order retained segments (for example starts0,8,0); binary-search lookup and
drift fallback cannot yet certify subsequent writes. The first genuine code
asset1 is RawFile at decoded10407..10419: name=-1,len140,buffer=-1,12disk/24native,
no flags. This metadata-only oracle verifies full original SHA/Adler and directory
bytes. Its header was NOT executed. Zombie's actual nextasset is TechniqueSet7;
it is deferred without searching ahead. No new asset dispatch/registry executes.

Native RawFile24 overwrites an old StringTable pointer atnative16 that Win32's
12-byte write leaves intact atdisk12. General escaping suffix references need an
evidenced policy or explicit unsupported failure. Never clear all maps on Pop or
revive overwritten mappings through drift fallback. Current genuine typed front
end/firsttables contain no encodedblock0 references; later bodies are unaudited.

Next TASK28: successful-write-aware TEMP exact/alias mappings, no-write retention,
same/smaller/larger/partial overwrites, deterministic duplicate/tombstone policy,
external-envelope and block4 preservation, short-read/OOM publication boundaries,
reset/detach. Only after that gate execute the checked genuine RawFile numeric
header atTEMP0 while the pooled firsttable and all1994queries stay valid. Stop
before RawFile name/buffer, registry, pointer dispatch or zone initialization.
Then real DBHASH/asset-entry ownership/init services and the smallest registry
publication boundary. Keep original zone expansion policies and disk layouts.

## Reproduce (PowerShell, Android worktree)

Use fresh build/output directory names. Device serial AKWJ9X5116G04618 must be
online. All commands run from D:/Desktop/Black Ops 2 Android/BlackOpsARM:

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_temp_cursor_host.ps1 -OutputDirectory build-android-audit/temp_cursor_host_repro
./tools/port/build_temp_cursor_arm64.ps1 -BuildDirectory build-android-audit/temp_cursor_arm64_repro
& $taskPython -B tools/port/run_temp_cursor_gate.py --host-build build-android-audit/temp_cursor_host_repro --android-build build-android-audit/temp_cursor_arm64_repro --output build-android-audit/temp_cursor_repro --serial AKWJ9X5116G04618
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/temp_cursor_full_repro
./tools/port/build_stringtable_owner_host.ps1 -OutputDirectory build-android-audit/temp_cursor_owner_host_repro
& $taskPython -B tools/port/run_stringtable_owner_gate.py --android-build build-android-audit/temp_cursor_full_repro --host-build build-android-audit/temp_cursor_owner_host_repro --output build-android-audit/temp_cursor_owner_repro --serial AKWJ9X5116G04618
./tools/port/build_stringtable_owner_optimized_arm64.ps1 -BuildDirectory build-android-audit/temp_cursor_owner_opt_repro
& $taskPython -B tools/port/run_stringtable_owner_gate.py --android-build build-android-audit/temp_cursor_owner_opt_repro --host-build build-android-audit/temp_cursor_owner_host_repro --output build-android-audit/temp_cursor_owner_opt_run_repro --serial AKWJ9X5116G04618
& $taskPython -B tools/port/verify_temp_cursor_source.py --output build-android-audit/temp_cursor_source_repro.json
& $taskPython -B tools/port/rawfile_header_oracle.py --previous-results build-android-audit/temp_cursor_owner_repro/results.json --output build-android-audit/rawfile_header_oracle_repro
```

Owner runner SHA-verifies and reuses cached original assets from
/data/local/tmp/blackopsarm-phase1-string-table-20261011-225356426113 using
asset_directory_run1/results.json (override --reuse-results if cache changes).
Native full decoding runs separately before zone allocation. No duplicate full
decoded buffer, assets or executable binaries enter Git. The source proof compares
fixed historical parent9a8534d; source evidence is distinct from runtime tests.

Latest successful runs: temp_cursor_run2, temp_cursor_owner_run2 and
temp_cursor_owner_optimized_run2. Exact compiler/ADB commands, executable hashes,
all raw logs, original references and classifications are frozen in
evidence/arm64_temp_cursor/. Old archives and all scratch remain intact.
