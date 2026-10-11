# TASK-0011 — Checked genuine FastFile zone allocation

TASK-0011 is complete as an isolated allocation gate. The genuine decoded36-byte XFile drives checked production-policy block requests on HONOR Pad X8a and Windows x86/x64. No new stream initialization, asset parsing, pointer fixups, zone initialization or rendering occurs. Nothing is committed or pushed.

Current Ledger: DEC-0021/0022, FACT-0032/0033/0034, FAIL-0006, WARN-0011. TASK-0012 is recommended, not started. The current checkpoint remains6f0787ab. Older reports preserve earlier milestones.

## Genuine metadata and production policy

Original inputs are read-only under ../BlackOps64/build/Release/zone/common. The provenance chain is original/device raw SHA256 -> unchanged bounded wrapped decoder checksum/end/exact decoded length -> header fields and full decoded Adler32 matching the frozen independent Python oracle -> extracted36-byte header SHA256 -> allocation-only process. The planner alone does not authenticate arbitrary header bytes; the harness supplies only these verified extraction artifacts. Three fresh decode processes per asset/ABI repeat the chain. No decompressed payload is retained in an allocation process, and no original asset enters Git or the APK.

| Asset | Original file bytes | Decoded bytes | XFile.size | externalSize |
|---|---:|---:|---:|---:|
| code_pre_gfx.ff | 13,280 | 49,593 | 49,557 | 0 |
| zombie_pentagon.ff | 48,229,216 | 108,009,041 | 108,009,005 | 308,074,740 |

Independent oracles: [small](evidence/arm64_inflate/retained.inflate-genuine-oracle.json), [large](evidence/arm64_inflate/retained.inflate-zombie-oracle.json). Raw digests, extracted header hex/SHA, independent decoded digests/Adler and every actual derived block are in the frozen JSON. Neither payload length nor externalSize supplies allocation descriptors. The large disk block sum108,078,487 differs legitimately from payload108,009,005.

The unchanged production DB_AllocXBlocks policy leaves Win32 sizes intact. Native64 nonzero blocks use floor(disk*factor100/100)+65,536: factor200 below1MiB,190 from1MiB,135 from32MiB. Zero stays zero. DB_MemAlloc requests another15 bytes for nonphysical blocks0/1/2/4/5; physical3/6 do not. All requests align4096. No new multipliers or Windows production policy changes were introduced.

| Asset / slot | Disk bytes | Native64 factor | x86 logical / request | x64 & ARM64 logical / request |
|---|---:|---:|---:|---:|
| code_pre_gfx / 0 | 36 | 200 | 36 / 51 | 65,608 / 65,623 |
| code_pre_gfx / 1 | 0 | 0 | 0 / 0 | 0 / 0 |
| code_pre_gfx / 2 | 0 | 0 | 0 / 0 | 0 / 0 |
| code_pre_gfx / 3 | 0 | 0 | 0 / 0 | 0 / 0 |
| code_pre_gfx / 4 | 49,092 | 200 | 49,092 / 49,107 | 163,720 / 163,735 |
| code_pre_gfx / 5 | 0 | 0 | 0 / 0 | 0 / 0 |
| code_pre_gfx / 6 | 0 | 0 | 0 / 0 | 0 / 0 |
| zombie_pentagon / 0 | 2,098,316 | 190 | 2,098,316 / 2,098,331 | 4,052,336 / 4,052,351 |
| zombie_pentagon / 1 | 4,538,228 | 190 | 4,538,228 / 4,538,243 | 8,688,169 / 8,688,184 |
| zombie_pentagon / 2 | 0 | 0 | 0 / 0 | 0 / 0 |
| zombie_pentagon / 3 | 0 | 0 | 0 / 0 | 0 / 0 |
| zombie_pentagon / 4 | 93,836,449 | 135 | 93,836,449 / 93,836,464 | 126,744,742 / 126,744,757 |
| zombie_pentagon / 5 | 0 | 0 | 0 / 0 | 0 / 0 |
| zombie_pentagon / 6 | 7,605,494 | 190 | 7,605,494 / 7,605,494 | 14,515,974 / 14,515,974 |

