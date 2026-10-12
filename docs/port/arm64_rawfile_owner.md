# TASK30: genuine RawFile persistent pool ownership

The canonical RawFile pool now executes on HONOR ARM64 and Windows x86/x64.
Genuine code_pre_gfx's second asset retains its complete header, 17-byte name,
141-byte buffer and original len140 through TEMP rewind and overwrite. All first
StringTable queries and complete block4 bytes remain intact. Zombie's next real
TechniqueSet/type7 remains deferred without searching ahead or allocating RawFile.
This completes a persistent-header prerequisite before real registry publication.

The additive BO1_RAWFILE_OWNER_GATE exposes the actual g_RawFilePool and size36
handler alongside StringTable. Canonical1024 normal/2048 mod policy moves once
into db_asset_pool.h; the existing pool physically reserves2048 entries. Init,
allocation, clone, size and native free bodies are unchanged. Default definition
order, BOM, original disk header12 and native header24 are preserved. Children
remain zone-owned, and no TEMP alias is retargeted to the pool header.

| Measurement | Windows x86 | Windows x64 / HONOR |
| --- | ---: | ---: |
| Header/entry bytes | 12 | 24 |
| Full actual pool storage | 24580 | 49160 |
| Normal/mod active capacities | 1024/2048 | 1024/2048 |

Final builds: host_pass3, debug_pass3 and optimized_pass3. Windows uses O2 with
assertions; Android uses O0/O3 with assertions and UBSan. Normal ungated Windows
compilation covers eight existing production TUs on each ABI. Sourcepass2 verifies
whole source against0a9dbfc with only exact named selector/policy edits. Existing
loader/stream/expander/query/hash code remains byte unchanged after line-ending
normalization. Independent read-only review found no correctness blocker.

Final debug_run2 and optimized_run2 pass all three ABIs. Each runs the retained
215 TASK29 cases and12 genuine cycles plus distinct TASK30 evidence: pool sizes
1/2/1024/2048,3075 allocations,4 exact null exhaustions without mutation,20 invalid
caller guards, LIFO reuse, payload-tail preservation and free/live conservation.
Six code cycles clone before Pop, overwrite the old header, retain all name/buffer
and zone bytes, and pass400 first-table queries. Six zombie cycles allocate none.
Both persistent asset headers free before their zone, restoring ascending baseline
without reset. All400/1594 first-table queries pass in the inherited checks.

The new collector adapter retains the full strict TASK29 validator. It requires
14 additional ownership records, exact field/type/value checks, placement,
6 allocated/6 deferred releases and TASK30 terminal schema. Final results record
adapter/verifier/build-tool hashes; build manifests record sources and all fixture
headers. Earlier runner versions/builds remain intact, including first compile
failure from a missing moved constant and the verifier's blank-line expectation
correction. No original string bytes, asset prefixes or binaries are in Git.

Protected-file comparison checked16875 baseline paths with zero changes, including
Windows/DXVK/root material, prior frozen evidence and rotation report. Ledger files
are deliberately excluded from this comparison because the new authorized work
appends records/updates state. The original five pending handoff paths are retained
in this checkpoint. Retail asset provenance is checked by the original complete
streaming oracles and separate native decoder processes on each platform. Small
prefixes and binaries remain in ignored scratch/device locations.

Limits: shallow ownership only; unchecked production leaves keep valid caller
contracts. The20 malformed cases are fixture rejection, not production recovery.
No DB_AddXAsset, registry publication, async/refill, longjmp/SEH/OOM/concurrency,
engine startup or rendering is certified. Child zones must outlive pool headers.
No new RSS/latency/16KB/S25 result. Next: actual asset-entry/DBHASH initialization
services, then smallest real registration/lookup boundary without skipping type7.
Local verified checkpoint commits authorized by the latest user; no push.

Reproduce from BlackOpsARM using fresh directories and an ADB-discovered authorized
HONOR serial (the final runs used AKWJ9X5116G04618):

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_rawfile_owner_host.ps1 -OutputDirectory build-android-audit/rawfile_owner_host_repro
./tools/port/build_rawfile_owner_arm64.ps1 -Optimization O0 -BuildDirectory build-android-audit/rawfile_owner_debug_repro
./tools/port/build_rawfile_owner_arm64.ps1 -Optimization O3 -BuildDirectory build-android-audit/rawfile_owner_opt_repro
./tools/port/build_stringtable_owner_windows_compile.ps1 -OutputDirectory build-android-audit/rawfile_owner_normal_windows_repro
& $taskPython -B tools/port/run_rawfile_owner_gate.py --host-build build-android-audit/rawfile_owner_host_repro --android-build build-android-audit/rawfile_owner_debug_repro --output build-android-audit/rawfile_owner_debug_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/run_rawfile_owner_gate.py --host-build build-android-audit/rawfile_owner_host_repro --android-build build-android-audit/rawfile_owner_opt_repro --output build-android-audit/rawfile_owner_opt_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/verify_rawfile_owner_source.py --output build-android-audit/rawfile_owner_source_repro.json
```

The ARM builders compile the bounded executable, not a bootable engine. Full
native decode finishes in separate processes before zone allocation. Evidence:
evidence/arm64_rawfile_owner/file_inventory.json. Ledger DEC45/FACT95/FACT96/
WARN31/FAIL29. Earlier milestone reports and failure logs remain preserved.
