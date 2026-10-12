#pragma once
#include <platform/fastfile_initialized_strings.h>
static unsigned conversionCalls;
static void planCellStrings(StringTablePayloadPlan&p,const unsigned char*data,std::size_t bytes){
    auto disk=p.cells.diskEnd,native=p.cells.nativeEnd;
    for(int i=0;p.cells.present&&i<p.cells.count;++i){CellStringEntry entry{};entry.index=static_cast<unsigned>(i);entry.tag=u32(data+p.cellSource+8*std::size_t(i));entry.source=p.end;
        if(!entry.tag||entry.tag==0xffffffffu){require(PlanRawString(entry.tag,data+p.end,bytes-p.end,p.diskCapacity,p.nativeCapacity,disk,native,entry.plan)==RawStringResult::Ok,"checked genuine cell string source/capacity before allocation");
            p.end+=static_cast<std::size_t>(entry.plan.bytes);disk=entry.plan.diskEnd;native=entry.plan.nativeEnd;entry.targetDisk=entry.plan.diskStart;entry.targetNative=entry.plan.nativeStart;entry.bytes=entry.plan.bytes;
        }else{require(entry.tag!=0xfffffffeu&&(entry.tag-1)>>29==4,"only genuine block4 ordinary cell offsets supported");
            const auto within=(entry.tag-1)&0x1fffffffu;const auto found=std::find_if(p.strings.begin(),p.strings.end(),[&](const CellStringEntry&v){return v.tag==0xffffffffu&&v.plan.diskStart==within;});
            require(found!=p.strings.end(),"ordinary offset targets earlier validated inline string start, not descriptors/padding/future");entry.offset=true;entry.targetDisk=found->plan.diskStart;entry.targetNative=found->plan.nativeStart;entry.bytes=found->plan.bytes;
        }p.strings.push_back(entry);
    }p.stringDiskEnd=disk;p.stringNativeEnd=native;
}
static InitializedStringResult convertWithProof(void*slot,unsigned tag,const InitializedStringOwner&owner,std::span<const InitializedStringRange>ranges,InitializedStringOffsetPlan&result,bool injectWrongMapping=false){
    uintptr_t value;std::memcpy(&value,slot,sizeof(value));require(value==widenTag(tag),"slot still contains original widened tag before conversion");auto status=PlanInitializedStringOffset(tag,owner,ranges,result);if(status!=InitializedStringResult::Ok)return status;
    const unsigned char*mapped;
#if BO1_NATIVE_64BIT
    mapped=DB_StreamExactLookup(4,result.diskStart);
#else
    mapped=owner.base+result.diskStart;
#endif
    if(injectWrongMapping)mapped=nullptr; // Controlled fixture rejection only.
    if(mapped!=result.address)return InitializedStringResult::MappingMismatch;
    ++conversionCalls;DB_ConvertOffsetToPointer(slot);std::memcpy(&value,slot,sizeof(value));require(value==reinterpret_cast<uintptr_t>(result.address),"actual converted pointer equals initialized native address");return InitializedStringResult::Ok;
}
static void executeCellStrings(const StringTablePayloadPlan&p,StringTableCell*cells,DecodedSource&r,XBlock(&blocks)[7],bool measurement){
    require(g_streamPosIndex==4,"cell strings use live block4 scope");
    static std::uint64_t generation=100;const InitializedStringOwner owner{blocks[4].data,p.diskCapacity,p.nativeCapacity,++generation};
    std::vector<InitializedStringRange>storage(p.strings.size());std::size_t initialized=0;const auto oldCalls=conversionCalls;unsigned inlineCount=0,offsetCount=0;
    if(measurement)std::printf("{\"resolved_string_measurements\":[");
    for(const auto&entry:p.strings){uintptr_t value;std::memcpy(&value,&cells[entry.index].string,sizeof(value));require(value==widenTag(entry.tag)&&r.Position()+52==entry.source,"each unresolved slot/source position agrees with original plan");
        const unsigned char*address=nullptr;
        if(entry.tag==0xffffffffu){const char*loaded=reinterpret_cast<const char*>(DB_AllocStreamPos(0));require(loaded==reinterpret_cast<const char*>(blocks[4].data+entry.plan.nativeStart),"actual inline cell string native alignment");reader=&r;Load_XStringCustom(&loaded);reader=nullptr;
            require(!std::memcmp(loaded,r.Current()-entry.plan.bytes,static_cast<std::size_t>(entry.plan.bytes))&&std::memchr(loaded,0,static_cast<std::size_t>(entry.plan.bytes))==loaded+entry.plan.bytes-1,"successful actual raw loading proves every byte/first NUL before registration");address=reinterpret_cast<const unsigned char*>(loaded);value=reinterpret_cast<uintptr_t>(loaded);
#if BO1_NATIVE_64BIT
            for(unsigned i=0;i<entry.plan.bytes;++i)require(DB_StreamExactLookup(4,static_cast<unsigned>(entry.plan.diskStart)+i)==address+i,"all initialized character offsets map exactly");
#endif
            storage[initialized++]={owner.base,address,owner.generation,entry.plan.diskStart,entry.plan.nativeStart,entry.plan.bytes};++inlineCount;
        }else if(entry.offset){InitializedStringOffsetPlan resolved;require(convertWithProof(&cells[entry.index].string,entry.tag,owner,{storage.data(),initialized},resolved)==InitializedStringResult::Ok,"guarded actual ordinary offset conversion in owned field");std::memcpy(&value,&cells[entry.index].string,sizeof(value));
            require(resolved.diskStart==entry.targetDisk&&resolved.nativeStart==entry.targetNative&&resolved.bytes==entry.bytes,"resolved target equals earlier original-plan range");address=resolved.address;++offsetCount;
        }else require(!value,"null cell remains absent without source consumption");
        std::memcpy(&cells[entry.index].string,&value,sizeof(value));
        if(measurement)std::printf("%s{\"index\":%u,\"tag\":%u,\"kind\":\"%s\",\"length\":%llu,\"adler32\":%lu,\"disk_target\":%llu,\"native_target\":%llu}",entry.index?",":"",entry.index,entry.tag,entry.offset?"offset":entry.tag?"inline":"null",(unsigned long long)entry.bytes,address?adler32(1,address,static_cast<unsigned>(entry.bytes)):1ul,(unsigned long long)(entry.tag?entry.targetDisk:0),(unsigned long long)(entry.tag?entry.targetNative:0));
    }
#if defined(BO1_STRINGTABLE_INDEX_GATE)
    const auto stringSourceEnd=p.indexSource;
#else
    const auto stringSourceEnd=p.end;
#endif
    if(measurement)std::printf("]}\n");require(conversionCalls-oldCalls==offsetCount&&r.Position()+52==stringSourceEnd&&DB_GetStreamPos()==blocks[4].data+p.stringNativeEnd,"actual inline/offset counts and source/native endpoint before indices");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(4)==p.stringDiskEnd,"actual string shadow endpoint");
