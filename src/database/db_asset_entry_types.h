#pragma once
#include "db_assetlist_types.h"
#include <cstddef>
#include <cstdint>

struct XAssetEntry // sizeof=0x10
{                                       // XREF: XAssetEntryPoolEntry/r
                                        // DB_AddXAsset/r
    XAsset asset;                       // XREF: DB_AddXAsset+9/w
                                        // DB_AddXAsset+F/w
    unsigned __int8 zoneIndex;
    bool inuse;
    unsigned __int16 nextHash;
    unsigned __int16 nextOverride;
    unsigned __int16 usageFrame;
};

union XAssetEntryPoolEntry // sizeof=0x10
{                                       // XREF: DB_Init+AB/w
    XAssetEntry entry;
    XAssetEntryPoolEntry *next;

    XAssetEntryPoolEntry()
    {
    }
};


static_assert(sizeof(XAssetEntry)==(sizeof(void*)==8?24:16));
static_assert(sizeof(XAssetEntryPoolEntry)==sizeof(XAssetEntry));
static_assert(alignof(XAssetEntryPoolEntry)==alignof(XAssetEntry));
static_assert(offsetof(XAssetEntryPoolEntry,entry)==0&&offsetof(XAssetEntryPoolEntry,next)==0);
static_assert(offsetof(XAssetEntry,zoneIndex)==sizeof(XAsset));
static_assert(offsetof(XAssetEntry,nextHash)==sizeof(XAsset)+2);
static_assert(offsetof(XAssetEntry,nextOverride)==sizeof(XAsset)+4);
static_assert(offsetof(XAssetEntry,usageFrame)==sizeof(XAsset)+6);
static_assert(sizeof(decltype(XAssetEntry::nextHash))==2&&sizeof(decltype(XAssetEntry::nextOverride))==2);
extern unsigned __int16 db_hashTable[0x8000];
extern XAssetEntryPoolEntry g_assetEntryPool[65536];
extern XAssetEntryPoolEntry *g_freeAssetEntryHead;
// Startup/quiescence caller contract; no hash/counter reset or synchronization.
void DB_InitAssetEntryPool(bool modZones);
