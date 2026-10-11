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

### TASK-0011 — Genuine decoded header to checked zone bootstrap (next)

Preflight native expansion/backend/per-block/aggregate budgets in uint64, then
feed genuine XFile to actual DB_AllocXBlocks/DB_InitStreams. Reject oversized or
unsupported requests before allocation; repeat/reset/detach/release with zero
owners/reservations on HONOR and both host ABIs; all sixteen prior probes pass.
Stop before asset-list/strings/payload loading or async integration.

## Beyond

GSC string-intern/callable services, asset-stream expansion on ARM64 (the
Win32→native offset remap is the known x64 blocker, see fastfile_32_to_64.md),
server init, headless `platform: ready`. Each gets its own bounded task when
its predecessor lands.
