#pragma once
#include "db_registry_add_api.h"
#include <clientscript/cscr_stringlist.h>
extern const char *g_defaultAssetName[43];
extern void (__cdecl *DB_XAssetSetNameHandler[43])(XAssetHeader*,const char*);
XAssetHeader __cdecl DB_FindXAssetDefaultHeaderInternal(XAssetType);
XAssetEntry *__cdecl DB_CreateDefaultEntry(XAssetType,const char*);
void DB_FreeDefaultEntries();
void __cdecl DB_SetXAssetName(XAsset*,const char*);
void __cdecl DB_DDLSetname(XAssetHeader*,const char*);
