#pragma once
#include <platform/fastfile_stringtable_cells.h>
static_assert(sizeof(StringTableCell)==(BO1_NATIVE_64BIT?16:8));
static_assert(offsetof(StringTableCell,string)==0&&offsetof(StringTableCell,hash)==sizeof(void*));
struct StringTablePayloadPlan{RawStringPlan name;StringTableCellsPlan cells;std::size_t nameSource{},cellSource{},end{};};
static StringTablePayloadPlan payloadPlan(const Front&f,const AssetDirectoryPlan&d,const StringTableHeaderPlan&h,const unsigned char*data,std::size_t bytes){
    StringTablePayloadPlan p;p.nameSource=f.end+static_cast<std::size_t>(d.diskBytes)+20;require(p.nameSource<=bytes,"name follows checked numeric header");
    require(PlanRawString(h.nameTag,data+p.nameSource,bytes-p.nameSource,f.envelope.zone.file.blockSize[4],f.envelope.zone.logical[4],d.diskEnd,d.nativeEnd,p.name)==RawStringResult::Ok,"checked genuine name capacity/source before allocation");
    p.cellSource=p.nameSource+static_cast<std::size_t>(p.name.bytes);require(p.cellSource<=bytes,"name end bounded");
    require(PlanStringTableCells(h.columns,h.rows,h.valuesTag,data+p.cellSource,bytes-p.cellSource,f.envelope.zone.file.blockSize[4],f.envelope.zone.logical[4],p.name.diskEnd,p.name.nativeEnd,p.cells)==StringTableCellsResult::Ok,"checked genuine cell records before allocation");
    p.end=p.cellSource+static_cast<std::size_t>(p.cells.diskBytes);return p;
}
static StringTableCell*executeCells(const StringTableCellsPlan&p,DecodedSource&r,XBlock(&blocks)[7],bool measurement){
    if(!p.present)return nullptr;require(g_streamPosIndex==4,"cell arrays use actual block4 loading scope");
    const auto sourceStart=r.Current();const auto position=r.Position();auto*cells=reinterpret_cast<StringTableCell*>(DB_AllocStreamPos(3));
    require(reinterpret_cast<unsigned char*>(cells)==blocks[4].data+p.nativeStart,"cell array owned native alignment");reader=&r;tableReadBytes=static_cast<std::size_t>(p.diskBytes);
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_StringTableCellArray(cells,p.count);
#else
    Load_Stream(true,reinterpret_cast<unsigned char*>(cells),static_cast<int>(p.diskBytes));
#endif
    reader=nullptr;tableReadBytes=0;require(r.Position()==position+p.diskBytes&&DB_GetStreamPos()==blocks[4].data+p.nativeEnd,"exact cell bytes and native cursor");
    if(measurement)std::printf("{\"cell_measurements\":[");
    for(int i=0;i<p.count;++i){const auto tag=u32(sourceStart+8*std::size_t(i));std::int32_t hash;std::memcpy(&hash,sourceStart+8*std::size_t(i)+4,4);
        require(reinterpret_cast<uintptr_t>(cells[i].string)==widenTag(tag)&&cells[i].hash==hash,"every genuine cell tag and signed hash preserved unresolved");
#if BO1_NATIVE_64BIT
        auto*element=reinterpret_cast<unsigned char*>(&cells[i]);for(unsigned j=12;j<16;++j)require(element[j]==0,"native cell padding zero");
        const auto w=static_cast<unsigned>(p.diskStart+8*std::uint64_t(i));require(DB_StreamExactLookup(4,w)==element,"cell starts map exactly");
        for(unsigned j=1;j<8;++j)require(!DB_StreamExactLookup(4,w+j),"cell interior including hash has no exact expanded mapping");
        require(DB_StreamAliasSlotLookup(4,w)==reinterpret_cast<uintptr_t*>(&cells[i].string)&&!DB_StreamAliasSlotLookup(4,w+4),"only string field alias at packed cell start");
#endif
        if(measurement)std::printf("%s{\"index\":%d,\"tag\":%u,\"signed_hash\":%d}",i?",":"",i,tag,hash);
    }
    if(measurement)std::printf("]}\n");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(4)==p.diskEnd&&!DB_StreamExactLookup(4,static_cast<unsigned>(p.diskEnd)),"cell shadow/end boundary");
