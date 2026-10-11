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

Bounded deflate gate complete (TASK-0010): twelve actual T5 units and six
unchanged bundled zlib C sources compile; sixteen HONOR probes and both Windows
host ABIs pass. Two genuine FastFiles decode with fixed 36-byte XFile and complete
checksum/end validation. Actual zone bootstrap remains fixture-only. Next:
TASK-0011 genuine decoded header to checked zone allocation/stream lifecycle.
See docs/port/arm64_inflate.md; no full engine/game/backend work.

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
