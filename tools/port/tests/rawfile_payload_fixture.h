#pragma once
#include <platform/fastfile_rawfile_payload.h>
#include <database/db_rawfile_load.h>
namespace rawfile_payload_fixture {
static void clear(){varRawFile=nullptr;varXString=nullptr;varConstChar=nullptr;}
static RawFilePayloadPlan plan(const RawFileHeaderPlan&h,const unsigned char*data,std::size_t size,
    std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n){
    RawFilePayloadPlan p;require(PlanRawFilePayload(h.nameTag,h.length,h.bufferTag,data,size,dc,nc,w,n,p)==RawFilePayloadResult::Ok,"complete actual RawFile children preflight before any writes");return p;
}
static unsigned fixtures(){
    const auto baseline=frontend_fixture::counts();const auto state=frontend_fixture::streamState();
    const unsigned char data[]={0x78,0,0x41,0,0xa7};unsigned cases=0;
    auto test=[&](unsigned tag,int len,unsigned buffer,const unsigned char*src,std::size_t bytes,
        std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,RawFilePayloadResult result){
        RawFilePayloadPlan p;require(PlanRawFilePayload(tag,len,buffer,src,bytes,dc,nc,w,n,p)==result,"RawFile children controlled plan status");
        require(state==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"payload plan never mutates streams/owners");
        if(result!=RawFilePayloadResult::Ok)require(!p.name.present&&!p.sourceBytes&&!p.bufferPresent&&!p.bufferBytes&&!p.diskEnd&&!p.nativeEnd,"failed payload plan clears all output");
        ++cases;
    };
    const auto ok=RawFilePayloadResult::Ok;
    test(0xffffffffu,2,0xffffffffu,data,5,64,64,0,0,ok);
    test(0,2,1,data,3,64,64,0,0,ok);
    test(0xffffffffu,INT32_MAX,0,data,2,64,64,0,0,ok);
    test(0xffffffffu,2,1,data,5,64,64,0,0,ok);
    test(0xffffffffu,2,0xfffffffeu,data,5,64,64,0,0,ok);
    test(1,2,1,data,5,64,64,0,0,RawFilePayloadResult::Name);
    test(0xfffffffeu,2,1,data,5,64,64,0,0,RawFilePayloadResult::Name);
    test(0xffffffffu,0,0,data,1,64,64,0,0,RawFilePayloadResult::Name);
    test(0xffffffffu,0,0,nullptr,2,64,64,0,0,RawFilePayloadResult::Name);
    test(0,0,1,nullptr,1,64,64,0,0,RawFilePayloadResult::SourceBounds);
    test(0xffffffffu,2,1,data,4,64,64,0,0,RawFilePayloadResult::SourceBounds);
    test(0,-1,1,data,5,64,64,0,0,RawFilePayloadResult::BufferLength);
    test(0,INT32_MAX,1,data,5,64,64,0,0,RawFilePayloadResult::BufferLength);
    test(0,0,0,nullptr,0,64,64,65,0,RawFilePayloadResult::DiskBounds);
    test(0,0,0,nullptr,0,64,64,0,65,RawFilePayloadResult::NativeBounds);
    test(0xffffffffu,2,1,data,5,1,64,0,0,RawFilePayloadResult::Name);
    test(0xffffffffu,2,1,data,5,64,1,0,0,RawFilePayloadResult::Name);
    test(0xffffffffu,2,1,data,5,4,64,0,0,RawFilePayloadResult::DiskBounds);
    test(0xffffffffu,2,1,data,5,64,4,0,0,RawFilePayloadResult::NativeBounds);
    test(0,0,1,data,1,64,UINT64_MAX,0,UINT64_MAX,RawFilePayloadResult::NativeBounds);
    test(0,0,1,data,1,UINT64_MAX,64,UINT32_MAX,0,RawFilePayloadResult::CommitOverflow);
    test(0,0,1,data,1,64,UINT64_MAX,0,UINT32_MAX,RawFilePayloadResult::CommitOverflow);
    test(0,0,1,data,1,64,64,0,0,ok);
    test(0,INT32_MAX,0,nullptr,0,64,64,0,0,ok);
    require(cases==24,"payload planner inventory complete");
    unsigned actual=0;
    for(unsigned fixture=0;fixture<5;++fixture){
        alignas(16) unsigned char temp[64]{},children[64]{};XBlock blocks[7]{};blocks[0]={temp,64};blocks[4]={children,64};
        std::array<unsigned char,17> src{};std::uint32_t header[3]={0xffffffffu,2,1};std::size_t size=17;
        src[12]=0x78;src[13]=0;src[14]=0x41;src[15]=0;src[16]=0xa7;
        if(fixture==1){header[0]=0;header[1]=0;header[2]=0xfffffffeu;src[12]=0xa7;size=13;}
        if(fixture==2){header[1]=INT32_MAX;header[2]=0;size=14;}
        if(fixture==3){header[0]=0;header[1]=140;header[2]=0;size=12;}
        if(fixture==4)size=15;
        std::memcpy(src.data(),header,12);DecodedSource r(src.data(),size);DB_InitStreams(blocks);DB_PushStreamPos(0);
        varRawFile=reinterpret_cast<RawFile*>(DB_AllocStreamPos(3));reader=&r;tableReadBytes=12;bool threw=false;
        try{Load_RawFile(true);}catch(const std::runtime_error&){threw=true;}
        reader=nullptr;tableReadBytes=0;
        require(threw==(fixture==4),"actual wrapper controlled exact read/failure");
        if(threw){
            require(g_streamPosIndex==4&&r.Position()==14,"finite short buffer fails after actual header and name; not atomic rollback");
            DB_PopStreamPos(); // Fixture unwind, not production longjmp recovery.
        }else{
            require(g_streamPosIndex==0&&r.Position()==size,"actual wrapper balanced children scope and exact source count");
            if(header[0])require(varRawFile->name==reinterpret_cast<char*>(children)&&!std::memcmp(varRawFile->name,src.data()+12,2),"actual inline name wrapper");
            else require(!varRawFile->name,"actual absent name");
            if(header[2]){const auto count=header[1]+1;const auto source=header[0]?14u:12u;
                require(varRawFile->buffer&&!std::memcmp(varRawFile->buffer,src.data()+source,count),"buffer presence and binary/nonzero extra byte preserve count semantics");}
            else require(!varRawFile->buffer,"absent buffer never adds a read even at INTMAX");
        }
        clear();DB_PopStreamPos();DB_InitStreams(detached);require(baseline==frontend_fixture::counts(),"controlled wrapper cleanup preserves logical owner baseline");++actual;
    }
    require(actual==5&&state==frontend_fixture::streamState(),"all wrapper cases finish detached");
    std::printf("{\"rawfile_payload_fixtures\":{\"planner_cases\":24,\"actual_wrapper_cases\":5,\"failed_plans_clear_output\":true,\"logical_baseline_restored\":true,\"buffer_presence_semantics\":true,\"binary_count_semantics\":true,\"controlled_failure_is_atomic\":false}}\n");
    return cases+actual;
}
static void execute(const RawFilePayloadPlan&p,RawFile*header,DecodedSource&r,XBlock(&blocks)[7],
    const StringTablePayloadPlan&first,std::size_t source,bool measurement){
    const auto firstHash=adler32(1,blocks[4].data,static_cast<unsigned>(first.indices.nativeEnd));
    std::array<unsigned char,sizeof(void*)>padding{};
    const auto pad=p.bufferNativeStart-p.name.nativeEnd;
    require(pad<padding.size(),"raw buffer native padding follows actual allocation policy");
    std::memcpy(padding.data(),blocks[4].data+p.name.nativeEnd,static_cast<std::size_t>(pad));
    varRawFile=header;reader=&r;tableReadBytes=0;Load_RawFile(false);reader=nullptr;
    require(g_streamPosIndex==0&&r.Position()+52==source+p.sourceBytes,"actual follow-up wrapper loads exact children and returns to live header scope");
    require(header->name==reinterpret_cast<char*>(blocks[4].data+p.name.nativeStart)&&!std::memcmp(header->name,r.Current()-p.sourceBytes,p.name.bytes),"genuine complete name bytes");
    require(header->buffer==reinterpret_cast<char*>(blocks[4].data+p.bufferNativeStart)&&!std::memcmp(header->buffer,r.Current()-p.bufferBytes,p.bufferBytes),"genuine exact len+1 buffer bytes");
    require(g_streamPosArray[4]==blocks[4].data+p.nativeEnd,"actual children native cursor matches checked plan");
    require(adler32(1,blocks[4].data,static_cast<unsigned>(first.indices.nativeEnd))==firstHash,"existing first asset child bytes preserved");
    require(!std::memcmp(padding.data(),blocks[4].data+p.name.nativeEnd,static_cast<std::size_t>(pad)),"buffer alignment padding is preserved");
#if BO1_NATIVE_64BIT
    uintptr_t value{};require(DB_StreamWin32Pos(4)==p.diskEnd,"actual child disk shadow matches original4-byte coordinates");
    require(DB_StreamResolveAliasValue(0,0,&value)==DB_StreamAliasResult::Active&&value==reinterpret_cast<uintptr_t>(header->name),"live name alias reads current pointer after wrapper");
    require(DB_StreamResolveAliasValue(0,8,&value)==DB_StreamAliasResult::Active&&value==reinterpret_cast<uintptr_t>(header->buffer),"live buffer alias reads current pointer after wrapper");
    require(DB_StreamResolveAliasValue(0,12,&value)==DB_StreamAliasResult::Retained&&value==reinterpret_cast<uintptr_t>(owner_fixture::persistent->values)&&!DB_StreamAliasSlotLookup(0,12),"retained first values stay authoritative after child reads");
    require(DB_StreamResolveAliasValue(0,16,&value)==DB_StreamAliasResult::Active&&value==reinterpret_cast<uintptr_t>(owner_fixture::persistent->cellIndex),"untouched first indices stay authoritative after child reads");
#endif
    for(auto offset:{1u,9u,13u,17u}){
        uintptr_t converted=offset;DB_ConvertOffsetToAlias(&converted);
        const auto expected=offset==1?reinterpret_cast<uintptr_t>(header->name):offset==9?reinterpret_cast<uintptr_t>(header->buffer):offset==13?reinterpret_cast<uintptr_t>(owner_fixture::persistent->values):reinterpret_cast<uintptr_t>(owner_fixture::persistent->cellIndex);
        require(converted==expected,"actual alias converters preserve current and retained values after children");
    }
    executeLookups(first,owner_fixture::persistent,false);require(!lastLookup.rowMisses&&!lastLookup.rowDisagreements&&!lastLookup.columnDisagreements,"all pooled first asset queries after complete next payload");
    const std::uint32_t nameHash=adler32(1,reinterpret_cast<const Bytef*>(header->name),static_cast<unsigned>(p.name.bytes));
    const std::uint32_t bufferHash=adler32(1,reinterpret_cast<const Bytef*>(header->buffer),static_cast<unsigned>(p.bufferBytes));clear();
    if(measurement)std::printf("{\"rawfile_payload_measurements\":{\"asset_type\":36,\"directory_index\":1,\"status\":\"payload_loaded\",\"source_start\":%zu,\"source_end\":%zu,\"name_source_bytes\":%llu,\"buffer_bytes\":%llu,\"name_adler32\":%u,\"buffer_adler32\":%u,\"name_disk_start\":%llu,\"name_native_start\":%llu,\"buffer_disk_start\":%llu,\"buffer_native_start\":%llu,\"block4_disk_end\":%llu,\"block4_native_end\":%llu,\"first_asset_bytes_preserved\":true,\"pooled_queries_after_payload_verified\":true,\"current_temp_values_verified\":true,\"loader_globals_cleared\":true,\"actual_wrapper_followup\":true,\"full_registry_executed\":false,\"next_asset_skipped\":false}}\n",source,source+static_cast<std::size_t>(p.sourceBytes),(unsigned long long)p.name.bytes,(unsigned long long)p.bufferBytes,nameHash,bufferHash,(unsigned long long)p.name.diskStart,(unsigned long long)p.name.nativeStart,(unsigned long long)p.bufferDiskStart,(unsigned long long)p.bufferNativeStart,(unsigned long long)p.diskEnd,(unsigned long long)p.nativeEnd);
}
static void deferred(int type,std::size_t source,const StringTablePayloadPlan&p,bool measurement){
    if(measurement)std::printf("{\"rawfile_payload_measurements\":{\"asset_type\":%d,\"directory_index\":1,\"status\":\"deferred_next_type\",\"source_start\":%zu,\"source_end\":%zu,\"name_source_bytes\":0,\"buffer_bytes\":0,\"name_adler32\":0,\"buffer_adler32\":0,\"name_disk_start\":0,\"name_native_start\":0,\"buffer_disk_start\":0,\"buffer_native_start\":0,\"block4_disk_end\":%llu,\"block4_native_end\":%llu,\"first_asset_bytes_preserved\":true,\"pooled_queries_after_payload_verified\":false,\"current_temp_values_verified\":false,\"loader_globals_cleared\":true,\"actual_wrapper_followup\":false,\"full_registry_executed\":false,\"next_asset_skipped\":false}}\n",type,source,source,(unsigned long long)p.indices.diskEnd,(unsigned long long)p.indices.nativeEnd);
}
}
