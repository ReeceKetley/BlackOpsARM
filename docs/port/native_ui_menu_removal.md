# Native UI menu roster removal

Frozen 2026-10-13. Only `src/ui/ui_shared.cpp` changed, inside
`Menus_RemoveMenu` plus its Android-only `<cstdlib>` include. No central target,
UI header/layout, asset, protected checkout, or unrelated UI body was modified.

The original removal copied `4 * (menuCount-i)-4` bytes. The native roster is an
array of `menuDef_t *`, whose rows are eight bytes on ARM64. Native removal now
copies `sizeof(uiDC.Menus[0]) * (menuCount-i-1)` bytes from the actual next row.
It skips the move when there is no remaining tail. Native preconditions require
a real `UI_GetInfo` result and a count within the actual 600-pointer capacity;
failure uses the real fatal error and aborts if that callback returns.

Count decrement and index retry are unchanged, so consecutive/repeated matches
are removed using the original algorithm. No other insertion or byte-stride
operation exists inside this function. The original Windows removal expression
and control flow remain in the Windows branch; no assertion was weakened.
Actual menu contents, DB asset ownership, and inactive tail policy are preserved.

## Evidence

- `build-android-audit/native_registry_closure/ui_shared-menu-removal-audit2-command.json`
  and log: actual FULL instrumented native whole-TU compilation passes. Defined
  and undefined symbol manifests are retained.
- `build-android-audit/menu-removal-windows2/{x86,x64}`: one normal Windows
  whole-TU comparison per ABI, both exit0 without native selectors; source
  manifests, commands and result JSON are retained.
- `build-android-audit/native_registry_closure/menu-removal-frozen-hashes2.json`:
  final source SHA256 `0d02889d2ecabc3da7aa24517f4cb841a3e134842f961b5fac132fa9de2f062a`.

The startup audit owner completed the combined actual `DB_FreeMenu` ->
`Menus_RemoveMenu` HONOR ARM64 UBSan fixture once: PASS, 854 observations.
Evidence is `build-android-audit/native_asset_free_handlers/honor-run1.log` and
`fixture-build2.log`; broader adapter scope is described in
`native_asset_free_handlers_integration.md`. It covers front/middle/last,
consecutive and separated duplicates, absent matches, a full 600-pointer roster,
survivor order/native pointer bits, unchanged other UI bytes and unused-tail
canaries, and the surrounding asset pool links/rows. Null UI and counts -1/601
mark the real fatal callback in shared memory and abort even when the scoped
reporter deliberately returns. This uses an isolated real roster and actual
engine bodies, not a manufactured initialized UI. No production run or duplicate
device fixture was performed by this change's owner.

The first reported freeze was incorrect: a multiline CRLF replacement silently
missed the move, leaving only the count guard. Root review found this. Audit1
and windows1 remain evidence for that partial source, not native stride repair.
The actual move was replaced by exact function-span anchors, then read back and
asserted before audit2/windows2 and the new final hash.
