#pragma once

// FastFile 32→64 stream-time expansion helpers (BO1_PORT_X64).
// Retail FFs store Win32 layouts; these read the 32-bit view from the file
// stream and write native structs into XBlock memory.

#include <cstdint>

#if defined(BO1_PORT_X64)

struct XAsset;
struct XAssetList;
struct StringTable;
struct StringTableCell;

// Zero-extend ordinary FF offsets; sign-extend -1 / -2 sentinels so existing
// `(T *)-1` / `(unsigned)-2` checks keep working on x64.
inline uintptr_t DB_ExpandPtr32(uint32_t p32)
{
    if (p32 == 0)
        return 0;
    if (p32 == 0xFFFFFFFFu || p32 == 0xFFFFFFFEu)
        return static_cast<uintptr_t>(static_cast<intptr_t>(static_cast<int32_t>(p32)));
    return static_cast<uintptr_t>(p32);
}

// XAssetList disk image is 16 bytes (Win32).
void DB_ExpandLoad_XAssetList(XAssetList *dst);

// Packed Win32 XAsset[] (8 bytes each) → native XAsset[] at stream pos.
void DB_ExpandLoad_XAssetArray(XAsset *dst, int count);

// Packed const char* [count] (4 bytes each) → native pointer array at stream pos.
void DB_ExpandLoad_Ptr32Array(void *nativeDst, int count);

// StringTable disk image is 20 bytes; cells are 8 bytes each.
void DB_ExpandLoad_StringTable(StringTable *dst);
void DB_ExpandLoad_StringTableCellArray(StringTableCell *dst, int count);

// GfxImage disk image is 52 bytes (Win32); native is larger (pointers widen).
struct GfxImage;
void DB_ExpandLoad_GfxImage(GfxImage *dst);

// Material leaf + child tables (Win32 → native).
struct Material;
struct MaterialTextureDef;
struct MaterialTechniqueSet;
struct MaterialTechnique;
struct MaterialPass;
struct MaterialShaderArgument;
struct MaterialVertexShader;
struct MaterialPixelShader;
struct MaterialVertexDeclaration;
struct water_t;

void DB_ExpandLoad_Material(Material *dst);
void DB_ExpandLoad_MaterialTextureDefArray(MaterialTextureDef *dst, int count);
void DB_ExpandLoad_MaterialTechniqueSet(MaterialTechniqueSet *dst);
void DB_ExpandLoad_MaterialTechnique(MaterialTechnique *dst); // header through passArray
void DB_ExpandLoad_MaterialPassArray(MaterialPass *dst, int count);
void DB_ExpandLoad_MaterialShaderArgumentArray(MaterialShaderArgument *dst, int count);
void DB_ExpandLoad_MaterialVertexShader(MaterialVertexShader *dst);
void DB_ExpandLoad_MaterialPixelShader(MaterialPixelShader *dst);
void DB_ExpandLoad_MaterialVertexDeclaration(MaterialVertexDeclaration *dst);
void DB_ExpandLoad_water_t(water_t *dst);

// XModel leaf + child tables (Win32 → native).
struct XModel;
struct XSurface;
struct XRigidVertList;
struct XSurfaceCollisionTree;
struct XModelCollSurf_s;
struct Collmap;
struct PhysGeomList;
struct PhysGeomInfo;
struct BrushWrapper;
struct cbrushside_t;
struct PhysPreset;
struct PhysConstraints;
struct PhysConstraint;

void DB_ExpandLoad_XModel(XModel *dst);
void DB_ExpandLoad_XSurfaceArray(XSurface *dst, int count);
void DB_ExpandLoad_XRigidVertListArray(XRigidVertList *dst, int count);
void DB_ExpandLoad_XSurfaceCollisionTree(XSurfaceCollisionTree *dst);
void DB_ExpandLoad_XModelCollSurfArray(XModelCollSurf_s *dst, int count);
void DB_ExpandLoad_CollmapArray(Collmap *dst, int count);
void DB_ExpandLoad_PhysGeomList(PhysGeomList *dst);
void DB_ExpandLoad_PhysGeomInfoArray(PhysGeomInfo *dst, int count);
void DB_ExpandLoad_BrushWrapper(BrushWrapper *dst);
void DB_ExpandLoad_cbrushside_tArray(cbrushside_t *dst, int count);
void DB_ExpandLoad_PhysPreset(PhysPreset *dst);
void DB_ExpandLoad_PhysConstraints(PhysConstraints *dst);
void DB_ExpandLoad_PhysConstraintArray(PhysConstraint *dst, int count);

