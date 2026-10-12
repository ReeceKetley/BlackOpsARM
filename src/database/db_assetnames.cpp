#if defined(BO1_STRINGTABLE_OWNER_GATE)
#include "db_asset_pool.h"
#include <cstring>
#if defined(BO1_ENTRY_ALLOC_GATE)
#include "db_entry_alloc_api.h"
#if defined(BO1_REGISTRY_LOOKUP_GATE)
#include "db_registry_lookup_api.h"
#if defined(BO1_REGISTRY_ADD_GATE)
#include "db_registry_add_api.h"
#if defined(BO1_REGISTRY_DEFAULT_GATE)
#include "db_registry_default_api.h"
#endif
#endif
#endif
#endif
#include <universal/assertive.h>
#else
#include <platform/bo1_native_width.h>
#include "db_assetnames.h"
#include "db_registry.h"

#if BO1_NATIVE_64BIT
#include <cstdio>
#endif

#include <gfx_d3d/r_material.h>
#include <gfx_d3d/fxprimitives.h>

#include <xanim/xmodel.h>
#include <universal/q_shared.h>

#include <physics/physpreset_load_obj.h>
#include <physics/physconstraints_load_obj.h>
#include <glass/glass.h>
#include "db_load.h"
#include <bgame/bg_weapons.h>
#include <bgame/bg_local.h>
#include <gfx_d3d/r_bsp_load_obj.h>
#include <bgame/bg_emblems.h>
#include <qcommon/com_bsp.h>
#include <gfx_d3d/r_extracam.h>
#include <stringed/stringed_hooks.h>
#include <universal/com_stringtable.h>
#include <ddl/ddl_api.h>
#include <ui/ui_shared.h>
#include <sound/snd_bank.h>
#include <game/g_bsp.h>
#include <qcommon/cm_load_obj.h>
#include <cgame/cg_effects_load_obj.h>

#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE)
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_ENTRY_ALLOC_GATE)
const char *(__cdecl *DB_XAssetGetNameHandler[43])(const XAssetHeader *) =
{
#if defined(BO1_ENTRY_ALLOC_GATE)
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
#if defined(BO1_TECHSET_GATE)
  DB_DDLGetName,
#else
  NULL,
#endif
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  DB_DDLGetName,
  DB_DDLGetName,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
#else
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_ImageGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  NULL,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_LocalizeEntryGetName,
  &DB_DDLGetName,
  NULL,
  NULL,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_DDLGetName,
  &DB_GetEmblemSetName
#endif
};
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE)


#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_ENTRY_ALLOC_GATE)
const char *g_assetNames[43] =
{
  "xmodelpieces",
  "physpreset",
  "physconstraints",
  "destructibledef",
  "xanim",
  "xmodel",
  "material",
  "techset",
  "image",
  "sound",
  "sound_patch",
  "col_map_sp",
  "col_map_mp",
  "com_map",
  "game_map_sp",
  "game_map_mp",
  "map_ents",
  "gfx_map",
  "lightdef",
  "ui_map",
  "font",
  "menufile",
  "menu",
  "localize",
  "weapon",
  "weapondef",
  "weaponvariant",
  "snddriverglobals",
  "fx",
  "impactfx",
  "aitype",
  "mptype",
  "mpbody",
  "mphead",
  "character",
  "xmodelalias",
  "rawfile",
  "stringtable",
  "packindex",
  "xGlobals",
  "ddl",
  "glasses",
  "emblemset"
};
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_DEFAULT_GATE)


void(__cdecl *DB_XAssetSetNameHandler[43])(XAssetHeader *, const char *) =
{
#if defined(BO1_REGISTRY_DEFAULT_GATE)
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  &DB_DDLSetname,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
#else
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  (void(*)(XAssetHeader*, const char*))&DB_ImageSetName,
  &DB_DDLSetname,
  NULL,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  NULL,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_LocalizeEntrySetName,
  &DB_DDLSetname,
  NULL,
  NULL,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  &DB_DDLSetname,
  NULL,
  NULL
#endif
};

#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_RAWFILE_OWNER_GATE)
#if !BO1_NATIVE_64BIT
static_assert(sizeof(RawFile) == 12);
#endif
int __cdecl DB_SizeofXAsset_RawFile_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(RawFile);
#else
    return 12;
#endif
}

#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE)


int __cdecl DB_SizeofXAsset_LocalizeEntry_()
{
    return (int)sizeof(LocalizeEntry);
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(PhysPreset) == 84);
#endif
int __cdecl DB_SizeofXAsset_PhysPreset_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(PhysPreset);
#else
    return 84;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(PhysConstraints) == 2696);
