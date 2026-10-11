# BlackOpsARM agent instructions

Work only in this Android worktree unless the user explicitly authorizes another
location. The branch is `android-arm64`; `../BlackOps64` is the Windows reference
checkout on `main`. Never edit its files, index, or branch as part of Android work.
Do not commit or push without explicit user authorization.

## Source and platform boundaries

- This is a Git worktree of BlackOps64, not an independent engine fork. Never copy
  or independently duplicate engine sources. Use this worktree's `src/`, `tl/`,
  `DemonWare/`, existing CMake source lists, and platform interfaces.
- Prefer minimal, isolated changes. Avoid broad cosmetic refactors of decompiled
  code. Keep platform implementations separate from shared gameplay logic.
- Put native Android implementations in `src/platform/android/`; small Win32 OS
  service compatibility shims belong in `src/platform/win32_compat/`.
- Use native Android implementations for graphics, audio, application lifecycle
  and controller input. Never add dummy Windows functions that silently succeed.
  Unsupported operations must produce an explicit error or documented failure.
- Preserve 32-bit serialization, original FastFile layouts, packaged shader
  bytecode and original game assets. Native runtime pointers must use native
  width; disk offsets/handles are not pointers. Do not apply blanket packing or
  disable layout checks to make ARM64 compile.
- Use correct ARM64 atomics and memory ordering. Do not assume x86 ordering,
  pointer-sized operations using 32-bit integers, or low-address allocation.
- Android ARM64 selects shared native-width layouts through
  `platform/bo1_native_width.h`; never enable `BO1_PORT_X64` globally on Android.
  That legacy option still controls Windows comparison builds and diagnostics.
  Consult the per-site flag audit before changing guards. Four-byte VM/parse
  slots use region-tagged Narrow32 handles by default; retain fixed disk/slot
  assertions. Test with the existing ARM64 narrow-slot and ABI probes, generated
  production declaration probes, and Win32/x64 host probes in this worktree.
- Shared engine changes must remain compatible with Windows. Record which
  Windows checks are needed; ask for authorization before writing or building in
  the Windows checkout. Read-only reference inspection is allowed.
- `../DXVK-Native` is a separate repository. Do not change it as part of this
  setup or routine engine work without explicit authorization. Initial renderer
  target: DXVK Native 1.9.2 with the tested SDL2/Vulkan integration.

## Ledger and reproducibility

Read `.ledger/AGENT-INSTRUCTIONS.md` and run `ledger context "<task>"` here at the
start of significant work. Also consult `../.ledger` for shared T5 history and
Windows evidence. Root Ledger owns shared project facts; this Ledger owns Android
setup, decisions, blockers and progress. Qualify root entry references by path;
IDs in the two stores are independent. Do not copy the root database or rewrite
its history. Current explicit user instructions supersede historical plans.

Use `ledger add` for significant decisions, findings, failures, blockers and
verification. Preserve history via `ledger supersede`/`invalidate` when needed.
Update `.ledger/STATE.md`, run `ledger status` and `ledger doctor` at handoff.
Keep `docs/port/android_arm64.md` build/test steps reproducible and distinguish
toolchain/APK checks from engine, renderer and on-device gameplay evidence.

Current milestone: TASK-0010 bounded deflate isolation complete. Twelve actual
T5 units and six unchanged bundled zlib C units compile; sixteen HONOR probes
and both Windows host ABIs pass. Current handoff: docs/port/arm64_inflate.md,
FACT-0029/0030, DEC-0019 and WARN-0010. Older reports preserve earlier milestones.
T5 zlib removes stream.reserved; use matching bundled sources and native sizeof,
never link engine stream headers to system libz. Disk XFile stays36; size counts
payload after the header, external/block sizes have separate meanings. Isolated
decoder validates whole wrapped stream/checksum/end and exact output before
publishing fields. Optional bounded0..31 trailing-zero policy is measured on
two files only. Decoded length cap is not an exact heap/capacity budget.
Next TASK-0011 connects genuine decoded headers to actual zone bootstrap only
after checked uint64 native expansion/backend/per-block/aggregate preflight.
Do not pass arbitrary header block sizes to the existing allocation arithmetic.
Native zones use real debug HunkUser/first-fit named ownership; register main
thread first, then Begin -> allocations -> End -> detach streams/clear blocks ->
Release. This is not legacy PMem pool/tracker/full API. Ordinary DB_FreeXBlocks
clears metadata, not ownership. Keep zero slots, owner-thread and handle lifetime
rules explicit; mixed Z/physical flags remain unvalidated. First-fit reset/destroy
requires quiescence; hosts using native ownership do not certify Windows PMem.
Missing test-only VM/over-allocation/compressed-byte callbacks abort and cannot
ship; dependent stream tracing stays disabled. Preserve all earlier evidence.
Stop before asset-list/string/payload deserialization or async integration. No
full engine launch, Zombies, DXVK/audio/input, restructuring or gameplay work.
Original assets stay outside Git/APK. Actual16KB Android/S25 remains untested.
User authorized two read-only audit agents for this gate; root owned writes and
builds. TASK-0010 is complete; further work follows user direction. User now
authorizes one local checkpoint through TASK-0010 (DEC-0020); no push or later
commit is authorized. See docs/port/android_checkpoint.md. Windows checkout,
DXVK and root Ledger stay intact.