#endif
    return cells;
}
static void executePayload(const StringTablePayloadPlan&p,StringTable*header,DecodedSource&r,XBlock(&blocks)[7],bool measurement){
    require(g_streamPosIndex==0&&r.Position()+52==p.nameSource,"header alive before child loading scope");const auto headerCursor=DB_GetStreamPos();
    DB_PushStreamPos(4);if(p.name.present){const char*name=reinterpret_cast<const char*>(DB_AllocStreamPos(0));require(name==reinterpret_cast<char*>(blocks[4].data+p.name.nativeStart),"name native alignment");
        reader=&r;Load_XStringCustom(&name);reader=nullptr;require(!std::memcmp(name,r.Current()-p.name.bytes,static_cast<std::size_t>(p.name.bytes)),"actual raw name bytes including NUL match source");header->name=name;
#if BO1_NATIVE_64BIT
        for(unsigned i=0;i<p.name.bytes;++i)require(DB_StreamExactLookup(4,static_cast<unsigned>(p.name.diskStart)+i)==blocks[4].data+p.name.nativeStart+i,"name character offsets exact");
#endif
    }else require(!header->name,"absent name remains absent");
    require(r.Position()+52==p.cellSource&&DB_GetStreamPos()==blocks[4].data+p.name.nativeEnd,"name source/native endpoint before cells");
    header->values=executeCells(p.cells,r,blocks,measurement);require(r.Position()+52==p.end,"prefix stops before per-cell string payloads");
    if(measurement)std::printf("{\"name_measurements\":{\"source_start\":%zu,\"length\":%llu,\"adler32\":%lu,\"disk_start\":%llu,\"disk_end\":%llu,\"native_start\":%llu,\"native_end\":%llu}}\n",p.nameSource,(unsigned long long)p.name.bytes,p.name.present?adler32(1,blocks[4].data+p.name.nativeStart,static_cast<unsigned>(p.name.bytes)):1ul,(unsigned long long)p.name.diskStart,(unsigned long long)p.name.diskEnd,(unsigned long long)p.name.nativeStart,(unsigned long long)p.name.nativeEnd);
    DB_PopStreamPos();require(g_streamPosIndex==0&&DB_GetStreamPos()==headerCursor,"child scope retains live header end before outer TEMP pop");
}
static unsigned cellsFixtures(const ZonePlan&zone){
    unsigned count=0;const auto baseline=frontend_fixture::counts();unsigned char data[48]{};
    auto test=[&](int columns,int rows,unsigned tag,const unsigned char*source,std::size_t bytes,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,StringTableCellsResult expected){StringTableCellsPlan p;const auto before=frontend_fixture::streamState();
        require(PlanStringTableCells(columns,rows,tag,source,bytes,dc,nc,w,n,p)==expected,"controlled cell plan result");require(before==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"cell preflight no mutation");if(expected!=StringTableCellsResult::Ok)require(!p.present&&!p.count&&!p.diskBytes&&!p.nativeBytes&&!p.diskEnd&&!p.nativeEnd,"cell failed plan cleared");
        std::printf("{\"fixture\":\"cells_preflight_%u\",\"status\":\"%s\",\"no_mutation\":true}\n",++count,StringTableCellsResultName(expected));};
    test(-1,1,0,nullptr,0,0,0,0,0,StringTableCellsResult::NegativeDimension);
    test(1,-1,0,nullptr,0,0,0,0,0,StringTableCellsResult::NegativeDimension);
    test(INT32_MAX,INT32_MAX,0,nullptr,0,0,0,0,0,StringTableCellsResult::CountOverflow);
    test(2,3,0,nullptr,0,0,0,0,0,StringTableCellsResult::Ok);
    test(0,3,1,nullptr,0,0,0,0,0,StringTableCellsResult::NativeBounds);
    test(0,3,1,nullptr,0,0,sizeof(void*),0,0,StringTableCellsResult::Ok);
    test(2,3,1,data,47,100,100,0,0,StringTableCellsResult::SourceBounds);
    test(2,3,1,nullptr,48,100,100,0,0,StringTableCellsResult::InvalidSource);
    test(2,3,0xfffffffeu,data,48,47,100,0,0,StringTableCellsResult::DiskBounds);
    test(2,3,0x80000001u,data,48,100,sizeof(StringTableCell)*6-1,0,0,StringTableCellsResult::NativeBounds);
    test(0,0,0,nullptr,0,0,1,1,0,StringTableCellsResult::DiskBounds);
    test(0,0,0,nullptr,0,1,0,0,1,StringTableCellsResult::NativeBounds);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,UINT64_MAX,0,StringTableCellsResult::DiskBounds);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,0,UINT64_MAX,StringTableCellsResult::NativeBounds);
    test((1<<20)+1,1,1,data,48,100,100,0,0,BO1_NATIVE_64BIT?StringTableCellsResult::HelperDomain:StringTableCellsResult::SourceBounds);
    test(INT32_MAX,1,1,data,48,UINT64_MAX,UINT64_MAX,0,0,BO1_NATIVE_64BIT?StringTableCellsResult::HelperDomain:StringTableCellsResult::CommitOverflow);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,UINT32_MAX,0,StringTableCellsResult::CommitOverflow);
    test(0,0,1,nullptr,0,UINT64_MAX,UINT64_MAX,0,UINT32_MAX,StringTableCellsResult::CommitOverflow);
    const unsigned tags[]={0,0xffffffffu,0xfffffffeu,0x800003e9u,0x12345678u,0x80000001u};const std::int32_t hashes[]={INT32_MIN,INT32_MAX,-1,0,1,-890};
    for(unsigned i=0;i<6;++i){std::memcpy(data+8*i,&tags[i],4);std::memcpy(data+8*i+4,&hashes[i],4);}
    frontend_fixture::Owner owner;XBlock blocks[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),blocks,failed)==ZoneResult::Ok,"controlled cell zone");
    for(unsigned offset:{0u,1u})for(int n:{0,1,6}){DB_InitStreams(blocks);DB_PushStreamPos(4);if(offset)DB_IncStreamPos(1);StringTableCellsPlan p;require(PlanStringTableCells(1,n,0xfffffffeu,data,48,zone.file.blockSize[4],zone.logical[4],offset,offset,p)==StringTableCellsResult::Ok,"actual mixed/zero cell plan");DecodedSource r(data,48);executeCells(p,r,blocks,false);DB_PopStreamPos();++count;}
    DB_InitStreams(blocks);DB_PushStreamPos(4);auto*dst=reinterpret_cast<StringTableCell*>(DB_AllocStreamPos(3));const auto before=frontend_fixture::streamState();const auto owned=frontend_fixture::counts();
    std::array<unsigned char,96>snapshot{};std::memcpy(snapshot.data(),dst,snapshot.size());std::array<uintptr_t*,6>aliases{};std::array<unsigned char*,6>exact{};
