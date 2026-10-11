# TASK-0014 — Genuine script-string pointer tables

Completed 2026-10-12 local time. Actual production `DB_AllocStreamPos(3)` and
`DB_ExpandLoad_Ptr32Array` pass on HONOR ARM64 and Windows x64; Windows x86 uses
its original raw-table read/commit branch. No string bytes, string interning,
asset directory/payload, full async/refill loader or graphics execute in this gate.
The active autonomous objective continues through successive verified milestones;
this is one completed boundary, not completion of the whole overnight goal.

DEC-0029, FACT-0040/0041, FAIL-0008/0009 and WARN-0014 record this gate. Prior
verified gates are checkpointed at `ffc4241`; local verified checkpoints remain
authorized by the user's explicit DEC-0026 answer, no push. Historical reports
retain their original pre-commit state/hashes. Current state is in `.ledger/STATE.md`.

## Results

| Genuine file | Table entries | Disk/native64 bytes | Decoded source end |
|---|---:|---:|---:|
| code_pre_gfx.ff | absent | 0 / 0 | 52 |
| zombie_pentagon.ff | 860 | 3440 / 6880 | 3492 |

Originals from `D:/Games/Call of Duty - Black Ops/zone/common` match the prior
read-only Windows mirror hashes. Independent streaming Python verifies complete
raw/decoded SHA256, Adler32, length/padding and exact prefix/table range; each ABI
independently fully decodes with the unchanged matching bundled codec in a
separate process, extracts52/3492 bytes and releases its one decoded vector.
Native/host/device/executable/prefix hashes match. Allocation processes retain
only the small prefixes, avoiding a duplicate108MB decompressed buffer.

Zombie slot0 is null, all859 other tags FFFFFFFF. Every native field matches
independent widening rules; every alias targets its actual owned table slot;
each disk element start maps to its native element. Expanded interiors1..3 and
the committed endpoint have no exact mapping. Pop restores block0 and retains
the updated block4 position. Detach clears every alias before named release.
Twelve alternating small/large low/high cycles per ABI restore logical ownership.

Twenty-six controlled fixtures per ABI cover checked negative/extreme counts,
source/disk/native capacities, uint64 alignment overflow, exact/one-over bounds,
absent arrays including positive count/null root, present-zero without a block,
counts0/1/2/6, aligned/+1 positions and all six pointer-tag classes. Full seeded
cursor/shadow/alias/delay/stack snapshots prove no preflight mutation. A controlled
pre-copy read throw checks destination bytes/aliases/owner counters before reset;
then persistent detach/descriptor clear/named release restores baseline. It does
not certify post-read vector allocation exception rollback.

Final NDK30/Clang21/API28 Debug C99/C++20 build:0 compiler/linker errors,2859
retained warnings,114 verified AArch64 artifacts and20 test executables. The
existing12 full selected T5 units plus actual bounded expander subset and6
unchanged custom-ABI codec C sources compile. Passing instrumented paths have
no UBSan findings; full Android engine/expander coverage is not claimed.
Previous16 HONOR probes and independently run Windows x86/x64 zone/four-byte
Narrow32/production declaration/483 guard regressions pass. Full normal Windows
x64 expander (gate disabled), normal registry x86/x64 and light legacy-off layout
checks pass. No table/asset function body changed; only bounded compile selection
includes existing table bodies. New table executable excludes string and asset
directory functions after section GC.

## Checked policy and fixture boundary

`PlanScriptTable` accepts actual checked zone capacities/cursors, keeps file
consumption separate from memory alignment, checks widths/source/remaining block
space before reservation, and uses disk mask3/native pointer alignment. Present
zero still aligns and requires a real nonnull block; absent root skips all work.
The native helper's existing `1<<20` clamp domain is rejected explicitly before
calling it. Its4096 branch is diagnostic, not an imposed limit. Windows x86 retains
its normal count/read behavior within checked commit/capacity bounds.