// clipMap / map leaf + child tables (Win32 → native).
struct clipMap_t;
struct cStaticModel_s;
struct cNode_t;
struct cLeafBrushNode_s;
struct CollisionPartition;
struct cbrush_t;
struct MapEnts;
struct DynEntityDef;
struct rope_t;

void DB_ExpandLoad_clipMap_t(clipMap_t *dst);
void DB_ExpandLoad_cStaticModel_tArray(cStaticModel_s *dst, int count);
void DB_ExpandLoad_cNode_tArray(cNode_t *dst, int count);
void DB_ExpandLoad_cLeafBrushNode_tArray(cLeafBrushNode_s *dst, int count);
void DB_ExpandLoad_CollisionPartitionArray(CollisionPartition *dst, int count);
void DB_ExpandLoad_cbrush_tArray(cbrush_t *dst, int count);
void DB_ExpandLoad_MapEnts(MapEnts *dst);
void DB_ExpandLoad_DynEntityDefArray(DynEntityDef *dst, int count);
void DB_ExpandLoad_rope_tArray(rope_t *dst, int count);

// GfxWorld leaf + child tables (Win32 → native).
struct GfxWorld;
struct GfxCell;
struct GfxAabbTree;
struct GfxPortal;
struct GfxSurface;
struct GfxStaticModelDrawInst;
struct GfxLightmapArray;
struct GfxReflectionProbe;
struct MaterialMemory;
struct GfxShadowGeometry;
struct GfxLightRegion;
struct GfxLightRegionHull;
struct sunflare_t;
struct GfxLight;

void DB_ExpandLoad_GfxWorld(GfxWorld *dst);
void DB_ExpandLoad_GfxCellArray(GfxCell *dst, int count);
void DB_ExpandLoad_GfxAabbTreeArray(GfxAabbTree *dst, int count);
void DB_ExpandLoad_GfxPortalArray(GfxPortal *dst, int count);
void DB_ExpandLoad_GfxSurfaceArray(GfxSurface *dst, int count);
void DB_ExpandLoad_GfxStaticModelDrawInstArray(GfxStaticModelDrawInst *dst, int count);
void DB_ExpandLoad_GfxLightmapArrayArray(GfxLightmapArray *dst, int count);
void DB_ExpandLoad_GfxReflectionProbeArray(GfxReflectionProbe *dst, int count);
void DB_ExpandLoad_MaterialMemoryArray(MaterialMemory *dst, int count);
void DB_ExpandLoad_GfxShadowGeometryArray(GfxShadowGeometry *dst, int count);
void DB_ExpandLoad_GfxLightRegionArray(GfxLightRegion *dst, int count);
void DB_ExpandLoad_GfxLightRegionHullArray(GfxLightRegionHull *dst, int count);
void DB_ExpandLoad_sunflare_t(sunflare_t *dst);
void DB_ExpandLoad_GfxLight(GfxLight *dst);


// Zone leaves after GfxWorld (Win32 → native).
struct ComWorld;
struct ComPrimaryLight;
struct ComBurnableCell;
struct GameWorldSp;
struct GameWorldMp;
struct pathnode_t;
struct pathnode_tree_t;
struct pathnode_tree_nodes_t;
struct Glasses;
struct Glass;
struct GlassDef;
struct GfxLightDef;
struct Font_s;
struct LocalizeEntry;
struct RawFile;

void DB_ExpandLoad_ComWorld(ComWorld *dst);
void DB_ExpandLoad_ComPrimaryLightArray(ComPrimaryLight *dst, int count);
void DB_ExpandLoad_ComBurnableCellArray(ComBurnableCell *dst, int count);
void DB_ExpandLoad_GameWorldSp(GameWorldSp *dst);
void DB_ExpandLoad_GameWorldMp(GameWorldMp *dst);
void DB_ExpandLoad_pathnode_tArray(pathnode_t *dst, int count);
void DB_ExpandLoad_pathnode_tree_tArray(pathnode_tree_t *dst, int count);
void DB_ExpandLoad_pathnode_tree_nodes_t(pathnode_tree_nodes_t *dst);
void DB_ExpandLoad_Glasses(Glasses *dst);
void DB_ExpandLoad_GlassArray(Glass *dst, int count);
void DB_ExpandLoad_GlassDef(GlassDef *dst);
void DB_ExpandLoad_GfxLightDef(GfxLightDef *dst);
void DB_ExpandLoad_Font(Font_s *dst);
void DB_ExpandLoad_LocalizeEntry(LocalizeEntry *dst);
void DB_ExpandLoad_RawFile(RawFile *dst);


