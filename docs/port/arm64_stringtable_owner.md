# TASK-0026: actual persistent StringTable pool-header ownership

Actual production pool allocation, header-size dispatch, shallow clone and free
now execute on HONOR ARM64 and Windows x86/x64. Both genuine first StringTables
retain all1994 lookup results after TEMP Pop and deliberate overwrite of their
old temporary headers. This establishes the persistent-header prerequisite,
while the children remain owned by the live zone. It does not execute DB_AddXAsset.

## Production primitives and preserved policy

The canonical XAssetPoolEntry/XAssetPool declarations and unchanged InitPool/
AllocXAsset templates move once into database/db_asset_pool.h. Existing576 normal
and768 mod constants move with them; the actual g_StringTablePool still reserves
768 entries in db_registry.cpp. Narrow compile selection exposes that real storage,
actual clone/free bodies and db_assetnames.cpp's actual StringTable size leaf and
type dispatcher. Only real handler37 is selected; other in-range handlers are
null and retain the actual assert path. The getter's original0..42 index caller
contract remains. There are no fabricated size handlers or successful registration.

| ABI | Header / entry | Alignment | Actual full pool storage |
| --- | ---: | ---: | ---: |
| Windows x86 |20 bytes|4|15364 bytes|
| Windows x64 / Android ARM64 |32 bytes|8|24584 bytes|

These are actual sizeof/storage measurements, not RSS or allocation expansion
multipliers. Original disk header remains20 bytes and serialized tags remain four
bytes. Raw allocation pops the ascending freelist and returns null on exhaustion,
without clearing data. Header clone uses the unchanged actual type-size dispatch
and memcpy; it copies name/cell/index pointers, not child arrays. Free pushes LIFO
and releases no children. Existing policy remains576/768, even though SHA-pinned
original executable evidence records a historical80-entry pool configuration.

The existing generic-free leaf retains its signature and handler/wrapper behavior.
Its sole implementation correction transfers native pointer representations with
four qualified ::memcpy calls: copy header representation to a void pointer,
save old head, set head to the released node, store old head in that node's first
pointer-width slot. This avoids unrelated EmblemSet/XModelPieces storage lvalues
and inactive-union access in this function. Width/offset assertions and compiled
historical Windows reference verify identical valid-call LIFO/payload-tail results.
This is not a full audit of existing handler function-pointer casts, union lifetime
conventions or registry concurrency. Valid live pool/owned-node caller contracts
remain required; no production double-free/foreign-node recovery is invented.

## Original and source evidence

Original executable SHA256 remains
`2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759`.
Read-only mapped instructions prove original type-size dispatcher0x514840,
StringTable20-byte getter0x7a2420, pool pop0x7a47e0/push0x7a4800 and fresh link
allocation followed by type-size dispatch/CRT memcpy at0x7a30b3. Original processes
and registry services were not executed. VA/RVA/file offsets, range hashes,
commands and disassemblies from ownership_boundary_audit_run1/2 are frozen.

Historical Windows generic-free control is extracted from fa27a73, renamed only,
compiled and compared on both Windows ABIs. Its317-byte body SHA256 is
`eef33b9eb78a9bb7221d62e035e7c6dd3cc06b30f6be7ef272844f7fe3826968`.
Source proof1 reverses exact selectors and declaration/template/policy moves,
preserves original UTF-8 BOM and complete default type-size table order, and
identifies the isolated free-body change. Init/Alloc/Clone/size leaves and all
earlier loader/query/hash/normalization sources remain byte unchanged.
Full ungated Windows compilation covers eight TUs on each ABI, including registry
and assetnames. No source duplication or broad engine restructuring is introduced.

## Verified lifecycle

The finite genuine-load fixture finishes raw header/children/index loading, allocates
an actual pool entry, executes actual clone BEFORE normalizing, then normalizes
and queries the pooled header exactly once. Sorting equal keys again can change
their order, so no second normalization occurs. After actual TEMP Pop, the fixture
overwrites20/32 bytes of old TEMP header storage and repeats all400/1594 queries
through the persistent copy. Every header pointer/byte and block4 child data remains
correct, with zero misses/disagreements and unchanged stream/zone counters.
Pooled headers are freed and the original full free chain restored BEFORE releasing
zone children. Existing TEMP alias addresses are not retargeted or dereferenced
after overwrite; maps are detached before zone release. Registry publication,
inserted-pointer ownership and a second real asset remain separate boundaries.