The finite reader is an explicit substantive test adapter: bounds-checked memcpy
and exception on unavailable input. This verifies the actual expander with genuine
validated bytes, not Win32 async/refill/APC behavior. New fixture diagnostic
logging writes actual formatted output to stderr; old measurement fixtures keep
their recorded logging configuration. Disabled legacy tracking hooks remain
disabled production behavior, not a tracking implementation claim.
Actual Windows PMem/global pool and native named Hunk remain different ownership
contracts. One owner at a time avoids unvalidated legacy multi-owner stride;
fixture-only cumulative freeTot reset preserves ownership. Prior timing/RSS
baseline remains frozen; no new throughput/RSS or gameplay performance claim.

## Evidence and failures

[Final results](evidence/arm64_string_table/string_table_run2.results.json),
[manifest](evidence/arm64_string_table/manifest.json),
[boundary symbols](evidence/arm64_string_table/string_table_symbols.txt),
[independent reviews](evidence/arm64_string_table/review-evidence.md),
[preservation](evidence/arm64_string_table/preservation.json) and
[inventory](evidence/arm64_string_table/file_inventory.json) are frozen.
Initial host compile hit Windows RPC's `small` macro expanding the test variable;
renaming frame variables fixed it without engine changes. First orchestration run
rejected a successful x86 probe because it expected CamelCase enum labels rather
than the actual lower_snake status names. Exact contract corrected, not relaxed.
All original compiler errors, failure.json, commands/stdout/stderr and successful
runs are retained. No genuine table probe failed or tablet memory was exhausted.

## Reproduction

Run in `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, use fresh names each time:

```powershell
$probePython = 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH = (Split-Path $probePython) + ';C:/Program Files (x86)/Microsoft Visual Studio/Installer;' + $env:PATH
.\tools\port\build_arm64_audit.ps1 -BuildDirectory build-android-audit/string-table-repro-arm
.\tools\port\build_string_table_host.ps1 -OutputDirectory build-android-audit/string-table-repro-host
& $probePython .\tools\port\run_string_table_gate.py --android-build build-android-audit/string-table-repro-arm --host-build build-android-audit/string-table-repro-host --output build-android-audit/string-table-repro-run --serial AKWJ9X5116G04618
.\tools\port\run_arm64_audit_tests.ps1 -Serial AKWJ9X5116G04618 -BuildDirectory build-android-audit/string-table-repro-arm
.\tools\port\check_assetlist_headers.ps1 -OutputDirectory build-android-audit/string-table-repro-headers
.\tools\port\check_zone.ps1
.\tools\port\check_narrow32.ps1 -PythonExe $probePython
```

Latest native/host output: `string_table_pass2`/`string_table_host_pass2`, run2.
The exact retained device path and argv are in results/invocations JSON; all
device inputs are SHA-checked. No APK install or persistent device-setting change.
Use the already verified device originals for later gates to avoid redundant
large asset transfers; retain past directories as evidence.

## Next three milestones

1. TASK-0015 actual `Load_XStringCustom` with bounded terminator/source/offset/
   capacity checks. Zombie859 inline strings,15393 bytes including terminators;
   slot12 empty is distinct from slot0 null. Native alignment adds3366 bytes,
   memory cursor25639 versus disk shadow18833; file cursor18885 has no padding.
   Raw pointers are not SL IDs. Reject -2/invalid encoded references explicitly.
2. Actual asset-directory expansion at the verified next file cursor:37/1045
   records, true types/tags, alias/stride/ownership checks before dispatch.
3. Real SL/MT string-pool init/intern/user4 release with recursive critical
   sections and fixed32 ARM64 atomics, then integrate genuine strings. No VM
   diagnostic no-ops or invented IDs. Native engine/app execution and game-owned
   graphics follow dependencies, not synthetic frames or placeholder menus.

WARN-0014 retains full loader/post-read bad_alloc/Init/fatal OOM, legacy multi-owner
PMem, broader VM/Z/backend and16KB/S25 limits. No engine-owned graphics achieved.
