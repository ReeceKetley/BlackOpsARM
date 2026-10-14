# Native CG animation-tree allocator callbacks

Root's actual full419 reached G_InitGame/G_LoadAnimTreeInstances and stopped when the unsigned XAnimCreateTree callback ABI received the differently typed signed Hunk_AllocActorXAnimServer provider. Root owns those actor callsites; startup_audit owns the tree constructor's exact native allocator overloads. This batch owns three additional CG sites only:

- `cgame/cg_scr_main.cpp`: script entity animtree assignment.
- `cgame_mp/cg_ents_mp.cpp`: SP animation mover tree creation and the general missing-tree creation branch.

The actual CG_AllocAnimTree provider is `char *(__cdecl *)(int)`, returning real MT_Alloc(size,5,SCRIPTINSTANCE_SERVER) memory. The three sites cast it to `void *(__cdecl *)(unsigned int)`, changing both callback return and argument types. Android now passes the actual provider directly to startup_audit's exact native XAnimCreateTree(XAnim_s*,char*(__cdecl*)(int)) overload. Windows retains each original cast. No wrapper captures state, allocator substitution, field algorithm change, source/header selection or flag was introduced. CP1252/CRLF retained.

Read-only allocation/ownership audit:

- XAnimTreeSize already returns sizeof(XAnimTree_s). The native tree is16 bytes/alignment8, holding its actual native XAnim_s pointer and two16-bit scalar fields; it is not a packed script code/stack handle record.
- Canonical creation clears the full returned tree size and assigns its actual anims pointer. The coordinated native constructor owner validates actual storage before publication; this batch does not implement that constructor.
- MT allocation uses an aligned128-byte global memory-tree base plus16*node index, sufficient for this tree. Existing default server/client hunk allocators request32-byte alignment; precache's native XAnimParts alignment also covers the tree.
- Actor signed int low-hunk allocations and their two callsites are root-owned and use the new exact signed void-return overload. Existing CG actor/client Hunk_AllocXAnimClient and UI Hunk_AllocXAnimPrecache unsigned void callbacks already have the correct signatures. The common small-tree path already has genuine stable typed allocator/free adapters.
- Tree free paths invoke canonical XAnimClearTree and the actual supplied free callback; hunk-owned bank instances clear trees before low-hunk retirement, while CG MT-owned small trees use real MT release. No independent lifetime replacement was added here.
- Separate unmodified `cgame/cg_weapons.cpp:CG_AllocViewModelTree` uses four-byte low-hunk alignment on the Zombie branch despite the native tree's eight-byte requirement. This was reported to root for its assigned owner; this batch does not own that source or claim its repair.

Evidence:

- `build-android-audit/native_registry_closure/cg_scr_main-tree-native.log`, `cg_ents_mp-tree-native.log`: both actual whole NDK TUs compile exit0.
- `cg-tree-callback-symbols.log`: compiled CG references select the char-return signed tree overload.
- `build-android-audit/takeover_cg_tree_callbacks_windows1/{x86,x64}/build-result.json`: both normal Windows TUs pass once.
- No synthetic allocator/renderer fixture, production build or device run. Actual CG tree creation/retirement and rendered behavior remain unverified by this narrow batch.

Frozen SHA256:

- cg_scr_main.cpp: 10F6507466B5F867E5F7901D17A2DDCDFC33FE0F8D6B18D677225D081FECF04C
- cg_ents_mp.cpp: E781BA35BD148857E4BC50D8B272E80E936298881831F88F9D27B20696647DF6

Shared xanim.h coordinated frozen interface hash:7A30A16DFD78204AAE9AADD1FFECA1A760BF43D591669173A0A95FC0C81FF096. Integration needs existing TUs and that producer overload only; no new CMake or flags.
