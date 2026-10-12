# TASK-0019 — genuine StringTable name and cell records

Actual `Load_XStringCustom` and `DB_ExpandLoad_StringTableCellArray` now run on
HONOR ARM64 and Windows x64; Win32 uses actual `Load_Stream` for packed cells.
Both genuine first StringTables pass 98 adversarial cases and 12 allocation/
release cycles per ABI. All names, cell tags and signed hashes match independent
original-file references. No passing-path UBSan error occurred. All tests ended.

| Original file | Name bytes incl. NUL | Cells | Cell source start/end | Block4 disk end | Block4 native64 end |
|---|---:|---:|---:|---:|---:|
| code_pre_gfx.ff |34|400|402 /3602|3532|7032|
| zombie_pentagon.ff |58|1594|27323 /40075|40008|67928|

Names are ordinary owned character pointers, separately from TASK-0017 script
string IDs. Name source begins368/27265. Win32 string alignment is1; native64
minimum alignment is8. After names, cells align to disk4/native8 without consuming
decoded bytes. Cell starts are disk332/27256 and native632/42424. Packed cells
are8 bytes, native64 cells16: pointer0, signed hash8, zero padding12..15. Native
cell aliases map packed starts to owned string slots. Record starts map exactly;
interior1..7, including the hash field, do not become exact pointer mappings.

There are2 and49 genuine ordinary cell-string offsets. Every tag retains its
original bits, including sign-extension of FFFFFFFF/FFFFFFFE in native64. This
gate leaves all cell-string tags unresolved and stops at3602/40075, before their
inline strings or the index arrays. The future oracle knows more of the original
file; that metadata is explicitly labeled reference evidence beyond execution.

The test follows the actual caller's scope order: parent block4, header block0,
child block4, header block0, parent block4, root block0. The header stays live
through name/cell pointer publication. Its dimensions, unresolved index tag and
three field aliases remain intact. Afterwards TEMP pop rewinds native block0
workspace to0 while native64 shadow stays20. Header aliases remain temporary;
no registered asset or general duplicate-alias ordering guarantee is claimed.

The complete bounded name/cell frame is planned before owner allocation, using
original validated XFile capacities and unchanged production expansion policy.
New cell planning checks signed dimensions before uint64 multiplication, signed
read/commit widths, the inherited native helper domain, source and aligned disk/
native capacities. Any nonzero values tag means presence in the actual loader;
zero-count presence still aligns, while absent positive counts consume nothing.
No tag is dereferenced here or fabricated from payload length.

Runtime fixtures cover mixed pointer tags, signed INT_MIN/INT_MAX hashes, zero,
single/six records, aligned/offset1 starts and short48-byte read from47 bytes.
Failure compares bytes, stream/ownership state and six aliases/exact mappings
before reset. Earlier raw-string fixtures also cover missing NUL and bounded
character-copy failures. After each genuine cycle, every cell/header/directory/
script mapping is detached before actual zone release, restoring logical counts.
Earlier block4 prefix bytes remain intact. Post-read allocation exceptions and
transactional rollback of already loaded children remain unvalidated.

## Verification

Authoritative `stringtable_cells_run2` uses current-source hostpass2 and ARMpass1
plus `cells-final-build.log`; run1 also passed before extra assertions that the
index tag, dimensions and header aliases survive child publication. Full fresh
ARM build has0 errors,5603 retained warnings,207 AArch64 artifacts/27 executables.
Final rebuild updates the cell target. All163 actual expander bodies, two
declaration headers and six stream-loader bodies remain byte-identical to
4c6eb35. Normal full Windows expander compiles x86/x64 with gates off. Existing
four-byte Narrow32 and real pool evidence remains intact; their code is unchanged.

Evidence under `docs/port/evidence/arm64_stringtable_cells` freezes both passing
runs, exact invocations/stdout/stderr, every cell's numeric tag/hash, name digests,
source proofs, architecture/symbol/source hashes, independent oracle and review.
The native binary contains the actual cell/name/header leaves and omits pointer
conversion through section GC; Win32's actual Load_Stream symbol is verified.
No original string text/hex, decoded buffers, original FastFiles or executables
enter Git. No unexpected compiler/runtime test failed in TASK-0019. An ignored
edit script's EOL assertion stopped before writes and was corrected; its scratch
and prior failed gates remain preserved.

Full SHA/Adler provenance is independently checked in bounded host streaming
passes. Actual complete native decode processes release their single vector
before zone tests, retaining only3602/40075-byte prefixes. Previously verified
HONOR originals are reused and SHA-checked. Windows2528 files, independent DXVK
2552, root Ledger15 and2013 older frozen paths remain unchanged. No APK installed.
No new timing, RSS, decoder/allocator peak or gameplay performance claim is made;
TASK-0011 measurements remain separately frozen. Logical release does not imply
immediate recovery of Windows retained pages or resident memory.

## Reproduce

From `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, choose fresh directories:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/stringtable_cells_repro1
./tools/port/build_stringtable_cells_host.ps1 -OutputDirectory build-android-audit/stringtable_cells_host_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_cells_gate.py --android-build build-android-audit/stringtable_cells_repro1 --host-build build-android-audit/stringtable_cells_host_repro1 --output build-android-audit/stringtable_cells_repro_run1 --serial AKWJ9X5116G04618
./tools/port/build_expander_windows_compile.ps1 -OutputDirectory build-android-audit/stringtable_cells_windows_repro1
```

Runner uses original game root `D:/Games/Call of Duty - Black Ops`, cached device
raw provenance from `asset_directory_run1/results.json`, fresh unique device
scratch and exact SHA/record/cycle parity. It validates TASK19 execution endpoints
separately from the full-first-asset oracle. No duplicate raw transfer or cleanup
of old outputs is needed. All scratch and tests remain reproducible.

DEC-0034, FACT-0057/0058, WARN-0019. Local verified checkpoints approved DEC-0026;
never push. Exact paths: `arm64_stringtable_cells_changes.md`. Finite readers do
not certify full async/refill/APC; no full dispatcher, VM, registry or graphics.

Next3: TASK-0020 genuine cell strings plus earlier-initialized-start provenance
and actual checked offset conversion; signed16 index data with permutation/hash/
column-order validation; actual StringTable hash/lookup leaves before registration.
The production hash's signed overflow and negative-char ctype need isolated,
Windows-compatible validation before UBSan lookup. Do not run CellCompare's
overflow-prone subtraction or disable sanitizers to claim a successful lookup.
Keep these changes bounded, original formats and Windows behavior intact.
