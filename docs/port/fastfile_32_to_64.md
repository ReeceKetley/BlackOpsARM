# FastFile 32→64 Expansion Design

Status: **Zone + content core + Weapon/Menu + remaining content leaves landed** (2026-10-09). x64 `BO1Zombies.exe` links; stream-time expansion covers the Zombies XAsset set through **Weapon**, **Menu*** (incl. nested UI typeData), **SndDriverGlobals**, **PackIndex**, **XGlobals**, **DDL**, **EmblemSet**, and **FxImpactTable**. Retail FFs are still not playable on x64 until a full map load is smoked.

## Problem

`DB_ConvertOffsetToPointer` / `DB_ConvertOffsetToAlias` write `uintptr_t` into pointer slots. That is necessary and not sufficient:

1. **On-disk XAsset blobs** were authored for 32-bit `sizeof` / `offsetof` (pointers are 4 bytes, alignment often 4).
2. **Runtime C++ structs** grow when `void *` / handles become 8 bytes; embedded arrays of pointers shift every following field.
3. **Script / save / net field tables** often hard-code byte offsets into those structs.

Loading a retail `.ff` into a naïve x64 process therefore mis-parses asset headers long before gameplay runs.

## Goals

1. Keep **Win32** as the playable path until expansion is proven (Godot host can still be Win32-first).
2. On **x64 / ARM64**, expand 32-bit FastFile images into native runtime layouts **once per load**, then run the existing gameplay code against native pointers.
3. Prefer **one expansion layer** at the DB/stream boundary over scattering `#ifdef` through every asset type.

## Non-goals

- Rewriting every asset offline into a new “x64 FF” format as a hard requirement (optional later for ship size).
- Making SP savegames / script debugger layouts bit-identical to 32-bit (those asserts are already gated under `BO1_PORT_X64`).

## Approach

### A. Stream-time expansion (recommended)

At the point where the loader currently does `Load_Stream` of a Win32-sized blob:

1. Read the **32-bit view** from the FF (`DB_LoadXFileData` with the Win32 byte count).
2. Write a **native-sized** object at the current XBlock stream pos (`DB_IncStreamPos(sizeof(native))`).
3. Copy scalars; run pointer slots through `DB_ExpandPtr32` (zero-extend offsets; sign-extend `-1` / `-2` sentinels).
4. Existing `Load_XString` / child loaders / `DB_ConvertOffsetTo*` then run on the native object.

### B. Field maps

| Field | kind | notes |
|---|---|---|
| scalars | `copy` | typed assign |
| pointers | `ptr` | `DB_ExpandPtr32` then existing fixup |
| inline arrays of ptr | `ptr_array` | `DB_ExpandLoad_Ptr32Array` |
| opaque blobs | `blob` | size from header; no pointer widen |

### C. Zone / dependency order

Preserve existing `DB_LoadXAssets` ordering. Expand an asset only after its dependencies are expanded so child pointers can be wired to native addresses.

### D. Verification

1. **Struct size gates**: keep Win32 `static_assert(sizeof…)`; on x64 assert only fields that must stay fixed.
2. **Golden loads**: headless load of `zombie_pentagon` (and later BO2 maps) comparing key counts against Win32 — blocked until more types expand.
3. **Build gates**: x64 + Win32 Release both link after expander changes; Win32 headless still prints `platform: ready`.

## Landed (2026-10-09)

| Piece | Location |
|---|---|
| `DB_ExpandPtr32` + load helpers | `src/database/db_expand.hpp` / `db_expand.cpp` |
| `DB_AllocStreamPos` pointer-width align | `db_stream.cpp` |
| `DB_InsertPointer` uses `sizeof(void*)` | `db_stream.cpp` |
| `XAssetList` 16→native | `Load_XAssetListCustom` |
| `XAsset[]` 8-byte packed → native | `Load_XAssetArrayCustom` |
| `const char*` arrays (script strings) | `Load_TempStringArray` |
| `StringTable` 20→native + cells 8→native | `Load_StringTable` / `Load_StringTableCellArray` |
| `GfxImage` 52→native | `DB_ExpandLoad_GfxImage` / `Load_GfxImage` |
| `GfxImage*` / raw texture ptr arrays | `Load_GfxImagePtrArray` / `Load_GfxRawTextureArray` |
| `Material` 192→~208 + texture/techset/pass/shader/water | `DB_ExpandLoad_Material*` / `Load_Material*` |
| `XModel` 252→~328 + surfaces/coll/phys | `DB_ExpandLoad_XModel*` / `Load_XModel*` |
| `clipMap_t` 332→native + children | `DB_ExpandLoad_clipMap_t*` / `Load_clipMap_t*` |
| `GfxWorld` 1084→~1672 + children | `DB_ExpandLoad_GfxWorld*` / `Load_GfxWorld*` |
| Zone remainder (ComWorld / GameWorld / Glasses / LightDef / Font / Localize / RawFile) | `DB_ExpandLoad_*` / matching `Load_*` |
| Content core (XAnimParts / Sound / FX / Destructible) | `DB_ExpandLoad_*` / matching `Load_*` |
| Weapon / Menu / remaining content leaves | `DB_ExpandLoad_*` / matching `Load_*` |

