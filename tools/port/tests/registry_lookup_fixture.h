#pragma once
#include <database/db_registry_lookup_api.h>
#include <string>
#include <vector>
#include <array>
namespace registry_lookup_fixture {
#if defined(BO1_REGISTRY_ADD_GATE)
#define BO1_MEMBERSHIP_FIXTURE "false"
#define BO1_PUBLICATION_EXECUTED "true"
#define BO1_LOOKUP_CONTROL_SCHEMA "BlackOpsARM.TASK36.lookup-controls.v1"
#elif defined(BO1_REGISTRY_INSERT_GATE)
#define BO1_MEMBERSHIP_FIXTURE "false"
#define BO1_PUBLICATION_EXECUTED "true"
#define BO1_LOOKUP_CONTROL_SCHEMA "BlackOpsARM.TASK35.lookup-controls.v1"
#else
#define BO1_MEMBERSHIP_FIXTURE "true"
#define BO1_PUBLICATION_EXECUTED "false"
#define BO1_LOOKUP_CONTROL_SCHEMA "BlackOpsARM.TASK34.lookup-controls.v1"
#endif
static std::uint32_t reference(const char*name,XAssetType type){std::uint32_t h=static_cast<std::uint32_t>(type);for(auto*p=reinterpret_cast<const unsigned char*>(name);*p;++p){require(*p<128,"bounded ASCII hash contract");unsigned c=*p;if(c>='A'&&c<='Z')c+=32;if(c=='\\')c='/';h=static_cast<std::uint32_t>(std::uint64_t(h)*65599+c);}return h;}
static std::string upper(const char*name){std::string text(name);for(char&c:text){require(static_cast<unsigned char>(c)<128,"bounded ASCII name contract");if(c>='a'&&c<='z')c=static_cast<char>(c-32);}return text;}
static unsigned index(XAssetEntryPoolEntry*p){auto n=static_cast<unsigned>(p-g_assetEntryPool);require(n>=16&&n<32768&&static_cast<unsigned short>(n)==n,"actual entry index fixed16 and native stride");return n;}
static void genuine(unsigned asset){
 Sys_EnterCriticalSection(CRITSECT_DBHASH);require(!g_modDeferredCount,"genuine no-mod lookup");
#if !defined(BO1_REGISTRY_ADD_GATE)
for(auto h:db_hashTable)require(!h,"genuine private entries start without membership");
#endif

 std::array<unsigned,2>buckets{};std::array<std::array<unsigned char,sizeof(XAssetEntryPoolEntry)>,2>before{};unsigned count=0,queries=0;
 for(unsigned i=0;i<2;++i)if(auto*p=entry_owner_fixture::entries[i]){std::memcpy(before[i].data(),p,sizeof(*p));
#if defined(BO1_REGISTRY_ADD_GATE)
XAssetEntryPoolEntry expected;std::memcpy(&expected,p,sizeof(expected));expected.entry.nextHash=0;std::memcpy(before[i].data(),&expected,sizeof(expected));
#endif
auto*name=DB_GetXAssetName(&p->entry.asset);auto h=DB_HashForName(name,p->entry.asset.type);require(h==reference(name,p->entry.asset.type),"actual genuine name hash matches independent polynomial");buckets[i]=h%32768;
#if defined(BO1_REGISTRY_ADD_GATE)
 require(DB_FindXAssetEntry(p->entry.asset.type,name)==p,"genuine entry already published by Add before TEMP Pop");
#elif defined(BO1_REGISTRY_INSERT_GATE)
 DB_InsertNewXAssetEntry(&p->entry,h);require(db_hashTable[buckets[i]]==index(p),"actual insertion publishes native pool index");
#else
 p->entry.nextHash=db_hashTable[buckets[i]];db_hashTable[buckets[i]]=static_cast<unsigned short>(index(p));
#endif
 ++count;}
 for(unsigned i=0;i<2;++i)if(auto*p=entry_owner_fixture::entries[i]){auto*name=DB_GetXAssetName(&p->entry.asset);require(DB_FindXAssetEntry(p->entry.asset.type,name)==p,"actual Find returns genuine entry/header by original name");auto folded=upper(name);require(DB_FindXAssetEntry(p->entry.asset.type,folded.c_str())==p,"actual Find genuine case variant");std::string missing="absent/";missing+=name;require(!DB_FindXAssetEntry(p->entry.asset.type,missing.c_str()),"actual genuine miss preserves null behavior");queries+=3;}
 for(unsigned i=0;i<2;++i)if(auto*p=entry_owner_fixture::entries[i]){db_hashTable[buckets[i]]=0;p->entry.nextHash=0;require(!std::memcmp(before[i].data(),p,sizeof(*p)),"fixture membership restored exact entry bytes before release");}
 for(auto h:db_hashTable)require(!h,"fixture-installed hash membership fully removed");require(count==(asset?1u:2u)&&XAssetEntryHighCount==static_cast<int>(count),"lookup leaves actual ownership counters unchanged");Sys_LeaveCriticalSection(CRITSECT_DBHASH);
 #if defined(BO1_REGISTRY_ADD_GATE)
 std::printf("{\"registry_add_cycle\":{\"asset_index\":%u,\"actual_add_calls\":%u,\"engine_allocated_cloned_published\":true,\"returned_header_used_before_temp_pop\":true,\"full_link_new_name_path\":true,\"fixture_unlink_before_free\":true}}\n",asset,count);
#endif
 std::printf("{\"registry_lookup_cycle\":{\"asset_index\":%u,\"actual_name_queries\":%u,\"fixture_installed_hash_membership\":" BO1_MEMBERSHIP_FIXTURE ",\"actual_find_header_identity\":true,\"ascii_case_variants_match\":true,\"miss_returns_null\":true,\"entry_bytes_restored\":true,\"hash_membership_removed_before_free\":true,\"registry_publication_executed\":" BO1_PUBLICATION_EXECUTED "}}\n",asset,queries);
}
static void controls(){
 require(!g_modDeferredCount&&!XAssetEntryHighCount,"controlled lookup process begins clean");Sys_InitializeCriticalSections();InitializeEntryOwnerInput();DB_InitAssetEntryPool(false);DB_InitPool<decltype(g_RawFilePool)>(&g_RawFilePool,1024);DB_InitPool<decltype(g_StringTablePool)>(&g_StringTablePool,576);
 unsigned vectors=0;for(auto type:{ASSET_TYPE_XMODELPIECES,ASSET_TYPE_RAWFILE,ASSET_TYPE_STRINGTABLE})for(const std::string&name:std::vector<std::string>{"","a","A","Path/Name","path\\name","Case_Fold","123_-",std::string(256,'Z'),std::string(4096,'x')}){require(DB_HashForName(name.c_str(),type)==reference(name.c_str(),type),"actual hash independent ASCII polynomial vectors");++vectors;}
 require(DB_HashForName("Path/Name",ASSET_TYPE_RAWFILE)==DB_HashForName("path\\name",ASSET_TYPE_RAWFILE),"actual hash normalizes slash and case");
 std::vector<std::string>seen(32768);std::string first,second;for(unsigned i=0;i<32769;++i){auto name="collision/"+std::to_string(i);auto b=reference(name.c_str(),ASSET_TYPE_RAWFILE)%32768;if(!seen[b].empty()){first=seen[b];second=name;break;}seen[b]=name;}require(!first.empty()&&!second.empty(),"bounded pigeonhole bucket collision");
 Sys_EnterCriticalSection(CRITSECT_DBHASH);auto*a=DB_AllocXAssetEntry(ASSET_TYPE_RAWFILE,1),*b=DB_AllocXAssetEntry(ASSET_TYPE_RAWFILE,1),*other=DB_AllocXAssetEntry(ASSET_TYPE_STRINGTABLE,1),*overrideEntry=DB_AllocXAssetEntry(ASSET_TYPE_RAWFILE,1);
 RawFile ar{first.c_str(),0,nullptr},br{second.c_str(),0,nullptr},orr{"override-only",0,nullptr};StringTable st{};st.name="other-type";std::memcpy(a->entry.asset.header.data,&ar,sizeof(ar));std::memcpy(b->entry.asset.header.data,&br,sizeof(br));std::memcpy(other->entry.asset.header.data,&st,sizeof(st));std::memcpy(overrideEntry->entry.asset.header.data,&orr,sizeof(orr));
 unsigned bucket=DB_HashForName(first.c_str(),ASSET_TYPE_RAWFILE)%32768;require(bucket==DB_HashForName(second.c_str(),ASSET_TYPE_RAWFILE)%32768,"actual collision bucket matches oracle");
#if defined(BO1_REGISTRY_INSERT_GATE)
DB_InsertNewXAssetEntry(&b->entry,bucket);DB_InsertNewXAssetEntry(&a->entry,bucket);DB_InsertNewXAssetEntry(&other->entry,bucket);
require(other->entry.nextHash==index(a)&&a->entry.nextHash==index(b)&&!b->entry.nextHash,"actual insertion prepends collision chain preserving prior head");
#else
db_hashTable[bucket]=static_cast<unsigned short>(index(other));other->entry.nextHash=static_cast<unsigned short>(index(a));a->entry.nextHash=static_cast<unsigned short>(index(b));
#endif
a->entry.nextOverride=static_cast<unsigned short>(index(overrideEntry));
 auto validOther=other->entry.asset.header;other->entry.asset.header.data=reinterpret_cast<void*>(uintptr_t(1));require(DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,first.c_str())==a&&DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,second.c_str())==b,"actual collision traversal filters type before unrelated header access");other->entry.asset.header=validOther;
 require(DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,upper(first.c_str()).c_str())==a,"actual controlled case hit");require(!DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,"missing-name"),"actual miss");
 orr.name=second.c_str();std::memcpy(overrideEntry->entry.asset.header.data,&orr,sizeof(orr));a->entry.nextHash=0;require(!DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,second.c_str()),"same-bucket tail miss does not traverse nextOverride");a->entry.nextHash=static_cast<unsigned short>(index(b));
 br.name=first.c_str();std::memcpy(b->entry.asset.header.data,&br,sizeof(br));require(DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,first.c_str())==a,"duplicate primary names return first matching chain node");g_modDeferredCount=1;require(DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,first.c_str())==a,"ordinary hit bypasses deferred endpoint even countpositive");g_modDeferredCount=0;db_hashTable[bucket]=0;a->entry.nextHash=0;a->entry.nextOverride=0;other->entry.nextHash=0;
 ar.name="path/name";std::memcpy(a->entry.asset.header.data,&ar,sizeof(ar));bucket=DB_HashForName(ar.name,ASSET_TYPE_RAWFILE)%32768;db_hashTable[bucket]=static_cast<unsigned short>(index(a));require(DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,"PATH/NAME")==a&&!DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,"path\\name"),"hash slash normalization does not change original I_stricmp confirmation semantics");db_hashTable[bucket]=0;
 ar.name="";std::memcpy(a->entry.asset.header.data,&ar,sizeof(ar));bucket=DB_HashForName("",ASSET_TYPE_RAWFILE)%32768;db_hashTable[bucket]=static_cast<unsigned short>(index(a));require(bucket==36&&DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,"")==a,"empty name preserves type seed and lookup");db_hashTable[bucket]=0;
 for(auto*p:{overrideEntry,other,b,a})DB_FreeXAssetEntry(p);require(!XAssetEntryHighCount&&XAssetEntryHighWaterMark==4&&g_freeAssetEntryHead==g_assetEntryPool+16,"controlled real ownership baseline without highwater reset");Sys_LeaveCriticalSection(CRITSECT_DBHASH);
 std::printf("{\"schema\":\"" BO1_LOOKUP_CONTROL_SCHEMA "\",\"pointer_bytes\":%zu,\"hash_vectors\":%u,\"bucket_collision_verified\":true,\"type_filter_before_header_access\":true,\"first_primary_match\":true,\"override_not_primary\":true,\"slash_hash_equal_lookup_miss\":true,\"deferred_count_hit_bypasses_endpoint\":true,\"ownership_baseline_restored\":true,\"registry_publication_executed\":" BO1_PUBLICATION_EXECUTED "}\n",sizeof(void*),vectors);
}
static void deferredUnavailable(){Sys_InitializeCriticalSections();Sys_EnterCriticalSection(CRITSECT_DBHASH);require(!g_modDeferredCount,"fresh deferred control");g_modDeferredCount=1;DB_FindXAssetEntry(ASSET_TYPE_RAWFILE,"not-present");require(false,"deferred endpoint never succeeds");}
#if defined(BO1_REGISTRY_ADD_GATE)
static void defaultUnavailable(){Sys_InitializeCriticalSections();InitializeRegistryAddInputs();RawFile raw{",not-present",0,nullptr};DB_AddXAsset(ASSET_TYPE_RAWFILE,XAssetHeader(&raw));require(false,"unavailable default dependency never succeeds");}
#endif
}
