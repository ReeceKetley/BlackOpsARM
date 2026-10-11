# Native weapon and flame config fields

Batch41, following pushed checkpoint `17003a7`.

## Changes

- All 748 weapon config fields and 119 flame config fields use named native
  `offsetof` expressions. Retail x86 offsets remain as compile-time checks on
  Win32, rather than addresses used on x64.
- The loose flame loader allocates `sizeof(flameTable)` instead of 476 bytes.
- Developer weapon updates copy the backing model, animation, tag, notetrack,
  bounce and damage multiplier arrays into the temporary full definition before
  parsing. Successful edits copy these arrays back and restore the original
  storage pointers. Temporary stack pointers cannot escape through the headers.
- The route launcher retains its process handle, records the exit code, and
  bounds failed runs while checking the exact game path before termination.
- The warning summary reads both UTF-8 and PowerShell UTF-16 logs.

## Focused verification

Run from the repository:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/port/check_weapon_fields.ps1
```

The script extracts the current production tables, initializers,
`KeyValueToField`, and developer snapshot/commit helpers into a standalone test.
It uses the repository headers and MSVC for Win32 and native x64.

Both executables pass:

- 867 config mappings, including compile-time retail offset checks on Win32.
- 838 generic field writes, with bytes outside each field preserved.
- Weapon/flame initialization beside guard pages and prefix canaries.
- x64 objects and opaque asset pointers at `0x180000000`, above 2 GB.
- Model, animation, damage multiplier, tag and notetrack edits survive commit;
  adjacent elements and unrelated scalars remain intact. Original storage
  pointers are restored, and optional null arrays remain null.

Negative controls generated only under `build/run` replace each table separately
with its old numeric offsets. The old weapon table fails the definition pointer
check; the old flame table fails the tail string check. Production source is
unchanged by these controls.

## Limits

Asset registration is stubbed in the focused test. The 29 special weapon fields
use the enum/script-string parser and are not exercised by the generic write
test. This does not verify a complete loose-file content pipeline, actual
developer commands, missing backing storage for array edits, or loading models
and effects from disk. Normal fastfile gameplay routes serve as regression
checks, separately from the focused config memory checks.

The earlier all-map report remains the batch40 evidence; no new all-map weapon
sweep is implied by this change. LOW_VA remains ON by default. Client glass,
script debugger, remaining pointer/stride audit, expanded box navigation and
final port gates remain outstanding. Dead Ops gameplay is out of scope.

## Fresh gameplay regressions

| Build | Run | Passed steps | Failed steps | Exit | Save readbacks |
| --- | --- | ---: | ---: | ---: | --- |
| w32 | `w32_fields41` | 24 | 0 | 0 | All four passed |
| x64hi | `x64hi_fields41` | 32 | 0 | 0 | All four passed |
| x64low | `x64low_fields41` | 24 | 0 | 0 | All four passed |
| x64guard | `x64guard_fields41` | 24 | 0 | 0 | All four passed |

All routes completed the original Pentagon plan through Pack-a-Punch. The high-address route includes additional automatic thief recovery steps. These are headless fastfile regression routes with god mode, not a manual sound/visual soak or expanded mystery-box gate. Saved evidence: `build/run/native_fields41_results.json`.

A full unrestricted x64 rebuild (`tools/port/native_full41.log`) completed in194 seconds with0errors and184 unique pointer-truncation warning sites:182 in src/tl and2 in DemonWare. Offset/size fixes do not necessarily reduce these warnings. Next audit target: common/brush_edges.cpp has packed x86 queue storage and pointer-array copies with four-byte strides.
