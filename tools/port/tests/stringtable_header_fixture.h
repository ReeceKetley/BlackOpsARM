#pragma once
// Included after the finite reader and genuine directory fixture definitions.
#include <platform/fastfile_stringtable_header.h>
#include <universal/com_stringtable.h>
static_assert(sizeof(StringTable)==(BO1_NATIVE_64BIT?32:20));
static_assert(offsetof(StringTable,name)==0&&offsetof(StringTable,columnCount)==sizeof(void*));
static_assert(offsetof(StringTable,values)==(BO1_NATIVE_64BIT?16:12));
static_assert(offsetof(StringTable,cellIndex)==(BO1_NATIVE_64BIT?24:16));
#if defined(BO1_STRINGTABLE_CELLS_GATE)
#include "stringtable_cells_fixture.h"
#endif
static StringTableHeaderPlan headerPlan(const Front&f,const AssetDirectoryPlan&d,const unsigned char*data,std::size_t bytes,std::uint64_t decoded){
    require(d.present&&d.count>0,"first genuine directory record exists");
    const auto start=f.end+static_cast<std::size_t>(d.diskBytes);require(start<=bytes&&start<=decoded,"header source after directory");
    StringTableHeaderPlan p;require(PlanStringTableHeader(static_cast<int>(u32(data+f.end)),u32(data+f.end+4),data+start,bytes-start,decoded-start,f.envelope.zone.file.blockSize[0],f.envelope.zone.logical[0],0,0,p)==StringTableHeaderResult::Ok&&p.present,"first genuine StringTable header checked before allocation");return p;
}
static void seedEnvelope(const unsigned char*data,XAssetList&envelope){
    DecodedSource source(data+36,16);reader=&source;tableReadBytes=16;
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_XAssetList(&envelope);
    require(DB_StreamWin32Pos(0)==0,"side envelope has no shadow commit");
    require(DB_StreamAliasSlotLookup(0,4)==reinterpret_cast<uintptr_t*>(&envelope.stringList.strings)&&DB_StreamAliasSlotLookup(0,12)==reinterpret_cast<uintptr_t*>(&envelope.assets),"persistent side-envelope aliases");
#else
    // Production Win32 envelope is a side-buffer read, never an owned stream commit.
    DB_LoadXFileData(reinterpret_cast<unsigned char*>(&envelope),16);
#endif
    reader=nullptr;tableReadBytes=0;require(source.Position()==16&&DB_GetStreamPos()==g_streamBlocks[0].data,"side envelope preserves owned block0 cursor");
}
static void executeHeader(const StringTableHeaderPlan&p,DecodedSource&r,XBlock(&blocks)[7],XAssetList*envelope,bool measurement
#if defined(BO1_STRINGTABLE_CELLS_GATE)
    ,const StringTablePayloadPlan*payload=nullptr
#endif
){
    if(!p.present)return;
    const auto sourceStart=r.Position()+52;const auto block4=g_streamPosIndex==4?DB_GetStreamPos():g_streamPosArray[4];
    const auto prefixBytes=static_cast<unsigned>(block4-blocks[4].data);const auto prefixAdler=adler32(1,blocks[4].data,prefixBytes);
    DB_PushStreamPos(0);auto*header=reinterpret_cast<StringTable*>(DB_AllocStreamPos(3));
    require(reinterpret_cast<unsigned char*>(header)==blocks[0].data+p.nativeStart,"header native aligned block0 owner");
    reader=&r;tableReadBytes=20;
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_StringTable(header);
#else
    Load_Stream(true,reinterpret_cast<unsigned char*>(header),20);
#endif
    reader=nullptr;tableReadBytes=0;
    require(header->columnCount==p.columns&&header->rowCount==p.rows,"actual signed header dimensions preserved");
    require(reinterpret_cast<uintptr_t>(header->name)==widenTag(p.nameTag)&&reinterpret_cast<uintptr_t>(header->values)==widenTag(p.valuesTag)&&reinterpret_cast<uintptr_t>(header->cellIndex)==widenTag(p.indexTag),"all inner pointer tags preserved without dereference");
    require(DB_GetStreamPos()==blocks[0].data+p.nativeEnd&&r.Position()+52==sourceStart+20,"header commit before TEMP pop");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(0)==p.diskEnd,"header shadow20 before pop");
    require(DB_StreamExactLookup(0,static_cast<unsigned>(p.diskStart))==reinterpret_cast<unsigned char*>(header),"header exact start maps");
    for(unsigned i=1;i<20;++i)require(!DB_StreamExactLookup(0,static_cast<unsigned>(p.diskStart)+i),"header expanded interiors do not resolve exactly");
    require(DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart))==reinterpret_cast<uintptr_t*>(&header->name)&&DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart)+12)==reinterpret_cast<uintptr_t*>(&header->values)&&DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart)+16)==reinterpret_cast<uintptr_t*>(&header->cellIndex),"three actual header field aliases");
    if(envelope)require(DB_StreamAliasSlotLookup(0,4)==reinterpret_cast<uintptr_t*>(&envelope->stringList.strings),"unreplaced envelope alias remains valid");
