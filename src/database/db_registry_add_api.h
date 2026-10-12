#pragma once
#include "db_registry_lookup_api.h"
#include "db_clone_types.h"
struct XModel;
extern unsigned int g_zoneIndex;
extern bool g_isRecoveringLostDevice;
extern bool g_mayRecoverLostAssets;
extern const dvar_t *db_xassetdebug;
extern const dvar_t *db_xassetdebugtype;
extern const char *g_defaultAssetName[43];
extern int g_defaultAssetCount;
extern int g_modDeferredUsed;
extern int g_dbTopAssetType;
extern bool g_modLazyModels;
extern XAsset *varXAsset;
XAssetHeader __cdecl DB_AddXAsset(XAssetType,XAssetHeader);
XAssetEntryPoolEntry *__cdecl DB_LinkXAssetEntry(XAssetEntry *,int);
void DB_SyncLostDevice();
const char *__cdecl DB_GetXAssetTypeName(unsigned int);
const char *__cdecl Dvar_GetString(const char *);
const char *__cdecl I_stristr(const char *,const char *);
XAssetEntry *__cdecl DB_CreateDefaultEntry(XAssetType,const char *);
bool __cdecl DB_OverrideAsset(unsigned int,unsigned int);
void __cdecl DB_DelayedCloneXAsset(XAssetEntry *);
void __cdecl DB_SwapXAsset(XAsset *,XAsset *);
void __cdecl DB_CloneXAssetEntry(const XAssetEntry *,XAssetEntry *,DBCloneMethod);
void __cdecl Mark_XAsset();
void __cdecl NET_Sleep(unsigned int);
// Standalone selection must supply explicitly failing unavailable mod endpoints.
XAssetHeader DB_ModDefer(const XAssetEntry *);
void DB_ModLinkModelMaterials(XModel *);
