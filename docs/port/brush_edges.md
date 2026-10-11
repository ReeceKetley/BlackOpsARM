# Native brush geometry fixes (batch42)

`common/brush_edges.cpp` retained a decompiled16-byte x86 BFS node represented
by cross-local stack aliases, integer pointer slots and integer predecessor
addresses. It now uses typed nodes with full-width point pointers and predecessor
indices. Point uniqueness storage is also pointer-typed.

Pointer-array removals, cycle selection copies and partition reordering now copy
`sizeof(*element)` bytes per element. `PartitionEdges` now supplies an actual
1024-pointer cycle array instead of treating one pointer local as stack storage.
`ChooseEdgeToRemove` selects named locals explicitly rather than indexing across
unrelated locals.

Run the focused tests with:

```powershell
.\tools\port\check_brush_edges.ps1
```

The generator extracts actual production functions and the real epsilon
comparison into a standalone executable. Win32 and x64 pass cycle ordering,
disconnected cycles, epsilon uniqueness, middle-pointer removal, rare-plane
removal and partition reordering of disconnected loops. A1024-point cycle fills
the output array up to a no-access guard page. Native geometry is allocated at
0x180000000. This exercises brush algorithms that ordinary fastfile gameplay
does not guarantee to call.

All four engine builds compile with 0 errors; recompiled brush code has no
pointer-truncation warnings. Original Pentagon routes finish through Pack-a-Punch:
`w32_brush42`25PASS, `x64hi_brush42`31PASS, `x64low_brush42`24PASS,
`x64guard_brush42`31PASS. All0FAIL/complete/exit 0, all four SaveGame checks pass,
no recorded assert or emergency crash. These are god-mode headless regressions.

Full high-address rebuild `native_full42.log` completes216 seconds with 0 errors:
178 unique truncation sites (176 src/tl plus2 DemonWare), down from 184. The saved
aggregate is `build/run/native_brush42_dxvk43_results.json`. These fixes do not
establish all remaining cold geometry/developer paths are clean. LOW_VA default
is still ON; the Vulkan experiment uses the explicit unrestricted high build.