#endif
#if defined(BO1_STRINGTABLE_CELLS_GATE)
    require(payload&&g_streamPosStackIndex==2,"actual parent block4 to header0 scope");const auto untouchedIndex=header->cellIndex;
    executePayload(*payload,header,r,blocks,measurement);
#if defined(BO1_STRINGTABLE_INDEX_GATE)
    require(reinterpret_cast<uintptr_t>(untouchedIndex)==widenTag(p.indexTag)&&header->cellIndex==(payload->indices.present?reinterpret_cast<__int16*>(blocks[4].data+payload->indices.nativeStart):nullptr),"original index tag replaced only by successfully loaded owned array");
    require(header->columnCount==p.columns&&header->rowCount==p.rows,"child loading preserves dimensions");
#else
    require(header->columnCount==p.columns&&header->rowCount==p.rows&&header->cellIndex==untouchedIndex,"child loading preserves dimensions and unresolved index tag");
#endif
#if BO1_NATIVE_64BIT
    require(DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart))==reinterpret_cast<uintptr_t*>(&header->name)&&DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart)+12)==reinterpret_cast<uintptr_t*>(&header->values)&&DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart)+16)==reinterpret_cast<uintptr_t*>(&header->cellIndex),"header aliases survive raw name and unresolved cell publication");
#endif
#endif
    DB_PopStreamPos();
#if defined(BO1_STRINGTABLE_CELLS_GATE)
    require(g_streamPosArray[0]==blocks[0].data&&g_streamPosStackIndex==1&&g_streamPosIndex==4&&DB_GetStreamPos()==blocks[4].data+
#if defined(BO1_STRINGTABLE_INDEX_GATE)
        payload->indices.nativeEnd
#elif defined(BO1_STRINGTABLE_STRINGS_GATE)
        payload->stringNativeEnd
#else
        payload->cells.nativeEnd
#endif
        ,"TEMP pop restores block0 workspace while returning to parent4");
#else
    require(DB_GetStreamPos()==blocks[0].data&&g_streamPosArray[0]==nullptr&&g_streamPosStackIndex==0&&g_streamPosIndex==0,"TEMP pop restores entry workspace cursor");
#endif
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(0)==p.diskEnd,"TEMP pop retains shadow and mappings");
#endif
#if !defined(BO1_STRINGTABLE_CELLS_GATE)
    require(g_streamPosArray[4]==block4,"header-only block4 cursor preserved");
#endif
    require(adler32(1,blocks[4].data,prefixBytes)==prefixAdler,"header/children preserve preceding block4 bytes");
    if(measurement)std::printf("{\"header_measurements\":{\"source_start\":%zu,\"name_tag\":%u,\"column_count\":%d,\"row_count\":%d,\"values_tag\":%u,\"cell_index_tag\":%u,\"cell_count\":%llu,\"disk_start\":%llu,\"native_start\":%llu,\"disk_end\":%llu,\"native_end_before_pop\":%llu,\"native_cursor_after_pop\":0,\"shadow_after_pop\":%llu}}\n",sourceStart,p.nameTag,p.columns,p.rows,p.valuesTag,p.indexTag,(unsigned long long)p.cells,(unsigned long long)p.diskStart,(unsigned long long)p.nativeStart,(unsigned long long)p.diskEnd,(unsigned long long)p.nativeEnd,(unsigned long long)(BO1_NATIVE_64BIT?p.diskEnd:0));
#if defined(BO1_STRINGTABLE_OWNER_GATE)
    owner_fixture::verifyAfterPop(p,*payload,blocks,header,measurement);
