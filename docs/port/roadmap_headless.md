# Roadmap to a native Android ARM64 headless T5 engine

Living plan. Each milestone is bounded, keeps Windows compatible, preserves
32-bit disk layouts, freezes its own evidence, and never widens into
gameplay/backend work without authorization.

## Where we are

Twelve actual T5 units compile to AArch64. TASK-0007 normal-to-debugger
expression compilation, TASK-0008 raw FastFile reader and TASK-0009 actual
zone allocation/stream bootstrap are complete. Fifteen HONOR probes and all
host x86/x64/Narrow32 regressions pass. TASK-0010 now adds six unchanged bundled
codec C units and the sixteenth HONOR probe; complete code_pre_gfx and
zombie_pentagon decode on all three targets. See arm64_inflate.md for current
evidence/limits/reproduction. Actual zone bootstrap remains fixture-only; no game runs.

## Next three bounded milestones

### TASK-0008 — FastFile preamble reader (complete 2026-10-11)

Shared parser + synchronous file-I/O boundary land in
`src/platform/fastfile_reader.h`: 12-byte raw head (production-accurate,
verified pre-inflate) plus the logical 48-byte stream head; 14/14 HONOR
probes and all host checks pass with no engine changes
(docs/port/arm64_fastfile.md, FACT-0022). The device runner's stderr-stop
abort is fixed in the same gate (FACT-0023). Retail files at `BO1_GAME_DIR`
verified genuine: all 158 zone FastFiles are `IWffu100` / 473, and
`code_pre_gfx.ff` reads back identically on HONOR. Decompressed `XFile`
content still needs the inflate stage. No zone loading, no async/APC
pipeline.

### TASK-0009 — Zone block allocation and stream init (complete 2026-10-11)

Actual complete DB source paths pass on HONOR and Windows x86/x64 using
real native named HunkUser/first-fit ownership. Seven blocks, fixed disk sizes,
cursor/alias/stride reset and release pass. Native pending-stride reset fixes
a retained x64 control failure. This is a logical XFile fixture, not an inflated
retail zone or the legacy PMem pool/tracker. See arm64_zone.md; no asset parsing.

### TASK-0010 — Deflate stage isolation (complete 2026-10-11)

Complete synchronous bounded wrapped decoding and genuine XFile validation
pass on HONOR/MSVC x86/x64, with checksum/end/exact output, custom bundled codec,
explicit errors/padding and independent fixtures/oracles. No assets parsed;
engine async pipeline remains deferred. See arm64_inflate.md.

### TASK-0011 — Checked genuine FastFile allocation (complete 2026-10-11)

Genuine code_pre_gfx/zombie_pentagon decoded headers drive checked unchanged
production allocation policies on HONOR and Windows x86/x64. uint64 preflight,
actual native capacities, controlled rollback, warm/cold latency and distinct
decoder/allocator memory peaks are frozen. Native64 plans match; Windows retained
pool and Android touched mappings are different contracts. No new stream init,
asset parsing or fixups. See arm64_checked_zone.md, FACT-0032/0033, WARN-0011.

### TASK-0012 — Genuine checked blocks to DB_InitStreams (complete 2026-10-11)

Actual stream initialization on both genuine files/HONOR/Windows x86/x64 passes
seven cursors, zero slots, independent pending size/stride/alias/drift reset and
repeated detach/free/named release.12 cycles and61 Init calls per ABI; invalid
plans/partial failures never publish streams. No shared engine edits or payload
reads. See arm64_stream_init.md, FACT-0035/0036, WARN-0012 and exact inventory.

### TASK-0013 — Genuine16-byte asset-list envelope (complete 2026-10-11)

Actual native expander/Win32 branch, checked count/footprint metadata, persistent
aliases and exact16-byte side-buffer read pass both files/3 ABIs.72 fixtures and
12 repeated ownership cycles each; cursors unchanged, reset/release baseline.
Independent full decoded SHA/Adler/prefix provenance; actual body/declaration
bytes unchanged. Finite reader does not certify async/refill loader. Full normal
Windows expander passes; bounded Android subset only. See arm64_assetlist.md,
FACT-0037/0038, FAIL-0007, WARN-0013 and exact inventory.