struct XAnimParts;
struct XAnimDeltaPart;
struct XAnimPartTrans;
struct XAnimDeltaPartQuat;
struct XAnimPartTransFrames;
struct XAnimDeltaPartQuatDataFrames;
struct SndBank;
struct SndPatch;
struct snd_alias_list_t;
struct snd_alias_t;
struct SoundFile;
struct snd_asset;
struct LoadedSound;
struct StreamedSound;
struct PrimedSound;
struct FxEffectDef;
struct FxElemDef;
struct FxTrailDef;
struct FxElemMarkVisuals;
struct DestructibleDef;
struct DestructiblePiece;
struct DestructibleStage;

void DB_ExpandLoad_XAnimParts(XAnimParts *dst);
void DB_ExpandLoad_XAnimDeltaPart(XAnimDeltaPart *dst);
void DB_ExpandLoad_XAnimPartTrans(XAnimPartTrans *dst);
void DB_ExpandLoad_XAnimDeltaPartQuat(XAnimDeltaPartQuat *dst);
void DB_ExpandLoad_XAnimPartTransFrames(XAnimPartTransFrames *dst);
void DB_ExpandLoad_XAnimDeltaPartQuatDataFrames(XAnimDeltaPartQuatDataFrames *dst);
void DB_ExpandLoad_SndBank(SndBank *dst);
void DB_ExpandLoad_SndPatch(SndPatch *dst);
void DB_ExpandLoad_snd_alias_list_t(snd_alias_list_t *dst);
void DB_ExpandLoad_snd_alias_list_tArray(snd_alias_list_t *dst, int count);
void DB_ExpandLoad_snd_alias_t(snd_alias_t *dst);
void DB_ExpandLoad_snd_alias_tArray(snd_alias_t *dst, int count);
void DB_ExpandLoad_SoundFile(SoundFile *dst);
void DB_ExpandLoad_SoundFileArray(SoundFile *dst, int count);
void DB_ExpandLoad_snd_asset(snd_asset *dst);
void DB_ExpandLoad_LoadedSound(LoadedSound *dst);
void DB_ExpandLoad_StreamedSound(StreamedSound *dst);
void DB_ExpandLoad_PrimedSound(PrimedSound *dst);
void DB_ExpandLoad_FxEffectDef(FxEffectDef *dst);
void DB_ExpandLoad_FxElemDef(FxElemDef *dst);
void DB_ExpandLoad_FxElemDefArray(FxElemDef *dst, int count);
void DB_ExpandLoad_FxTrailDef(FxTrailDef *dst);
void DB_ExpandLoad_FxElemMarkVisuals(FxElemMarkVisuals *dst);
void DB_ExpandLoad_FxElemMarkVisualsArray(FxElemMarkVisuals *dst, int count);
void DB_ExpandLoad_DestructibleDef(DestructibleDef *dst);
void DB_ExpandLoad_DestructiblePieceArray(DestructiblePiece *dst, int count);
void DB_ExpandLoad_DestructibleStageArray(DestructibleStage *dst, int count);


struct WeaponDef;
struct WeaponVariantDef;
struct ExpressionStatement;
struct windowDef_t;
struct MenuList;
struct menuDef_t;
struct itemDef_s;
struct MenuCell;
struct MenuRow;
struct GenericEventHandler;
struct ItemKeyHandler;
struct GenericEventScript;
struct SndDriverGlobals;
struct PackIndex;
struct XGlobals;
struct ddlMemberDef_t;
struct ddlStructDef_t;
struct ddlEnumDef_t;
struct ddlDef_t;
struct ddlRoot_t;
struct EmblemCategory;
struct EmblemIcon;
struct EmblemBackground;
struct EmblemSet;
struct FxImpactTable;
struct FxImpactEntry;

