# TASK-0018 — genuine first StringTable header

The actual production `DB_ExpandLoad_StringTable` executes on HONOR ARM64 and
Windows x64; Windows x86 uses actual `Load_Stream`. Each platform passes73
adversarial cases and12 genuine allocation/release cycles across both production
allocation types. Assertions and ARM UBSan remain enabled. All tests completed.

The original first asset in both validated directories is inline StringTable
type37. Original XFile allocation metadata and production expansion factors
supply the zone; payload lengths never fabricate allocation descriptors.

| Original FastFile | Header source | Dimensions | Header disk/native64 bytes | Small prefix |
|---|---:|---:|---:|---:|
| code_pre_gfx.ff |348|1×400|20 /32|368|
| zombie_pentagon.ff |27245|2×797|20 /32|27265|

Each name/values/index field retains its genuine FFFFFFFF tag. Native fields
are at0/16/24 versus disk0/12/16. Counts remain signed32. Exact offset mapping
exists at header start; expanded object interiors do not resolve exactly.
Disk alignment4 and native alignment8 consume no decoded file bytes.

Production TEMP block0 is deliberately rewound by `DB_PopStreamPos`, returning
the native cursor to its entry position0. Before pop, native cursor is32 versus
Win32's20. Native64 disk shadow retains20 and mappings after pop; Win32 has no
independent shadow. This gate certifies temporary owned header workspace, not
a registered persistent asset. No subsequent allocation reuses it in a test.

The actual side-buffer envelope stays alive until detach. Its alias12 is
replaced by the header values slot; its alias4 still points into that envelope.
This exact small map history passes. General duplicate-key ordering is not
certified: the production alias merge uses nonstable `std::sort`. No rewrite
of that behavior or broader stream lifecycle was made.

Preflight rejects unsupported asset/root tags, missing/truncated headers,
negative dimensions, unsafe products/signed commit sizes, actual native helper
domain violations and source/header block capacity/alignment overflow. Child
tag bits are preserved without dereference, including controlled mixed tags.
The future cell/index disk footprint is only a source-budget prerequisite;
it does not certify future block4 allocations or name/cell/index payloads.

Runtime fixtures verify aligned and offset1 headers. An injected19-byte reader
fails before copying: source, bytes, ownership, stream cursors, block0 aliases
0/4/12/16 and exact mapping are compared **before reset**, with a live old
envelope seeded. All mappings detach before actual zone owner release; logical
counts return to baseline. Earlier block4 bytes, cursors, strings and directory
tags remain intact. Actual interning is separately certified by TASK-0017;
this header fixture uses the bounded raw-string predecessor.

## Verification and evidence

Authoritative `stringtable_header_run1` uses current-source Windows hostpass3
and ARMpass2 plus `header-final-build.log`. Full ARM build:0errors,5183 retained
warnings,190 verified AArch64 artifacts/26 executables. Final header rebuild
adds the reviewed failure snapshots. Native passing paths have no UBSan errors.
All163 production expander bodies, two declaration headers and six stream-loader
bodies are byte-identical to c51db37; only standalone compile selection changed.
Normal full Windows expander TU compiles x86/x64 with gates off. Existing Narrow32
and earlier pool/regression evidence remains valid; their sources did not change.

FAIL-0015 preserves the initial fixture-only mixed `auto` type compile error on
Windows/ARM. Split declarations fix it. Hostpass2 is an intermediate build;
hostpass3 includes the stronger reviewed snapshots. Failed compiler diagnostics,
complete invocation/stdout/stderr logs, numeric oracle records, SHA manifests,
source proofs, symbol evidence and architecture inventory are frozen in
`docs/port/evidence/arm64_stringtable_header`. No asset buffers or binaries enter
Git. Evidence-freezing tooling initially used the wrong baseline key and then
required a native `Load_Stream` symbol removed by section GC; corrected tooling
verifies the Win32 symbol where that function actually runs. Existing copied
files were byte-compared on resume, never deleted.

Independent host reference uses three bounded full-stream passes, verifies raw
and complete decoded SHA/Adler and retains only the small prefix. Actual native
complete decoders run in separate processes and release their single decoded
vector before allocation tests. The reusable original HONOR files are SHA-
verified before testing. Windows2528 files, independent DXVK2552, root Ledger15
and1937 older frozen reports/evidence/handoffs are unchanged. No APK installed.
This gate adds no allocation latency, RSS or performance claim; TASK-0011's
decoder/allocation measurements remain separate and unchanged.

## Reproduce

From `D:/Desktop/Black Ops 2 Android/BlackOpsARM`, use fresh output directories:

```powershell
$env:PATH='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python;'+$env:PATH
./tools/port/build_arm64_audit.ps1 -BuildDirectory build-android-audit/stringtable_header_repro1
./tools/port/build_stringtable_header_host.ps1 -OutputDirectory build-android-audit/stringtable_header_host_repro1
& 'C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/port/run_stringtable_header_gate.py --android-build build-android-audit/stringtable_header_repro1 --host-build build-android-audit/stringtable_header_host_repro1 --output build-android-audit/stringtable_header_repro_run1 --serial AKWJ9X5116G04618
./tools/port/build_expander_windows_compile.ps1 -OutputDirectory build-android-audit/stringtable_header_windows_repro1
```

Runner uses read-only originals `D:/Games/Call of Duty - Black Ops/zone/common`
and cached device provenance from `asset_directory_run1/results.json`. Device
raw files stay in the SHA-verified TASK14 directory; only the executable is
pushed into a new unique directory. It rejects unknown/missing records and
compares Windows x64/HONOR logical contracts exactly. All scratch is retained.

DEC-0033, FACT-0054/0055, WARN-0018. Local verified checkpoints approved DEC-0026;
no push. Exact pending/durable paths are in `arm64_stringtable_header_changes.md`.
Finite fixture callbacks are not the asynchronous/refill/APC reader. Post-read
alias/helper allocation exceptions and fatal initializer recovery remain open.

Next3: TASK-0019 actual StringTable name34/58 bytes and cell arrays400/1594;
real bounded cell strings with2/49 initialized-range offset conversions;
signed16 index arrays plus actual table lookup before asset registration. The
preparatory independent payload oracle records metadata/digests only; it does
not execute those leaves. No rendering, gameplay or persistent asset loading
is claimed. Continue bounded native-engine work under DEC-0029.
