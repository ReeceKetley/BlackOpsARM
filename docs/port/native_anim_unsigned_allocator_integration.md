# Native unsigned animation allocator propagation

The canonical `bgs_t::AllocXAnim` callback takes unsigned int. G_InitGame binds the actual server low-hunk allocator; CG initialization binds the actual client hunk allocator. BG_LoadAnim previously cast that callback to the signed Scr_PrecacheAnimTrees allocator signature. Matching register placement does not satisfy the native function callback type.

Android now exposes unsigned allocator overloads alongside all existing signed APIs:

1. Scr_PrecacheAnimTrees.
2. Scr_LoadAnimTreeAtIndex.
3. Scr_CreateAllAnimTreeData.
4. Scr_CreateAnimationTree, including recursion.

Native overloads instantiate shared typed internal templates containing the exact canonical bodies. The allocator travels as a typed function argument through synchronous loops and recursive construction. Scr_LoadAnimTreeAtIndex selects the real signed or unsigned XAnimCreateAnimsWithValues overload supplied by the separate XAnim owner. The tree-data builders only forward the allocator and do not invoke it themselves. There is no captured global/TLS state, erased function pointer, replacement parser/VM, callback suppression or readiness substitute. Existing script indices, assets, checksum behavior and allocation publication remain unchanged.

The existing cscr_animtree XAnimPrecache cast also selects the XAnim owner's real Hunk_AllocXAnimPrecacheSigned adapter on Android; it checks the signed request then calls the actual unsigned hunk provider. Windows retains its original bodies/signatures/casts. CP1252 and original line endings preserved.

Interfaces coordinated with startup_audit's native XAnimCreateAnimsWithValues(const char*, int, unsigned int, void*(__cdecl*)(unsigned int)) and stable Hunk_AllocXAnimPrecacheSigned(int). Root separately owns changing native BG_LoadAnim to call the unsigned overload without a cast. No CMake, flag, new production source or dependency is needed.

Verification:

- Actual NDK whole two translation units compile exit0: `build-android-audit/native_registry_closure/cscr_main-unsigned-native.log`, `cscr_animtree-unsigned-native.log`.
- `anim-unsigned-chain-symbols.log` confirms all eight signed/unsigned compiled entrypoints and the corresponding real constructor dependencies.
- Normal Windows x86/x64 once, both TUs pass: `build-android-audit/takeover_anim_unsigned_chain_windows1/{x86,x64}/build-result.json`.
- No synthetic VM/provider fixture, production build or tablet run. Actual production allocation/script construction remains root's next integration proof.

Read-only BG allocation findings reported separately to root before edits: sizeof(loadAnim_t)*1024 is already the real native scratch requirement; fixed scr_anim_s links remain Narrow32 and native LargeLocal rounds/aligned storage correctly. However, BG_AnimParseAnimScript publishes g_pLoadAnims into the local LargeLocal span. Returning from BG_LoadAnim retires that scratch reservation before the subsequent BG_PostLoadAnim/Finalize reads it. This exists in the Windows reference too; potential reuse/clobber was not executed or claimed. Root owns that adjacent lifetime policy, not this callback batch. The helper's next-record write lacks a preceding1024 bound, while later finalization checks animation-tree size; this is a separate parser capacity audit, not evidence of a native sizeof defect. BG_CheckThread accepts main/server but the actual server hunk allocator still requires main; callback propagation preserves that actual owner requirement.

Frozen SHA256:

- cscr_main.cpp: 9B33EA7D18310C9F20FEAAB74ADDDF9FA90E62AB1955ABA2898065FBBA56B01A
- cscr_main.h: F975673E8AD5369CF48E41B62B93A71CD5E36086ECDB85EC0DE6B87B819ACD44
- cscr_animtree.cpp: 5B7EEAB72FE9990B0E362E2616BFAABD8E608AB5B328FABB4A88D531A24A8B41
- cscr_animtree.h: 8546D6CCF8A9874EABF8509A48A982D814DCC966B742700A84DFFC53B3CCEA30