#endif
int __cdecl DB_SizeofXAsset_PhysConstraints_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(PhysConstraints);
#else
    return 2696;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(DestructibleDef) == 24);
#endif
//int __cdecl SV_GetMaxAttachCount()
int __cdecl DB_SizeofXAsset_DestructibleDef_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(DestructibleDef);
#else
    return 24;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(Font_s) == 24);
#endif
int __cdecl DB_SizeofXAsset_Font_s_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(Font_s);
#else
    return 24;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(XAnimParts) == 104);
#endif
//int __cdecl PM_MediumLandingForSurface()
int __cdecl DB_SizeofXAsset_XAnimParts_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(XAnimParts);
#else
    return 104;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(XModel) == 252);
#endif
int __cdecl DB_SizeofXAsset_XModel_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(XModel);
#else
    return 252;
#endif
}

// (for some reason the intellisense size is wrong unless it's right next to the struct definition)
int __cdecl DB_SizeofXAsset_Material_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(Material);
#else
    return 192;
#endif
}

#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_TECHSET_GATE)
#if !BO1_NATIVE_64BIT
static_assert(sizeof(MaterialTechniqueSet) == 528);
#endif
int __cdecl DB_SizeofXAsset_MaterialTechniqueSet_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(MaterialTechniqueSet);
#else
    return 528;
#endif
}
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE)

#if !BO1_NATIVE_64BIT
static_assert(sizeof(SndDriverGlobals) == 52);
#endif
int __cdecl DB_SizeofXAsset_SndDriverGlobals_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(SndDriverGlobals);
#else
    return 52;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(GfxImage) == 52);
#endif
int __cdecl DB_SizeofXAsset_GfxImage_()
{
    // Win32 table reused SndDriverGlobals size (==52 == sizeof(GfxImage)). Native GfxImage is larger on x64.
#if BO1_NATIVE_64BIT
    return (int)sizeof(GfxImage);
#else
    return 52;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(MenuList) == 12);
#endif
int __cdecl DB_SizeofXAsset_MenuList_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(MenuList);
#else
    return 12;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(XGlobals) == 40);
#endif
int __cdecl DB_SizeofXAsset_XGlobals_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(XGlobals);
#else
    return 40;
#endif
}


#endif
#if !BO1_NATIVE_64BIT
static_assert(sizeof(StringTable) == 20);
#endif
int __cdecl DB_SizeofXAsset_StringTable_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(StringTable);
#else
    return 20;
#endif
}
#if !defined(BO1_STRINGTABLE_OWNER_GATE)


#if !BO1_NATIVE_64BIT
static_assert(sizeof(clipMap_t) == 332);
#endif
int __cdecl DB_SizeofXAsset_clipMap_t_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(clipMap_t);
#else
    return 332;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(ComWorld) == 64);
#endif
int __cdecl DB_SizeofXAsset_ComWorld_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(ComWorld);
#else
    return 64;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(EmblemSet) == 44);
#endif
int __cdecl DB_SizeofXAsset_EmblemSet_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(EmblemSet);
#else
    return 44;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(GfxWorld) == 1084);
#endif
int __cdecl DB_SizeofXAsset_GfxWorld_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(GfxWorld);
#else
    return 1084;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(GfxLightDef) == 16);
#endif
int __cdecl DB_SizeofXAsset_GfxLightDef_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(GfxLightDef);
#else
    return 16;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(menuDef_t) == 400);
#endif
int __cdecl DB_SizeofXAsset_menuDef_t_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(menuDef_t);
#else
    return 400;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(XAnimTree_s) == 8);
#endif
//int __cdecl XAnimTreeSize()
//{
//    return 8;
//}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(ddlRoot_t) == 8);
#endif
int __cdecl DB_SizeofXAsset_ddlRoot_t_()
{
    // Win32 reused XAnimTreeSize() (==8 == sizeof(ddlRoot_t)). On x64 ddlRoot is two pointers.
    return (int)sizeof(ddlRoot_t);
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(WeaponVariantDef) == 228);
#endif
int __cdecl DB_SizeofXAsset_WeaponVariantDef_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(WeaponVariantDef);
#else
    return 228;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(FxEffectDef) == 60);
#endif
int __cdecl DB_SizeofXAsset_FxEffectDef_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(FxEffectDef);
#else
    return 60;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(PackIndex) == 28);