#endif
}
static unsigned headerFixtures(const ZonePlan&zone){
    unsigned count=0;const auto baseline=frontend_fixture::counts();std::array<unsigned char,20>data{};
    auto dimensions=[&](std::int32_t cols,std::int32_t rows,std::uint32_t values,std::uint32_t indices){data={};std::memcpy(data.data()+4,&cols,4);std::memcpy(data.data()+8,&rows,4);std::memcpy(data.data()+12,&values,4);std::memcpy(data.data()+16,&indices,4);};
    auto test=[&](int type,unsigned tag,const unsigned char*source,std::size_t bytes,std::uint64_t remaining,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,StringTableHeaderResult expected){
        const auto streams=frontend_fixture::streamState();StringTableHeaderPlan p;require(PlanStringTableHeader(type,tag,source,bytes,remaining,dc,nc,w,n,p)==expected,"controlled header preflight status");
        require(streams==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"header preflight no mutation");if(expected!=StringTableHeaderResult::Ok)require(!p.present&&!p.cells&&!p.diskEnd&&!p.nativeEnd,"failed header plan cleared");
        std::printf("{\"fixture\":\"header_preflight_%u\",\"status\":\"%s\",\"no_mutation\":true}\n",++count,StringTableHeaderResultName(expected));};
    test(36,0,nullptr,0,0,0,0,0,0,StringTableHeaderResult::UnsupportedAsset);
    test(37,0,nullptr,0,0,0,0,0,0,StringTableHeaderResult::Ok);
    for(auto tag:{1u,0xfffffffeu,0x80000001u})test(37,tag,data.data(),20,20,100,100,0,0,StringTableHeaderResult::UnsupportedTag);
    test(37,0xffffffffu,nullptr,20,20,100,100,0,0,StringTableHeaderResult::ShortHeader);
    test(37,0xffffffffu,data.data(),19,20,100,100,0,0,StringTableHeaderResult::ShortHeader);
    test(37,0xffffffffu,data.data(),20,19,100,100,0,0,StringTableHeaderResult::SourceBounds);
    dimensions(-1,1,0,0);test(37,0xffffffffu,data.data(),20,100,100,100,0,0,StringTableHeaderResult::NegativeDimension);
    dimensions(1,-1,0,0);test(37,0xffffffffu,data.data(),20,100,100,100,0,0,StringTableHeaderResult::NegativeDimension);
    dimensions(INT32_MAX,INT32_MAX,0,0);test(37,0xffffffffu,data.data(),20,UINT64_MAX,100,100,0,0,StringTableHeaderResult::CellCountOverflow);
    dimensions(INT32_MAX,1,1,0);test(37,0xffffffffu,data.data(),20,UINT64_MAX,100,100,0,0,StringTableHeaderResult::CellCountOverflow);
    dimensions((1<<20)+1,1,1,0);test(37,0xffffffffu,data.data(),20,UINT64_MAX,100,100,0,0,BO1_NATIVE_64BIT?StringTableHeaderResult::HelperDomain:StringTableHeaderResult::Ok);
    dimensions(1,4,1,1);test(37,0xffffffffu,data.data(),20,59,100,100,0,0,StringTableHeaderResult::SourceBounds);
    dimensions(0,0,0,0);test(37,0xffffffffu,data.data(),20,20,19,100,0,0,StringTableHeaderResult::DiskBounds);
    test(37,0xffffffffu,data.data(),20,20,100,sizeof(StringTable)-1,0,0,StringTableHeaderResult::NativeBounds);
    test(37,0xffffffffu,data.data(),20,20,23,100,1,0,StringTableHeaderResult::DiskBounds);
    test(37,0xffffffffu,data.data(),20,20,100,sizeof(void*)+sizeof(StringTable)-1,0,1,StringTableHeaderResult::NativeBounds);
    test(37,0xffffffffu,data.data(),20,20,UINT64_MAX,UINT64_MAX,UINT64_MAX,0,StringTableHeaderResult::DiskBounds);
    test(37,0xffffffffu,data.data(),20,20,UINT64_MAX,UINT64_MAX,0,UINT64_MAX,StringTableHeaderResult::NativeBounds);
    test(37,0xffffffffu,data.data(),20,20,UINT64_MAX,UINT64_MAX,UINT32_MAX-3,0,StringTableHeaderResult::DiskBounds);
    test(37,0xffffffffu,data.data(),20,20,UINT64_MAX,UINT64_MAX,0,UINT32_MAX-3,StringTableHeaderResult::NativeBounds);
    test(37,0xffffffffu,data.data(),20,20,24,sizeof(void*)+sizeof(StringTable),1,1,StringTableHeaderResult::Ok);
    // Actual mixed child tags are never interpreted at this boundary.
    dimensions(2,3,0xfffffffeu,0x80000001u);unsigned name=0x12345678u;std::memcpy(data.data(),&name,4);
    frontend_fixture::Owner owner;XBlock blocks[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),blocks,failed)==ZoneResult::Ok,"controlled header owned zone");
    for(unsigned offset:{0u,1u}){DB_InitStreams(blocks);if(offset)DB_IncStreamPos(1);StringTableHeaderPlan p;require(PlanStringTableHeader(37,0xffffffffu,data.data(),20,1000,zone.file.blockSize[0],zone.logical[0],offset,offset,p)==StringTableHeaderResult::Ok,"mixed actual header plan");
        // Actual aligned offset fixture uses a normal parent block4 scope.
        DB_InitStreams(blocks);DB_PushStreamPos(4);DB_PushStreamPos(0);if(offset)DB_IncStreamPos(1);auto*dst=reinterpret_cast<StringTable*>(DB_AllocStreamPos(3));DecodedSource r(data.data(),20);reader=&r;tableReadBytes=20;
#if BO1_NATIVE_64BIT
        DB_ExpandLoad_StringTable(dst);
#else
        Load_Stream(true,reinterpret_cast<unsigned char*>(dst),20);
#endif
        reader=nullptr;tableReadBytes=0;require(dst->columnCount==2&&dst->rowCount==3&&reinterpret_cast<uintptr_t>(dst->name)==widenTag(name)&&reinterpret_cast<uintptr_t>(dst->values)==widenTag(0xfffffffeu)&&reinterpret_cast<uintptr_t>(dst->cellIndex)==widenTag(0x80000001u),"mixed bits preserved actual header");
        require(reinterpret_cast<unsigned char*>(dst)==blocks[0].data+p.nativeStart&&DB_GetStreamPos()==blocks[0].data+p.nativeEnd,"mixed aligned actual native cursor");
#if BO1_NATIVE_64BIT
        require(DB_StreamWin32Pos(0)==p.diskEnd&&DB_StreamExactLookup(0,static_cast<unsigned>(p.diskStart))==reinterpret_cast<unsigned char*>(dst),"mixed aligned header exact and shadow");
        for(unsigned i=1;i<20;++i)require(!DB_StreamExactLookup(0,static_cast<unsigned>(p.diskStart)+i),"mixed expanded interiors absent");
        require(DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart))==reinterpret_cast<uintptr_t*>(&dst->name)&&DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart)+12)==reinterpret_cast<uintptr_t*>(&dst->values)&&DB_StreamAliasSlotLookup(0,static_cast<unsigned>(p.diskStart)+16)==reinterpret_cast<uintptr_t*>(&dst->cellIndex),"mixed aligned field aliases");
