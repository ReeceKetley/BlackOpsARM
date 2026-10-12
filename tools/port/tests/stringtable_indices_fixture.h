#pragma once
#include <database/db_scalar_load.h>
static void seedEnvelope(const unsigned char*,XAssetList&);
static_assert(sizeof(*varshort)==2);
static __int16*executeIndices(const StringTableIndicesPlan&p,DecodedSource&r,XBlock(&blocks)[7],bool measurement){
    require(!varshort,"scalar loader storage clear on entry");if(!p.present)return nullptr;
    require(g_streamPosIndex==4,"original indices use child block4 scope");const auto source=r.Current();const auto position=r.Position();
    auto*dst=reinterpret_cast<__int16*>(DB_AllocStreamPos(1));require(reinterpret_cast<unsigned char*>(dst)==blocks[4].data+p.nativeStart,"original mask1/native minimum alignment");
    varshort=dst;reader=&r;tableReadBytes=static_cast<std::size_t>(p.diskBytes);
    try{Load_shortArray(true,p.count);}catch(...){reader=nullptr;tableReadBytes=0;varshort=nullptr;throw;}
    reader=nullptr;tableReadBytes=0;varshort=nullptr;
    require(r.Position()==position+p.diskBytes&&DB_GetStreamPos()==blocks[4].data+p.nativeEnd,"scalar wrapper reads exactly two bytes per original index");
    require(!std::memcmp(dst,source,static_cast<std::size_t>(p.diskBytes)),"every original index byte preserved without sorting");
    if(measurement)std::printf("{\"index_measurements\":{\"source_start\":%zu,\"count\":%d,\"bytes\":%llu,\"adler32\":%lu,\"disk_start\":%llu,\"disk_end\":%llu,\"native_start\":%llu,\"native_end\":%llu,\"values\":[",position+52,p.count,(unsigned long long)p.diskBytes,adler32(1,source,static_cast<unsigned>(p.diskBytes)),(unsigned long long)p.diskStart,(unsigned long long)p.diskEnd,(unsigned long long)p.nativeStart,(unsigned long long)p.nativeEnd);
    std::vector<bool>seen(static_cast<std::size_t>(p.count));
    for(int i=0;i<p.count;++i){std::int16_t expected;std::memcpy(&expected,source+2*std::size_t(i),2);require(dst[i]==expected&&dst[i]>=0&&dst[i]<p.count&&!seen[std::size_t(dst[i])],"every signed16 word equals original permutation");seen[std::size_t(dst[i])]=true;if(measurement)std::printf("%s%d",i?",":"",dst[i]);}
#if BO1_NATIVE_64BIT
    for(unsigned i=0;i<p.diskBytes;++i)require(DB_StreamExactLookup(4,static_cast<unsigned>(p.diskStart)+i)==blocks[4].data+p.nativeStart+i&&!DB_StreamAliasSlotLookup(4,static_cast<unsigned>(p.diskStart)+i),"both bytes of scalar indices exact, no pointer aliases");
    require(DB_StreamWin32Pos(4)==p.diskEnd&&!DB_StreamExactLookup(4,static_cast<unsigned>(p.diskEnd)),"scalar shadow and end boundary");
#endif
    if(measurement)std::printf("]}}\n");return dst;
}
static unsigned indicesFixtures(const ZonePlan&zone){
    unsigned count=0;const auto baseline=frontend_fixture::counts();unsigned char data[12]{};
    auto words=[&](std::initializer_list<std::int16_t>values){unsigned i=0;for(auto v:values){std::memcpy(data+2*i,&v,2);++i;}};words({5,2,4,0,3,1});
    auto test=[&](int columns,int rows,unsigned tag,const unsigned char*source,std::size_t bytes,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,StringTableIndicesResult expected){StringTableIndicesPlan p;const auto before=frontend_fixture::streamState();
        require(PlanStringTableIndices(columns,rows,tag,source,bytes,dc,nc,w,n,p)==expected,"controlled original-index plan status");require(before==frontend_fixture::streamState()&&baseline==frontend_fixture::counts()&&!varshort,"index preflight no mutation/owner/loader state");if(expected!=StringTableIndicesResult::Ok)require(!p.present&&!p.count&&!p.diskBytes&&!p.nativeBytes&&!p.diskEnd&&!p.nativeEnd,"failed index plan cleared");
        std::printf("{\"fixture\":\"indices_preflight_%u\",\"status\":\"%s\",\"no_mutation\":true}\n",++count,StringTableIndicesResultName(expected));};
    test(-1,1,0,nullptr,0,0,0,0,0,StringTableIndicesResult::NegativeDimension);
    test(1,-1,0,nullptr,0,0,0,0,0,StringTableIndicesResult::NegativeDimension);
    test(INT32_MAX,INT32_MAX,0,nullptr,0,0,0,0,0,StringTableIndicesResult::CountOverflow);
    test(2,3,0,nullptr,0,0,0,0,0,StringTableIndicesResult::Ok);
    test(INT32_MAX,1,0,nullptr,0,0,0,0,0,StringTableIndicesResult::Ok);
    test(0,3,1,nullptr,0,0,0,0,0,StringTableIndicesResult::NativeBounds);
    test(0,3,1,nullptr,0,0,sizeof(void*),0,0,StringTableIndicesResult::Ok);
    test(2,3,1,data,11,100,100,0,0,StringTableIndicesResult::SourceBounds);
    test(2,3,1,nullptr,12,100,100,0,0,StringTableIndicesResult::InvalidSource);
    test(2,3,0xfffffffeu,data,12,11,100,0,0,StringTableIndicesResult::DiskBounds);
    test(2,3,0x80000001u,data,12,100,11,0,0,StringTableIndicesResult::NativeBounds);
    test(0,0,0,nullptr,0,0,1,1,0,StringTableIndicesResult::DiskBounds);
    test(0,0,0,nullptr,0,1,0,0,1,StringTableIndicesResult::NativeBounds);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,UINT64_MAX,0,StringTableIndicesResult::DiskBounds);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,0,UINT64_MAX,StringTableIndicesResult::NativeBounds);
    test(32769,1,1,data,12,UINT64_MAX,UINT64_MAX,0,0,StringTableIndicesResult::IndexDomain);
    test(INT32_MAX,1,1,data,12,UINT64_MAX,UINT64_MAX,0,0,StringTableIndicesResult::CommitOverflow);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,UINT32_MAX,0,StringTableIndicesResult::CommitOverflow);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,0,UINT32_MAX,StringTableIndicesResult::CommitOverflow);
    test(2,3,1,data,12,13,100,1,0,StringTableIndicesResult::DiskBounds);
    test(2,3,1,data,12,100,(BO1_NATIVE_64BIT?8:2)+11,0,1,StringTableIndicesResult::NativeBounds);
    test(2,3,0x12345678u,data,12,14,100,1,1,StringTableIndicesResult::Ok);
    words({-1,2,4,0,3,1});test(2,3,1,data,12,100,100,0,0,StringTableIndicesResult::IndexRange);
    words({6,2,4,0,3,1});test(2,3,1,data,12,100,100,0,0,StringTableIndicesResult::IndexRange);
    words({5,2,4,0,3,3});test(2,3,1,data,12,100,100,0,0,StringTableIndicesResult::DuplicateIndex);
    std::vector<unsigned char>large(65536);for(unsigned i=0;i<32768;++i){auto v=static_cast<std::int16_t>(32767-i);std::memcpy(large.data()+2*i,&v,2);}
    test(32768,1,1,large.data(),large.size(),65536,65536,0,0,StringTableIndicesResult::Ok);words({5,2,4,0,3,1});
    frontend_fixture::Owner owner;XBlock blocks[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),blocks,failed)==ZoneResult::Ok,"actual original-index owner");
    for(unsigned offset:{0u,1u})for(int n:{0,1,6}){DB_InitStreams(blocks);DB_PushStreamPos(4);if(offset)DB_IncStreamPos(1);if(n==1)words({0});else words({5,2,4,0,3,1});StringTableIndicesPlan p;require(PlanStringTableIndices(1,n,0xfffffffeu,data,12,zone.file.blockSize[4],zone.logical[4],offset,offset,p)==StringTableIndicesResult::Ok,"actual scalar plan");DecodedSource r(data,12);executeIndices(p,r,blocks,false);DB_PopStreamPos();++count;}
    DB_InitStreams(blocks);std::array<unsigned char,52>envelopeBytes{};XAssetList envelope{};seedEnvelope(envelopeBytes.data(),envelope);
    DB_PushStreamPos(4);StringTableIndicesPlan earlier;require(PlanStringTableIndices(1,6,1,data,12,zone.file.blockSize[4],zone.logical[4],0,0,earlier)==StringTableIndicesResult::Ok,"seed real earlier scalar mappings");DecodedSource goodReader(data,12);executeIndices(earlier,goodReader,blocks,false);
    auto*dst=reinterpret_cast<__int16*>(DB_AllocStreamPos(1));const auto before=frontend_fixture::streamState();const auto owned=frontend_fixture::counts();std::array<unsigned char,12>snapshot{};std::memcpy(snapshot.data(),dst,12);
