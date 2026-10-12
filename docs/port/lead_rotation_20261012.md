# Verified lead rotation — 12 October 2026

Implementation is stopped. **Current HEAD is `0a9dbfc`, not `3f7dc56`.** The
requested20-file inventory describes the earlier morning pause; it is preserved
and reproduced below. No production edits, tests, commits, pushes, resets,
cleanup, stash or discarded work occurred during this handoff. Await user resume.
Latest no-commit instruction overrides earlier milestone-commit approval.

Objective: native Android ARM64 port of the original T5/BO1 Zombies engine, with
original assets/FastFile layouts, Windows x64 functional reference, controller
input and native lifecycle/audio/graphics. Small Win32 OS shims are acceptable;
graphics target is the independent DXVK Native1.9.2/SDL2/Vulkan integration.
Do not duplicate engine source, broadly refactor gameplay/decompiled code, change
disk/four-byte Narrow32 slots or modify Windows/DXVK/root/original assets.

## Git and roadmap location

Android worktree: `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, branch
`android-arm64`, shared history with `../BlackOps64`, independent directory/index.
Windows `main` remains76edfd5; separate DXVK `master` remains0e271c6.

| Checkpoint | Verified meaning |
| --- | --- |
| `ee13677` / `3f7dc56` | TASK27 source/Ledger; last checkpoint at the earlier pause |
| `acd48e5` / `7f7f005` | TASK28 TEMP writes/aliases and genuine RawFile numeric header complete |
| `8d3efb2` / `0a9dbfc` | TASK29 genuine RawFile name/buffer complete; current source/Ledger |

Git was clean before this handoff. All earlier implementation is committed;
there are no unfinished production changes. Only these five new handoff paths
are uncommitted (no staging/commit performed):

- `.ledger/AGENT-INSTRUCTIONS.md`
- `.ledger/STATE.md`
- `.ledger/ledger.jsonl`
- `docs/port/lead_rotation_20261012.md`
- `docs/port/evidence/lead_rotation_20261012/verification.json`

Ledger `NOTE-0009` records the stop. FACT89–91/FAIL28/DEC43 cover TASK28;
FACT92–94/DEC44/WARN30 cover current TASK29. Earlier historical statements in
STATE/AGENTS remain preserved; read the newest stop/current state first.

## Exact historical20 files at3f7dc56

Original evidence: `build-android-audit/morning_handoff_20261012/inventory.json`,
byte-identical frozen copy
`docs/port/evidence/arm64_temp_write/morning_handoff_20261012-inventory.json`.
Nine modified and11untracked then; all20 are tracked now. Historical SHA256 and
current hashes/status are in the new verification artifact. No old inventory
or scratch was overwritten. This table is **not** today's Git pending list.

| Path | At pause | Category |
| --- | --- | --- |
| `.ledger/STATE.md` | Modified | Pause bookkeeping |
| `.ledger/ledger.jsonl` | Modified | Pause bookkeeping |
| `src/database/db_expand.cpp` | Modified | Implementation |
| `src/database/db_rawfile_types.h` | Untracked | Implementation |
| `src/database/db_registry.h` | Modified | Implementation |
| `src/database/db_stream.cpp` | Modified | Implementation |
| `src/database/db_stream.h` | Modified | Implementation |
| `src/database/db_stream_load.cpp` | Modified | Implementation |
| `src/platform/fastfile_rawfile_header.cpp` | Untracked | Implementation |
| `src/platform/fastfile_rawfile_header.h` | Untracked | Implementation |
| `tools/port/build_rawfile_header_host.ps1` | Untracked | Tests/build/collection/proof |
| `tools/port/build_rawfile_header_optimized_arm64.ps1` | Untracked | Tests/build/collection/proof |
| `tools/port/build_temp_write_arm64.ps1` | Untracked | Tests/build/collection/proof |
| `tools/port/build_temp_write_host.ps1` | Untracked | Tests/build/collection/proof |
| `tools/port/rawfile_header_oracle.py` | Modified | Reference evidence tool |
| `tools/port/run_rawfile_header_gate.py` | Untracked | Tests/build/collection/proof |
| `tools/port/run_temp_write_gate.py` | Untracked | Tests/build/collection/proof |
| `tools/port/tests/stream_init_tests.cpp` | Modified | Tests/build/collection/proof |
| `tools/port/tests/temp_write_tests.cpp` | Untracked | Tests/build/collection/proof |
| `tools/port/verify_temp_write_source.py` | Untracked | Tests/build/collection/proof |

## TASK28 tests: earlier pause versus final results

At pause: `temp_write_run1` had passed9normal processes/18cases/12shortreads
per process and78native overlap checks; whole runner failed at the intentional
Windows fatal child timeout60s. Corrected CRT reporting/opaque-value probes and
genuine RawFile integration still needed rerunning. The numeric header had not
yet executed. These historical limitations have since been resolved and rerun.

Final `temp_write_run4` with host/ARM pass6:9normal processes pass; native opaque6,
staged invalid tombstone/C-string checks pass. Three fatal modes per native ABI
produce required diagnostics (Windows exit3/HONOR134). Genuine header debug
`rawfile_header_run2` and optimized `rawfile_header_optimized_run1` pass all3ABIs,
186cases/12cycles; code header12→24 atTEMP0 ends10419, all400/1594 first-table
queries survive. Zombie nexttype7 stays deferred65678. Sourceproof_rawfile3 and
normal Windows checks pass;16 prior HONOR regression executables had already
passed, including fixed4/native8 ABI probes. **No corrected tests remain pending.**

Failures preserved: hostpass1 min/max macro collision; run1 CRT timeout;
run2 wrong Invalid expectation for a retained value; run3 genuine new-slot
publication bug (partial/outside staged slot incorrectly became Active), fixed
with complete logical4/native-slot commit bounds; rawfile_header_run1 prefix
producer read mutated front-end cursor, emitted10407 instead of10419. Logs,
failed results and correction evidence remain in ignored directories and the
TASK28 frozen archive. No unresolved current test regression is identified.

TASK29 final hostpass3/debugpass2/O3pass3 runs also pass215cases/12cycles eachABI,
268 malformed measurement controls rejected. Actual header then original
`Load_RawFile(false)` reads genuine name17 and buffer141 to10577; disk block4
10496/native15357 (Win32 native10496). All first-table queries/bytes, aliases and
seven padding bytes survive. Canonical bodies/order are unchanged (source proof
restores all542 loader bodies). RawFile header is still transient, not registered.

## Environment and reproducibility

Fresh device check: HONOR `AKWJ9X5116G04618`, NDL-L09, API36,4096-byte pages,
ADB authorized/online. No local compiler/collector or device bo1 process is
running; delegated agents completed or errored, none active. S25/16KB untested.
SDK `D:/Android/Sdk`, NDK30.0.16248370, CMake3.22.1/Ninja, MSVC via
`C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat`.

Existing genuine assets are cached at
`/data/local/tmp/blackopsarm-phase1-string-table-20261011-225356426113`;
collectors SHA-verify/reuse them through `asset_directory_run1/results.json`.
Native full decode runs in separate processes before zone allocation. Retain
only small prefixes; protect HONOR4GB RAM. No originals/prefixes/binaries inGit.

Useful build/result directories under `build-android-audit/`:
`temp_write_host_pass6`, `temp_write_arm64_pass6`, `temp_write_run4`;
`rawfile_header_audit_pass1` (plus prefix-fix log), `rawfile_header_host_pass2`,
`rawfile_header_optimized_pass1`, header run2/optimizedrun1;
`rawfile_payload_host_pass3`, `rawfile_payload_debug_pass2`,
`rawfile_payload_optimized_pass3`, payload run1/optimizedrun1.
All scratch and earlier iterations remain. Frozen counterparts:
`docs/port/evidence/arm64_temp_write/` and `arm64_rawfile_payload/`.
Reports: `arm64_temp_write.md`, `arm64_rawfile_payload.md`; inventories:
`arm64_temp_write_changes.md`, `arm64_rawfile_payload_changes.md` and each archive's
`file_inventory.json`. The16-executable mixed-encoding regression log is retained
verbatim; do not normalize it or rerun everything just for rotation.

To reproduce focused current tests after a user resume, run from BlackOpsARM and
use **fresh** output directory names (commands were not executed during handoff):

```powershell
$taskPython='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
./tools/port/build_temp_write_host.ps1 -OutputDirectory build-android-audit/temp_write_host_rotation_repro
./tools/port/build_temp_write_arm64.ps1 -BuildDirectory build-android-audit/temp_write_arm_rotation_repro
& $taskPython -B tools/port/run_temp_write_gate.py --host-build build-android-audit/temp_write_host_rotation_repro --android-build build-android-audit/temp_write_arm_rotation_repro --output build-android-audit/temp_write_rotation_repro --serial AKWJ9X5116G04618
./tools/port/build_rawfile_payload_host.ps1 -OutputDirectory build-android-audit/rawfile_payload_host_rotation_repro
./tools/port/build_rawfile_payload_optimized_arm64.ps1 -Optimization O3 -BuildDirectory build-android-audit/rawfile_payload_opt_rotation_repro
& $taskPython -B tools/port/run_rawfile_payload_gate.py --host-build build-android-audit/rawfile_payload_host_rotation_repro --android-build build-android-audit/rawfile_payload_opt_rotation_repro --output build-android-audit/rawfile_payload_rotation_repro --serial AKWJ9X5116G04618
```

Use `-Optimization O0` for the corresponding debug build. Exact standalone TASK28
header build/reproduction commands remain in `arm64_temp_write.md`; that frozen
milestone's collector is distinct from the current payload gate. The new
`verify_rawfile_payload_source.py` bridges current selectors to7f7f005;
TASK28's old verifier intentionally rejects later changed-src whitelists.

## Risks and next three milestones

No full registry, engine startup or genuine graphics yet. C++ throw adapters do
not certify production Com_Error longjmp/SEH/async refill recovery. A truncated
RawFile child leaves header/name progress; fixture unwind is not atomic rollback.
Metadata retention preserves pointer values, not overwritten pointees. Broader
multi-read helpers, concurrency, OOM, encoded RawFile names, persistent RawFile
ownership, multiple-owner PMem/retained pages, S25/16KB and hash locale remain
unverified. No new latency/RSS measurement; TASK11 decoder/allocator baselines
remain separate and frozen.

1. TASK30: actual canonical RawFile pool1024 init/alloc/clone/free and genuine
   child lifetime before TEMP reuse. Require Windows/HONOR/O3 exact headers,
   source uniqueness, preserved bytes/queries, exhaustion/ownership/release
   baseline controls. Stop before full registry success.
2. Registry-entry/DBHASH initialization services with original ownership and
   correct native synchronization; validate original capacities and failure
   behavior before registration.
3. Smallest actual registration/lookup boundary for already validated assets,
   using real DB_AddXAsset behavior, preserving ordering/lifetime. Do not skip
   zombie's real TechniqueSet boundary or fabricate successful registration.

Fresh protection checks compare Windows2528/DXVK2552/root15 against the original
baselines, including HEAD/branch/index/status/content. Android source bytes and
index/HEAD, originals/APK and all existing archives remain unchanged. Machine
verification and final pending paths are in `evidence/lead_rotation_20261012/verification.json`.
