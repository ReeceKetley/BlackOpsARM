#pragma once
#include "db_entry_alloc_api.h"
extern int g_modDeferredCount; // exposed only by standalone lookup selection
unsigned int __cdecl DB_HashForName(const char *,XAssetType);
// DBHASH-held, pool-owned entry, live header/zone, new-name decision complete.
void __cdecl DB_InsertNewXAssetEntry(XAssetEntry *,unsigned int);
XAssetEntryPoolEntry *__cdecl DB_FindXAssetEntry(XAssetType,const char *);
const char *__cdecl DB_GetXAssetName(const XAsset *);
int __cdecl I_stricmp(const char *,const char *);
// Normal builds retain the real static helper. The standalone gate must provide
// an explicit unavailable endpoint rather than pretend deferred mods are linked.
XAssetEntryPoolEntry *DB_ModLinkDeferredByName(XAssetType,const char *);
