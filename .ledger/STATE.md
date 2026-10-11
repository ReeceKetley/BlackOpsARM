# Current State

Updated 2026-10-11: TASK-0010 bounded deflate isolation complete/archived.
FACT-0029/0030, DEC-0019, FAIL-0005 and WARN-0010; docs/port/arm64_inflate.md
is the current handoff/reproduction guide. Earlier zone/handoff reports remain
historical. TASK-0011 genuine decoded XFile to checked zone bootstrap is next.
Two user-authorized read-only agents audited production order and ABI/tests;
root performed all writes/builds/device tests/Ledger. No new task started.

## Architecture and completed milestones

- Android branch android-arm64; prior checkpoint 64c5d3c; Windows main76edfd5. Shared history,
  independent directory/index. The user now authorizes a local checkpoint of
  accumulated TASK-0007/0008/0009/0010 work (DEC-0020). The commit containing
  docs/port/android_checkpoint.md records it; resolve its ID with git log.
  No push or further porting is authorized by this checkpoint request.
- Twelve actual T5 units compile through the existing inventory: com_encode,
  com_math_anglevectors, timing, profile, cscr_instance, cscr_readwrite,
  cscr_parsetree, mem_userhunk, cscr_evaluate, mem_firstfit, db_memory, db_stream.
  Six unchanged bundled zlib C units also compile with their custom native
  stream ABI (52/80/104 bytes on x86/x64/ARM64); never mix engine headers with
  system libz. Actual allocation bodies match HEAD after CRLF normalization. Lightweight
  XFile/XBlock definitions are shared once; 36-byte/four-byte disk checks remain.
- Four-byte Narrow32/VM/parse slots coexist with native pointers. Android uses
  acquire/release publication and real mmap/mprotect/madvise/munmap services.
  Caller-owned buffers are never unmapped; handles cannot outlive their arenas.
- Real profiler/timing/error, Windows-reference first-party per-thread CRT RNG,
  lookup, normal parse/default HunkUser, debugger/first-fit/Z ownership and
  normal-to-debugger numeric/boolean compile+evaluation gates are complete.
  Operators have compile-only evidence; missing VM endpoints abort in fixtures.
- FastFile raw 12-byte magic/version reader and synchronous I/O are complete.
  All 158 BO1_GAME_DIR retail FastFiles are IWffu100/473. code_pre_gfx.ff was
  verified on HONOR; code_pre_gfx.ff and zombie_pentagon.ff on Windows hosts.
  Raw bytes after offset 12 are compressed. TASK-0010 now decodes complete
  code_pre_gfx.ff and zombie_pentagon.ff on HONOR and x86/x64: 49593/108009041
  decoded bytes, XFile.size 49557/108009005, externalSize 0/308074740, padding
  23/9 zeros; independent Python fields/counts/Adler32 and raw SHA256 match.
  The known retail root is ../BlackOps64/build/Release, inspected read-only.
  Genuine XFile fields are verified; actual zone bootstrap remains fixture-only.
- Actual DB_AllocXBlocks/DB_InitStreams now execute with real debug HunkUser /
  first-fit named zone ownership. Register main thread before debug startup.
  Begin -> allocate -> End -> detach streams/clear blocks -> Release; ordinary
  DB_FreeXBlocks clears metadata, Release frees the pool. Unsupported requests
  fail explicitly. This is not the legacy PMem low/high ordering or full API.
- Native DB_InitStreams now clears both pending element strides across zones;
  a retained Windows x64 failure demonstrates the old stale-mapping bug. Win32
  behavior and Windows production PMem remain unchanged. Android diagnostics
  use stderr/optional files and native unwind; Windows diagnostics retain their
  existing implementation. No source fork, success stubs or asset parsing.

## Latest verification

- Final inflate_pass3: NDK r30/Clang21/API28/Debug C99+C++20, zero compiler errors/
  linker failures, 1658 retained warnings, 75 verified AArch64 artifacts.