Native backend totals are229,358 and154,001,266 bytes; x86 totals49,158 and108,078,532. Native first-fit capacities are241,664 and154,017,792 bytes. Capacity derives from sizeof(HunkUserDebug), sizeof(FirstFitHunkUser), heapnode/back-pointer placement, native node alignment, actual4096 payload alignment and final free tail strictly greater than1024, then native page rounding/minimum65,536. The bounded ceiling uses the existing Windows328/492MiB pool; it is an explicit gate budget, not a new production expansion factor.

## Measurement protocol and timings

31 retained warm samples follow3 excluded warmups per asset/backend.20 cold samples each use a fresh process: process-cold, filesystem cache/CPU frequency/thermal conditions uncontrolled, no storage-cold claim. Decode uses3 separate fresh processes per asset/backend. Runs are sequential and retain one decoded vector; no allocator measurement overlaps decoding.

Clock: HONOR CLOCK_MONOTONIC, reported resolution1ns; Windows QueryPerformanceCounter, resolution100ns. Median uses the middle value or mean of middle pair; p95 is nearest rank ceil(.95*N). Raw nanosecond samples are retained. Windows warm results near/below one tick cannot establish sub-tick costs; a raw0 means below measurement resolution. Logging and /proc/PSAPI reads are excluded from timed sections; a synchronized1ms RSS sampler runs concurrently. Creation includes actual reservation/protection or PMem initialization/named Begin. Allocation includes actual backend work and Android memset. Release measures owner release. total_phase_ns is their sum, excluding planning, checks, descriptor clear and snapshots; it is not full wall-clock latency.

Each timing pair below is median / p95, milliseconds.

| Platform / asset | Warm creation | Warm allocation | Warm release | Cold creation | Cold allocation | Cold release |
|---|---:|---:|---:|---:|---:|---:|
| windows_x86 / code_pre_gfx | 0.000100 / 0.000100 | 0.000100 / 0.000200 | <0.000100 / 0.000100 | 0.009850 / 0.010300 | 0.000400 / 0.000400 | 0.000100 / 0.000100 |
| windows_x86 / zombie_pentagon | 0.000100 / 0.000100 | 0.000100 / 0.000200 | 0.000100 / 0.000100 | 0.009800 / 0.010300 | 0.000400 / 0.000400 | 0.000100 / 0.000100 |
| windows_x64 / code_pre_gfx | <0.000100 / 0.000100 | <0.000100 / 0.000200 | <0.000100 / 0.000100 | 0.009850 / 0.010200 | 0.002150 / 0.002500 | 0.000100 / 0.000200 |
| windows_x64 / zombie_pentagon | <0.000100 / 0.000100 | 0.000100 / 0.000200 | <0.000100 / 0.000100 | 0.009900 / 0.010400 | 0.002200 / 0.002500 | 0.000100 / 0.000200 |
| honor_arm64 / code_pre_gfx | 0.090417 / 0.150781 | 0.245729 / 0.565365 | 0.149895 / 0.310781 | 0.129401 / 0.147969 | 0.213854 / 0.278073 | 0.117630 / 0.178229 |
| honor_arm64 / zombie_pentagon | 0.059532 / 0.069791 | 69.365468 / 69.577657 | 17.960677 / 18.052031 | 0.137526 / 0.179947 | 70.260025 / 75.006563 | 17.914818 / 18.038333 |

## Memory and ownership contracts

Android reserves a separate mmap(PROT_NONE) owner, mprotects the full capacity read/write, then zeroes each actual request. Accessible/protected virtual pages do not establish eager physical commitment. Zeroed payload bytes are229,358 /154,001,266; payload page spans touched are233,472 /154,009,600 on this4096-page device, excluding allocator metadata. VmSize, VmPeak, VmRSS and VmHWM snapshots before/create/allocate/cleanup are retained. VmHWM is cumulative within a process; the1ms RSS sampler may miss brief peaks and Linux RSS accounting can batch updates (visible on the small allocation). Measured process RSS includes runtime/allocator overhead, not only zone bytes.