#endif
int __cdecl DB_SizeofXAsset_PackIndex_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(PackIndex);
#else
    return 28;
#endif
}

#if !BO1_NATIVE_64BIT
static_assert(sizeof(Glasses) == 56);
#endif
int __cdecl DB_SizeofXAsset_Glasses_()
{
#if BO1_NATIVE_64BIT
    return (int)sizeof(Glasses);
#else
    return 56;
#endif
}

// The retail Win32 table reused same-sized getters for different types (decompiler folded them). On x64 their sizes differ,
// so each type gets its own getter; the Win32 values are unchanged.
#if BO1_NATIVE_64BIT
#define BO1_ASSET_SIZE(T, win32) ((int)sizeof(T))
#else
#define BO1_ASSET_SIZE(T, win32) (win32)
#endif
int __cdecl DB_SizeofXAsset_XModelPieces_() { return BO1_ASSET_SIZE(XModelPieces, 12); }
int __cdecl DB_SizeofXAsset_SndBank_() { return BO1_ASSET_SIZE(SndBank, 40); }
int __cdecl DB_SizeofXAsset_SndPatch_() { return BO1_ASSET_SIZE(SndPatch, 20); }
int __cdecl DB_SizeofXAsset_GameWorldSp_() { return BO1_ASSET_SIZE(GameWorldSp, 44); }
int __cdecl DB_SizeofXAsset_GameWorldMp_() { return BO1_ASSET_SIZE(GameWorldMp, 44); }
int __cdecl DB_SizeofXAsset_MapEnts_() { return BO1_ASSET_SIZE(MapEnts, 12); }
int __cdecl DB_SizeofXAsset_FxImpactTable_() { return BO1_ASSET_SIZE(FxImpactTable, 8); }


#endif
int(__cdecl *DB_GetXAssetSizeHandler[43])() =
{
#if defined(BO1_STRINGTABLE_OWNER_GATE)
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
#if defined(BO1_TECHSET_GATE)
  &DB_SizeofXAsset_MaterialTechniqueSet_,
#else
  NULL,
#endif
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
#if defined(BO1_RAWFILE_OWNER_GATE)
  &DB_SizeofXAsset_RawFile_,
#else
  NULL,
#endif
#else
  &DB_SizeofXAsset_XModelPieces_,
  &DB_SizeofXAsset_PhysPreset_,
  &DB_SizeofXAsset_PhysConstraints_,
  &DB_SizeofXAsset_DestructibleDef_,
  //&PM_MediumLandingForSurface,
  DB_SizeofXAsset_XAnimParts_,
  &DB_SizeofXAsset_XModel_,
  &DB_SizeofXAsset_Material_,
  &DB_SizeofXAsset_MaterialTechniqueSet_,
  &DB_SizeofXAsset_GfxImage_,
  &DB_SizeofXAsset_SndBank_,
  &DB_SizeofXAsset_SndPatch_,
  &DB_SizeofXAsset_clipMap_t_,
  &DB_SizeofXAsset_clipMap_t_,
  &DB_SizeofXAsset_ComWorld_,
  &DB_SizeofXAsset_GameWorldSp_,
  &DB_SizeofXAsset_GameWorldMp_,
  &DB_SizeofXAsset_MapEnts_,
  &DB_SizeofXAsset_GfxWorld_,
  &DB_SizeofXAsset_GfxLightDef_,
  NULL,
  &DB_SizeofXAsset_Font_s_,
  &DB_SizeofXAsset_MenuList_,
  &DB_SizeofXAsset_menuDef_t_,
  &DB_SizeofXAsset_LocalizeEntry_,
  &DB_SizeofXAsset_WeaponVariantDef_,
  NULL,
  NULL,
  &DB_SizeofXAsset_SndDriverGlobals_,
  &DB_SizeofXAsset_FxEffectDef_,
  &DB_SizeofXAsset_FxImpactTable_,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  &DB_SizeofXAsset_RawFile_,
#endif
  &DB_SizeofXAsset_StringTable_,
#if !defined(BO1_STRINGTABLE_OWNER_GATE)
  &DB_SizeofXAsset_PackIndex_,
  &DB_SizeofXAsset_XGlobals_,
  &DB_SizeofXAsset_ddlRoot_t_,
  &DB_SizeofXAsset_Glasses_,
  &DB_SizeofXAsset_EmblemSet_
#endif
};
#if !defined(BO1_STRINGTABLE_OWNER_GATE)






