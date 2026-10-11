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
