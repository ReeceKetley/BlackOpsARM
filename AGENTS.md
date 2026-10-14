# BlackOpsARM agent instructions

## Start here

Read [.ledger/AGENT-INSTRUCTIONS.md](.ledger/AGENT-INSTRUCTIONS.md), then
[.ledger/GOAL.md](.ledger/GOAL.md) and [.ledger/STATE.md](.ledger/STATE.md).
STATE is the single current handoff. Historical snapshots are evidence, not
current instructions; explicit user direction takes precedence.

## Scope and source boundaries

- Work in this Android worktree on android-arm64. BlackOps64 is the working
  Windows reference; inspect it read-only. Preserve its files, index and branch.
- Preserve ../DXVK-Native, original retail assets and all prior test evidence.
  Use the tested DXVK Native 1.9.2 / SDL2 / Vulkan integration when needed.
- No commit or push in this takeover without explicit current authorization.
  Preserve pending changes; no reset, clean, discard or destructive Git actions.
- Do not spawn additional agents unless the user or applicable instructions ask.
- Use canonical engine sources and declarations. Never copy an independent
  engine implementation. Avoid broad cosmetic changes to decompiled code.
- Native Android services belong in src/platform/android; narrow Win32 service
  compatibility belongs in src/platform/win32_compat.
- Implement real OS, thread, memory, graphics, audio and lifecycle behavior.
  Never add dummy success, fake readiness or assertion bypasses to advance boot.
  Missing behavior must fail explicitly. Existing intentional stubs accepted
  in the working Windows reference do not require unrelated repair.
- Shared changes must preserve Windows behavior. Build comparison checks in
  this worktree; do not write/build inside the Windows reference checkout.

## ABI and resource invariants

- Preserve original FastFiles, assets, shader bytecode and fixed 32-bit disk/
  VM slots. Native runtime pointers use native width; handles are not pointers.
- Use platform/bo1_native_width.h and BO1_NATIVE_64BIT on Android. Never globally
  enable BO1_PORT_X64, blanket-pack structures or disable layout assertions.
- Consult the existing per-site flag audit for guard changes. Use Narrow32
  region handles for fixed four-byte slots; preserve publication/lifetime rules.
- Use correct ARM64 atomics and memory ordering; never assume x86 ordering or
  low-address allocation. Check ownership, retirement and failure behavior.
- Use the engine's matching modified zlib ABI, not a system zlib substitute.
- Reuse staged tablet assets. Transfer changed binaries or proven missing/
  changed assets only; do not repeatedly transfer or test every similar file.
- Prioritize actual Com_Init, DB loading, Com_Frame and headless Zombies
  integration. Dead Ops Arcade is excluded. Tests should cover concrete new
  ABI, synchronization, ownership or recovery risks, with proportionate checks.

## Persistent memory

Run Ledger from this worktree. Record significant progress/failures and update
STATE in place at handoff; do not append another competing current-state section.
Keep historical milestones in reports/archive and link the relevant evidence.
Write reports with an explicit encoding. For full report rewrites, write and
verify a temporary copy before replacing existing prose; preserve prior evidence.