### XModel leaf (x64 MSVC sizes)

| Disk (Win32) | Native | Expander |
|---|---|---|
| XModel 252 | ~328 | `DB_ExpandLoad_XModel` |
| XSurface 68 | ~104 | `DB_ExpandLoad_XSurfaceArray` |
| XRigidVertList 12 | 16 | `DB_ExpandLoad_XRigidVertListArray` |
| XSurfaceCollisionTree 40 | 56 | `DB_ExpandLoad_XSurfaceCollisionTree` |
| XModelCollSurf 44 | 48 | `DB_ExpandLoad_XModelCollSurfArray` |
| Collmap 4 | 8 | `DB_ExpandLoad_CollmapArray` |
| PhysGeomList 12 | 24 | `DB_ExpandLoad_PhysGeomList` |
| PhysGeomInfo 68 | 72 | `DB_ExpandLoad_PhysGeomInfoArray` |
| BrushWrapper 96 | 112 | `DB_ExpandLoad_BrushWrapper` |
| cbrushside_t 12 | 16 | `DB_ExpandLoad_cbrushside_tArray` |
| PhysPreset 84 | 96 | `DB_ExpandLoad_PhysPreset` |
| PhysConstraints 2696 | native | `DB_ExpandLoad_PhysConstraints` |
| PhysConstraint 168 | 192 | `DB_ExpandLoad_PhysConstraintArray` |

`XModelLodInfo`, `XBoneInfo`, `DObjAnimMat`, `GfxPackedVertex`, and collision leaf/node blobs are pointer-free and stream at Win32 sizes. `MaterialHandle` arrays use `DB_ExpandLoad_Ptr32Array`.

### clipMap leaf (x64 MSVC sizes)

| Disk (Win32) | Native | Expander |
|---|---|---|
| clipMap_t 332 | ~568 | `DB_ExpandLoad_clipMap_t` |
| cStaticModel 80 | 88 | `DB_ExpandLoad_cStaticModel_tArray` |
| cNode 8 | 16 | `DB_ExpandLoad_cNode_tArray` |
| cLeafBrushNode 20 | 24 | `DB_ExpandLoad_cLeafBrushNode_tArray` |
| CollisionPartition 20 | 24 | `DB_ExpandLoad_CollisionPartitionArray` |
| cbrush_t 96 | 104 | `DB_ExpandLoad_cbrush_tArray` |
| MapEnts 12 | 24 | `DB_ExpandLoad_MapEnts` |
| DynEntityDef 84 | 112 | `DB_ExpandLoad_DynEntityDefArray` |
| rope_t 3188 | native | `DB_ExpandLoad_rope_tArray` |

`cplane_s`, `cLeaf_s`, `cmodel_t`, `dmaterial_t`, `CollisionBorder`/`AabbTree`, and dynent pose/client/server/coll blobs stay pointer-free at Win32 sizes. `cbrushside_t` already expands from the XModel phys path.

### GfxWorld leaf (x64 MSVC sizes)

| Disk (Win32) | Native | Expander |
|---|---|---|
| GfxWorld 1084 | ~1672 | `DB_ExpandLoad_GfxWorld` |
| GfxWorldDraw 192 | ~376 | (embedded in header) |
| GfxWorldDpvsStatic 112 | ~184 | (embedded) |
| GfxWorldDpvsDynamic 48 | ~80 | (embedded) |
| GfxLightGrid 56 | ~88 | (embedded) |
| sunflare_t 96 | ~112 | `DB_ExpandLoad_sunflare_t` |
| GfxCell 56 | ~88 | `DB_ExpandLoad_GfxCellArray` |
| GfxAabbTree 40 | ~48 | `DB_ExpandLoad_GfxAabbTreeArray` |
| GfxPortal 68 | ~96 | `DB_ExpandLoad_GfxPortalArray` |
| GfxSurface 80 | ~88 | `DB_ExpandLoad_GfxSurfaceArray` |
| GfxStaticModelDrawInst 76 | ~80 | `DB_ExpandLoad_GfxStaticModelDrawInstArray` |
| GfxLightmapArray 12 | ~24 | `DB_ExpandLoad_GfxLightmapArrayArray` |
| GfxReflectionProbe 24 | ~40 | `DB_ExpandLoad_GfxReflectionProbeArray` |
| MaterialMemory 8 | ~16 | `DB_ExpandLoad_MaterialMemoryArray` |
| GfxShadowGeometry 12 | ~24 | `DB_ExpandLoad_GfxShadowGeometryArray` |
| GfxLightRegion 8 | ~16 | `DB_ExpandLoad_GfxLightRegionArray` |
| GfxLightRegionHull 80 | ~88 | `DB_ExpandLoad_GfxLightRegionHullArray` |
| GfxLight 368 | 368 | `DB_ExpandLoad_GfxLight` (def ptr at +0x160) |

