# Native per-local-client companion memory ownership

Changes are limited to `cgame_mp/cg_main_mp.cpp` and `server_mp/sv_init_mp.cpp`. `EffectsCore/fx_system.cpp` was audited and compiled without modifying it: its three allocations already use 128-byte alignment and its budget includes exactly three maximum 127-byte alignment gaps. Existing pending edits in that file were preserved.

The actual NDK ARM64 layout probe compiled to assembly, without a device run, establishes:

| Type | Size | Alignment |
|---|---:|---:|
| cg_s | 475008 | 128 |
| cgs_t | 18520 | 8 |
| fake_centity_s | 864 | 8 |
| ViewModelInfo | 72 | 8 |
| weaponInfo_s | 64 | 8 |
| centity_s | 856 | 8 |
| Destructible | 192 | 8 |
| IKState | 5344 | 16 |
| client_t | 553024 | 8 |
| MatchState | 128 | 64 |
| clientState_s | 208 | 8 |
| cachedClient_s | 10368 | 8 |
| FxSystemContainer | 3040 | 16 |
| FxSystemBuffers | 500864 | 16 |
| FxMarksSystem | 164912 | 8 |

The two server MatchState buffers previously requested four-byte alignment despite the actual 64-byte requirement. CG pointer-bearing arrays and server client/cached-client arrays likewise requested four bytes despite eight-byte requirements. Android now requests max(original alignment, alignof(actual element)). Original Windows alignment expressions remain unchanged. Existing Bo1_MaxSize allocation strides are retained.

Native SizeRequired derives every actual allocation plus its maximum alignment gap (`alignment - 1`), using size_t arithmetic, block/final INT_MAX checks, canonical one-local-client capacity, and server configured domain up to32. CG has nine blocks and187 maximum padding bytes; SV has eleven blocks and425; FX has three blocks and381. Original Windows budgets, including its CG16384 padding, remain unchanged. No blanket arena growth or packing was introduced.

Exact companion budgets for one local client on this ABI:

| Owner/configuration | Bytes |
|---|---:|
| CG, default1024 entities | 2160859 |
| CG, extended2048 entities | 3043547 |
| FX | 669197 |
| SV,1 client | 18204649 |
| SV,2 clients | 19404457 |
| SV,18 clients | 43729577 |
| SV,30 clients | 67693481 |
| SV,32 clients | 72164137 |

These exclude root-owned primary client/connection buffers and the HunkUser header/start alignment. SV allocation counts remain canonical: client_t*N,32*N MatchState,2688*N entityState,32*N*N clientState,512 cachedSnapshot, N cached MatchState,80*N archivedEntity,max(N,2)^2 cachedClient,1200 archivedSnapshot,16MiB archived bytes and32 IK states. Compile-time assertions retain fixed snapshot payload sizes128/224/208/36/256/8; no serialized-width expansion occurred.

Native CG release now clears every released per-client pointer, including IK and fake entities, after invoking real Hunk_UserFree/IK publication owners. Map entity initialization clears512*sizeof(fake_centity_s), covering442368 actual native bytes instead of the original415872 Win32 bytes. This prevents stale pointer-bearing tail records on reuse. Windows retains its existing clear/publication behavior.

Lifetime audit: all three free companions dispatch through Hunk_UserFree, which reads the HunkUser scheme even if its default individual free is physically a no-op. Thus the borrowed arena owner must remain alive until FX/CG/SV releases finish. Root separately owns moving borrowed hunk retirement/destruction before PMem_Free and after these releases; this report does not claim that root change was executed here.

Evidence:

- `build-android-audit/native_registry_closure/client-companions-layout.cpp`, `.s`, `-values.log`: actual ARM64 sizeof/alignof constants, no physical execution.
- `cg_main_mp-memory-native.log`, `sv_init_mp-memory-native.log`, `fx_system-memory-native.log`: whole native translation units compile exit0.
- `build-android-audit/takeover_client_companion_memory_windows1/{x86,x64}/build-result.json`: all three whole normal Windows translation units pass once.
- No production full build, tablet run, asset transfer, GPU fixture or claim of Zombies memory initialization was made by this batch. Root's next production map load supplies the actual integration proof.

Frozen SHA256:

- cg_main_mp.cpp: 4C1042FF56E70FFE1E9ED99BF458F0726FB6F804E412E949FB1F930D315357D0
- sv_init_mp.cpp: 562E953EE0969AD8852E022902A698EFD39F3AC4EA63455E8DB67868C3BCC342

Integration requires existing production TUs only; no new header, source, CMake or flag. Hunk tracking support and primary client arena ownership are assigned to other owners.
