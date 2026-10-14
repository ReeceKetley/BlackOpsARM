# Native UI visibility and rectangle copy alignment

Updated 2026-10-14. Source repair is frozen; real SDK442 startup remains the integration authority. Root owns shared builds, device, configuration and Ledger.

## Actual failure

The isolated actual SDK441 log ends at `ui_utils.cpp:129:96`: unsigned64 load requires8-byte alignment at address `0x006e5d2d43bc`. This is `Item_IsVisible`, not a rectangle load. `itemDef_s` is8-aligned and its real unsigned64 `showBits`/`hideBits` fields are naturally8-aligned. The decompiled read begins4bytes into showBits and spans showBits high32 plus hideBits low32. No item allocation or asset packing change is needed.

On little-endian ARM64 the original mixed read equals `(hideLow << 32) | showHigh`. Its four effective predicates require both low/high show bits in visibility and require both low/high visibility lanes to be subsets of hideBits. The native replacement is exactly `(visibility & showBits) == showBits && (visibility | hideBits) == hideBits`; hideBits is intentionally not changed into an inverse exclusion mask. Window visibility, dvar gating and visible expression evaluation retain their order and bodies. The Windows original expression remains under the existing platform guard.

## Scoped copy audit and repairs

- `Window_Paint`: real `rectDef_s` size24/align4, alignment ints at offsets16/20. Replace the QWORD copy with the two real int assignments on native Android. x/y/w/h, border adjustment, flags, alignment values and subsequent draw calls are unchanged.
- `UI_DrawHighlightRect`: local `float verts[4][2]` has only a4-byte alignment contract. Replace both first-row QWORD writes with x/y float assignments after the existing two `ScrPlace_ApplyRect` transformations. Four quad emissions and the remaining vertices/colors are unchanged.
- `Item_ListBox_PaintTextElem` ownerdraw: actual windowDef_t is size176/align8 but foreColor float[4] begins at offset100, hence4mod8. The original cast constructs a fictitious align8 DvarValue at that address. Native memcpy to `color.vector` copies exactly its16bytes from the real float[4] object. The rect assignments, full RGBA contents and ownerdraw call remain unchanged.
- The separate QWORD read of `listPtr->elementHighlightColor` is retained. Native listBoxDef_s is size688/align8, that float[4] member begins at608, and the actual typed owner guarantees its8-byte alignment. A proposed replacement was removed after the actual layout proved it aligned; only the final source is frozen.

The audit covers wide rect/float bit copies in src/ui. Real unsigned64 xuid/mask fields, pointer arrays, fixed32 address slots, char prefix copies and cursor/scroll pointer reinterpretations are separate families and were not blanket replaced. No asset layout, allocation stride, DXVK/SDL/Vulkan, producer padding, assertion, sanitizer flag or readiness implementation changed. BlackOps64 and original assets remained read-only.

## Validation and limits

All nine complete translation-unit checks pass: ui_utils/ui_shared/ui_main for native Android ARM64 with the actual SDK engine compile flags, and Windows x86/x64. Native flags retain function instrumentation, UBSan and no recovery. Native record layouts and three preprocessing outputs are retained. Active native outputs no longer contain the mixed-field visibility load, wide rect alignment copy, two wide vertex stores or float-to-DvarValue cast.

All12 Windows before/after preprocessing calls pass. Selected active function bodies are unchanged after normalizing compiler line metadata: Item_IsVisible, Window_Paint, Item_ListBox_PaintTextElem, UI_DrawHighlightRect, Menus_Open and Menu_PerformTransitionEffects. All four inherited native menuStack guard blocks are byte-identical to the source at handoff (prior6F31039B repair).

The focused CPU mask oracle passes196608 comparisons: all4096 combinations of four chosen bits spanning low/high lanes, rotated over32 positions, plus65536 deterministic full64-bit triplets. It reconstructs the original little-endian mixed read through legal byte extraction. Zero visibility/zero show/all hide and all visibility/all show/all hide remain visible. This is a bounded algebra/extent check, not actual engine or GPU runtime evidence. No UI draw fixture was added: exercising canonical Window_Paint requires renderer, dvar, menu and material owners, and the next real SDK launch checks that integration directly.

Source edits preserve CP1252 and existing line-ending exceptions; all newly inserted lines use CRLF. Existing source LF exceptions (ui_shared16/ui_main2) were retained. Baseline bytes, source diffs, compile commands/logs, layout extracts, preprocessing comparisons, oracle results and final manifests are in `build-android-audit/native_ui_mask_rect_alignment1/`. The first repair-script assertion exposed those preexisting LF exceptions before shared/main edits; the final recipe checks preservation instead of demanding whole-file normalization. No device use, SDK package mutation, master build, commit, reset or clean was performed by this owner.

## Frozen SHA256

- `ui_utils.cpp`: `5C296C6A14396D33A583B75C281CD4252853E725440860E20569180EC8CE7567`
- `ui_shared.cpp`: `B0D013D794D5DF1BEBFB81A3613FFAE131A618454A90B499BC996184E5E591B1`
- `ui_main.cpp`: `D11CAAB8C0E895ABDEA4FC75A3F3444686B6682E18C883CB09280FA21F8B1E2B`

Root can build SDK442 from these released sources; its launch must establish whether the actual visibility failure and downstream UI copies pass. Rendering/frame/gameplay remains unverified by these checks.
