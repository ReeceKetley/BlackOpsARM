# TASK34: actual registry lookup on genuine owned headers

Actual DB_HashForName, DB_GetXAssetName and DB_FindXAssetEntry execute unchanged on
native ARM64 and Windows x86/x64. After genuine first StringTable/RawFile loading,
clone and TEMP overwrite, fixture-installed membership lets actual Find return the
real entry/header by original and uppercase names and return null on misses.
Every entry byte, bucket and ownership count is restored before actual frees and
zone teardown. Code owns two entries, zombie one; TechniqueSet7 remains unskipped.
This certifies lookup with controlled membership, not DB_Link/DB_Add publication.

Final host_pass2/debug_pass2/optimized_pass2 and debug_run3/optimized_run2 pass all
three ABIs. Assertions and UBSan remain enabled in O0/O3. All inherited 215 cases,
12 genuine cycles and all1994 table queries pass, plus12 typed lookup records and
four malformed collector controls. Independent 27 ASCII polynomial vectors cover
case/slash normalization, empty type seed, long names and uint32 overflow.
Actual same-bucket collision traversal filters type before header access, returns
first matching primary, and ignores a matching nextOverride after that primary
is removed. Slash variants hash equally but original I_stricmp still rejects them.
A positive deferred count does not affect a primary hit. A deferred miss reaches
an explicit test-only unavailable endpoint, exact exits3/134 and diagnostic.

Compile selection alone exposes the three canonical bodies; normal static mod
counter/helper remain intact. Whole-source proof passes against5d4d641. Full
ungated Windows eight-unit builds pass per ABI. Protected17521 files and exact
Windows/DXVK Git state remain unchanged. No retail files or binaries in Git.

Initial collector failed on an argument mismatch after Windows controls passed.
It now accepts only the recorded expected fatal invocation, rejecting launch,
timeout, unexpected exit/output/diagnostic. Review strengthened the override test
to the same collision bucket, followed by all-target rebuild/final runs. Failed
and superseded evidence remains archived. No full DB_Init/Dvar registration,
mod linking, malformed chain/null/high-byte names, concurrent publication,
error recovery, engine boot, APK gameplay or graphics claim.

Reproduce with fresh directories from BlackOpsARM:

```powershell
./tools/port/build_registry_lookup_host.ps1 -OutputDirectory build-android-audit/registry_lookup_host_repro
./tools/port/build_registry_lookup_arm64.ps1 -Optimization O0 -BuildDirectory build-android-audit/registry_lookup_debug_repro
./tools/port/build_registry_lookup_arm64.ps1 -Optimization O3 -BuildDirectory build-android-audit/registry_lookup_opt_repro
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $taskPython -B tools/port/run_registry_lookup_gate.py --host-build build-android-audit/registry_lookup_host_repro --android-build build-android-audit/registry_lookup_debug_repro --output build-android-audit/registry_lookup_debug_repro_run
& $taskPython -B tools/port/run_registry_lookup_gate.py --host-build build-android-audit/registry_lookup_host_repro --android-build build-android-audit/registry_lookup_opt_repro --output build-android-audit/registry_lookup_opt_repro_run
& $taskPython -B tools/port/verify_registry_lookup_source.py --output build-android-audit/registry_lookup_source_repro.json
```

Detect authorized HONOR with ADB before running; final serialAKWJ9X5116G04618,
NDL-L09/API36/page4096. Full decode runs separately before bounded prefix loading.
Evidence: evidence/arm64_registry_lookup/file_inventory.json. Ledger DEC50,
FACT103/104/WARN35/FAIL33. Standing DEC47 continues; local commits/no push.
Next: actual production insertion/registration, returned-header loader integration,
then next genuine asset. Full registration dependencies must remain explicit.
