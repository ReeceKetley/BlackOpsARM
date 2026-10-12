# TASK33: genuine headers owned through actual asset entries

Genuine first StringTable headers and code_pre_gfx's second RawFile now acquire
persistent pool headers through actual DB_AllocXAssetEntry, clone through actual
DB_CloneXAssetInternal before TEMP Pop and release through actual DB_FreeXAssetEntry
before zone teardown. This extends the real codec/zone/loader boundary into actual
entry ownership; it remains a finite standalone integration gate, not an APK boot.

Each code cycle owns a StringTable entry and RawFile entry; zombie owns its table
entry and stops at the genuine next TechniqueSet7 without skipping. Every live
entry retains its actual type, zone1, header pointer and zero hash/override/inuse
fields through loading/TEMP overwrite. Complete child bytes and all400/1594 first
table queries remain exact. Code source ends10577; zombie65678. Header cloning
precedes the original normalization exactly once, preserving original query order.

RawFile entry frees first, StringTable second, restoring the entire ascending
32752-entry chain and both pool baselines before zone detachment/release. Live
count returns0; actual highwater remains2 across cycles without reset. Six code
and six zombie cycles allocate18 entries perABI/run. Hash buckets stay empty
throughout: allocation/clone/ownership is not registry publication or DB_AddXAsset.

Ownership is serial/private/unpublished. Actual DBHASH guards allocation/free;
clone occurs after acquisition releases the lock while the private entry/header
remains owned by this serial loader. No concurrent registration or live zone reset
is certified. Fixture dvar/log/error endpoints retain explicit scope; LoadObj is
explicitly unavailable. No endpoint is added to application production source.

Final host_pass2/debug_pass2/optimized_pass2, debug_run2/optimized_run2 passWindows
x86/x64/HONOR ARM64. Android O0/O3 keeps assertions/test checks/UBSan. Full inherited
215 prerequisite cases,12 cycles/all1994 queries and previous ownership controls
pass, plus12 exact typed entry-cycle records. Four new malformed record controls
reject missing/duplicate cycles, bool-as-int and wrong asset order. The collector
retains all strict TASK30/29 provenance/loader/query contracts and raw logs.

Whole production src/tl/DemonWare trees remain unchanged against4f2d4bd. The source
proof checks exact additive fixture hooks, retained existing CMake targets and
parent tree identities. New manifests include all fixture headers and entry/native
service dependencies. No extra default Windows compile was needed: production
code is unchanged from TASK32's passing full ungated Windows8TUs/ABI check.

Initial O0/O3 ARM link diagnostics caught duplicate va with existing hunk fixture
support. Android now reuses that formatter; the new definition is host-only.
Final host was rebuilt after this fix. All failed/superseded logs and old evidence
remain. Independent read-only review found no ownership/order blocker and suggested
the new malformed controls, now passing. Protected17393 baseline files and exact
Windows/DXVK Git remain unchanged. All tests completed. No original assets/prefixes/
proprietary binaries in Git, no new memory/timing/16KB/S25/graphics/startup claim.

Reproduce from BlackOpsARM using fresh directories and an ADB-discovered authorized
HONOR (the final runs used AKWJ9X5116G04618):

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_entry_genuine_host.ps1 -OutputDirectory build-android-audit/entry_genuine_host_repro
./tools/port/build_entry_genuine_arm64.ps1 -Optimization O0 -BuildDirectory build-android-audit/entry_genuine_debug_repro
./tools/port/build_entry_genuine_arm64.ps1 -Optimization O3 -BuildDirectory build-android-audit/entry_genuine_opt_repro
& $taskPython -B tools/port/run_entry_genuine_gate.py --host-build build-android-audit/entry_genuine_host_repro --android-build build-android-audit/entry_genuine_debug_repro --output build-android-audit/entry_genuine_debug_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/run_entry_genuine_gate.py --host-build build-android-audit/entry_genuine_host_repro --android-build build-android-audit/entry_genuine_opt_repro --output build-android-audit/entry_genuine_opt_repro_run --serial AKWJ9X5116G04618
& $taskPython -B tools/port/verify_entry_genuine_source.py --output build-android-audit/entry_genuine_source_repro.json
```

Native complete decode finishes in separate processes before zone allocation; only
small SHA-verified prefixes remain in ignored/device scratch. Entry allocation
runs in that genuine finite loader, without duplicate asset source or fictitious IDs.
Evidence: evidence/arm64_entry_owned_genuine/file_inventory.json. Ledger DEC49/
FACT101/102/WARN34/FAIL32. Standing autonomous goal continues; local checkpoint/no push.

Next: actual name/hash lookup with original collision/miss behavior, then smallest
real registration/returned-header path, then next genuine asset deserialization.
FullDB_Init/Dvar/async/refill/error recovery/moddeferred/publication/engine startup
and rendering remain unverified; preserve TechniqueSet7 boundary rather than skip.
