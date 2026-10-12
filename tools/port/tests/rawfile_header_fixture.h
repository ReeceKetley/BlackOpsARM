#pragma once
#include <platform/fastfile_rawfile_header.h>
#include <database/db_rawfile_types.h>
namespace rawfile_fixture {
struct Next { int type{};unsigned tag{};std::size_t source{},end{};RawFileHeaderPlan header; };
static Next plan(const Front& f,const AssetDirectoryPlan& d,const StringTablePayloadPlan& payload,
                 const unsigned char* data,std::size_t bytes,std::uint64_t decoded){
    require(d.present&&d.count>1&&f.end<=bytes&&16<=bytes-f.end,"genuine next directory entry exists");
    Next p;p.type=static_cast<int>(u32(data+f.end+8));p.tag=u32(data+f.end+12);p.source=p.end=payload.end;
    require(p.source<=bytes&&p.source<=decoded,"next header follows completed first payload");
    if(p.type==36){
        require(PlanRawFileHeader(p.type,p.tag,data+p.source,bytes-p.source,decoded-p.source,
            f.envelope.zone.file.blockSize[0],f.envelope.zone.logical[0],0,0,p.header)==RawFileHeaderResult::Ok&&p.header.present,
            "genuine inline RawFile numeric header checked before owned writes");
        p.end+=12;
    }
    return p;
}
static unsigned fixtures(){
    const auto before=frontend_fixture::streamState();const auto owners=frontend_fixture::counts();
    std::uint32_t data[3]={0xffffffffu,140,0xffffffffu};unsigned count=0;
    auto test=[&](int type,unsigned tag,const unsigned char* src,std::size_t bytes,std::uint64_t remaining,
                  std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,RawFileHeaderResult expected){
        RawFileHeaderPlan p;require(PlanRawFileHeader(type,tag,src,bytes,remaining,dc,nc,w,n,p)==expected,"checked RawFile preflight status");
        require(frontend_fixture::streamState()==before&&frontend_fixture::counts()==owners,"RawFile plan never changes streams or ownership");
        if(expected!=RawFileHeaderResult::Ok)require(!p.present&&!p.nameTag&&!p.bufferTag&&!p.length&&!p.futureBufferBytes&&!p.diskEnd&&!p.nativeEnd,"failed RawFile plan clears output");
        ++count;
    };
    const auto*src=reinterpret_cast<const unsigned char*>(data);
    test(36,0xffffffffu,src,12,1000,36,64,0,0,RawFileHeaderResult::Ok);
    test(7,0xffffffffu,src,12,1000,36,64,0,0,RawFileHeaderResult::UnsupportedAsset);
    test(36,0,nullptr,0,0,36,64,0,0,RawFileHeaderResult::Ok);
    test(36,1,src,12,1000,36,64,0,0,RawFileHeaderResult::UnsupportedTag);
    test(36,0xfffffffeu,src,12,1000,36,64,0,0,RawFileHeaderResult::UnsupportedTag);
    test(36,0xffffffffu,src,11,1000,36,64,0,0,RawFileHeaderResult::ShortHeader);
    test(36,0xffffffffu,nullptr,12,1000,36,64,0,0,RawFileHeaderResult::ShortHeader);
    test(36,0xffffffffu,src,12,11,36,64,0,0,RawFileHeaderResult::SourceBounds);
    data[1]=0xffffffffu;test(36,0xffffffffu,src,12,1000,36,64,0,0,RawFileHeaderResult::NegativeLength);
    data[1]=INT32_MAX;test(36,0xffffffffu,src,12,UINT64_MAX,36,64,0,0,RawFileHeaderResult::LengthOverflow);
    data[1]=140;test(36,0xffffffffu,src,12,152,36,64,0,0,RawFileHeaderResult::SourceBounds);
    test(36,0xffffffffu,src,12,1000,11,64,0,0,RawFileHeaderResult::DiskBounds);
    test(36,0xffffffffu,src,12,1000,36,sizeof(RawFile)-1,0,0,RawFileHeaderResult::NativeBounds);
    test(36,0xffffffffu,src,12,1000,36,64,37,0,RawFileHeaderResult::DiskBounds);
    test(36,0xffffffffu,src,12,1000,36,64,0,65,RawFileHeaderResult::NativeBounds);
    test(36,0xffffffffu,src,12,1000,UINT64_MAX,64,UINT64_MAX,0,RawFileHeaderResult::DiskBounds);
    test(36,0xffffffffu,src,12,1000,36,UINT64_MAX,0,UINT64_MAX,RawFileHeaderResult::NativeBounds);
    test(36,0xffffffffu,src,12,1000,36,64,1,1,RawFileHeaderResult::Ok);
    data[1]=0;test(36,0xffffffffu,src,12,13,36,64,0,0,RawFileHeaderResult::Ok);
    data[1]=INT32_MAX;data[2]=0;test(36,0xffffffffu,src,12,12,36,64,0,0,RawFileHeaderResult::Ok);
    RawFileHeaderPlan absent;require(PlanRawFileHeader(36,0xffffffffu,src,12,12,36,64,0,0,absent)==RawFileHeaderResult::Ok&&!absent.bufferTag&&!absent.futureBufferBytes,"absent buffer does not fabricate a child request");
    data[1]=140;test(36,0xffffffffu,src,12,12,36,64,0,0,RawFileHeaderResult::Ok);data[2]=0xffffffffu;
    test(36,0xffffffffu,src,12,1000,36,0,0,0,RawFileHeaderResult::NativeBounds);
    test(36,0xffffffffu,src,12,1000,UINT64_MAX,64,UINT32_MAX-3,0,RawFileHeaderResult::DiskBounds);
    test(36,0xffffffffu,src,12,1000,36,UINT64_MAX,0,UINT32_MAX-7,RawFileHeaderResult::NativeBounds);
    require(count==24,"complete RawFile planner case inventory");
    std::printf("{\"rawfile_fixtures\":{\"cases\":24,\"failed_plans_clear_output\":true,\"no_stream_or_owner_mutation\":true,\"buffer_absence_preserved\":true}}\n");
    return count;
}
static void execute(const Next& p,const StringTablePayloadPlan& payload,DecodedSource& r,XBlock(&blocks)[7],bool measurement){
    require(g_streamPosIndex==4&&r.Position()+52==p.source,"next header begins at original source boundary in parent4");
    if(!p.header.present){
        require(p.end==p.source,"unsupported next asset is deferred without skipping");
        if(measurement)std::printf("{\"rawfile_measurements\":{\"asset_type\":%d,\"directory_index\":1,\"status\":\"deferred_next_type\",\"source_start\":%zu,\"source_end\":%zu,\"disk_start\":0,\"native_start\":0,\"disk_end\":0,\"native_end_before_pop\":0,\"native_cursor_after_pop\":0,\"shadow_after_pop\":0,\"name_tag\":null,\"length\":null,\"buffer_tag\":null,\"future_buffer_bytes\":0,\"native_header_bytes\":0,\"retained_value_alias_verified\":false,\"retained_alias_address_exposed\":false,\"pooled_queries_after_raw_verified\":false,\"block4_bytes_cursors_preserved\":true,\"children_unread\":true,\"full_registry_executed\":false,\"next_asset_skipped\":false}}\n",p.type,p.source,p.end);
        return;
    }
    const auto block4=DB_GetStreamPos();const auto disk4=payload.indices.diskEnd;
    const auto childHash=adler32(1,blocks[4].data,static_cast<unsigned>(payload.indices.nativeEnd));
    const auto* pooled=owner_fixture::persistent;require(pooled,"first real StringTable has persistent ownership");
    const auto previousValues=reinterpret_cast<uintptr_t>(pooled->values),previousIndices=reinterpret_cast<uintptr_t>(pooled->cellIndex);
    DB_PushStreamPos(0);auto* header=reinterpret_cast<RawFile*>(DB_AllocStreamPos(3));
    require(reinterpret_cast<unsigned char*>(header)==blocks[0].data+p.header.nativeStart,"native TEMP restarts at the checked entry");
    reader=&r;tableReadBytes=12;
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_RawFile(header);
#else
    Load_Stream(true,reinterpret_cast<unsigned char*>(header),12);
#endif
    reader=nullptr;tableReadBytes=0;
    const auto tag=[](unsigned v)->uintptr_t{return v==0xffffffffu?UINTPTR_MAX:v==0xfffffffeu?UINTPTR_MAX-1:v;};
    require(reinterpret_cast<uintptr_t>(header->name)==tag(p.header.nameTag)&&header->len==p.header.length&&reinterpret_cast<uintptr_t>(header->buffer)==tag(p.header.bufferTag),"actual RawFile body preserves all genuine numeric fields");
    require(r.Position()+52==p.end&&DB_GetStreamPos()==blocks[0].data+p.header.nativeEnd,"only the numeric RawFile header was read and committed");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(0)==p.header.diskEnd,"RawFile keeps twelve-byte disk extent");
    uintptr_t value{};
    require(DB_StreamResolveAliasValue(0,12,&value)==DB_StreamAliasResult::Retained&&value==previousValues&&!DB_StreamAliasSlotLookup(0,12),"genuine old values are retained separately from overwritten storage");
    require(DB_StreamResolveAliasValue(0,16,&value)==DB_StreamAliasResult::Active&&value==previousIndices,"untouched old index pointer slot remains live");
    require(DB_StreamResolveAliasValue(0,0,&value)==DB_StreamAliasResult::Active&&value==tag(p.header.nameTag),"new RawFile name slot takes latest authority");
    require(DB_StreamResolveAliasValue(0,8,&value)==DB_StreamAliasResult::Active&&value==tag(p.header.bufferTag),"new RawFile buffer slot takes latest authority");
    require(DB_StreamResolveAliasValue(0,4,&value)==DB_StreamAliasResult::External,"external envelope registration is preserved separately");
    require(DB_StreamExactLookup(0,0)==reinterpret_cast<unsigned char*>(header)&&!DB_StreamMapLookup(0,12),"new object start is exact; retained value does not fabricate an address");
    const auto* bytes=reinterpret_cast<const unsigned char*>(header);for(unsigned i=12;i<16;++i)require(bytes[i]==0,"native RawFile padding is initialized");
#endif
    // Controlled alias inputs use the genuine live pointers. They are not claims
    // of encoded block0 offsets in this FastFile (none in the covered metadata).
    uintptr_t alias=13;DB_ConvertOffsetToAlias(&alias);require(alias==previousValues,"actual alias conversion matches the untouched original Win32 word");
    alias=17;DB_ConvertOffsetToAlias(&alias);require(alias==previousIndices,"actual alias conversion preserves untouched original index word");
    DB_PopStreamPos();require(g_streamPosIndex==4&&DB_GetStreamPos()==block4&&g_streamPosArray[0]==blocks[0].data&&tempDiskCursor()==0,"both TEMP cursors rewind after next genuine header");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(4)==disk4,"persistent block4 disk cursor unchanged");
