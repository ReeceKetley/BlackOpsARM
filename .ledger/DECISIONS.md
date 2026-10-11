# Decisions

Record stable, high-level decisions here. Use Ledger entries for searchable details and evidence.

## Android setup direction â€” 2026-10-11

- Branch `android-arm64` starts at committed Windows checkpoint
  `76edfd5e75976c41bfab5a5db262b8b593432518` (including native water/material fixes).
  The worktree has independent files and index, and a shared Git object store.
- Preserve the existing initialized `.ledger`; use the Ledger CLI for events.
  Parent `../.ledger` stays authoritative for shared T5/Windows evidence and is
  not modified. Local entries own only Android scope; cite parent IDs by path.
- Native Android ARM64, original T5 gameplay/assets/FastFiles, Windows-compatible
  shared changes, native-width runtime pointers, preserved 32-bit disk formats.
- DXVK Native 1.9.2 using the tested SDL2/Vulkan integration is the initial
  renderer target. DXVK-Native remains a separately maintained repository.
- Targets: HONOR Pad X8a and Samsung Galaxy S25; Xbox controller-first input.
  Validate device GPU/features directly rather than assuming them from old notes.
- Use small Win32 OS service shims when appropriate, native Android graphics,
  audio, lifecycle and input. Unsupported shims must fail explicitly.
- Keep the root Windows CMake and engine source lists unchanged for setup.
  Build a separate minimal ARM64 environment probe; reuse the existing engine
  source lists through a later isolated platform-aware CMake change, not a copy.

## Phase 1 compilation and native-width audit â€” 2026-10-11

- DEC-0005 supersedes the infrastructure-only scope: bounded shared-source
  compilation, dependency/ABI audit, minimal compatibility and connected-HONOR
  probes are authorized. Full engine and backend integration remain deferred.
- DEC-0007 separates native-width layouts from Windows bring-up flags. Use
  BO1_NATIVE_64BIT for reviewed shared paths on Android, retaining Windows
  comparison behavior and genuine platform/diagnostic gates. Keep fixed disk,
  VM and count assertions active; never blanket-replace BO1_PORT_X64.
- Four-byte region handles are mandatory on Android ARM64. Implement Android
  publication with C++ acquire/release and mutex registration; retain the
  existing Windows implementation. Handles are process-local, not persistent
  serialized pointers. Exhaustion aborts explicitly.
- Demonstrate MSVC spelling compatibility only on the bounded subset. Use
  scoped fixed-width Win32 timing/error types, native semantics and explicit
  unsupported failures, without impersonating Windows or conflicting with DXVK.
- Preserve the APK environment target and Windows defaults. The inventory-only
  CMake mode reuses existing lists; test code is generated only in ignored
  build directories from current production declarations, not a second engine.
- Record exact compiler/ELF/device evidence and preservation hashes. Runtime
  probe success does not certify uncompiled headers, VM, assets or gameplay.

- DEC-0008 authorizes the next bounded actual-header gate after Phase 1. Keep
  previous evidence frozen; compile actual headers and one VM utility, with
  active profiling and Windows maths comparisons.
- DEC-0009 routes profiling through real native services. Android monotonic
  nanoseconds/gettid/atomics and release/acquire list publication are distinct
  from the preserved Windows TSC/thread/intrinsic path. Report consumers must
  adopt the native timebase and publication API before integration.
- BO1_Random avoids Android POSIX random() collisions; only the symbol/callers
  change. Windows keeps the old entry point. The rand()/32768 calculation is
  preserved; WARN-0004 records the unresolved range/seeded-stream contract.

- DEC-0010/0011: the authorized continuation implements Windows-reference CRT
  RNG for first-party T5 calls and selects unchanged cscr_readwrite.cpp. Android
  uses per-thread uint32 wrapping state/default seed 1/15-bit draws; Windows
  forwards to its actual CRT. Explicit service calls preserve scaling/call order.
- Do not override libc symbols or rewrite vendor RNG. Vendor libtommath/Speex
  behavior and full replay interleaving require separate runtime evidence.
- Keep earlier gate evidence immutable. Current results supersede the old RNG
  range blocker without claiming VM/game execution or allocator certification.

- DEC-0012/0013: reuse actual parse-tree/default HunkUser source; remove unsafe
  dispatch function casts using typed adapters. Use lightweight unchanged
  declarations and Android-only header narrowing, retaining Windows dependencies.
- Android reserve/commit/decommit/free uses real mmap/mprotect/madvise/munmap,
  native page size, bounds/type/ownership checks and mutex-protected metadata.
  Windows page/allocation policy is retained; corrected exact-end rounding is
  Android-only. Default HunkUser remains owner-thread confined.
- Reject Android tracking/nondefault schemes until validated; test-only missing
  services abort. Never decode arena handles after release. Existing frozen
  evidence is immutable. Normal constructor success does not certify debugger
  evaluator, nondefault heaps, full VM or 16 KB hardware/S25 (WARN-0006).

