# TASK32: actual header dispatch and asset-entry ownership

Actual RawFile/StringTable header dispatch and DB_AllocXAssetEntry/DB_FreeXAssetEntry
now execute on Windows x86/x64 and HONOR ARM64. Canonical allocation/free bodies,
1024 RawFile/576 StringTable policy and reserved16 entry head are preserved. The
standalone selector exposes only real types36/37; default Windows tables remain
full. This is production-function execution with controlled inputs, not registration.

Three shared ABI corrections: generic free takes its handler's exact void* signature
while retaining the four memcpy body; DB_PrintAssetName uses its callback's exact
void* signature and typed int payload; DB_DDLGetName copies native pointer
representations from header and object's first field instead of accessing unrelated
XModelPieces lvalues. Valid-call behavior is preserved; null/malformed name/callback
inputs are not certified. Canonical FF_DIR/XZoneName move once to a light80-byte
header with original offsets; genuine dvar_s is used as the fixture mode input.

Final host_pass1/debug_pass1/optimized_pass1 and debug_run3/optimized_run2 pass:
1600 actual entry/header allocations,6 fixture ownership rejection controls,
exact full32752-entry/1024-raw/576-table free baselines, cross-type entry LIFO reuse,
usageFrame unchanged, livecount0 and retainedhighwater1600. Type/zone/inuse/hash/
override initialization policy is checked. Raw leaf caller contracts remain unchecked.
Android O0/O3 retains UBSan and test checks; Windows uses real recursive Win32 locks.

The header-exhaustion path executes the original DB_PrintXAssetsForType and actual
FastFile traversal over a controlled primary/override/nonmatching entry chain,
with exact printed name/zone order. The typed callback also executes separately.
Both actual header limits and a separately labeled injected empty entry head abort
through a fixture logging/Com_Error endpoint. A live worker proves the engine
released DBHASH before reporting. Raw limit retains count/highwater1026; table578:
capacity plus one nonmatching entry plus the failed popped entry. This proves
existing partial progress, not atomic rollback or production longjmp recovery.
LoadObj diagnostic dependency is an explicit failing test endpoint, not implemented
or falsely successful. No such endpoint is added to application production source.

All4 fatal modes perABI produce expected diagnostics/exit3Windows/134HONOR. The
collector requires exact nested types/records/stderr and rejects8 malformed
controls, including bool/int substitutions. It retains timeout/launch output.
Initial collector equality/order weaknesses were fixed after read-only review;
previous runs stay intact. A source-edit parser correction was not a native failure.

Whole-source proof passes againstf523687 with exactly named selectors/type moves/
ABI fixes. Canonical entry alloc/free/header allocation/FF traversal bodies remain
unchanged. Normal ungatedWindows8TUs perABI compile. Fresh current genuine RawFile/
StringTable owner regression on3ABIs/O3 passes all215 prerequisites/12cycles/all1994
queries and prior ownership controls, including historical Windows generic-free
parity. This validates the ABI correction against already-loaded genuine children;
it has not yet allocated those genuine headers through entry wrappers.

Protected17199 baseline files and Windows/DXVK Git match. All tests completed.
Source/binary/manifests/commands/final logs/protection/review are archived below;
all old scratch/evidence preserved. No assets/prefixes/proprietary binaries in Git.
Retail files remain read-only; current HONOR serial discovered/authorized online.
No new RSS/timing/graphics/startup/16KB/S25 claim. No push.

Reproduce with fresh directories from BlackOpsARM and a discovered authorized serial:

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_entry_alloc_host.ps1 -OutputDirectory build-android-audit/entry_alloc_host_repro
./tools/port/build_entry_alloc_arm64.ps1 -Optimization O0 -BuildDirectory build-android-audit/entry_alloc_debug_repro
./tools/port/build_entry_alloc_arm64.ps1 -Optimization O3 -BuildDirectory build-android-audit/entry_alloc_opt_repro
./tools/port/build_stringtable_owner_windows_compile.ps1 -OutputDirectory build-android-audit/entry_alloc_normal_windows_repro
& $taskPython -B tools/port/run_entry_alloc_gate.py --host-build build-android-audit/entry_alloc_host_repro --android-build build-android-audit/entry_alloc_debug_repro --output build-android-audit/entry_alloc_debug_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/run_entry_alloc_gate.py --host-build build-android-audit/entry_alloc_host_repro --android-build build-android-audit/entry_alloc_opt_repro --output build-android-audit/entry_alloc_opt_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/verify_entry_alloc_source.py --output build-android-audit/entry_alloc_source_repro.json
```

Use TASK30's current build_rawfile_owner/run_rawfile_owner tools for the genuine
regression; final outputs are entry_alloc_genuine_owner_host/arm64/run. Source
proofs of old milestones remain frozen; the new proof names successor differences.
Evidence inventory: evidence/arm64_entry_allocation/file_inventory.json.
Ledger DEC47/48, FACT99/100, WARN33, FAIL31. Standing goal continues automatically.

Next: genuine loaded StringTable/RawFile headers allocated/cloned/freed through real
entries before zone release, keeping hash buckets empty; actual hash/name lookup;
smallest real registration/returned-header behavior. FullDB_Init/dvar startup,
LoadObj/other asset types, moddeferred, error recovery, publication/game startup
remain separate. Preserve zombie's actual TechniqueSet7 boundary without skipping.