#endif
    require(adler32(1,blocks[4].data,static_cast<unsigned>(payload.indices.nativeEnd))==childHash,"RawFile header does not touch first asset children");
    executeLookups(payload,owner_fixture::persistent,false);
    require(!lastLookup.rowMisses&&!lastLookup.rowDisagreements&&!lastLookup.columnDisagreements,"pooled first asset queries pass immediately after real RawFile overwrite");
    if(measurement)std::printf("{\"rawfile_measurements\":{\"asset_type\":%d,\"directory_index\":1,\"status\":\"header_loaded\",\"source_start\":%zu,\"source_end\":%zu,\"disk_start\":%llu,\"native_start\":%llu,\"disk_end\":%llu,\"native_end_before_pop\":%llu,\"native_cursor_after_pop\":0,\"shadow_after_pop\":%u,\"name_tag\":%u,\"length\":%d,\"buffer_tag\":%u,\"future_buffer_bytes\":%llu,\"native_header_bytes\":%zu,\"retained_value_alias_verified\":true,\"retained_alias_address_exposed\":false,\"pooled_queries_after_raw_verified\":true,\"block4_bytes_cursors_preserved\":true,\"children_unread\":true,\"full_registry_executed\":false,\"next_asset_skipped\":false}}\n",p.type,p.source,p.end,(unsigned long long)p.header.diskStart,(unsigned long long)p.header.nativeStart,(unsigned long long)p.header.diskEnd,(unsigned long long)p.header.nativeEnd,tempDiskCursor(),p.header.nameTag,p.header.length,p.header.bufferTag,(unsigned long long)p.header.futureBufferBytes,sizeof(RawFile));
}
}
