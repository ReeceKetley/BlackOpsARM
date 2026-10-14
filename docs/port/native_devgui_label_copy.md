# Native DevGui label byte cursor

2026-10-14. Scoped source repair for the actual SDK438 UBSan failure. Source owner changes only `src/devgui/devgui.cpp`; root owns builds, device execution, config and Ledger. Runtime verification belongs to the combined SDK439 launch.

## Observed failure and exact cause

`build-android-audit/native_registry_closure/sdk-native-entry438-launch1-only.log` reports member access through a `DevMenuItem*` at odd address `0x006f1994f6d1` in `DevGui_CreateMenu`, original source line 143. The loop initializes a typed menu pointer from the actual aligned record and then increments it by one byte after every copied character. Its next `v6->label[0]` access fabricates an unaligned structure object even though the operation is simply a byte copy. This is a proven cursor bug, not evidence of a bad menu allocation.

Native compiler record layouts confirm `DevMenuChild` size 8/alignment 8, `DevMenuItem` size 48/alignment 8 with `label[26]` at offset 0 and `child` at offset 40, and `devguiGlob_t` size 98400/alignment 8. The 2048-menu array spans 98304 bytes. Existing record indexing and free-list construction use `sizeof(DevMenuItem)` and actual next-array-element addresses. These allocation, handle and free-list operations remain unchanged.

## Repair and contract audit

Under `defined(__ANDROID__) && BO1_NATIVE_64BIT`, the destination is now a real `char*` initialized to `menu->label`. The same do/while loop reads and writes exactly one byte per iteration, advances both byte cursors and stops after writing the original terminating NUL. The original source cursor advancement and termination byte are preserved. There is no truncation, extra write, replacement allocation or hidden memcpy. The original typed-pointer declaration and loop remain in the other branch; the native-width header is explicitly included.

This file contains exactly one copy in the confirmed fabricated typed-byte-cursor family. The other labels are parsed into byte arrays, compared as strings or handled through correctly indexed actual menu records. Repository source calls `DevGui_CreateMenu` only from `DevGui_RegisterMenu`; canonical `DevGui_ConstructPath_r` supplies labels from `DevGui_PathToken`, which copies at most 25 characters and appends a NUL into the 26-byte menu-label extent. Existing path validity, 120-character path limit, escaped separator handling, sort keys and label semantics remain unchanged. No broader UI or registration repair is included.

## Verification and preserved evidence

- Actual production native whole `devgui.cpp` TU compiled exit 0 using its current `native_engine_entry1/compile_commands.json` command with isolated output, retaining O0/debug/PIC/function instrumentation/UBSan/no-recovery. An added compiler layout dump supplies the native layout evidence; no master build was run.
- Normal Windows x86 and x64 whole TUs compiled exit 0 with the established MSVC comparison options; all outputs stay in this Android worktree.
- Before, after and read-only BlackOps64 sources were preprocessed under both Windows ABIs. `DevGui_CreateMenu` nonempty code bytes match the saved original and the validated Windows oracle exactly after omitting only compiler `#line` directives and empty lines. Full before/after source differences are limited to compiler-expanded source paths/line numbers in one assertion and line-derived profiler symbol names, as expected after adding guarded source lines. The meaningful whole-source comparison otherwise matches; retained raw differences make this limitation explicit.
- No synthetic DevGui implementation or runtime fixture was used. No SDK/device result, rendering readiness or playable gameplay is claimed here. Root's actual SDK439 run is the integration authority.
- Two check-harness failures are retained: `check-results-failed1.json` records an invalid Python encoding spelling before Windows compilation; `check-results-failed2.json` records the initial comparison that included compiler line metadata. Corrected `compare-existing.py` verifies the already successful compile/preprocess outputs without rerunning the builds. No production changes were made to fix either harness issue.

Evidence here includes the byte-exact pre-edit source, scoped source diff, exact commands, native/MSVC logs and objects, raw before/after/oracle preprocessing, extracted function bodies, native record layouts, comparison results and source manifest. BlackOps64, assets, DXVK and the user's spike are unmodified.

Frozen source SHA256: **1080223A2D8343FF45D151EB68A543DE2D9001E78237BD37601F3614A6C6F5B2**.