#endif
    // Invalidate provenance while owner is still live, before any reset/release.
    initialized=0;std::fill(storage.begin(),storage.end(),InitializedStringRange{});
    InitializedStringOffsetPlan reset;require(PlanInitializedStringOffset(0x80000001u,owner,{storage.data(),initialized},reset)==InitializedStringResult::Uninitialized,"provenance cleared before outer stream reset/owner release");
}
static unsigned initializedFixtures(const ZonePlan&zone){
    unsigned count=0;const auto baseline=frontend_fixture::counts();unsigned char data[32]{};const unsigned char word[]={'x','y',0};std::memcpy(data+16,word,3);
    InitializedStringOwner owner{data,32,32,1};InitializedStringRange range{data,data+16,1,8,16,3};
    auto test=[&](unsigned tag,const InitializedStringOwner&own,std::span<const InitializedStringRange>records,InitializedStringResult expected){const auto before=frontend_fixture::streamState();uintptr_t slot=widenTag(tag);InitializedStringOffsetPlan p;
        require(PlanInitializedStringOffset(tag,own,records,p)==expected,"controlled initialized-range guard status");require(slot==widenTag(tag)&&before==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"rejected/accepted pure guard leaves slot/streams/ownership unchanged");if(expected!=InitializedStringResult::Ok)require(!p.address&&!p.bytes&&!p.nativeStart&&!p.diskStart,"unsuccessful offset plan cleared");
        std::printf("{\"fixture\":\"initialized_preflight_%u\",\"status\":\"%s\",\"no_mutation\":true}\n",++count,InitializedStringResultName(expected));};
    test(0,{}, {},InitializedStringResult::Absent);test(0xffffffffu,{}, {},InitializedStringResult::Inline);test(0xfffffffeu,owner,{&range,1},InitializedStringResult::UnsupportedTag);
    for(unsigned tag:{1u,0xe0000001u,0xfffffffdu})test(tag,owner,{&range,1},InitializedStringResult::WrongBlock);
    test(0x80000009u,owner,{&range,1},InitializedStringResult::Ok);
    for(unsigned within:{0u,9u,11u,12u,31u})test(0x80000001u+within,owner,{&range,1},InitializedStringResult::Uninitialized);
    auto badOwner=owner;badOwner.base=nullptr;test(0x80000009u,badOwner,{&range,1},InitializedStringResult::InvalidOwner);badOwner=owner;badOwner.generation=0;test(0x80000009u,badOwner,{&range,1},InitializedStringResult::InvalidOwner);
    badOwner=owner;badOwner.generation=2;test(0x80000009u,badOwner,{&range,1},InitializedStringResult::StaleOwner);
    auto badRange=range;badRange.owner=data+1;test(0x80000009u,owner,{&badRange,1},InitializedStringResult::StaleOwner);
    badRange=range;badRange.bytes=0;test(0x80000009u,owner,{&badRange,1},InitializedStringResult::InvalidString);
    badRange=range;badRange.bytes=2;test(0x80000009u,owner,{&badRange,1},InitializedStringResult::InvalidString);
    badRange=range;badRange.bytes=4;test(0x80000009u,owner,{&badRange,1},InitializedStringResult::InvalidString);
    badOwner=owner;badOwner.diskCapacity=10;test(0x80000009u,badOwner,{&range,1},InitializedStringResult::DiskBounds);
    badOwner=owner;badOwner.nativeCapacity=18;test(0x80000009u,badOwner,{&range,1},InitializedStringResult::NativeBounds);
    badRange=range;badRange.nativeStart=UINT64_MAX;test(0x80000009u,owner,{&badRange,1},InitializedStringResult::NativeBounds);
    badRange=range;badRange.bytes=UINT64_MAX;test(0x80000009u,owner,{&badRange,1},InitializedStringResult::DiskBounds);
    badRange=range;badRange.address=data+17;test(0x80000009u,owner,{&badRange,1},InitializedStringResult::InvalidAddress);
    badOwner=owner;badOwner.base=reinterpret_cast<unsigned char*>(UINTPTR_MAX-4);test(0x80000009u,badOwner,{&range,1},InitializedStringResult::NativeBounds);
    InitializedStringRange duplicates[]={range,range};test(0x80000009u,owner,duplicates,InitializedStringResult::AmbiguousStart);
    frontend_fixture::Owner zoneOwner;XBlock blocks[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(zoneOwner),blocks,failed)==ZoneResult::Ok,"actual conversion fixture owned zone");DB_InitStreams(blocks);DB_PushStreamPos(4);DB_IncStreamPos(1);
    std::array<InitializedStringRange,2>proofs{};InitializedStringOwner live{blocks[4].data,zone.file.blockSize[4],zone.logical[4],77};unsigned disk=1,native=1;
    for(unsigned i=0;i<2;++i){const unsigned char empty[]={0};const auto*source=i?empty:word;const auto bytes=i?1u:3u;RawStringPlan p;require(PlanRawString(0xffffffffu,source,bytes,live.diskCapacity,live.nativeCapacity,disk,native,p)==RawStringResult::Ok,"controlled real inline string plan");const char*loaded=reinterpret_cast<const char*>(DB_AllocStreamPos(0));DecodedSource r(source,bytes);reader=&r;Load_XStringCustom(&loaded);reader=nullptr;
        require(!std::memcmp(loaded,source,bytes),"actual fixture raw loading before proof");proofs[i]={live.base,reinterpret_cast<const unsigned char*>(loaded),live.generation,p.diskStart,p.nativeStart,p.bytes};disk=static_cast<unsigned>(p.diskEnd);native=static_cast<unsigned>(p.nativeEnd);}
    for(const auto&proof:proofs){const auto tag=0x80000001u+static_cast<unsigned>(proof.diskStart);uintptr_t slot=widenTag(tag);InitializedStringOffsetPlan result;const auto before=frontend_fixture::streamState();require(convertWithProof(&slot,tag,live,proofs,result)==InitializedStringResult::Ok&&slot==reinterpret_cast<uintptr_t>(proof.address)&&before==frontend_fixture::streamState(),"actual guarded conversion and empty string preserve stream/source state");++count;}
    const auto tag=0x80000001u+static_cast<unsigned>(proofs[0].diskStart);uintptr_t slot=widenTag(tag);const auto before=frontend_fixture::streamState();const auto calls=conversionCalls;InitializedStringOffsetPlan result;require(convertWithProof(&slot,tag,live,proofs,result,true)==InitializedStringResult::MappingMismatch&&slot==widenTag(tag)&&calls==conversionCalls&&before==frontend_fixture::streamState(),"mapping mismatch rejects before real converter or slot mutation");++count;
    auto reused=live;++reused.generation;require(PlanInitializedStringOffset(tag,reused,proofs,result)==InitializedStringResult::StaleOwner&&slot==widenTag(tag),"stale generation rejects same physical address");++count;
    std::fill(proofs.begin(),proofs.end(),InitializedStringRange{});require(PlanInitializedStringOffset(tag,live,{},result)==InitializedStringResult::Uninitialized,"actual fixture provenance invalidated before reset/release");++count;
    std::printf("{\"fixture\":\"initialized_actual\",\"status\":\"ok\",\"no_mutation\":false,\"cases\":5}\n");DB_InitStreams(detached);DB_FreeXBlocks(blocks,0);frontend_fixture::release(&zoneOwner);require(frontend_fixture::counts()==baseline,"actual conversion fixture reset/release baseline");return count;
}