#if BO1_NATIVE_64BIT
    const auto envelopeAlias=DB_StreamAliasSlotLookup(0,12);const auto envelopeExact=DB_StreamExactLookup(0,0);require(envelopeAlias==reinterpret_cast<uintptr_t*>(&envelope.assets)&&DB_StreamExactLookup(4,0)==blocks[4].data,"earlier scalar and envelope mappings exist before failed read");
    std::array<unsigned char*,24>exact{};std::array<uintptr_t*,24>aliases{};for(unsigned i=0;i<24;++i){exact[i]=DB_StreamExactLookup(4,i);aliases[i]=DB_StreamAliasSlotLookup(4,i);}
#endif
    DecodedSource shortReader(data,11);reader=&shortReader;tableReadBytes=12;varshort=dst;bool threw=false;try{Load_shortArray(true,6);}catch(const std::runtime_error&){threw=true;}reader=nullptr;tableReadBytes=0;
    require(threw&&varshort==dst&&!shortReader.Position()&&before==frontend_fixture::streamState()&&owned==frontend_fixture::counts()&&!std::memcmp(snapshot.data(),dst,12),"short scalar wrapper fails before any copy/cursor/map mutation");varshort=nullptr;
#if BO1_NATIVE_64BIT
    require(envelopeAlias==DB_StreamAliasSlotLookup(0,12)&&envelopeExact==DB_StreamExactLookup(0,0),"failed scalar preserves existing envelope alias/exact mapping");
    for(unsigned i=0;i<24;++i)require(exact[i]==DB_StreamExactLookup(4,i)&&aliases[i]==DB_StreamAliasSlotLookup(4,i),"short index mappings unchanged before reset");
#endif
    ++count;std::printf("{\"fixture\":\"indices_actual\",\"status\":\"ok\",\"no_mutation\":false,\"cases\":7}\n");DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
    require(!DB_StreamAliasSlotLookup(0,12)&&!DB_StreamExactLookup(0,0),"earlier envelope maps detached");
    for(unsigned i=0;i<24;++i)require(!DB_StreamExactLookup(4,i)&&!DB_StreamAliasSlotLookup(4,i),"scalar maps detached before owner release");
#endif
    DB_FreeXBlocks(blocks,0);frontend_fixture::release(&owner);require(!varshort&&frontend_fixture::counts()==baseline,"scalar loader invalidated and cleanup baseline");return count;
}