Each final ABI run passes162 earlier loader cases,86 normalization cases/60 actual
Windows CRT controls,12857 hash vectors/7974 wrapping,262180 comparison cases and
22 controlled queries, plus12 genuine cycles. Pool lifecycle controls initialize
sizes1/2/576/768, allocate1347 nodes, observe4 null exhaustions with no mutation,
verify LIFO reuse and restore all unique ascending links without using reset to
hide ownership. Twenty-one controlled caller-precondition guards reject invalid
init/live reset, null/foreign/interior/double release and invalid clone type,
released/null/foreign destination, short/misaligned/wrapped/overlapping source or
destination before invoking the original unchecked leaves. These guards are fixture
contracts, not evidence that raw production leaves validate malformed calls.

Strict177 ordered records preserve raw scalar bytes before normalization, exact
original normalized words/digests and every actual query result, plus pool/TEMP
measurements. ABI-dependent sizes/addresses are individually validated and retained;
remaining records compare across platforms. Reused terminal TASK23 schema reports
prerequisite cases; mandatory owner records and enclosing TASK26 schema distinguish
this gate. The original row reference remains an explicitly labeled digest-based
model from TASK24, while native engine query leaves actually execute here.

Final debugpass3/hostpass3/run1 and optimizedpass1/run1 all pass. Assertions and
UBSan remain enabled at -O3; Windows binaries are reused in optimized orchestration,
which runs a distinct optimized HONOR binary. FullARM0 errors/8192 existing warnings,
347 AArch64 ELF files including CMake checks; optimized0 errors/764 warnings,
46 ELF files including checks. NormalWindows8TUs perABI compile16 objects.
All tests completed. No new latency/RSS/peak-memory or graphics claim.

FAIL26 retains first Windows/host/ARM compile diagnostics: a historical BOM was
initially placed inside the selector, and unqualified calls triggered ADL through
an incomplete EmblemSet specialization. BOM byte0 and explicit call qualification
corrected both before final builds. Initial/final builds, failed diagnostics,
original reference evidence and ignored scratch remain retained. Frozen archive
contains150 copied records plus review, summaries, source/binary hashes and
preservation proof. No asset, executable, prefix, full decoded buffer or binary is
committed. Native decoder process releases its buffer before zone allocation;
existing SHA-verified HONOR FF cache is reused without duplicate transfers.

## Reproduction on HONOR and Windows

Run from BlackOpsARM in PowerShell; output directories must be fresh.

```powershell
$ownerPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH=(Split-Path $ownerPython)+';'+$env:PATH
& tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/stringtable_owner_pass4
& tools/port/build_stringtable_owner_host.ps1 -OutputDirectory build-android-audit/stringtable_owner_host_pass4
& tools/port/build_stringtable_owner_windows_compile.ps1 -OutputDirectory build-android-audit/stringtable_owner_windows_compile_pass3
& tools/port/build_stringtable_owner_optimized_arm64.ps1 -BuildDirectory build-android-audit/stringtable_owner_optimized_pass2
& $ownerPython -B tools/port/run_stringtable_owner_gate.py --android-build build-android-audit/stringtable_owner_pass4 --host-build build-android-audit/stringtable_owner_host_pass4 --output build-android-audit/stringtable_owner_run2 --serial AKWJ9X5116G04618
& $ownerPython -B tools/port/run_stringtable_owner_gate.py --android-build build-android-audit/stringtable_owner_optimized_pass2 --host-build build-android-audit/stringtable_owner_host_pass4 --output build-android-audit/stringtable_owner_optimized_run2 --serial AKWJ9X5116G04618
& $ownerPython -B tools/port/verify_stringtable_owner_source.py --baseline build-android-audit/stringtable_owner-baseline.json --output build-android-audit/stringtable_owner_source_proof2.json
```

## Preservation and next gate

Windows2528 files/DXVK2552/root Ledger15/APK/original assets/executable and3161
older frozen files remain unchanged. Exact changed paths/hashes are in
arm64_stringtable_owner_changes.md and archive/file_inventory.json. Local verified
milestone commits authorized, no push; Git/STATE identify later pending work.

TASK27 must audit TEMP physical cursor, Win32 shadow, segment and alias lifetimes
against original evidence before another asset. Code FF block0 disk capacity36
is a reusable workspace, not a cumulative header budget. Current native Pop rewinds
physical TEMP while retaining shadow20/maps; historical first-header tests do not
certify reuse. Preserve bounds and aliases; do not arbitrarily relax capacities.
Acceptance: original transition/offset provenance, guarded native reuse matching
Win32 workspace semantics, actual repeated header loads with persistent pool copies,
failure atomicity and reset/owner lifecycle on3ABIs/optimized, all frozen evidence.
Then validate real DBHASH synchronization/asset-entry ownership/initialization
services and integrate the smallest real registry boundary. Full DB_AddXAsset also
needs dvars, zone/override policy and lost-device synchronization, never successful
stand-ins. Async/refill, dispatch, zone init, engine-owned graphics, S25/16KB,
hash locale/highbytes and retained-page/multiowner limitations remain open.
