# TASK31: actual entry-list initialization and native DBHASH

The original DB_Init entry freelist now has one canonical helper callable on
ARM64. The normal DB_Init path calls it after its unchanged asset-pool initialization.
The native service bridge now supports DBHASH74 through a third real recursive
lock, independent of script strings27 and memory tree28. Windows production
win_common remains unchanged; standalone Windows comparison builds use real
Win32 recursive critical sections through the tested native-service host backend.

This gate stops before actual entry allocation/free, hash lookup or publication.
It does not execute full DB_Init, dvar registration, commandline startup or all
asset pools. No loader or asset content is changed or newly certified here.

| Original policy | Retail | bo1_mod_zones |
| --- | ---: | ---: |
| Physical pool entries |65536|65536|
| Free head index |16|16|
| Last free index |32767|65535|
| Allocatable nodes |32752|65520|

The extracted helper retains the original links1..last, while its head excludes
reserved0..15. Index0 remains the null hash sentinel. It modifies only the first
pointer-width slot of each initialized entry, leaves sentinel0 and inactive upper
entries alone and does not reset hash buckets, counters or payload tails. Native
entry stride is24; Win32 stride16. Hash buckets and links retain16 bits. Full pool
storage is1572864/1048576 bytes; hash table32768 buckets/65536 bytes. These are
sizeof measurements, not RSS. Canonical declarations move once into the new light
header with native offsets/width assertions; no packing change is introduced.

Initialization requires startup or quiescence, with no live entry owners. The
helper does not acquire a lock. Tests deliberately hold DBHASH and initialize
six controlled retail/mod/retail cycles over two rounds; this does not certify
live engine reset. Test poison and hash patterns establish exact write boundaries
and original nonreset behavior, not genuine game assets or production recovery.

Final host_pass2/debug_pass2/optimized_pass2 and debug_run3/optimized_run3 pass
on Windows x86/x64 and HONOR ARM64. Android O0/O3 retain UBSan and active test
checks. Each ABI reports8 exact ordered records: eight-thread first-use service
initialization, recursive depth3, worker exclusion after partial release, script
section independence,16000 mutex-protected increments, six complete list traversals
and an explicitly bounded terminal summary. Unsupported section75 must abort
with its specific diagnostic (Windows3/HONOR134). Four malformed collector records
are rejected. The collector hashes the compiled inputs, verifies binary ABI and
uploaded executable SHA, discovers authorization via adb devices and retains
stdout/stderr/invocations even for timeout or launch error. Final HONOR is online,
API36 with4096-byte pages; no new S25/16KB claim.

Exact source proof compares whole affected production sources to63c2fa4 using
only named reversible edits. All original initialization assignments/bounds and
existing native locking/role bodies are retained; only mode decision moves to
the DB_Init call argument. Default Windows initialization retains its commandline
policy. Existing loader/stream/expander/pools/size dispatch and Windows platform
source are unchanged. Final ungated Windows compilation covers8TUs perABI.

Regression: existing script-services Windows/HONOR gates pass80000 atomic and
80000 locked increments, server identity and both existing fatal controls. The
old invalid PoolLock2 fixture now uses3 because2 is the newly supported Database
lock; old valid0/1 values and behavior remain unchanged. Actual SL/MT fixture and
oversize rejection gates pass on all3ABIs,6 lifecycle cycles each. This is fixture
regression, not new genuine-string or game-startup evidence.

Initial failed builds are retained: MSVC would not discard unreferenced clone
link dependencies, so the new init-only gate excludes the unchanged clone body;
ARM's required signed-char assertion caught the missing flag, supplied without
disabling it. The first collector syntax correction occurred before tests started.
Independent read-only review found no production blocker and requested improved
timeout capture, now implemented. All scratch and earlier frozen evidence survive.
Protected-file comparison checked17014 baseline paths with zero changes and exact
Windows/DXVK HEAD/branch/status parity. Root Ledger and originals remain read-only.

Reproduce from BlackOpsARM with fresh output directories and an ADB-discovered
currently authorized device (final runs used AKWJ9X5116G04618):

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_asset_entry_init_host.ps1 -OutputDirectory build-android-audit/asset_entry_host_repro
./tools/port/build_asset_entry_init_arm64.ps1 -Optimization O0 -BuildDirectory build-android-audit/asset_entry_debug_repro
./tools/port/build_asset_entry_init_arm64.ps1 -Optimization O3 -BuildDirectory build-android-audit/asset_entry_opt_repro
./tools/port/build_stringtable_owner_windows_compile.ps1 -OutputDirectory build-android-audit/asset_entry_normal_windows_repro
& $taskPython -B tools/port/run_asset_entry_init_gate.py --host-build build-android-audit/asset_entry_host_repro --android-build build-android-audit/asset_entry_debug_repro --output build-android-audit/asset_entry_debug_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/run_asset_entry_init_gate.py --host-build build-android-audit/asset_entry_host_repro --android-build build-android-audit/asset_entry_opt_repro --output build-android-audit/asset_entry_opt_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/verify_asset_entry_init_source.py --output build-android-audit/asset_entry_source_repro.json
```

Regression commands: build_script_string_services_host.ps1 and
run_script_string_services_host.py; build the existing
bo1_script_string_services_tests target and run_script_string_services.py with
fresh host results. Build_string_pool_host.ps1 plus existing bo1_string_pool_tests
and run_string_pool_gate.py --fixtures-only reproduce the SL/MT checks. Original
raw stdout/stderr, commands and results are frozen in evidence/arm64_asset_entry_init.

Next three boundaries: actual header dispatch and entry allocation/free with
real exhaustion diagnostics and partial-progress semantics; actual hash lookup;
smallest real registration/returned-header behavior. Header exhaustion occurs
AFTER entry removal and high-count increment, so atomic rollback must not be
invented. Publication, async/refill/longjmp/SEH/OOM/concurrency recovery, startup,
graphics and zombie's real TechniqueSet7 remain unverified. No latency/RSS claim.
Ledger DEC46/FACT97/FACT98/WARN32/FAIL30. Local checkpoints authorized; no push.