Windows actual physicalmemory.cpp creates its328MiB x86 or492MiB x64 shared pool with VirtualAlloc(MEM_COMMIT,PAGE_READWRITE), demand-zero. Subsequent actual DB_MemAlloc/PMem requests bump low/high offsets without touching payload bytes. PMem_Free rewinds named LIFO ownership and retains the pool. The fixture reads real g_mem positions/list counts; disabled production tracker hooks do not provide allocation counters. Native-only counters are null in Windows JSON. A first cold Windows run explicitly records pool_initialized_and_retained and raw_counters_match=false while logical_ownership_restored=true. Further runs/warm loops return to the initialized pool baseline. These are different commitment/residency contracts. Windows working-set/HWM/private-commit fields remain separately named; VmSize/VmPeak are unavailable there.

Peak observed process memory, MiB (max over separate runs; allocator includes runtime baseline):

| Platform / asset | Decode process resident HWM | Allocation sampled RSS peak (warm+cold) | Last warm cleanup RSS |
|---|---:|---:|---:|
| windows_x86 / code_pre_gfx | 4.270 | 4.465 | 4.465 |
| windows_x86 / zombie_pentagon | 235.797 | 4.453 | 4.453 |
| windows_x64 / code_pre_gfx | 3.340 | 3.543 | 3.543 |
| windows_x64 / zombie_pentagon | 235.961 | 3.547 | 3.547 |
| honor_arm64 / code_pre_gfx | 2.988 | 3.133 | 2.984 |
| honor_arm64 / zombie_pentagon | 131.250 | 149.805 | 3.055 |

The HONOR large decoder HWM131.25MiB is a separate-process observation, not an allocator peak or a strict decoded heap budget. Its final vector capacity134,217,728 exceeds108,009,041 output bytes; growth can transiently hold previous/new buffers. Windows final vector capacity145,282,681 and observed decoder HWM about236MiB illustrate different library growth. No speculative optimization is added. Android runtime VmSize is already around2.2GB before zone allocation; do not equate that VA with RAM. Large zone VmSize changes by exactly154,017,792 bytes and returns after unmap. Post-decode libc/runtime VA/cache may remain, small RSS may stay above entry, and HWM never resets. Native owner/allocation/request/reservation counters restore exactly; these retained process observations are not automatically leaks.

## Checked failures and verification

Planner checks truncated lengths0..35/null/unaligned storage, exact decoded size, UINT32_MAX and signed backend bounds, aggregate crossing32-bit/capacity limits, uint64 checked-add overflow helper, alignment0/3/8192, unsupported alloc types/flags, and inconsistent/tampered derived plans. Seven32-bit fields cannot themselves overflow uint64; that fixture tests capacity/32-bit cumulative overflow, with the primitive checked-add tested separately. All-zero makes no owner; sparse and all seven physical/nonphysical slots remain valid.

67 HONOR and66 per Windows ABI fixtures pass. Controlled null before each nonzero genuine block leaves real earlier allocations to roll back; output slots stay empty, End/Release restores counters, subsequent allocations succeed. Additional fixtures cover Begin(false), duplicate ownership without releasing the existing owner, backend misalignment, post-allocation exception and Android post-reservation owner-registration bad_alloc. Begin now publishes active only after successful map registration and destroys the hunk on exception; mmap registry exceptions unmap, and release lookup allocates no string key. Callback contract requires atomic failed Begin and nonthrowing End/Release. These localized fixes follow observed failure-path audit findings.

Real backend exhaustion is separately captured after a real earlier allocation: Android Hunk fatal exit134/SIGABRT; Windows production PMem OOM invokes the standalone terminal fixture endpoint exit23. Production exhaustion remains fatal; process teardown owns cleanup there. Controlled recoverable tests do not turn fatal engine paths into successful returns. The Windows probe uses a single named owner per primitive to avoid the unvalidated native PMem_FreeIndex previous-entry8-byte stride. It resets only the signed cumulative freeTot diagnostic in the fixture between releases to avoid repeated-run overflow; g_mem ownership policy is unchanged.