void DB_ExpandLoad_WeaponDef(WeaponDef *dst);
void DB_ExpandLoad_WeaponVariantDef(WeaponVariantDef *dst);
void DB_ExpandLoad_ExpressionStatement(ExpressionStatement *dst);
struct expressionRpn;
void DB_ExpandLoad_expressionRpnArray(expressionRpn *dst, int count);
void DB_ExpandLoad_windowDef_t(windowDef_t *dst);
void DB_ExpandLoad_MenuList(MenuList *dst);
void DB_ExpandLoad_menuDef_t(menuDef_t *dst);
void DB_ExpandLoad_itemDef_t(itemDef_s *dst);
void DB_ExpandLoad_MenuCell(MenuCell *dst);
void DB_ExpandLoad_MenuCellArray(MenuCell *dst, int count);
void DB_ExpandLoad_MenuRow(MenuRow *dst);
void DB_ExpandLoad_MenuRowArray(MenuRow *dst, int count);
struct ScriptCondition;
void DB_ExpandLoad_ScriptCondition(ScriptCondition *dst);
void DB_ExpandLoad_GenericEventHandler(GenericEventHandler *dst);
void DB_ExpandLoad_ItemKeyHandler(ItemKeyHandler *dst);
void DB_ExpandLoad_GenericEventScript(GenericEventScript *dst);
void DB_ExpandLoad_SndDriverGlobals(SndDriverGlobals *dst);
void DB_ExpandLoad_PackIndex(PackIndex *dst);
void DB_ExpandLoad_XGlobals(XGlobals *dst);
void DB_ExpandLoad_ddlMemberDef_tArray(ddlMemberDef_t *dst, int count);
void DB_ExpandLoad_ddlStructDef_t(ddlStructDef_t *dst);
void DB_ExpandLoad_ddlStructDef_tArray(ddlStructDef_t *dst, int count);
void DB_ExpandLoad_ddlEnumDef_t(ddlEnumDef_t *dst);
void DB_ExpandLoad_ddlEnumDef_tArray(ddlEnumDef_t *dst, int count);
void DB_ExpandLoad_ddlDef_t(ddlDef_t *dst);
void DB_ExpandLoad_ddlRoot_t(ddlRoot_t *dst);
void DB_ExpandLoad_EmblemCategory(EmblemCategory *dst);
void DB_ExpandLoad_EmblemCategoryArray(EmblemCategory *dst, int count);
void DB_ExpandLoad_EmblemIcon(EmblemIcon *dst);
void DB_ExpandLoad_EmblemIconArray(EmblemIcon *dst, int count);
void DB_ExpandLoad_EmblemBackground(EmblemBackground *dst);
void DB_ExpandLoad_EmblemBackgroundArray(EmblemBackground *dst, int count);
void DB_ExpandLoad_EmblemSet(EmblemSet *dst);
void DB_ExpandLoad_FxImpactTable(FxImpactTable *dst);
void DB_ExpandLoad_FxImpactEntry(FxImpactEntry *dst);
void DB_ExpandLoad_FxImpactEntryArray(FxImpactEntry *dst, int count);


struct rectData_s;
struct imageDef_s;
struct ownerDrawDef_s;
struct textDef_s;
struct focusItemDef_s;
struct animParamsDef_t;
struct UIAnimInfo;
struct listBoxDef_s;
struct multiDef_s;

void DB_ExpandLoad_rectData_t(rectData_s *dst);
void DB_ExpandLoad_imageDef_t(imageDef_s *dst);
void DB_ExpandLoad_ownerDrawDef_t(ownerDrawDef_s *dst);
void DB_ExpandLoad_textDef_t(textDef_s *dst);
void DB_ExpandLoad_focusItemDef_t(focusItemDef_s *dst);
void DB_ExpandLoad_animParamsDef_t(animParamsDef_t *dst);
void DB_ExpandLoad_UIAnimInfo(UIAnimInfo *dst);
void DB_ExpandLoad_listBoxDef_t(listBoxDef_s *dst);
void DB_ExpandLoad_multiDef_t(multiDef_s *dst);
struct enumDvarDef_s;
void DB_ExpandLoad_enumDvarDef_t(enumDvarDef_s *dst);

struct flameTable;
void DB_ExpandLoad_flameTable(flameTable *dst);

#endif // BO1_PORT_X64
