# TASK40 - real default services and retained-zone name ownership

Actual DB_FindXAssetDefaultHeaderInternal, DB_CreateDefaultEntry, DB_SetXAssetName
and DB_FreeDefaultEntries now execute with genuine SL/MT on HONOR ARM64 and
Windows x86/x64. A real MP bootstrap default provider remains alive while the
first Pentagon StringTable and its intentional TechniqueSet reference load.
This is production-function asset integration, not Com_Init or headless gameplay.

Two Low/High cycles keep both checked zone owners in the same prim. Three live
entries have the actual policy: provider zone1, table zone2 and fallback zone0.
The fallback borrows the complete provider graph and receives the stripped name
from one real SL user4 interned allocation/reference (ID2,19 bytes,Adler1186006616).
Repeated actual Add on the still-live original reference returns the same clone
without increasing entry/default/string counts. The provider remains valid after
switching stream maps and overwriting map TEMP; all1594 map table queries pass.

For teardown, the fixture detaches nondefault memberships while retaining their
headers. Actual FreeDefaultEntries frees the zone-zero clone; reverse table and
provider frees restore32752 entries and2048 TechniqueSet pool nodes. Both owners
release in reverse order, then user4 and SL shutdown restore the complete MT/hash/
debug-reference baseline. Owner and borrowed-pointer lifetimes stay explicit.
Windows same-prim counts2->1->0 pass; native offsetof/count arithmetic explains
that two-owner case. The existing third-plus-owner native PMem stride remains
unchanged and uncertified. Android uses actual native named Hunk owners.

Review corrected an existing generic setter aliasing risk. DB_DDLSetname formerly
accessed every payload as XModelPieces, although the actual object here is a
TechniqueSet. Passing O3/UBSan did not prove that access valid. It now copies the
payload pointer and native name pointer using two memcpy transfers, matching the
existing getter, preserving the first-word assignment and unchecked nonnull caller
contract. Normal setter dispatch remains exact. This removes the need for the
proposed XModelPieces declaration move: xmodel.h is unchanged. The discarded
owned type snapshot is preserved as superseded source evidence.

Authoritative corrected checks:

- default_resolution_host_corrected and O0/O3_corrected build with assertions and
 ARM UBSan; matching O0/O3_corrected_run1 collectors pass all three ABIs. Full
 retail provenance and native provider/map prefix SHA are independently verified.
- Actual missing-provider fatal returns Windows3/HONOR134 with exact production
 message, after observing zero entries/defaults/string allocations.
- default_resolution_prereq_O0/O3_corrected_run1 pass all inherited TASK36/37
 checks:215 cases/12 cycles/1994 queries and complete37-asset zone,4 cycles/
 148 Add calls perABI. The new target's prior missing-default control checks the
 actual rawfile default error rather than the formerly unavailable fixture
 endpoint. Old targets/collectors are unchanged; exact error/exit checks remain.
- default_resolution_provider_O0/O3_corrected_run1 retain full TASK39 MP/SP provider
 graphs, TEMP/cleanup checks and explicit renderer-enabled refusal on allABIs.
- default_resolution_normal_corrected compiles five full normal Windows units per
 ABI:load,expand,material,registry,assetnames. Source proof now verifies46 unchanged
 production bodies plus the exact reviewed setter correction; SL/MT,PMem,material
 types and XModel declarations remain byte-equivalent to76a4956.
- default_resolution_protected_final.json verifies all19930 protected hashes and
 exact Windows/DXVK Git states. User headless guidance was recorded before this
 protection snapshot; prior snapshots are retained. No stable checkout changes.

689 evidence records are indexed under evidence/arm64_default_resolution. Logs and
large JSON metadata use deterministic lossless gzip; inventory records compressed
and original SHA/byte counts. Source and small metadata remain readable. Retail
files, decoded prefixes and compiled binaries never enter Git. Native full decoder
processes release buffers before zone allocation; original device map data is
reused only after SHA verification. All tests completed; no probe remains running.

Superseded evidence is retained: probe1 lacked two fixture declarations; initial
47-body/type-move proof and successful runtimes preceded the setter review and
are not authoritative for the corrected source. The superseded lightweight header
is archived and its local copy remains in default_resolution_pre_review. Earlier
checks are passing-path evidence, not strict-aliasing certification. FAIL38 records
the correction. No checks or gameplay behavior were disabled to obtain success.

Reproduce from BlackOpsARM with fresh outputs:

```powershell
tools/port/build_default_resolution_host.ps1 -OutputDirectory build-android-audit/default_host_new
tools/port/build_default_resolution_arm64.ps1 -BuildDirectory build-android-audit/default_O0_new -Optimization O0
tools/port/build_default_resolution_arm64.ps1 -BuildDirectory build-android-audit/default_O3_new -Optimization O3
tools/port/build_bootstrap_default_windows_compile.ps1 -OutputDirectory build-android-audit/default_normal_new
```

Use bundled Python with run_default_resolution_gate.py --host-build/--android-build/
--output for each optimization, then matching run_default_resolution_prerequisites.py
and run_default_provider_regressions.py. verify_default_resolution_source.py
--output records source evidence. Each output directory must be fresh. The gate
rediscovers authorized HONOR, verifies executable architecture/source manifests/
remote SHA, hashes complete decoded prefixes, and rejects six malformed ownership
records. A real missing-provider error remains an explicit death test.

Next production boundary is the actual first startup zone code_pre_gfx_mp.ff:
150108 decoded bytes,71 assets (3StringTables+68RawFiles),no script strings.
Its already-supported types allow an entire genuine boot dependency to load via a
reusable native reader/owner and production root dispatch. Replace borrowed-prefix
fixture plumbing with real production services, then integrate actual file/zone
worker startup and Com_Init/Com_Frame. Android currently has no complete DBthread/
event/TLS/APC/overlapped reader/resource-pump implementation; APK remains the
environment scaffold. Presence of native expanders does not prove full-loader
compilation. The next postgfx asset is Image8, not Material6; a historical failure
label was inaccurate but numeric directory metadata always checked8 correctly.

User's verified x64 intentional stubs are accepted content, not repair tasks.
Dead Ops Arcade is excluded. Genuine dedicated headless startup and Zombies
simulation are prioritized before rendered gameplay (DEC59). No asset harness,
renderer-off helper or fabricated frame proves headless gameplay.