- DEC-0014/0015: authorized bounded debugger/allocation gate reuses whole actual
  evaluator and first-fit source. Typed Narrow32 pointer assignment replaces
  native-pointer truncation and encoded-address arithmetic; buffer return is a
  node handle. Normal constructors and evaluator function bodies are unchanged.
- Android Z_Malloc/Free have genuine zeroed, aligned ownership/type metadata;
  debug HunkUser requires explicit main-thread registration and first-fit uses
  native recursive synchronization. Native-width split/back-pointer alignment
  is corrected; Win32 placement and Windows heap services remain unchanged.
- Test-owned cleanup reference state and generated unavailable compiler/VM
  endpoints are explicitly confined to standalone probes. Every unavailable
  endpoint aborts. Host debugger tests of the Android heap do not certify the
  production Windows Z heap's legacy prefix alignment (WARN-0007).
- Preserve earlier evidence; current gate uses arm64_debugger. TASK-0006 is
  complete; TASK-0007 begins with actual normal-to-debugger numeric/boolean
  expression compilation, without widening into full VM/game/backend work.

- DEC-0016: at agent rotation the user authorizes one local checkpoint of the
  completed accumulated Android work. No push or further porting is authorized.
  Preserve all scratch/failure evidence and Windows/DXVK/root Ledger. Evidence
  Git attributes disable text normalization to retain archived byte hashes.
  docs/port/agent_handoff.md defines commands, limits and TASK-0007 acceptance
  criteria; its containing commit is the durable fresh-agent checkpoint.

## Expression, FastFile and zone gates — 2026-10-11

- TASK-0007/0008 user work is preserved: actual numeric/boolean expression compile
  and evaluation, other operators compile-only; raw 12-byte FastFile reader with
  logical fixture header. FACT-0024 supersedes earlier lack-of-assets statements:
  158 genuine retail files exist, compressed XFile still needs inflate.
- DEC-0018: reuse complete actual database sources, shared unchanged lightweight
  disk/block declarations and native diagnostic services. Named native ownership
  uses actual debug HunkUser/first-fit under reviewed types/flags and explicit
  main-thread Begin/End/detach/Release; not full legacy PMem ordering/tracking.
- FAIL-0004 / FACT-0027: preserve the stale-stride failure; clear native pending
  element strides at stream init. Retain Win32 behavior and production Windows
  services. Fresh-process negative probes avoid inherited parent thread identity.
- TASK-0009 complete; WARN-0009 retains all fixture/runtime/asset/hardware limits.
  Freeze evidence/arm64_zone and older evidence; no commit/push. TASK-0010 deflate
  isolation follows only on user direction, without asset parsing or async loading.

## Bounded deflate isolation — 2026-10-11

- DEC-0019: user continuation/delegation authorizes two read-only audits, root
  implementation/builds/Ledger. Reuse six matching bundled zlib1.2.3 C sources;
  custom stream lacks reserved and differs on LP64, so system libz is invalid.
- Wrapped unsigned decode validates complete checksum/end and36+XFile.size with
  independent uint64 budgets. Candidate output publishes only on success.
  Authenticated input/dictionaries/wrong wrappers/trailing data fail explicitly.
  Optional0..31 trailing zeros are deliberate policy measured on two assets;
  decoded length limit does not certify exact total heap usage.
- FACT-0029/0030: sixteen HONOR/host regressions and two genuine full decodes pass.
  Windows/DXVK/root/index and595 earlier evidence files remain frozen. No source
  fork, assets in Git/APK, async/game work, commit/push or scratch deletion.
- TASK-0010 complete; WARN-0010 bounds remaining integration. TASK-0011 genuine
  decoded header to actual zone bootstrap requires checked allocation preflight
  first and must stop before asset-list loading. Not started in this gate.

## Local checkpoint after deflate — 2026-10-11

- DEC-0020: user authorizes committing all accumulated durable Android work
  through TASK-0010, including their expression/FastFile changes and retained
  evidence. No push, further porting, Windows/DXVK/root changes or scratch cleanup.
- Stage source/docs/Ledger/evidence only; retain ignored builds/assets locally.
  Verify archived byte hashes through staging/commit and clean durable status.
  docs/port/android_checkpoint.md and its pre-commit manifest identify the scope;
  ignored post-commit verification records the final ID without another commit.

## TASK-0011 checked genuine allocation — 2026-10-11

- DEC-0021 supersedes earlier TASK-0011 bootstrap scope: no new DB_InitStreams,
  zone initialization, assets/fixups/rendering. Two read-only agents; lead owns
  integration. No commit/push or Windows/DXVK/root Ledger writes.
