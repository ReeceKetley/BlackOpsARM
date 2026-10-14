# Native typed menu-stack top access

Frozen source: `src/ui/ui_shared.cpp`, SHA256
`6F31039BD437D45CAF58A4C850A02802B78708C73D3B6D39C6A99E3AC63E5132`.
No header changes. Original source bytes are retained in `ui_shared.cpp.before`;
the repair preserves CP1252 bytes and CRLF endings.

## Actual trigger

SDK439 (BuildId `c1f7844eacb7ebe2d7ed2efd773fbf44eed1d7d0`) genuinely
returns Com_Init at launch log line19054 and enters its input/frame loop.
Natural `map frontend` then reaches loading-menu rendering and aborts at
ui_shared.cpp4379: index601 outside `menuDef_t *Menus[600]`.
Evidence: `../native_registry_closure/sdk-native-entry439-launch1-only.log`
and `sdk-native-launch439-crash1.txt`. SDK440 actual integration remains owned
by the root agent; this report makes no rendered gameplay/runtime pass claim.

## Repair and behavioral contract

Four exact decompiled `Menus[2 * openMenuCount + 599]` reads are native-only
replaced by `menuStack[openMenuCount - 1].menu`: both reads in Menus_Open and
both transition-completion focus comparisons in Menu_PerformTransitionEffects.
Windows retains the original expressions and bodies.

Menus_Open retains its original positive-count outer guard and repeated lookup
after Menu_LoseFocusDueToOpen. Native setup checks positive count again after
that callback because leaveFocus scripts can close all menus. Both transition
comparisons also check positive count: an empty stack has no top menu to focus.
No cached top pointer overrides script-induced stack changes; no null pointer
is passed to the focus/setup callee. Ordering, counts, localClientNum, focus
script ordering and window flag effects otherwise stay canonical.

Producer audit: Menu_Setup starts count0. Menus_AddToStack removes an existing
occurrence, raises the existing ERR_DROP at16 before increment, then inserts
by the existing priority comparison and moves menu/localClientNum together.
Menus_RemoveFromStack decrements only after finding an actual matching entry
and shifts the same typed pair. Existing top consumer UI_GetTopActiveMenuName
uses `openMenuCount-1` and `.menu`. No array enlargement/count clamp/assertion
or UBSan weakening was used; previous full411 menu pointer-array movement stays
intact.

## Measured layouts

These are actual compiler layouts from the full translation unit and unchanged
headers, not inferred from decompiler sizeof comments.

| Field/size | Android ARM64 | Windows x86 | Windows x64 |
|---|---:|---:|---:|
| menuClientNum_t.menu offset | 0 | 0 | 0 |
| menuClientNum_t.localClientNum offset | 8 | 4 | 8 |
| sizeof(menuClientNum_t) | 16 | 8 | 16 |
| UiContext.Menus offset | 576 | 572 | 576 |
| UiContext.menuCount offset | 5376 | 2972 | 5376 |
| UiContext.menuStack offset | 5384 | 2976 | 5384 |
| UiContext.openMenuCount offset | 5640 | 3104 | 5640 |
| UiContext.localVars offset | 5648 | 3112 | 5648 |
| sizeof(UiContext) | 11792 | 6184 | 11792 |

Native menuClientNum_t and UiContext align8. The legacy byte address actually
coincides with the typed top field for a positive valid count on all measured
layouts. On ARM64/x64: `576+8*(599+2*n) == 5384+16*(n-1)`; on x86:
`572+4*(599+2*n) == 2976+8*(n-1)`. The defect is an illegal C++ cross-array
access, not a demonstrated physical offset mismatch. The native field access
removes that defect without changing any structure or asset representation.

## Verification

`check.py` results in `check-results.json`:

- Actual native compile_commands whole ui_shared.cpp TU: exit0 with unchanged
  production instrumentation/UBSan flags. Native preprocessor exit0: zero active
  legacy cross-array expressions, exactly four typed top expressions.
- Windows MSVC x86/x64 full TU compilation: exit0. Both affected function bodies
  preprocessed byte-identical to the preserved before source and read-only
  BlackOps64 oracle. Whole source differs from before only in compiler-generated
  source path/line metadata (normalized for comparison).
- Native layouts: `native-layouts.txt`. Windows layouts:
  `windows-{x86,x64}.log` for menuClientNum_t and
  `windows-{x86,x64}-uicontext.log` for UiContext; all compiler exits0.
- Source unchanged through checks; original Windows bodies and existing full411
  pointer-array movement verified by preprocessing/source comparison.

No fake UI engine fixture was added. Real SDK440 execution is the next runtime
authority. No master build, device, providers, Vulkan, assets, ledger, git
commit/reset/clean, or Windows reference mutation was performed by this owner.