#if BO1_NATIVE_64BIT
    for(unsigned i=0;i<6;++i){aliases[i]=DB_StreamAliasSlotLookup(4,8*i);exact[i]=DB_StreamExactLookup(4,8*i);}
#endif
    DecodedSource shortReader(data,47);reader=&shortReader;tableReadBytes=48;bool threw=false;try{
#if BO1_NATIVE_64BIT
        DB_ExpandLoad_StringTableCellArray(dst,6);
#else
        Load_Stream(true,reinterpret_cast<unsigned char*>(dst),48);
#endif
    }catch(const std::runtime_error&){threw=true;}reader=nullptr;tableReadBytes=0;require(threw&&!shortReader.Position()&&before==frontend_fixture::streamState()&&owned==frontend_fixture::counts()&&!std::memcmp(snapshot.data(),dst,snapshot.size()),"short cells no partial output or cursor commit before reset");
#if BO1_NATIVE_64BIT
    for(unsigned i=0;i<6;++i)require(aliases[i]==DB_StreamAliasSlotLookup(4,8*i)&&exact[i]==DB_StreamExactLookup(4,8*i),"short cells preserve aliases/exact before reset");
#endif
    ++count;std::printf("{\"fixture\":\"cells_actual\",\"status\":\"ok\",\"no_mutation\":false,\"cases\":7}\n");DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
    for(unsigned i=0;i<6;++i)require(!DB_StreamAliasSlotLookup(4,8*i)&&!DB_StreamExactLookup(4,8*i),"cells detached before owner release");
#endif
    DB_FreeXBlocks(blocks,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"cell failure cleanup baseline");return count;
}