- DEC-0022: use genuine full-decode header/oracle SHA provenance, real production
  expansion/additive requests and exact first-fit capacity, uint64 preflight and
  atomic ownership rollback. No arbitrary multipliers or allocator rewrite.
- Native Begin/mmap registration exceptions release resources; rollback release
  allocates no temporary string key. End/Release nonthrowing; fatal engine OOM
  remains fatal, distinct from controlled transactional fixtures.
- FACT-0032/0033:67 HONOR/66 each Windows fixtures, genuine low/high production
  parity,83 AArch64 artifacts, prior16 device and Windows zone/four-byte tests.
  Separate decoder/allocation processes,31 warm+20 process-cold retained samples,
  actual VA/accessibility/touch/RSS and Windows committed-pool contracts recorded.
- FAIL-0006 preserves initial linkage/runtime/macros and measurement corrections.
  WARN-0011 bounds genuine next stream/init, multi-owner PMem/native diagnostics,
  full VM/asset/backend and hardware gaps. TASK-0012 remains recommended only.
- FACT-0034 verifies protected state/index/763 prior archives. Current report
  docs/port/arm64_checked_zone.md; all TASK-0011 changes remain uncommitted.

## TASK-0012 genuine stream initialization — 2026-10-11

- DEC-0023: user continuation authorizes actual DB_InitStreams on checked genuine
  blocks, cursor/reset/detach/free/release only. No payload/assets/fixups/zone
  initialization, commit/push or protected-checkout writes.
- FACT-0035:12 cycles/61 Init calls,6 preflight and6 partial failures per ABI;
  genuine fresh decode/header provenance, all7 cursors/zero blocks, independent
  pending size/stride and alias/drift reset, exact logical baseline pass.
- No shared engine/allocator code changes. Static lifetime detached descriptors
  prevent dangling stream descriptor globals; detach before descriptor clear and
  named release. Inactive stack bytes/vector capacity/diagnostics can remain.
- Two passing builds/runs retained; final strengthens unequal-size stride reset
  observability. Prior16 HONOR tests,67/66 allocation fixtures, Windows zone and
  Narrow32/declaration/483 guards pass;91 AArch64 artifacts verified.
- WARN-0012 supersedes WARN-0011; next TASK-0013 is only the16-byte asset-list
  envelope, before strings/assets/fixups, not started. FACT-0036 records final
  preservation/uncommitted inventory. Current report arm64_stream_init.md.

## TASK-0013 genuine asset-list envelope — 2026-10-11

- DEC-0024: user authorizes next bounded16-byte envelope and two read-only audits.
- DEC-0025: actual envelope/helper bytes unchanged;3 declarations moved once
  into lightweight production header, actual pointer-width/field ABI asserted.
  Standalone source selection excludes other expanders; normal Windows unchanged.
- Checked policy is additional preflight, not a legacy behavior claim: negative
  counts rejected, present-only uint64 minimum disk/native table footprints
  checked against genuine decoded length/block4. Null-positive counts retained.
- Finite validated byte-reader fixture is substantive but not async/refill loader;
  persistent destination prevents copied-temporary alias bugs. Preflight/read
  failures snapshot aliases/cursors/output/owners before reset (review correction).
- FACT-0037: both genuine files/3 ABIs,72 fixtures/12 cycles, exact16 consumption,
  no cursor commits/fixups, full source/field/prefix provenance and regressions.
- FAIL-0007 retains network-table linkage and heavy legacy-off graphics assertions
  plus LF/CRLF verification correction. Full normal Windows expander and selected
  layout tests pass; no guard disabled. WARN-0013 supersedes WARN-0012.
- FACT-0038 records102 AArch64 artifacts, protected state/frozen997 prior files
  and uncommitted inventory. TASK-0014 pointer-table recommendation not started.

## Approved overnight milestone

DEC-0026: user selects required structural front end plus interning stretch.
Pointer tables -> bounded raw strings -> actual asset directory, all3 ABIs/both
files, before dispatch. Local verified checkpoint commits now authorized, no
push. Preserve archives/protected state. Lead integration plus3 read-only audits.
Exact acceptance/stop conditions in docs/port/overnight_frontend.md.

## TASK-0014 — actual script-string pointer tables

DEC-0029 updated overnight objective: autonomous bounded native engine/assets
progress, genuine game-owned graphics optional; no fixture-as-gameplay claims.
Lead source/CMake/native tests; runner delegated distinct files, independent
review and Windows tests. Null root skips; present-zero needs real block/alignment.
Actual helper-domain1<<20 guarded,4096 diagnostic retained. Disk/native/source
positions separate. All860 aliases/tags and element mappings checked. Fixture
pre-copy failure bytes/state compared before reset; post-read bad_alloc notclaimed.
FACT-0040/0041 verify result/preservation; FAIL-0008/0009 retain RPC macro and
runner spelling failures. WARN-0014 supersedes WARN-0013. Next3milestones above.