### TASK-0014 — Actual script-string pointer table (complete)

Both genuine files/HONOR/x86/x64,26 fixtures/12cycles, exact3440 disk/6880 native
zombie860slot tags, allaliases/elements/cursors and cleanup verified. No strings.
See arm64_string_table.md/FACT-0040/0041/WARN-0014; failures retained.

### Next3 under active autonomous objective

- TASK-0015 actual Load_XStringCustom raw bytes, bounded terminators/offsets/
  capacity,859 genuine inline strings, empty versus null, exact file/memory cursors.
- Actual37/1045 asset directory at proven next file offset, before dispatch.
- Real SL/MT intern/user4release with recursive locks/fixed32 ARM64 atomics,
  then integration. No fabricated IDs/VM callbacks. Game-owned graphics follow
  real engine/app/backend dependencies; no synthetic-frame success claims.

## Beyond

GSC string-intern/callable services, asset-stream expansion on ARM64 (the
Win32→native offset remap is the known x64 blocker, see fastfile_32_to_64.md),
server init, headless `platform: ready`. Each gets its own bounded task when
its predecessor lands.


TASK-0015 completed: see docs/port/arm64_raw_strings.md, FACT-0043/WARN-0015.
Actual bounded raw strings pass HONOR/x86/x64; raw pointers are not interned IDs.
Next TASK-0016 actual asset directory before dispatch, TASK17 real SL/MT pool,
then first genuine StringTable header before payload/fixups. Continue bounded
autonomously per DEC-0029; local verified checkpoints DEC-0026, no push.


CurrentTASK16structuralcorecomplete: arm64_asset_directory.md, FACT46/WARN16.
RealpoolprerequisitesverifiedFACT48/FAIL11(correctedliveworker), actualSLMTnext.
Next3 TASK17intern/user4release, TASK18firstStringTableheader, boundedcells/index
beforefixups/registry. ContinueautonomouslyDEC29, localverifiedcommitsDEC26/no push.


TASK-0017 complete: actual pool and genuine859 interned IDs match HONOR/Windows,
including optimized ARM assertions/UBSan. Read arm64_string_pool.md, FACT-0051/
0052 and WARN-0017; failures preserved. Only serial pool lifecycle certified.
Next TASK-0018 first genuine StringTable header; TASK19 name/cell array, then
real encoded cell-string fixups before registry. Block0 Pop rewind/transient
alias lifetime must be preserved. Continue autonomously DEC-0029, local verified
checkpoints DEC-0026; no push. Protected checkouts/assets/evidence unchanged.


TASK-0018 is complete: actual genuine first StringTable headers on HONOR and
Windows x86/x64, 73 adversarial cases and12 cycles each. Read
docs/port/arm64_stringtable_header.md, FACT-0054/0055 and WARN-0018. TEMP block0
Pop rewinds native workspace while retaining shadow/maps; aliases are transient,
not registered asset ownership or a general duplicate-key ordering guarantee.
Next TASK-0019 actual name34/58 bytes and cell arrays400/1594, then initialized
cell-string range checks for2/49 genuine encoded offsets, then signed16 indices
and actual lookup before registration. Reference-only payload oracle does not
certify execution. Continue autonomously DEC-0029, verified local checkpoints
DEC-0026; no push. Preserve prior evidence, original assets and protected repos.


TASK-0019 is complete: actual genuine StringTable raw names and unresolved cell
records on HONOR/Windows x86/x64,98 cases/12 cycles each. Read
docs/port/arm64_stringtable_cells.md, FACT-0057/0058 and WARN-0019. Parent4/header0/
child4 scope keeps TEMP header live; per-cell strings/index/fixups remain unread.
Next TASK-0020 real cell strings and checked2/49 offsets to earlier initialized
character starts/NUL/ranges, then actual signed16 indices and hash/lookup before
registration. Original tag, exact mapping and native owned address must agree;
never infer initialized strings from zero pages or capacity/drift fallback.
Com_HashString overflow/ctype and CellCompare subtraction require bounded review
before UBSan lookup. Continue DEC-0029, verified local commits DEC-0026; no push.
Preserve protected repos/assets/frozen evidence. No game-owned graphics yet.
