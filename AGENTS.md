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

Current milestone: bounded debugger handle/allocation gate complete. Ten actual
T5 units compile, including the whole evaluator and first-fit source; thirteen
HONOR probes and Windows x86/x64 checks pass. See docs/port/arm64_debugger.md,
DEC-0014/0015 and WARN-0007. Android has real owned Z allocations, registered
main-thread identity and first-fit synchronization; register the main thread
before debug HunkUser startup. Native-width split headers/back-pointers are
aligned; Win32 placement remains unchanged. Host debugger tests use the Android
heap and do not certify the unchanged Windows engine Z heap's alignment.
Default HunkUser/global debugger compile lists are owner-thread confined;
first-fit reset/destroy require quiescent users. Handles must not outlive their
allocations. Android tracking/fixed schemes remain rejected. Test-owned cleanup
state and fifty aborting compiler/VM endpoints are fixtures, absent from the APK.
Preserve all earlier frozen evidence. TASK-0007 is the next bounded actual
normal-to-debugger expression compilation boundary; inspect before expanding.
Do not attempt a full engine build, launch Zombies, integrate DXVK, port
audio/input, restructure the engine, modify gameplay or import assets. The user
authorized one local checkpoint at agent rotation; future commits/pushes need
explicit authorization. See docs/port/agent_handoff.md before resuming. No actual
16 KB Android device or Samsung S25 validation is claimed.
