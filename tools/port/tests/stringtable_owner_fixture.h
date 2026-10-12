#pragma once
#include <database/db_asset_pool.h>
#if defined(BO1_ENTRY_GENUINE_GATE)
#include "entry_owner_fixture.h"
#endif
#include <bitset>
#include <array>
#if !defined(__ANDROID__)
void WindowsOriginalFreeReference(XAssetPoolEntry<EmblemSet>**,XAssetHeader);
#endif
namespace owner_fixture {
static_assert(sizeof(StringTable)==(sizeof(void*)==8?32:20));
static_assert(sizeof(XAssetPoolEntry<StringTable>)==sizeof(StringTable));
static_assert(alignof(XAssetPoolEntry<StringTable>)==alignof(StringTable));
static_assert(offsetof(XAssetPoolEntry<StringTable>,entry)==0&&offsetof(XAssetPoolEntry<StringTable>,next)==0);
static_assert(sizeof(XAssetHeader)==sizeof(void*));
static std::bitset<POOLSIZE_STRINGTABLE_MOD> live;
static unsigned active;
static StringTable* persistent;
static std::array<unsigned char,sizeof(StringTable)> copiedHeader;
static unsigned indexOf(const void* p){
    const auto n=reinterpret_cast<std::uintptr_t>(p),base=reinterpret_cast<std::uintptr_t>(&g_StringTablePool.entries[0]);
    if(!p||n<base||(n-base)%sizeof(StringTable)||((n-base)/sizeof(StringTable))>=active)return POOLSIZE_STRINGTABLE_MOD;
    return static_cast<unsigned>((n-base)/sizeof(StringTable));
}
static bool initialize(int size){
    if(size<1||size>POOLSIZE_STRINGTABLE_MOD||live.any()||persistent)return false;
    DB_InitPool<decltype(g_StringTablePool)>(&g_StringTablePool,size);active=static_cast<unsigned>(size);return true;
}
static unsigned freeCount(bool ascending=false){
    std::bitset<POOLSIZE_STRINGTABLE_MOD> seen;auto* p=g_StringTablePool.freeHead;unsigned n=0;
    while(p){const auto id=indexOf(p);require(id<active&&!seen[id]&&!live[id],"free list has unique aligned owned nodes");if(ascending)require(id==n,"initial/recovered ascending pool order");seen[id]=true;++n;XAssetPoolEntry<StringTable>* next;std::memcpy(&next,p,sizeof(next));p=next;}
    require(n+live.count()==active,"exact pool free/live ownership conservation");return n;
}
static XAssetHeader allocate(){
    const auto h=DB_AllocXAsset<StringTable>(&g_StringTablePool);
    if(h.data){const auto id=indexOf(h.data);require(id<active&&!live[id],"actual allocation owns one unique node");live[id]=true;}
    return h;
}
static bool release(XAssetHeader h){
    const auto id=indexOf(h.data);if(id>=active||!live[id]||h.data==persistent)return false;
    ::DB_FreeXAssetHeader_EmblemSet_(reinterpret_cast<XAssetPoolEntry<EmblemSet>**>(&g_StringTablePool.freeHead),h);live[id]=false;return true;
}
static bool span(const void* p,std::size_t bytes){const auto address=reinterpret_cast<std::uintptr_t>(p);return p&&!(address%alignof(StringTable))&&bytes>=sizeof(StringTable)&&sizeof(StringTable)<=UINTPTR_MAX-address;}
static bool clone(const XAsset& from,XAsset& to,std::size_t sourceBytes,std::size_t destinationBytes){
    if(from.type!=ASSET_TYPE_STRINGTABLE||to.type!=ASSET_TYPE_STRINGTABLE)return false;
    const auto id=indexOf(to.header.data);if(id>=active||!live[id]||!span(from.header.data,sourceBytes)||!span(to.header.data,destinationBytes))return false;
    const auto a=reinterpret_cast<std::uintptr_t>(from.header.data),b=reinterpret_cast<std::uintptr_t>(to.header.data);
    if(a<b+sizeof(StringTable)&&b<a+sizeof(StringTable))return false;
    DB_CloneXAssetInternal(&from,&to);return true;
}
static void fixtures(){
    require(!persistent&&!live.any(),"pool fixture begins with no ownership");unsigned allocated=0,exhaustions=0,invalid=0;
    require(DB_GetXAssetTypeSize(ASSET_TYPE_STRINGTABLE)==sizeof(StringTable)&&DB_SizeofXAsset_StringTable_()==sizeof(StringTable),"actual size leaf and actual dispatcher");
    for(unsigned i=0;i<43;++i)require((DB_GetXAssetSizeHandler[i]!=nullptr)==(i==37
#if defined(BO1_RAWFILE_OWNER_GATE)
        ||i==36
#endif
        ),"other compile-selected size types unsupported, never fabricated");
    for(int size:{1,2,POOLSIZE_STRINGTABLE,POOLSIZE_STRINGTABLE_MOD}){
        require(initialize(size)&&freeCount(true)==static_cast<unsigned>(size),"actual production init and active pool sizes");
        std::vector<XAssetHeader> headers;for(int i=0;i<size;++i){auto h=allocate();require(indexOf(h.data)==static_cast<unsigned>(i),"actual ascending allocation");headers.push_back(h);++allocated;}
        std::vector<unsigned char> before(sizeof(g_StringTablePool));std::memcpy(before.data(),&g_StringTablePool,before.size());
        require(!allocate().data&&!std::memcmp(before.data(),&g_StringTablePool,before.size())&&!freeCount(),"actual null exhaustion no pool writes");++exhaustions;
        for(auto i=headers.rbegin();i!=headers.rend();++i)require(release(*i),"release every allocated entry");
        require(freeCount(true)==static_cast<unsigned>(size)&&!live.any(),"exact free-list baseline restored without reset");
    }
    require(initialize(2),"LIFO fixture init");auto a=allocate(),b=allocate();require(release(a)&&release(b),"actual LIFO releases");auto first=allocate(),second=allocate();require(first.data==b.data&&second.data==a.data,"actual free/pop LIFO reuse");require(release(first)&&release(second)&&freeCount(true)==2,"LIFO fixture restores baseline");
    auto rejects=[&](auto operation){std::array<unsigned char,sizeof(g_StringTablePool)> before;std::memcpy(before.data(),&g_StringTablePool,before.size());const auto was=live;require(!operation()&&live==was&&!std::memcmp(before.data(),&g_StringTablePool,before.size()),"invalid caller precondition never invokes unchecked leaf or mutates pool");++invalid;};
    for(int size:{0,-1,POOLSIZE_STRINGTABLE_MOD+1,INT32_MAX})rejects([&]{return initialize(size);});
    a=allocate();rejects([&]{return initialize(2);});rejects([&]{return release(XAssetHeader{});});StringTable foreign{};rejects([&]{return release(XAssetHeader(&foreign));});
    rejects([&]{return release(XAssetHeader(static_cast<unsigned char*>(a.data)+1));});require(release(a),"valid release before double-free rejection");rejects([&]{return release(a);});
    a=allocate();XAsset from{},to{};from.type=to.type=ASSET_TYPE_STRINGTABLE;from.header.data=&foreign;to.header=a;
    auto bad=from;bad.type=ASSET_TYPE_RAWFILE;rejects([&]{return clone(bad,to,sizeof(StringTable),sizeof(StringTable));});
    bad=from;bad.header.data=nullptr;rejects([&]{return clone(bad,to,sizeof(StringTable),sizeof(StringTable));});
    rejects([&]{return clone(from,to,sizeof(StringTable)-1,sizeof(StringTable));});rejects([&]{return clone(from,to,sizeof(StringTable),sizeof(StringTable)-1);});
    bad=from;bad.header.data=to.header.data;rejects([&]{return clone(bad,to,sizeof(StringTable),sizeof(StringTable));});
    require(clone(from,to,sizeof(StringTable),sizeof(StringTable))&&!std::memcmp(a.data,&foreign,sizeof(StringTable))&&release(a)&&freeCount(true)==2,"actual shallow clone and release with live borrowed source");
    rejects([&]{return clone(from,to,sizeof(StringTable),sizeof(StringTable));}); // destination was released
    auto badTo=to;badTo.header.data=nullptr;rejects([&]{return clone(from,badTo,sizeof(StringTable),sizeof(StringTable));});
    a=allocate();to.header=a;
    bad=from;bad.header.data=reinterpret_cast<unsigned char*>(&foreign)+1;rejects([&]{return clone(bad,to,sizeof(StringTable),sizeof(StringTable));});
    bad=from;bad.header.data=reinterpret_cast<void*>(UINTPTR_MAX-(alignof(StringTable)-1));rejects([&]{return clone(bad,to,sizeof(StringTable),sizeof(StringTable));});
    bad=from;bad.header.data=static_cast<unsigned char*>(a.data)+alignof(StringTable);rejects([&]{return clone(bad,to,sizeof(StringTable),sizeof(StringTable));});
    badTo=to;badTo.type=ASSET_TYPE_RAWFILE;rejects([&]{return clone(from,badTo,sizeof(StringTable),sizeof(StringTable));});
    badTo=to;badTo.header.data=&foreign;rejects([&]{return clone(from,badTo,sizeof(StringTable),sizeof(StringTable));});
    require(release(a)&&freeCount(true)==2,"additional lifetime/span guard fixtures restore pool without reset");
#if !defined(__ANDROID__)
    XAssetPool<StringTable,4> oldPool,newPool;DB_InitPool<decltype(oldPool)>(&oldPool,4);DB_InitPool<decltype(newPool)>(&newPool,4);
    std::array<XAssetHeader,4> oldHeaders,newHeaders;
    for(unsigned i=0;i<4;++i){oldHeaders[i]=DB_AllocXAsset<StringTable>(&oldPool);newHeaders[i]=DB_AllocXAsset<StringTable>(&newPool);std::memset(oldHeaders[i].data,0x35,sizeof(StringTable));std::memset(newHeaders[i].data,0x35,sizeof(StringTable));}
    for(unsigned id:{2u,0u,3u,1u}){
        ::WindowsOriginalFreeReference(reinterpret_cast<XAssetPoolEntry<EmblemSet>**>(&oldPool.freeHead),oldHeaders[id]);
        ::DB_FreeXAssetHeader_EmblemSet_(reinterpret_cast<XAssetPoolEntry<EmblemSet>**>(&newPool.freeHead),newHeaders[id]);
        require(!std::memcmp(static_cast<unsigned char*>(oldHeaders[id].data)+sizeof(void*),static_cast<unsigned char*>(newHeaders[id].data)+sizeof(void*),sizeof(StringTable)-sizeof(void*)),"historical Windows free payload tail parity");
    }
    for(unsigned id:{1u,3u,0u,2u}){auto old=DB_AllocXAsset<StringTable>(&oldPool),now=DB_AllocXAsset<StringTable>(&newPool);require(old.data==&oldPool.entries[id].entry&&now.data==&newPool.entries[id].entry,"historical Windows free exact LIFO parity");}
#endif
    require(initialize(POOLSIZE_STRINGTABLE)&&freeCount(true)==POOLSIZE_STRINGTABLE,"production normal active baseline");
    std::printf("{\"owner_fixtures\":{\"active_sizes\":[1,2,576,768],\"allocated_nodes\":%u,\"exhaustions\":%u,\"invalid_caller_preconditions\":%u,\"header_bytes\":%zu,\"pool_bytes\":%zu,\"entry_alignment\":%zu,\"logical_baseline_restored\":true,\"lifo_verified\":true,\"size_dispatch_actual\":true,\"full_registry_executed\":false}}\n",allocated,exhaustions,invalid,sizeof(StringTable),sizeof(g_StringTablePool),alignof(StringTable));
}
static StringTable* prepare(StringTable* temp){
    require(!persistent&&!live.any()&&freeCount(true)==POOLSIZE_STRINGTABLE,"genuine clone starts at pool baseline");
    XAsset from{},to{};from.type=to.type=ASSET_TYPE_STRINGTABLE;from.header.data=temp;
#if defined(BO1_ENTRY_GENUINE_GATE)
    to.header=entry_owner_fixture::acquire(ASSET_TYPE_STRINGTABLE);const auto id=indexOf(to.header.data);require(id<active&&!live[id],"actual entry header unique in StringTable pool");live[id]=true;
#else
    to.header=allocate();
#endif
    require(clone(from,to,sizeof(StringTable),sizeof(StringTable)),"actual production pool allocation and clone before TEMP pop");
    persistent=to.header.stringTable;std::memcpy(copiedHeader.data(),temp,sizeof(StringTable));
    require(persistent!=temp&&!std::memcmp(persistent,temp,sizeof(StringTable)),"exact native-width persistent shallow header");return persistent;
}
static void verifyAfterPop(const StringTableHeaderPlan& header,const StringTablePayloadPlan& payload,XBlock(&blocks)[7],StringTable* temp,bool measurement){
    require(persistent&&indexOf(persistent)<active&&live[indexOf(persistent)]&&!std::memcmp(persistent,copiedHeader.data(),sizeof(StringTable)),"pool header unchanged after TEMP pop");
    const auto state=frontend_fixture::streamState();const auto owners=frontend_fixture::counts();
    require(reinterpret_cast<unsigned char*>(temp)==blocks[0].data+header.nativeStart,"only actual old TEMP header workspace overwritten");
    const auto childHash=adler32(1,blocks[4].data,static_cast<unsigned>(payload.indices.nativeEnd));std::memset(temp,0xa5,sizeof(StringTable));
    executeLookups(payload,persistent,false);
    require(!lastLookup.rowMisses&&!lastLookup.rowDisagreements&&!lastLookup.columnDisagreements&&!std::memcmp(persistent,copiedHeader.data(),sizeof(StringTable))&&adler32(1,blocks[4].data,static_cast<unsigned>(payload.indices.nativeEnd))==childHash&&state==frontend_fixture::streamState()&&owners==frontend_fixture::counts(),"all actual queries survive TEMP overwrite; zone children and stream/owner unchanged");
    if(measurement)std::printf("{\"owner_measurements\":{\"header_bytes\":%zu,\"pool_address\":%llu,\"pool_above_u32\":%s,\"temp_overwritten_bytes\":%zu,\"row_queries_after_overwrite\":%u,\"row_misses_after_overwrite\":0,\"row_disagreements_after_overwrite\":0,\"column_disagreements_after_overwrite\":0,\"children_unchanged\":true,\"persistent_header_unchanged\":true,\"normalized_once\":true,\"full_registry_executed\":false}}\n",sizeof(StringTable),static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(persistent)),reinterpret_cast<std::uintptr_t>(persistent)>UINT32_MAX?"true":"false",sizeof(StringTable),lastLookup.queries);
}
static void finish(){
    require(persistent,"genuine pool header release before zone child release");XAssetHeader header(persistent);persistent=nullptr;
#if defined(BO1_ENTRY_GENUINE_GATE)
    const auto id=indexOf(header.data);require(id<active&&live[id],"actual StringTable entry-owned release precondition");entry_owner_fixture::release(header.data);live[id]=false;require(!live.any()&&freeCount(true)==POOLSIZE_STRINGTABLE,
#else
    require(release(header)&&!live.any()&&freeCount(true)==POOLSIZE_STRINGTABLE,
#endif
"actual pool header freed; complete original chain restored before zone free");copiedHeader={};
}
}
