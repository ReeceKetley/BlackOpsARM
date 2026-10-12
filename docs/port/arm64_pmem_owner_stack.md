# TASK42 native Windows PMem owner-stack prerequisite

The original native-width `PMem_FreeIndex` misindexes earlier allocation records
when a primitive holds three or more owners. A new production-function regression
reproduced x64 exit1 (`FAIL exact one owner removed`) while x86 exited0. The
decompiled expression uses an eight-byte record stride from `allocListCount`;
native `PhysicalMemoryAllocation` has a pointer plus position and padding.

The native-width branch now indexes `prim->allocList[count-1]`. The Win32 branch
and all remaining allocator behavior are byte-for-byte preserved. The reader's
temporary two-owner refusal is removed; its original 32-record capacity, active
owner, cursor, name, stream and child-lifetime checks remain.

## Verified behavior

- Actual PMem Begin/Alloc/End/Free functions pass 3-, 8-, and 32-owner stacks on
  both low/high primitives, 86 releases per ABI. Independent cursor arithmetic,
  retained name/start positions and every retained payload byte are checked.
- Genuine MP pre-gfx reader passes the third owner, 32nd owner on both primitives,
  and atomic 33rd-owner refusal. Stream attachment retains the third owner.
  All earlier owners and final low/high baselines survive cleanup.
- Complete MP71 loading passes Windows x86/x64 and HONOR O0/O3 with fresh source
  manifests and independent retail oracle: 284 asset records per run, all table
  queries, child lifetime and removal baselines. This is asset integration, not
  actual `Com_Init`, DB worker execution or gameplay.
- Eight normal Windows translation units per ABI compile with native loader
  selection off. Ten current build manifests match. Exact two production deltas
  are proven against `c297348`; 19930 protected hashes, reference Git and all
  689+82+157 frozen prior evidence records are unchanged.

Android uses its existing separate named-zone allocator, so the PMem repair is
a Windows comparison prerequisite rather than execution of PMem on ARM64.
HONOR regressions verify the Android production loader remains working.
Existing PMem `freeTot` diagnostics and non-LIFO fatal behavior are outside this
repair; no policy or serialized layout has changed. Main-thread-only Android
zone ownership still needs a genuine DB-worker design before worker loading.

## Reproduction and evidence

Run scripts from the Android worktree, with fresh output paths:

```powershell
./tools/port/build_pmem_owner_stack_host.ps1 -OutputDirectory build-android-audit/pmem_owner_stack_after1
./tools/port/build_native_zone_file_host.ps1 -OutputDirectory build-android-audit/native_zone_file_multiowner2
./tools/port/build_native_startup_zone_host.ps1 -OutputDirectory build-android-audit/native_startup_zone_pmem1
./tools/port/build_native_zone_load_windows_compile.ps1 -OutputDirectory build-android-audit/native_zone_pmem_normal1
./tools/port/build_native_startup_zone_arm64.ps1 -BuildDirectory build-android-audit/native_startup_zone_pmem_O0_1 -Optimization O0
./tools/port/build_native_startup_zone_arm64.ps1 -BuildDirectory build-android-audit/native_startup_zone_pmem_O3_1 -Optimization O3
```

Execute each Windows reader/loader binary with
`D:/Games/Call of Duty - Black Ops/zone/Common/code_pre_gfx_mp.ff` and record
exit/log JSON. `check_startup_zone_runtime.py` validates host MP71 records.
Detect the connected HONOR, then use `run_native_startup_zone_gate.py` for both
optimizations, passing matching build/output, actual serial and optimization.
`verify_pmem_startup_evidence.py --output <fresh.json>` verifies the final source,
build/runtime and preservation set. Frozen logs/JSON/TXT are lossless under
`evidence/arm64_pmem_owner_stack`; original x64 failure is retained. No retail
assets, decoded payloads or build binaries are committed.

Next: `native_startup_services_plan.md` details genuine TLS/general locks,
fixed32 Dvar synchronization and command initialization. Close these production
services before actual DB worker/events/file publication and complete headless
`Com_Init`/frames.