const char *__cdecl DB_ImageGetName(const XAssetHeader *header)
{
    return header->image->name;
}

void __cdecl DB_ImageSetName(XAssetHeader *header, const char *name)
{
    //header->xmodelPieces[3].pieces = name;
    header->image->name = name;
}

const char *__cdecl DB_LocalizeEntryGetName(const XAssetHeader *header)
{
    // Win32 decomp overlaid LocalizeEntry::name onto XModelPieces::numpieces (both at +4).
    // On x64 name is at +8; use the real field.
    return header->localize->name;
}

void __cdecl DB_LocalizeEntrySetName(XAssetHeader *header, const char *name)
{
    header->localize->name = name;
}

#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_DEFAULT_GATE)
void __cdecl DB_DDLSetname(XAssetHeader *header, const char *name)
{
    // Generic assets carry their native-width name pointer in the first word.
    // Copy representations without accessing an unrelated payload structure.
    void *payload;
    ::memcpy(&payload, header, sizeof(payload));
    ::memcpy(payload, &name, sizeof(name));
}

#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_ENTRY_ALLOC_GATE)
const char *__cdecl DB_DDLGetName(const XAssetHeader *header)
{
    void *payload;
    const char *name;
    ::memcpy(&payload, header, sizeof(payload));
    ::memcpy(&name, payload, sizeof(name));
    return name;
}
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE)


const char *__cdecl DB_GetEmblemSetName(const XAssetHeader *header)
{
    return "emblemset";
}

#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_ENTRY_ALLOC_GATE)
const char *__cdecl DB_GetXAssetHeaderName(int type, const XAssetHeader *header)
{
    const char *v2; // eax
    const char *name; // [esp+0h] [ebp-4h]

    if ( !header
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp", 687, 0, "%s", "header") )
    {
        __debugbreak();
    }
    if ( !DB_XAssetGetNameHandler[type]
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp",
                    688,
                    0,
                    "%s",
                    "DB_XAssetGetNameHandler[type]") )
    {
        __debugbreak();
    }
    if ( !header->xmodelPieces
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp", 689, 0, "%s", "header->data") )
    {
        __debugbreak();
    }
    name = DB_XAssetGetNameHandler[type](header);
    if ( !name )
    {
#if defined(BO1_PORT_X64)
        // Stream-desync leftovers can leave image (etc.) names null mid-MENULIST;
        // keep loading with a placeholder instead of hard-asserting the DB thread.
        {
            static int s_nullNameLog;
            if (s_nullNameLog < 16)
            {
                ++s_nullNameLog;
            }
        }
        return "";
#else
        v2 = va("Name \"%s\" not found for asset type %s\n", 0, g_assetNames[type]);
        if ( !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp",
                        691,
                        0,
                        "%s\n\t%s",
                        "name",
                        v2) )
            __debugbreak();
#endif
    }
    return name;
}
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE)


#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_LOOKUP_GATE)
const char *__cdecl DB_GetXAssetName(const XAsset *asset)
{
    if ( !asset
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp", 698, 0, "%s", "asset") )
    {
        __debugbreak();
    }
    return DB_GetXAssetHeaderName(asset->type, &asset->header);
}
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_DEFAULT_GATE)


void __cdecl DB_SetXAssetName(XAsset *asset, const char *name)
{
    if ( !DB_XAssetSetNameHandler[asset->type]
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp",
                    705,
                    0,
                    "%s",
                    "DB_XAssetSetNameHandler[asset->type]") )
    {
        __debugbreak();
    }
    DB_XAssetSetNameHandler[asset->type](&asset->header, name);
}


#endif
int __cdecl DB_GetXAssetTypeSize(int type)
{
    if ( !DB_GetXAssetSizeHandler[type]
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp",
                    712,
                    0,
                    "%s",
                    "DB_GetXAssetSizeHandler[type]") )
    {
        __debugbreak();
    }
    return DB_GetXAssetSizeHandler[type]();
}
#if !defined(BO1_STRINGTABLE_OWNER_GATE)


#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_ADD_GATE)
const char *__cdecl DB_GetXAssetTypeName(unsigned int type)
{
    if ( type > 0x2A
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_assetnames.cpp",
                    718,
                    0,
                    "%s",
                    "type >= 0 && type < ASSET_TYPE_COUNT") )
    {
        __debugbreak();
    }
    return g_assetNames[type];
}
#endif
#if !defined(BO1_STRINGTABLE_OWNER_GATE)



#endif // !BO1_STRINGTABLE_OWNER_GATE