Final checked_zone_pass6:12 actual T5 units,6 unchanged bundled codec C units,0 compiler/linker errors,1919 retained warnings,83 verified AArch64 artifacts. New executable symbols exclude DB_InitStreams/Load_XAssetListCustom/DB_InitZone. Both host executables have verified x86/x64 PE machines and use actual db_memory.cpp plus physicalmemory.cpp. Original16 HONOR regressions pass; those historical fixture tests still exercise their prior stream-init boundary separately from this new allocation gate. Fresh Windows native-owner zone and four-byte Narrow32/483 guard regressions also pass. No UBSan finding appears on passing native paths; whole engine/native library coverage is not claimed.

Initial link/runtime-selection/macro diagnostics and all six Android/five host build attempts are frozen. Initial successful measurement pass is retained as superseded (sampler attribution race/unrelated debug arena/raw Windows baseline ambiguity); final.measurements.json is authoritative. Independent policy and adversarial/methodology reviewers performed read-only audits; lead owns all edits/builds/Ledger. Windows checkout2528 files/index/status/HEAD, DXVK2552 and root Ledger15 files, Android index/HEAD and763 old evidence files are checked unchanged. Scaffold APK SHA remainsDFA4C3BD6C8FDDF121CF87EE9B5847367FB6C29CDB5EAD8FC164F3A31F1135B5; no APK build/install needed.

## Reproduction

PowerShell, from D:/Desktop/Black Ops 2 Android/BlackOpsARM. Choose fresh directories for every run; originals/scratch and old device directories remain intact.

```powershell
$probePython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH=(Split-Path $probePython)+';C:/Program Files (x86)/Microsoft Visual Studio/Installer;'+$env:PATH
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/checked_zone_repro_01
./tools/port/build_checked_zone_host.ps1 -OutputDirectory build-android-audit/checked_zone_host_repro_01
& $probePython tools/port/measure_checked_zone.py `
  --host-build build-android-audit/checked_zone_host_repro_01 `
  --android-build build-android-audit/checked_zone_repro_01 `
  --output build-android-audit/checked_zone_measure_repro_01 `
  --serial AKWJ9X5116G04618 --warm 31 --warmups 3 --cold 20 --decode-runs 3
./tools/port/run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 `
  -BuildDirectory build-android-audit/checked_zone_repro_01
./tools/port/check_zone.ps1
./tools/port/check_narrow32.ps1 -PythonExe $probePython
```

NDK30.0.16248370/Clang21, API28 ARM64 Debug C99/C++20 and UBSan; HONOR NDL-L09/HNNDL-Q API36/native4096 pages. Windows MSVC builds use /O2,/MTd,/D_DEBUG; measurements are this probe configuration, not a whole-engine release benchmark. Clock/resolution and original/full device paths are in JSON. Actual successful build directories are checked_zone_pass6 and checked_zone_host_pass5; final harness output checked_zone_measure_pass2. Device directory: /data/local/tmp/blackopsarm-phase1-checked-zone-20261011-212015383323. Every command/exit is in final.invocations.json; binaries and raw logs remain local. Kernel/CPU governor context is frozen in final.device-metadata.stdout.log.

Freeze a new run with tools/port/collect_checked_zone_evidence.py; required arguments are shown by --help. It does not archive binaries or original assets. [Machine measurements](evidence/arm64_checked_zone/final.measurements.json), [artifact/source/diagnostic manifest](evidence/arm64_checked_zone/compiler_summary.json), [protected-state verification](evidence/arm64_checked_zone/preservation.json), [exact file inventory](arm64_checked_zone_changes.md).

## TASK-0012 recommendation

The next smallest real production loading boundary is DB_InitStreams on these checked genuine allocations. Acceptance: genuine code_pre_gfx and zombie_pentagon on HONOR and x86/x64; retain disk header/block policy, verify all seven initial cursors/zero blocks and pending alias/stride reset, repeat clear/detach/release with baseline owners; reject invalid/inconsistent plans before allocation; keep existing regressions and protected checkouts/evidence intact. Stop before Load_XAssetListCustom, asset list/string loading, stream deserialization, pointer fixups, zone registry/init, async integration or rendering. No TASK-0012 implementation starts here. WARN-0011 records full VM/remapping/backend and actual16KB/S25 limits.
