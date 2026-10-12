#pragma once
#include "db_asset_entry_types.h"
#include "db_asset_pool.h"
#include "db_zone_name_types.h"
#include <platform/critical_section_services.h>
#include <universal/com_errors.h>
#include <universal/dvar.h>
extern const dvar_t *useFastFile;
extern const char *g_assetNames[43];
extern int XAssetEntryHighCount;
extern int XAssetEntryHighWaterMark;
extern void *DB_XAssetPool[43];
extern XAssetHeader(__cdecl *DB_AllocXAssetHeaderHandler[43])(void *);
extern void(__cdecl *DB_FreeXAssetHeaderHandler[43])(void *, XAssetHeader);
XAssetEntryPoolEntry *__cdecl DB_AllocXAssetEntry(XAssetType, unsigned __int8);
void __cdecl DB_FreeXAssetEntry(XAssetEntryPoolEntry *);
XAssetHeader __cdecl DB_AllocXAssetHeader(XAssetType);
void __cdecl DB_FreeXAssetHeader(XAssetType, XAssetHeader);
void __cdecl DB_PrintAssetName(XAssetHeader, void *);
void __cdecl DB_PrintXAssetsForType(XAssetType,void(__cdecl *)(XAssetHeader,void *),void *,bool);
void __cdecl DB_PrintXAssetsForType_FastFile(XAssetType,void *,bool);
void __cdecl DB_EnumXAssets(XAssetType,void(__cdecl *)(XAssetHeader,void *),void *,bool);
const char *__cdecl DB_GetXAssetHeaderName(int,const XAssetHeader *);
const char *__cdecl DB_DDLGetName(const XAssetHeader *);
void Com_Printf(int,const char *,...);
void Com_PrintError(int,const char *,...);
void Com_Error(errorParm_t,const char *,...);
char *va(const char *,...);
