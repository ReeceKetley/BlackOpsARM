#pragma once
#include <database/db_entry_alloc_api.h>
void InitializeEntryOwnerInput();
namespace entry_owner_fixture {
static XAssetEntryPoolEntry*entries[2]{};
static XAssetType types[2]{};
static void*headers[2]{};
static bool started;
static unsigned peak;
static void begin(){require(!started&&!XAssetEntryHighCount&&!XAssetEntryHighWaterMark,"entry-owned gate starts BSS/quiescent baseline");Sys_InitializeCriticalSections();InitializeEntryOwnerInput();DB_InitAssetEntryPool(false);for(auto h:db_hashTable)require(!h,"entry ownership starts without publication");started=true;}
static XAssetHeader acquire(XAssetType type){require(started&&(type==ASSET_TYPE_RAWFILE||type==ASSET_TYPE_STRINGTABLE),"supported genuine entry type");unsigned slot=entries[0]?1:0;require(!entries[slot],"bounded genuine ownership at most two entries");Sys_EnterCriticalSection(CRITSECT_DBHASH);auto*p=DB_AllocXAssetEntry(type,1);Sys_LeaveCriticalSection(CRITSECT_DBHASH);entries[slot]=p;types[slot]=type;headers[slot]=p->entry.asset.header.data;require(p>=g_assetEntryPool+16&&p<g_assetEntryPool+32768&&p->entry.asset.type==type&&p->entry.zoneIndex==1&&!p->entry.inuse&&!p->entry.nextHash&&!p->entry.nextOverride&&headers[slot],"actual genuine entry owns actual selected pool header");return p->entry.asset.header;}
static void release(void*header){unsigned slot=headers[0]==header?0:1;require(entries[slot]&&headers[slot]==header&&header,"release exact genuine entry/header pair");Sys_EnterCriticalSection(CRITSECT_DBHASH);DB_FreeXAssetEntry(entries[slot]);Sys_LeaveCriticalSection(CRITSECT_DBHASH);entries[slot]=nullptr;headers[slot]=nullptr;}
static void verifyLive(unsigned asset){unsigned count=0;for(unsigned i=0;i<2;++i)if(entries[i]){++count;require(entries[i]->entry.asset.type==types[i]&&entries[i]->entry.asset.header.data==headers[i]&&!entries[i]->entry.nextHash&&!entries[i]->entry.nextOverride&&!entries[i]->entry.inuse&&entries[i]->entry.zoneIndex==1,"actual live entry retains header/type/zone after genuine load and TEMP overwrite");}require(count==(asset?1u:2u)&&XAssetEntryHighCount==static_cast<int>(count),"actual live genuine header/entry count by unskipped asset boundary");require(XAssetEntryHighWaterMark==2,"genuine highwater retains observed two code entries");for(auto h:db_hashTable)require(!h,"allocation/clone is not registry publication");peak=count;}
static void finish(unsigned asset){require(!entries[0]&&!entries[1]&&!headers[0]&&!headers[1]&&!XAssetEntryHighCount&&XAssetEntryHighWaterMark==2,"all actual genuine entries freed before zone release; highwater retained");unsigned count=0;for(auto*p=g_freeAssetEntryHead;p;p=p->next){require(p==g_assetEntryPool+16+count&&++count<=32752,"actual complete ascending entry chain restored without reset");}require(count==32752,"actual entire entry baseline");for(auto h:db_hashTable)require(!h,"hash remains unpublished through release");std::printf("{\"entry_owner_cycle\":{\"asset_index\":%u,\"owned_entries_before_release\":%u,\"high_count_after_release\":0,\"high_water_retained\":2,\"entry_stride\":%zu,\"free_entry_nodes\":32752,\"hash_buckets_empty\":true,\"entries_freed_before_zone\":true,\"entry_header_pairs_survived_temp\":true,\"registry_publication_executed\":%s}}\n",asset,peak,sizeof(XAssetEntryPoolEntry),
#if defined(BO1_REGISTRY_INSERT_GATE)
"true"
#else
"false"
#endif
);}
}

#if defined(BO1_REGISTRY_LOOKUP_GATE)
#include "registry_lookup_fixture.h"
#endif