- Sixteen HONOR NDL-L09/API36/4096-page probes pass; both genuine files decode
  fully on device and MSVC x86/x64. Independent Python-compressed fixtures cover
  exact bytes, boundaries, checksum/end, truncated/corrupt/dictionary/wrapper/
  size/limit/padding failures, output clearing and alias rejection.
  Seven block types, zero/sparse/Z paths, allocation type 1, expansion factors,
  >4GB aligned pointers, zeroed payloads/accounting, cursor/alias/stride reset,
  correct release and eight fresh-process ownership/exhaustion failures pass.
- Both MSVC x86/x64 inflate/zone probes and debugger/first-fit, parse/HunkUser,
  header/profile/maths/RNG/lookup, FastFile, Narrow32/four-byte/483-guard
  regressions pass, all built only in this Android worktree.
- Instrumented actual database/allocator/evaluator/new codec/decoder/probe paths have no UBSan
  findings on passing paths; whole-engine/native-library coverage is not claimed.
- Existing scaffold APK hash unchanged; no new APK build/install or lint in this
  gate. Prior assemble/lint results remain frozen. All tests/process sessions
  have completed; no implementation work remains unfinished for TASK-0010.
- evidence/arm64_inflate preserves three compiler/host attempts, initial signed
  char and Windows min/max failures, final native/regression runs, genuine
  independent oracles, manifests/source/artifact hashes and local build locations.
  All earlier archives remain frozen; no scratch files deleted. Windows 2528
  files/index/status/HEAD, independent DXVK 2552 files, root Ledger 15 files,
  Android index/HEAD and all 595 earlier evidence files are verified unchanged.

## Active limits (WARN-0010)

String interning/callable registry/lists, nonliteral operator evaluation, text
parsing/full VM, full asset stream mapping/expansion, malformed/oversized XFile
arithmetic, mixed Z/physical allocation flags, legacy PMem tracking/ordering and
async/APC and genuine-header-to-zone integration are unvalidated. Isolated
unsigned wrapped decode is complete; authentication and other wrappers/dictionaries
remain unsupported explicitly. Configurable zero padding0..31 is verified on
two assets only; decoded byte limit is not exact vector/heap budget. Existing
bundled codec is reused, not upgraded or security-certified. Zero-size blocks require caller
zero-initialized slots. Test-only compressed byte accounting/PMem over-allocation
callbacks abort; dependent stream tracing must stay disabled. Fifty unavailable
compiler/VM endpoints and cleanup reference callbacks remain probe fixtures.
Windows production heap/PMem integration is not certified by native-owner hosts.
Hunks/debugger/named-zone mutation remain owner-thread confined; first-fit reset
and destruction require quiescence. Tracking/fixed schemes remain rejected.
Vendor/full replay, profiler consumer synchronization, whole com_math.cpp/RNG
caller compilation, actual 16KB Android/S25, graphics/audio/input/gameplay remain
untested. Root Ledger owns shared Windows history; DXVK stays separate.

## Next task and checkpoint

TASK-0011: validate uint64 native expansion/per-block/backend/aggregate budgets
before feeding genuine decoded XFile to actual DB_AllocXBlocks/DB_InitStreams.
Acceptance: malformed/oversized or unsupported flags/types rejected before
allocation; genuine code_pre_gfx bootstrap on HONOR/x86/x64 with disk sizes/zero
slots intact; repeated reset/detach/free/release restores owners/reservations;
sixteen prior probes green; protected state and failed evidence preserved.
No asset deserialization, async pipeline, full engine/backend or TASK-0011 work
started. Continue with user direction; no push or later commit authorization.
The user authorized this local checkpoint only. Historical gate file inventories:
docs/port/arm64_inflate_changes.md and
evidence/arm64_inflate/preservation.json. Ignored builds/binaries/SQLite/assets
and scratch remain local; no original game bytes enter Git or the APK.
Current checkpoint inventory: docs/port/evidence/android_checkpoint/pre_commit_inventory.json.