#endif
        DB_PopStreamPos();DB_PopStreamPos();++count;}
    DB_InitStreams(blocks);std::array<unsigned char,52>envelopeBytes{};XAssetList envelope{};seedEnvelope(envelopeBytes.data(),envelope);
#if BO1_NATIVE_64BIT
    std::array<uintptr_t*,4>oldAliases{};unsigned aliasIndex=0;for(unsigned w:{0u,4u,12u,16u})oldAliases[aliasIndex++]=DB_StreamAliasSlotLookup(0,w);const auto oldExact=DB_StreamExactLookup(0,0);
#endif
    DB_PushStreamPos(0);auto*dst=reinterpret_cast<StringTable*>(DB_AllocStreamPos(3));const auto before=frontend_fixture::streamState();const auto owned=frontend_fixture::counts();std::array<unsigned char,sizeof(StringTable)>snapshot{};std::memcpy(snapshot.data(),dst,snapshot.size());DecodedSource shortReader(data.data(),19);reader=&shortReader;tableReadBytes=20;bool threw=false;
    try{
#if BO1_NATIVE_64BIT
        DB_ExpandLoad_StringTable(dst);
#else
        Load_Stream(true,reinterpret_cast<unsigned char*>(dst),20);
#endif
    }catch(const std::runtime_error&){threw=true;}reader=nullptr;tableReadBytes=0;
    require(threw&&!shortReader.Position()&&before==frontend_fixture::streamState()&&owned==frontend_fixture::counts()&&!std::memcmp(snapshot.data(),dst,snapshot.size()),"short header fails before output/commit");++count;
#if BO1_NATIVE_64BIT
    aliasIndex=0;for(unsigned w:{0u,4u,12u,16u})require(oldAliases[aliasIndex++]==DB_StreamAliasSlotLookup(0,w),"failed header preserves block0 side/header aliases before reset");require(oldExact==DB_StreamExactLookup(0,0),"failed header preserves exact mapping before reset");
#endif
    std::printf("{\"fixture\":\"header_actual\",\"status\":\"ok\",\"no_mutation\":false,\"cases\":3}\n");
    DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
    for(unsigned w:{0u,4u,12u,16u})require(!DB_StreamAliasSlotLookup(0,w)&&!DB_StreamExactLookup(0,w),"header mappings detached before release");
#endif
    DB_FreeXBlocks(blocks,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"header fixture release baseline");return count;
}
