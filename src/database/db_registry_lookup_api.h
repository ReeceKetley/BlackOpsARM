#pragma once
#include "db_entry_alloc_api.h"
extern int g_modDeferredCount; // exposed only by standalone lookup selection
unsigned int __cdecl DB_HashForName(const char *,XAssetType);
XAssetEntryPoolEntry *__cdecl DB_FindXAssetEntry(XAssetType,const char *);
const char *__cdecl DB_GetXAssetName(const XAsset *);
int __cdecl I_stricmp(const char *,const char *);
// Normal builds retain the real static helper. The standalone gate must provide
// an explicit unavailable endpoint rather than pretend deferred mods are linked.
XAssetEntryPoolEntry *DB_ModLinkDeferredByName(XAssetType,const char *);
