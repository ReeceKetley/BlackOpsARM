# Native viewmodel tree alignment

Frozen source: `src/cgame/cg_weapons.cpp`, SHA256 `08AC83D406EACBAE975E0930F85A1729C9049332E471471619F80A4E1B53116B`.

The Zombies callback `CG_AllocViewModelTree` now requests max(4, alignof(XAnimTree_s)) on Android. Actual native tree contains an XAnim_s pointer and two 16-bit fields: size16, alignment8. Windows retains the exact four-byte allocation expression and MP retains Hunk_AllocXAnimClient. No structure, callback signature or algorithm changed.

The allocation remains owned by the actual low hunk. Hunk_AllocLowAlign (com_memory.cpp:972) rounds the permanent offset by its power-of-two alignment, validates the actual address, commits real pages, tracks and clears the allocation. XAnimCreateTree invokes the provider synchronously. CG_ShutdownWeapons and viewmodel rebuild pass a null free callback to XAnimFreeTree, which clears animation state without individually freeing hunk storage (xanim.cpp:623). Server map transition calls CL_ShutdownAll before SV_ShutdownGameProgs (sv_init_mp.cpp:612-615); G_ShutdownGame releases animation/weapon resources and then Hunk_ClearToMarkLow (g_main_mp.cpp:2601-2605). Existing bulk retirement and rebuild storage retention are preserved.

Verification: actual FULL compile-command ARM64 whole TU passed in `build-android-audit/native_registry_closure/cg-viewmodel-alignment-native2.log`; exact argv saved alongside. Native1 failed because the evidence runner retained quotes inside -I arguments; that failed log is preserved and native2 corrects only argv quoting. Normal Windows whole TU x86 and x64 both passed once under `build-android-audit/cg-viewmodel-alignment-windows1`; their source manifests match the frozen source. Shared xanim.h/com_memory.h were confirmed frozen by their owner before compilation.

No fixture, production rebuild, device run, assets transfer or positive gameplay claim. No source list, definitions or linkage changes needed.
