# Project Goal

## Primary Objective

Native Android ARM64 port of the original T5 Zombies engine. Windows x64 is the
functional reference implementation; the original Win32 path remains a behavior
and serialization oracle. This Git worktree shares BlackOps64 history and sources.

## Success Criteria

Eventually load original game assets and FastFiles, render and play an offline
T5 Zombies round with sound and an Xbox controller on HONOR Pad X8a and Samsung
Galaxy S25. Initial renderer: DXVK Native 1.9.2 with tested SDL2/Vulkan integration.
Device validation must be recorded separately from Windows validation.

## Approved overnight milestone

Native FastFile structural front end: both genuine files/3 ABIs through pointer
tables, bounded raw strings and typed asset directory, before dispatch. Real
production interning is conditional stretch. DEC-0026 authorizes local verified
checkpoint commits, never push. See docs/port/overnight_frontend.md.

## Latest completed gate

TASK-0014 actual genuine script-string pointer table,26 fixtures/12cycles perABI.
Next: bounded raw strings -> asset directory -> real SL/MT pool, before dispatch.
See arm64_string_table.md/WARN-0014. Autonomous updated objective continues toward
actual game-owned execution/assets/graphics; no simulated progress or deadline.

## Historical completed gate

TASK-0013 genuine16-byte XAssetList envelope read/expansion is complete on HONOR
ARM64 and Windows x86/x64,72 fixtures/12 cycles each. Counts/tags, persistent
aliases, unchanged cursors and reset/release pass with independent full-decode
provenance. Actual body bytes unchanged; finite reader is fixture-only.
See docs/port/arm64_assetlist.md, FACT-0037/0038, WARN-0013. Next recommended
TASK-0014 is the block4 script-string pointer table, before strings/assets/fixups.
Not started. TASK-0011 timing/RSS and all older evidence remain frozen.

## Constraints

- Retain original game assets and 32-bit serialization/FastFile layouts.
- Keep runtime pointers native-width and use correct ARM64 memory ordering.
- Prefer small Win32 compatibility shims for OS services; use native Android
  graphics, audio, application lifecycle and controller-first Xbox input.
- Shared engine changes must remain compatible with Windows.
- DXVK-Native is maintained in `../DXVK-Native`, a separate repository.
- `../.ledger` owns shared project goals/history and Windows evidence; this
  Ledger owns Android setup, decisions, blockers and progress. User steering
  overrides older root planning without rewriting historical evidence.
- Keep original game assets out of source control and APK scaffold.

## Non-Goals

- Full engine conversion, gameplay changes or broad decompiled-code cleanup
  during the bounded compilation audit.
- Independent engine fork/source duplication or changes to the Windows branch.
- Changes to DXVK-Native, commits or pushes without explicit authorization.
- T6 compatibility work before native T5 bring-up.