Pointer-free world blobs (coronas, volumes, lod chains/infos, brush models, outdoor bounds, hero lights, static model insts, cull groups, drawsurfs, light-grid entries/colors) still stream at Win32 sizes. `terrainScorchImages[31]` and raw texture ptr arrays reuse `DB_ExpandLoad_Ptr32Array`.

### Remaining zone leaf (x64 MSVC sizes)

T5 has no separate FxWorld/GlassWorld XAsset types; glasses live under `ASSET_TYPE_GLASSES`.

| Disk (Win32) | Native | Expander |
|---|---|---|
| ComWorld 64 | ~88 | `DB_ExpandLoad_ComWorld` |
| ComPrimaryLight 220 | ~224 | `DB_ExpandLoad_ComPrimaryLightArray` |
| ComBurnableCell 12 | ~16 | `DB_ExpandLoad_ComBurnableCellArray` |
| GameWorldSp/Mp 44 | ~88 | `DB_ExpandLoad_GameWorldSp` / `Mp` |
| PathData 40 | ~80 | embedded via GameWorld (`DB_Expand_PathData`) |
| pathnode_t 128 | ~152 | `DB_ExpandLoad_pathnode_tArray` |
| pathnode_tree 16 | ~24 | `DB_ExpandLoad_pathnode_tree_tArray` |
| pathnode_tree_nodes 8 | ~16 | `DB_ExpandLoad_pathnode_tree_nodes_t` |
| Glasses 56 | ~72 | `DB_ExpandLoad_Glasses` |
| Glass 124 | ~136 | `DB_ExpandLoad_GlassArray` |
| GlassDef 60 | ~96 | `DB_ExpandLoad_GlassDef` |
| GfxLightDef 16 | ~32 | `DB_ExpandLoad_GfxLightDef` |
| Font_s 24 | ~40 | `DB_ExpandLoad_Font` |
| LocalizeEntry 8 | ~16 | `DB_ExpandLoad_LocalizeEntry` |
| RawFile 12 | ~24 | `DB_ExpandLoad_RawFile` |

### Content core leaf (x64 MSVC sizes)

| Disk (Win32) | Native | Expander |
|---|---|---|
| XAnimParts 104 | ~152 | `DB_ExpandLoad_XAnimParts` |
| XAnimDeltaPart 8 | ~16 | `DB_ExpandLoad_XAnimDeltaPart` |
| XAnimPartTransFrames hdr 28 | ~32 | `DB_ExpandLoad_XAnimPartTransFrames` |
| XAnimDeltaPartQuatDataFrames hdr 4 | ~8 | `DB_ExpandLoad_XAnimDeltaPartQuatDataFrames` |
| SndBank 40 | ~72 | `DB_ExpandLoad_SndBank` |
| SndPatch 20 | ~40 | `DB_ExpandLoad_SndPatch` |
| snd_alias_list_t 20 | ~32 | `DB_ExpandLoad_snd_alias_list_t*` |
| snd_alias_t 84 | ~104 | `DB_ExpandLoad_snd_alias_t*` |
| SoundFile 8 | ~16 | `DB_ExpandLoad_SoundFile*` |
| snd_asset 56 | ~72 | `DB_ExpandLoad_snd_asset` |
| LoadedSound 60 | ~80 | `DB_ExpandLoad_LoadedSound` |
| StreamedSound 8 | ~16 | `DB_ExpandLoad_StreamedSound` |
| PrimedSound 12 | ~24 | `DB_ExpandLoad_PrimedSound` |
| FxEffectDef 60 | ~72 | `DB_ExpandLoad_FxEffectDef` |
| FxElemDef 292 | ~336 | `DB_ExpandLoad_FxElemDef*` |
| FxTrailDef 28 | ~40 | `DB_ExpandLoad_FxTrailDef` |
| FxElemMarkVisuals 8 | ~16 | `DB_ExpandLoad_FxElemMarkVisuals*` |
| DestructibleDef 24 | ~48 | `DB_ExpandLoad_DestructibleDef` |
| DestructiblePiece 312 | ~496 | `DB_ExpandLoad_DestructiblePieceArray` |
| DestructibleStage 48 | ~80 | `DB_ExpandLoad_DestructibleStageArray` |

Inventory (Zombies FF path): `DB_LoadXAssets` → `Load_XAssetListCustom` → `Load_XAssetArrayCustom` → `Load_XAssetHeader` switch. Full type list in `database.h` (`ASSET_TYPE_COUNT` = 0x2B).

