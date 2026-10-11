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

## Current Milestone

Debugger handle/allocation gate complete. Ten actual T5 units compile to
AArch64, including the complete evaluator and first-fit source. Four-byte
handles, selected cleanup/literal paths and real native allocation/thread
services pass on HONOR and Win32/x64. Windows host debugger tests use the Android
heap; production Windows Z heap integration remains unvalidated. Next:
normal-to-debugger expression compilation. No full engine/game/backend work.

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