## Fallback

Until expansion covers a map’s asset set, **run gameplay under Win32** and treat x64 as compile/link + incremental expander work.

### Weapon / Menu / remaining content leaf (x64 MSVC sizes)

| Disk (Win32) | Native | Expander |
|---|---|---|
| WeaponDef 2056 | ~2664 | `DB_ExpandLoad_WeaponDef` |
| WeaponVariantDef 228 | ~288 | `DB_ExpandLoad_WeaponVariantDef` |
| MenuList 12 | ~24 | `DB_ExpandLoad_MenuList` |
| menuDef_t 400 | ~456 | `DB_ExpandLoad_menuDef_t` |
| itemDef_s 272 | ~336 | `DB_ExpandLoad_itemDef_t` |
| windowDef_t 164 | ~176 | `DB_ExpandLoad_windowDef_t` |
| ExpressionStatement 16 | ~24 | `DB_ExpandLoad_ExpressionStatement` |
| listBoxDef_s 668 | ~688 | `DB_ExpandLoad_listBoxDef_t` |
| multiDef_s 396 | ~656 | `DB_ExpandLoad_multiDef_t` |
| textDef / focusItem / animParams / UIAnimInfo / rectData / image / ownerDraw | Win32→native | matching `DB_ExpandLoad_*` |
| SndDriverGlobals 52 | ~104 | `DB_ExpandLoad_SndDriverGlobals` |
| PackIndex 28 | ~40 | `DB_ExpandLoad_PackIndex` |
| XGlobals 40 | ~48 | `DB_ExpandLoad_XGlobals` |
| ddlRoot / ddlDef / struct / enum / member | Win32→native | matching `DB_ExpandLoad_ddl*` |
| EmblemSet / Icon / Background / Category | Win32→native | matching `DB_ExpandLoad_Emblem*` |
| FxImpactTable 8 / Entry 140 | ~16 / ~280 | `DB_ExpandLoad_FxImpact*` |

## x64 smoke progress (2026-10-09)

Headless via `tools/headless.ps1 -BuildDir build-x64\Release -RunDir build\run\x64smoke`.

| Gate | Result |
|---|---|
| `Win_RegisterClass` (`cbSize` was hardcoded 48) | Fixed (`sizeof(wce)`) |
| Dvar string / enum pointer overlays | Fixed (`.string` / `enumeration.strings[]`) |
| `DB_AllocXAssetEntry` freelist | Fixed (use `.next`, not `asset.type`) |
| `DB_LocalizeEntryGetName` | Fixed (`header->localize->name`) |
| `HunkUserDefault` pos/end/locked | Widened to `psize_int`; Init/Alloc use struct fields |
| `Sys_ListFiles` find handles | `intptr_t` (was truncating `_findfirst64i32`) |
| IWD `qsort` / `searchpath_s` / `FS_LoadZipFile` | Pointer-width sizes; `iwd_t` field writes |
| `db_hashTable` entry index | Was `>> 4` (16-byte Win32 stride); now `entry - pool` |
| `DB_GetXAssetTypeSize` RawFile/StringTable/Localize/Fx/Material | Return `sizeof(T)` on x64 (clone was truncating) |
| `en_code_pre_gfx_mp` / `code_pre_gfx_mp` load | **Loaded** |
| `FS_Startup` | **OK** (51907 IWD files; path listing works) |
| `default_mp.cfg` from FF | **Works** (no longer missing) |
| `en_code_post_gfx_mp` start | **Starts**; dies on FX default `misc/missing_fx` (garbage FX name) |
| `+loadzone zombie_pentagon` finish | Not reached yet |

Win32 headless still prints `platform: ready host=win32 ...` after these changes.

## Next

1. **FX blocker root cause (diagnosed):** inline FX names (`name=-1`) expand fine
   (`weapon/muzzleflashes/fx_pistol_flash_base`, etc.). Failure is
   `Load_FxEffectDefRef` → `ConvertOffsetToPointer` on baked offsets like
   `0x800B7E9D` (`effectOnImpact` on `fx_rifle` elems). Native-sized stream
   commits (`FxEffectDef` 60→72, `FxElemDef` 292→336, `InsertPointer` 4→8)
   drift XBlock layout vs Win32-cooked offsets, so the “name” lands on float
   sample bytes (`?>??RN%AU&…`). Need a Win32→native stream offset remap (or
   Win32-sized ghost cursor) before `DB_ConvertOffsetTo*`.
2. Finish `+loadzone zombie_pentagon` smoke; keep Win32 `platform: ready`.

## Related

- Pointer fixups: `src/database/db_stream_load.*`
- x64 build status: `docs/port/x64_attempt.md`
- Platform hosts: `docs/port/platform_interfaces.md`
