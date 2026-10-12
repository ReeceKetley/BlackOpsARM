// Actual Load_XStringCustom, with bounded borrowed decoded bytes. This finite
// adapter does not certify the engine's asynchronous/refill file loader.
#include "frontend_zone_fixture.h"
#include <platform/fastfile_assetlist.h>
#include <platform/fastfile_script_table.h>
#include <platform/fastfile_raw_string.h>
#include <platform/fastfile_inflate.h>
#if defined(BO1_ASSET_DIRECTORY_GATE)
#include <platform/fastfile_asset_directory.h>
#include <database/db_assetlist_types.h>
#endif
#include <database/db_expand.hpp>
#include <database/db_stream_load.h>
#include <zlib/zlib.h>
#include <algorithm>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <limits>
#if defined(BO1_FRONTEND_INTERN_GATE)
#include "string_pool_fixture.h"
#include <platform/script_pool_services.h>
#include <platform/android/script_string_services.h>
#include <set>
#endif
using namespace bo1::fastfile;
using frontend_fixture::require;
#if defined(__ANDROID__)
bool Bo1_TestZoneRegistrationFailure(){return false;}
#endif
static XBlock detached[7]{};
static DecodedSource*reader;
static bool failRead;
static unsigned readCalls;
static std::size_t tableReadBytes;
// Report the actual post-Pop coordinate rather than a fixture constant.
static unsigned tempDiskCursor(){
#if BO1_NATIVE_64BIT
    return DB_StreamWin32Pos(0);
#else
    const auto*pos=g_streamPosIndex==0?DB_GetStreamPos():g_streamPosArray[0];
    return static_cast<unsigned>(pos-g_streamBlocks[0].data);
#endif
}
static std::size_t destinationCapacity(const unsigned char*dst){
    require(g_streamBlocks&&dst==DB_GetStreamPos(),"reader destination is current owned stream cursor");
    const auto&b=g_streamBlocks[g_streamPosIndex];const auto address=reinterpret_cast<uintptr_t>(dst),base=reinterpret_cast<uintptr_t>(b.data);
    require(b.data&&address>=base&&address-base<=b.size,"reader destination inside logical owned block");return b.size-(address-base);
}
void __cdecl DB_LoadXFileData(unsigned __int8*dst,int n){
    // Native helper reads a temporary vector sized to the checked table extent.
    // This audited scratch extent is separate from the owned raw-string block.
    const auto capacity=tableReadBytes?tableReadBytes:destinationCapacity(dst);
    if(tableReadBytes&&n!=static_cast<int>(tableReadBytes))throw std::runtime_error("unexpected table scratch read size");
    if(!reader||failRead||!reader->Read(dst,capacity,n))throw std::runtime_error("bounded decoded read failed");++readCalls;
#if defined(BO1_RAWFILE_PAYLOAD_GATE)
    if(tableReadBytes==12&&g_streamPosIndex==0)tableReadBytes=0; // Exactly one canonical RawFile header scratch read.
#endif
}
int DB_LoadXFileDataNullTerminated(unsigned __int8*dst){
    int n=0;if(!reader||failRead||reader->ReadCString(dst,destinationCapacity(dst),n)!=RawStringResult::Ok)throw std::runtime_error("bounded decoded string read failed");++readCalls;return n;
}
struct StringEntry{unsigned index;std::size_t source;RawStringPlan plan;};
struct Front{AssetListPlan envelope;ScriptTablePlan table;std::vector<StringEntry>strings;std::size_t end{52};std::uint64_t diskEnd{},nativeEnd{},padding{};unsigned nulls{},empty{},max{};};
static std::uint32_t u32(const unsigned char*p){std::uint32_t n;std::memcpy(&n,p,4);return n;}
static Front preflight(const unsigned char*data,std::size_t bytes,std::uint64_t decoded){
    Front f;require(bytes<=decoded,"prefix cannot exceed validated full decoded extent");require(PlanAssetList(data,bytes,decoded,frontend_fixture::limits(),frontend_fixture::page(),f.envelope)==AssetListResult::Ok,"validated genuine envelope metadata");
    require(bytes>=52,"complete envelope prefix");require(PlanScriptTable(f.envelope.disk.stringCount,f.envelope.disk.stringsTag,bytes-52,f.envelope.zone.file.blockSize[4],f.envelope.zone.logical[4],0,0,f.table)==ScriptTableResult::Ok,"bounded table before string scan");
    f.end=52+std::size_t(f.table.diskBytes);f.diskEnd=f.table.diskEnd;f.nativeEnd=f.table.nativeEnd;
    for(int i=0;f.table.present&&i<f.table.count;++i){RawStringPlan p;
        require(PlanRawString(u32(data+52+4*std::size_t(i)),data+f.end,bytes-f.end,f.envelope.zone.file.blockSize[4],f.envelope.zone.logical[4],f.diskEnd,f.nativeEnd,p)==RawStringResult::Ok,"bounded genuine inline string plan");
        f.strings.push_back({unsigned(i),f.end,p});f.end+=std::size_t(p.bytes);f.diskEnd=p.diskEnd;f.nativeEnd=p.nativeEnd;f.padding+=p.nativePadding;
        if(!p.present)++f.nulls;else{if(p.bytes==1)++f.empty;f.max=(std::max)(f.max,unsigned(p.bytes));}
    }return f;
}
static std::vector<unsigned char>readPrefix(const char*path){std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"prefix exists");auto n=file.tellg();require(n>=52&&n<=std::streamoff(INT32_MAX),"finite prefix extent");std::vector<unsigned char>b(static_cast<std::size_t>(n));file.seekg(0);file.read(reinterpret_cast<char*>(b.data()),n);require(bool(file),"exact prefix read");return b;}

#if defined(BO1_ASSET_DIRECTORY_GATE)
static AssetDirectoryPlan directoryPlan(const Front&f,const unsigned char*data,std::size_t bytes){AssetDirectoryPlan p;
    require(f.end<=bytes,"raw prefix fits before directory");require(PlanAssetDirectory(f.envelope.disk.assetCount,f.envelope.disk.assetsTag,data+f.end,bytes-f.end,f.envelope.zone.file.blockSize[4],f.envelope.zone.logical[4],f.diskEnd,f.nativeEnd,p)==AssetDirectoryResult::Ok,"checked genuine directory records before allocation");return p;}
static uintptr_t widenTag(std::uint32_t tag){
#if BO1_NATIVE_64BIT
    return tag==0xffffffffu?~uintptr_t(0):tag==0xfffffffeu?~uintptr_t(1):uintptr_t(tag);
#else
    return tag;
#endif
}
static void executeDirectory(const Front&f,const AssetDirectoryPlan&p,DecodedSource&r,XBlock(&blocks)[7],bool measurements){
    if(!p.present)return;
    const auto sourceStart=r.Position();require(sourceStart==f.end-52,"directory follows raw source without file alignment");
    const auto prefixBytes=static_cast<unsigned>(f.nativeEnd);const auto prefixAdler=adler32(1,blocks[4].data,prefixBytes);
    DB_PushStreamPos(4);auto*assets=reinterpret_cast<XAsset*>(DB_AllocStreamPos(3));require(reinterpret_cast<unsigned char*>(assets)==blocks[4].data+p.nativeStart,"directory native memory alignment");reader=&r;tableReadBytes=static_cast<std::size_t>(p.diskBytes);
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_XAssetArray(assets,p.count);
#else
    if(p.diskBytes)Load_Stream(true,reinterpret_cast<unsigned char*>(assets),static_cast<int>(p.diskBytes));
#endif
    tableReadBytes=0;reader=nullptr;
    if(measurements)std::printf("{\"directory_measurements\":[");
    for(int i=0;i<p.count;++i){auto*disk=r.Current()-p.diskBytes+8*std::size_t(i);const auto type=u32(disk),tag=u32(disk+4);uintptr_t actual;std::memcpy(&actual,&assets[i].header,sizeof(actual));
        require(static_cast<unsigned>(assets[i].type)==type&&actual==widenTag(tag),"every directory type and widened header tag matches source");
#if BO1_NATIVE_64BIT
        auto*element=reinterpret_cast<unsigned char*>(&assets[i]);for(unsigned j=4;j<sizeof(void*);++j)require(element[j]==0,"native internal XAsset padding zero");
        const auto w=static_cast<unsigned>(p.diskStart+8*std::uint64_t(i));require(DB_StreamExactLookup(4,w)==element,"directory element starts map exactly");
        for(unsigned j=1;j<8;++j)require(!DB_StreamExactLookup(4,w+j),"record interior not exact expanded object mapping");
        require(DB_StreamAliasSlotLookup(4,w+4)==reinterpret_cast<uintptr_t*>(&assets[i].header),"disk headerfield alias maps native header slot");
#endif
        if(measurements){if(i)std::printf(",");std::printf("{\"index\":%d,\"type\":%u,\"tag\":%u}",i,type,tag);}
    }
    if(measurements)std::printf("]}\n");
    require(r.Position()==sourceStart+p.diskBytes&&DB_GetStreamPos()==blocks[4].data+p.nativeEnd,"directory source and native footprint");
    require(adler32(1,blocks[4].data,prefixBytes)==prefixAdler,"preceding raw table/characters/alignment bytes unchanged");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(4)==p.diskEnd&&!DB_StreamExactLookup(4,static_cast<unsigned>(p.diskEnd)),"directory shadow endpoint not committed interior");
    for(const auto&e:f.strings){require(DB_StreamAliasSlotLookup(4,unsigned(4*e.index))==reinterpret_cast<uintptr_t*>(blocks[4].data+sizeof(void*)*e.index),"prior raw table aliases preserved");if(e.plan.present)require(DB_StreamExactLookup(4,static_cast<unsigned>(e.plan.diskStart))==blocks[4].data+e.plan.nativeStart,"prior raw character mapping preserved");}
#endif
    DB_PopStreamPos();require(!g_streamPosStackIndex&&!g_streamPosIndex&&g_streamPos==blocks[0].data&&g_streamPosArray[0]==nullptr,"directory balanced pop convention");
    for(unsigned i=1;i<7;++i)if(i!=4)require(g_streamPosArray[i]==blocks[i].data,"other block cursors unchanged");
}
static unsigned directoryFixtures(const ZonePlan&zone){unsigned count=0;const auto baseline=frontend_fixture::counts();std::array<std::uint32_t,12>records={0,0,42,1,36,0x80000000u,37,0xfffffffdu,1,0xfffffffeu,2,0xffffffffu};
    auto test=[&](int n,std::uint32_t tag,const unsigned char*data,std::size_t source,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t native,AssetDirectoryResult expected){const auto state=frontend_fixture::streamState();AssetDirectoryPlan p;
        require(PlanAssetDirectory(n,tag,data,source,dc,nc,w,native,p)==expected,"controlled directory preflight result");require(state==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"directory preflight no mutations");if(expected!=AssetDirectoryResult::Ok)require(!p.present&&!p.diskBytes,"directory failed plan cleared");
        std::printf("{\"fixture\":\"directory_preflight_%u\",\"status\":\"%s\",\"no_mutation\":true}\n",++count,AssetDirectoryResultName(expected));};
    const auto*data=reinterpret_cast<const unsigned char*>(records.data());
    test(-1,0,nullptr,0,0,0,0,0,AssetDirectoryResult::NegativeCount);test(INT32_MAX,0,nullptr,0,0,0,0,0,AssetDirectoryResult::Ok);
    test(0,1,nullptr,0,4,8,1,1,AssetDirectoryResult::Ok);test(0,1,nullptr,0,0,0,0,0,AssetDirectoryResult::NativeBounds);
    test(1,1,data,7,16,32,0,0,AssetDirectoryResult::SourceBounds);test(1,1,nullptr,8,16,32,0,0,AssetDirectoryResult::InvalidSource);
    test(1,1,data,8,7,32,0,0,AssetDirectoryResult::DiskBounds);test(1,1,data,8,8,sizeof(XAsset)-1,0,0,AssetDirectoryResult::NativeBounds);
    test(1,1,data,8,8,sizeof(XAsset),0,0,AssetDirectoryResult::Ok);test(1,1,data,8,11,32,1,1,AssetDirectoryResult::DiskBounds);
    test(0,1,data,0,UINT64_MAX,UINT64_MAX,UINT64_MAX,0,AssetDirectoryResult::DiskBounds);test(0,1,data,0,UINT64_MAX,UINT64_MAX,0,UINT64_MAX,AssetDirectoryResult::NativeBounds);
    test(INT32_MAX,1,nullptr,SIZE_MAX,UINT64_MAX,UINT64_MAX,0,0,BO1_NATIVE_64BIT?AssetDirectoryResult::HelperDomain:AssetDirectoryResult::CommitOverflow);
    for(auto type:{0xffffffffu,43u,0x7fffffffu}){auto invalid=records;invalid[10]=type;test(6,1,reinterpret_cast<const unsigned char*>(invalid.data()),48,48,6*sizeof(XAsset),0,0,AssetDirectoryResult::InvalidType);}
    frontend_fixture::Owner owner;XBlock blocks[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),blocks,failed)==ZoneResult::Ok,"directory fixtures real owner");
    for(unsigned off:{0u,1u})for(int n:{0,1,6}){DB_InitStreams(blocks);if(off){DB_PushStreamPos(4);DB_IncStreamPos(1);DB_PopStreamPos();}Front f;f.end=52;f.nativeEnd=off;f.diskEnd=off;AssetDirectoryPlan p;
        require(PlanAssetDirectory(n,1,data,48,zone.file.blockSize[4],zone.logical[4],off,off,p)==AssetDirectoryResult::Ok,"controlled actual directory plan");DecodedSource r(data,48);executeDirectory(f,p,r,blocks,false);++count;}
    DB_InitStreams(blocks);DB_PushStreamPos(4);auto*dst=reinterpret_cast<XAsset*>(DB_AllocStreamPos(3));const auto before=frontend_fixture::streamState();const auto owners=frontend_fixture::counts();std::array<unsigned char,96>snapshot{};std::memcpy(snapshot.data(),dst,snapshot.size());DecodedSource shortReader(data,47);reader=&shortReader;tableReadBytes=48;bool threw=false;
    try{
#if BO1_NATIVE_64BIT
        DB_ExpandLoad_XAssetArray(dst,6);
#else
        Load_Stream(true,reinterpret_cast<unsigned char*>(dst),48);
#endif
    }catch(const std::runtime_error&){threw=true;}reader=nullptr;tableReadBytes=0;require(threw&&!shortReader.Position()&&before==frontend_fixture::streamState()&&owners==frontend_fixture::counts()&&!std::memcmp(snapshot.data(),dst,snapshot.size()),"directory injected short read no partial output/commit/alias change before reset");++count;
    std::printf("{\"fixture\":\"directory_actual\",\"status\":\"ok\",\"no_mutation\":false,\"cases\":7}\n");
    DB_InitStreams(detached);DB_FreeXBlocks(blocks,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"directory failure cleanup baseline");return count;
}
#endif
#if defined(BO1_STRINGTABLE_HEADER_GATE)
#include "stringtable_header_fixture.h"
#endif
static void decode(const char*path,const char*out){Bo1InflatedFastFile f;require(Bo1_InflateFastFilePath(path,{},&f)==Bo1InflateResult::Ok,"complete genuine production decode");
    auto plan=preflight(f.decoded.data(),f.decoded.size(),f.decoded.size());
#if defined(BO1_ASSET_DIRECTORY_GATE)
    const auto directory=directoryPlan(plan,f.decoded.data(),f.decoded.size());
#if defined(BO1_STRINGTABLE_HEADER_GATE)
    const auto header=headerPlan(plan,directory,f.decoded.data(),f.decoded.size(),f.decoded.size());require(header.present,"genuine header exists");
#if defined(BO1_STRINGTABLE_CELLS_GATE)
    const auto payload=payloadPlan(plan,directory,header,f.decoded.data(),f.decoded.size());
#if defined(BO1_RAWFILE_HEADER_GATE)
    // The front-end end still names the directory source here. Compute next
    // record provenance before replacing it with the execution prefix endpoint.
    plan.end=rawfile_fixture::plan(plan,directory,payload,f.decoded.data(),f.decoded.size(),f.decoded.size()).end;
#else
    plan.end=payload.end;
#endif
#else
    plan.end+=20;
#endif
#endif
#if !defined(BO1_STRINGTABLE_CELLS_GATE)
    plan.end+=static_cast<std::size_t>(directory.diskBytes);
#endif
#endif
    const auto bytes=f.decoded.size(),capacity=f.decoded.capacity();const auto adler=adler32(1,f.decoded.data(),static_cast<unsigned>(bytes));
    std::ofstream file(out,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<char*>(f.decoded.data()),static_cast<std::streamsize>(plan.end));file.close();require(bool(file),"small validated string prefix output");
    std::vector<std::uint8_t>().swap(f.decoded);std::printf("{\"decoded_bytes\":%zu,\"decoded_adler32\":%lu,\"vector_capacity_bytes\":%zu,\"prefix_bytes\":%zu,\"decoded_buffer_released\":true}\n",bytes,adler,capacity,plan.end);
}
static void execute(const Front&f,DecodedSource&r,XBlock(&b)[7],bool measurements){
    if(!f.table.present)return;
    DB_PushStreamPos(4);void*table=DB_AllocStreamPos(3);reader=&r;readCalls=0;tableReadBytes=static_cast<std::size_t>(f.table.diskBytes);
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_Ptr32Array(table,f.table.count);
#else
    if(f.table.diskBytes)Load_Stream(true,static_cast<unsigned char*>(table),static_cast<int>(f.table.diskBytes));
#endif
    tableReadBytes=0;require(r.Position()==f.table.diskBytes,"table consumed packed disk bytes only");
    if(measurements)std::printf("{\"string_measurements\":[");
    bool comma=false;
    for(const auto&entry:f.strings){auto*slot=static_cast<unsigned char*>(table)+std::size_t(entry.index)*sizeof(void*);const auto&p=entry.plan;uintptr_t value;std::memcpy(&value,slot,sizeof(value));
        const auto tag=value==~uintptr_t(0)?0xffffffffu:static_cast<std::uint32_t>(value);
        require(tag==(p.present?0xffffffffu:0u),"table preserves raw tag until successful load");
        require(r.Position()+52==entry.source,"each string follows genuine FILE sequence");
        if(p.present){const char*loaded=reinterpret_cast<const char*>(DB_AllocStreamPos(0));require(loaded==reinterpret_cast<const char*>(b[4].data+p.nativeStart),"native raw alignment independent of FILE");
#if defined(BO1_FRONTEND_INTERN_GATE)
            Load_TempStringCustom(&loaded);value=reinterpret_cast<uintptr_t>(loaded);require(value&&value<65536,"actual TempStringCustom returns fixed pool ID");
            const auto*interned=SL_ConvertToString(static_cast<unsigned>(value),SCRIPTINSTANCE_SERVER);
            require(interned&&!std::memcmp(interned,r.Current()-p.bytes,static_cast<std::size_t>(p.bytes))&&SL_GetStringLen(SCRIPTINSTANCE_SERVER,static_cast<unsigned>(value))==static_cast<int>(p.bytes-1),"every actual pool ID roundtrip matches genuine source");
            require(!std::memcmp(b[4].data+p.nativeStart,r.Current()-p.bytes,static_cast<std::size_t>(p.bytes)),"raw owned bytes preserved before ID publication");
            std::memcpy(slot,&value,sizeof(value));
#else
            Load_XStringCustom(&loaded);require(loaded==reinterpret_cast<const char*>(b[4].data+p.nativeStart),"actual raw leaf retains native pointer");
            require(!std::memcmp(loaded,r.Current()-p.bytes,static_cast<std::size_t>(p.bytes)),"each loaded byte including NUL matches validated source");
            value=reinterpret_cast<uintptr_t>(loaded);std::memcpy(slot,&value,sizeof(value));
#endif
#if BO1_NATIVE_64BIT
            for(unsigned d=0;d<p.bytes;++d)require(DB_StreamExactLookup(4,static_cast<unsigned>(p.diskStart+d))==b[4].data+p.nativeStart+d,"all loaded character offsets map exactly");
#endif
        }else require(!value,"null slot remains absent");
#if BO1_NATIVE_64BIT
        require(DB_StreamAliasSlotLookup(4,unsigned(4*entry.index))==reinterpret_cast<uintptr_t*>(slot),"table alias still names native owned pointer slot");
        require(DB_StreamWin32Pos(4)==p.diskEnd,"raw leaf shadow bytes plus zero disk alignment");
#endif
        require(DB_GetStreamPos()==b[4].data+p.nativeEnd,"raw leaf native cursor exact");
        if(measurements){if(comma)std::printf(",");comma=true;
#if defined(BO1_FRONTEND_INTERN_GATE)
            std::printf("{\"index\":%u,\"length\":%llu,\"adler32\":%lu,\"id\":%llu}",entry.index,(unsigned long long)p.bytes,p.present?adler32(1,b[4].data+p.nativeStart,static_cast<unsigned>(p.bytes)):1ul,(unsigned long long)value);
#else
            std::printf("{\"index\":%u,\"length\":%llu,\"adler32\":%lu}",entry.index,(unsigned long long)p.bytes,p.present?adler32(1,b[4].data+p.nativeStart,static_cast<unsigned>(p.bytes)):1ul);
#endif
        }
    }
    if(measurements)std::printf("]}\n");reader=nullptr;DB_PopStreamPos();
    require(readCalls==(f.table.diskBytes?1u:0u)+f.strings.size()-f.nulls,"actual calls count inclusive NUL strings");
    require(r.Position()==f.end-52&&g_streamPosStackIndex==0&&g_streamPosIndex==0,"exact decoded string boundary and balanced stream stack");
    require(g_streamPosArray[4]==b[4].data+f.nativeEnd,"final owned block4 cursor");require(g_streamPos==b[0].data&&g_streamPosArray[0]==nullptr,"current block0 cursor convention");for(unsigned i=1;i<7;++i)if(i!=4)require(g_streamPosArray[i]==b[i].data,"other owned block cursors unchanged");
}
static unsigned fixtures(const ZonePlan&zone){unsigned count=0;const auto baseline=frontend_fixture::counts();const unsigned char nul[]={0},word[]={'a','b',0},bad[]={'a','b'};
    auto test=[&](std::uint32_t tag,const unsigned char*s,std::size_t length,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t n,RawStringResult expected){auto streams=frontend_fixture::streamState();RawStringPlan p;
        require(PlanRawString(tag,s,length,dc,nc,w,n,p)==expected,"controlled raw preflight result");require(frontend_fixture::streamState()==streams&&frontend_fixture::counts()==baseline,"preflight no mutation");if(expected!=RawStringResult::Ok)require(!p.present&&!p.bytes,"failed plan cleared");
        std::printf("{\"fixture\":\"preflight_%u\",\"status\":\"%s\",\"no_mutation\":true}\n",++count,RawStringResultName(expected));};
    test(0,nullptr,0,0,0,0,0,RawStringResult::Ok);test(0xffffffffu,nul,1,1,1,0,0,RawStringResult::Ok);
    test(0xffffffffu,word,3,3,3,0,0,RawStringResult::Ok);test(0xffffffffu,bad,2,8,8,0,0,RawStringResult::Unterminated);
    test(0xffffffffu,nullptr,1,8,8,0,0,RawStringResult::InvalidSource);test(0xffffffffu,nul,0,8,8,0,0,RawStringResult::Unterminated);
    for(auto tag:{1u,0x80000000u,0xfffffffdu,0xfffffffeu})test(tag,word,3,16,16,0,0,RawStringResult::UnsupportedTag);
    test(0xffffffffu,word,3,2,8,0,0,RawStringResult::DiskBounds);test(0xffffffffu,word,3,8,2,0,0,RawStringResult::NativeBounds);
    test(0xffffffffu,nul,1,3,sizeof(void*),1,1,BO1_NATIVE_64BIT?RawStringResult::NativeBounds:RawStringResult::Ok);test(0,nul,1,0,1,1,0,RawStringResult::DiskBounds);
    test(0,nul,1,1,0,0,1,RawStringResult::NativeBounds);test(0xffffffffu,nul,1,UINT64_MAX,UINT64_MAX,0,UINT64_MAX,RawStringResult::NativeBounds);
    test(0xffffffffu,nul,1,UINT64_MAX,UINT64_MAX,UINT32_MAX,0,RawStringResult::CommitOverflow);
    unsigned char dest[4]={7,7,7,7};int loaded=99;DecodedSource shortSource(bad,2);require(shortSource.ReadCString(dest,4,loaded)==RawStringResult::Unterminated&&loaded==0&&!shortSource.Position()&&dest[0]==7,"unterminated reader no writes");++count;
    std::printf("{\"fixture\":\"short_source\",\"status\":\"unterminated\",\"no_mutation\":true}\n");
    DecodedSource exact(word,3);require(exact.ReadCString(dest,2,loaded)==RawStringResult::NativeBounds&&!exact.Position()&&dest[0]==7,"short destination no partial writes");++count;
    std::printf("{\"fixture\":\"short_destination\",\"status\":\"native_bounds\",\"no_mutation\":true}\n");
    require(!exact.Read(dest,4,-1)&&!exact.Read(dest,4,4)&&!exact.Position(),"invalid finite read lengths no advance");++count;
    std::printf("{\"fixture\":\"invalid_lengths\",\"status\":\"invalid_source\",\"no_mutation\":true}\n");
    require(exact.ReadCString(dest,3,loaded)==RawStringResult::Ok&&loaded==3&&exact.Position()==3&&!std::memcmp(dest,word,3),"NUL at final source and destination byte");++count;
    std::printf("{\"fixture\":\"exact_end\",\"status\":\"ok\",\"no_mutation\":false}\n");
    require(exact.ReadCString(dest,3,loaded)==RawStringResult::Unterminated&&exact.Position()==3,"exhausted source bounded");++count;
    std::printf("{\"fixture\":\"exhausted\",\"status\":\"unterminated\",\"no_mutation\":true}\n");
    unsigned char shared[]={'x',0,9,9};DecodedSource overlap(shared,4);require(!overlap.Read(shared+2,2,1)&&overlap.ReadCString(shared+1,3,loaded)==RawStringResult::InvalidSource&&!overlap.Position()&&shared[0]=='x'&&shared[1]==0&&shared[2]==9,"overlapping destination cannot corrupt borrowed source");++count;
    std::printf("{\"fixture\":\"overlap\",\"status\":\"invalid_source\",\"no_mutation\":true}\n");
    frontend_fixture::Owner owner;XBlock b[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),b,failed)==ZoneResult::Ok,"controlled actual owned block");DB_InitStreams(b);DB_PushStreamPos(4);DB_IncStreamPos(1);
    const char*out=reinterpret_cast<const char*>(DB_AllocStreamPos(0));const auto before=frontend_fixture::streamState();const auto allocated=frontend_fixture::counts();std::array<unsigned char,8>bytes{};std::memcpy(bytes.data(),out,bytes.size());DecodedSource r(word,3);reader=&r;failRead=true;bool threw=false;
    try{Load_XStringCustom(&out);}catch(const std::runtime_error&){threw=true;}reader=nullptr;failRead=false;
    require(threw&&!r.Position()&&frontend_fixture::streamState()==before&&frontend_fixture::counts()==allocated&&!std::memcmp(bytes.data(),out,bytes.size()),"injected pre-copy failure leaves post-alignment state and bytes unchanged");++count;
    std::printf("{\"fixture\":\"injected_failure\",\"status\":\"invalid_source\",\"no_mutation\":true}\n");
    DB_InitStreams(detached);DB_FreeXBlocks(b,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"failure detach and release baseline");return count;
}
int main(int argc,char**argv){require(argc>=2,"decode/gate mode");if(!std::strcmp(argv[1],"decode")){require(argc==4,"decode input output");decode(argv[2],argv[3]);return 0;}
    require(argc==6&&!std::strcmp(argv[1],"gate"),"gate prefix1 decoded1 prefix2 decoded2");frontend_fixture::startup();
#if defined(BO1_FRONTEND_INTERN_GATE)
    Sys_InitializeCriticalSections();bo1::script::RegisterServerThread();
#endif
    DB_InitStreams(detached);const auto baseline=frontend_fixture::counts();
    auto a=readPrefix(argv[2]),b=readPrefix(argv[4]);auto ap=preflight(a.data(),a.size(),std::strtoull(argv[3],nullptr,10)),bp=preflight(b.data(),b.size(),std::strtoull(argv[5],nullptr,10));
#if defined(BO1_ASSET_DIRECTORY_GATE)
    auto ad=directoryPlan(ap,a.data(),a.size()),bd=directoryPlan(bp,b.data(),b.size());
#if defined(BO1_STRINGTABLE_HEADER_GATE)
    auto ah=headerPlan(ap,ad,a.data(),a.size(),std::strtoull(argv[3],nullptr,10)),bh=headerPlan(bp,bd,b.data(),b.size(),std::strtoull(argv[5],nullptr,10));
#if defined(BO1_STRINGTABLE_CELLS_GATE)
    auto ac=payloadPlan(ap,ad,ah,a.data(),a.size()),bc=payloadPlan(bp,bd,bh,b.data(),b.size());
#if defined(BO1_RAWFILE_HEADER_GATE)
    auto ar=rawfile_fixture::plan(ap,ad,ac,a.data(),a.size(),std::strtoull(argv[3],nullptr,10));
    auto br=rawfile_fixture::plan(bp,bd,bc,b.data(),b.size(),std::strtoull(argv[5],nullptr,10));
    require(ar.end==a.size()&&br.end==b.size(),"prefix stops at the genuine next numeric header or deferred next type");
#else
    require(ac.end==a.size()&&bc.end==b.size(),"prefix stops at selected cell/strings boundary before index");
#endif
#else
    require(ap.end+ad.diskBytes+20==a.size()&&bp.end+bd.diskBytes+20==b.size(),"prefix stops exactly after first numeric header");
#endif
#else
    require(ap.end+ad.diskBytes==a.size()&&bp.end+bd.diskBytes==b.size(),"prefix stops exactly after directory before dispatch");
#endif
#else
    require(ap.end==a.size()&&bp.end==b.size(),"prefix stops exactly before asset directory");
#endif
    auto tested=fixtures(ap.envelope.zone);
#if defined(BO1_ASSET_DIRECTORY_GATE)
    tested+=directoryFixtures(ap.envelope.zone);
#if defined(BO1_STRINGTABLE_HEADER_GATE)
    tested+=headerFixtures(ap.envelope.zone);
#if defined(BO1_STRINGTABLE_CELLS_GATE)
    tested+=cellsFixtures(ap.envelope.zone);
#if defined(BO1_STRINGTABLE_STRINGS_GATE)
    tested+=initializedFixtures(ap.envelope.zone);
#if defined(BO1_STRINGTABLE_INDEX_GATE)
    tested+=indicesFixtures(ap.envelope.zone);
#if defined(BO1_STRINGTABLE_HASH_GATE)
    hashFixtures();
#if defined(BO1_STRINGTABLE_LOOKUP_GATE)
    comparisonFixtures();lookupFixtures();
#if defined(BO1_STRINGTABLE_NORMALIZATION_GATE)
    normalizationFixtures();
#endif
#if defined(BO1_STRINGTABLE_OWNER_GATE)
    owner_fixture::fixtures();
#if defined(BO1_RAWFILE_OWNER_GATE)
    rawfile_owner_fixture::fixtures();
#endif
#if defined(BO1_RAWFILE_HEADER_GATE)
    tested+=rawfile_fixture::fixtures();
#if defined(BO1_RAWFILE_PAYLOAD_GATE)
    tested+=rawfile_payload_fixture::fixtures();
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#if defined(BO1_ENTRY_GENUINE_GATE)
    entry_owner_fixture::begin();
#endif
    unsigned cycles=0;
    for(unsigned round=0;round<3;++round)for(unsigned type=0;type<2;++type)for(unsigned asset=0;asset<2;++asset){auto&f=asset?bp:ap;auto&bytes=asset?b:a;auto zone=f.envelope.zone;zone.limits.allocType=type;frontend_fixture::Owner owner;owner.type=type;XBlock blocks[7]{};unsigned failed;
        require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),blocks,failed)==ZoneResult::Ok,"genuine actual allocator policy");DB_InitStreams(blocks);
#if defined(BO1_FRONTEND_INTERN_GATE)
        SL_Init(SCRIPTINSTANCE_SERVER);pool_fixture::baseline(SCRIPTINSTANCE_SERVER);
#endif
#if defined(BO1_STRINGTABLE_HEADER_GATE)
        XAssetList envelope{};seedEnvelope(bytes.data(),envelope);
#endif
        DecodedSource r(bytes.data()+52,bytes.size()-52);execute(f,r,blocks,asset&&round==0&&type==0);
#if defined(BO1_ASSET_DIRECTORY_GATE)
        const auto&directory=asset?bd:ad;executeDirectory(f,directory,r,blocks,round==0&&type==0);
#if defined(BO1_STRINGTABLE_HEADER_GATE)
        const auto&header=asset?bh:ah;
#if defined(BO1_STRINGTABLE_CELLS_GATE)
        const auto&payload=asset?bc:ac;DB_PushStreamPos(4);
        executeHeader(header,r,blocks,&envelope,round==0&&type==0,&payload
#if defined(BO1_RAWFILE_HEADER_GATE)
            ,asset?&br:&ar
#endif
        );DB_PopStreamPos();require(g_streamPosIndex==0&&!g_streamPosStackIndex&&DB_GetStreamPos()==blocks[0].data,"actual caller/header/child scopes balanced before detach");
#else
        executeHeader(header,r,blocks,&envelope,round==0&&type==0);
#endif
#endif
#endif
#if defined(BO1_FRONTEND_INTERN_GATE)
        std::set<unsigned>uniqueIds;
        for(const auto&e:f.strings){uintptr_t id;std::memcpy(&id,blocks[4].data+sizeof(void*)*e.index,sizeof(id));if(id)uniqueIds.insert(static_cast<unsigned>(id));}
        const auto poolAllocations=gScrMemTreeGlob[0].totalAlloc,poolBuckets=gScrMemTreeGlob[0].totalAllocBuckets;const auto poolReferences=bo1::atomic32::AtomicLoad32(&gScrStringDebugGlob[0]->totalRefCount);
        require(poolAllocations==static_cast<int>(uniqueIds.size())&&poolReferences==uniqueIds.size(),"genuine duplicates retain one user4 reference per unique ID");
        for(auto id:uniqueIds)require(SL_GetUser(id,SCRIPTINSTANCE_SERVER)==4&&GetRefString(SCRIPTINSTANCE_SERVER,id)->refCount==1,"all genuine IDs have exact user4 ownership");
        SL_ShutdownSystem(SCRIPTINSTANCE_SERVER,4);pool_fixture::baseline(SCRIPTINSTANCE_SERVER);
        std::array<int,32>diagnostics{};std::memcpy(diagnostics.data(),gScrStringGlob[0].stringsUsed,sizeof(diagnostics));SL_Shutdown(SCRIPTINSTANCE_SERVER);require(!gScrStringDebugGlob[0]&&!gScrStringGlob[0].inited,"genuine pool shutdown clears init and debug");
#endif
#if defined(BO1_STRINGTABLE_INDEX_GATE)
        require(!varshort,"scalar loader global invalid before reset/release");
#endif
#if defined(BO1_STRINGTABLE_OWNER_GATE)
#if defined(BO1_ENTRY_GENUINE_GATE)
        entry_owner_fixture::verifyLive(asset);
        rawfile_owner_fixture::finish();
        owner_fixture::finish();
        entry_owner_fixture::finish(asset);
#else
        owner_fixture::finish();
#if defined(BO1_RAWFILE_OWNER_GATE)
        rawfile_owner_fixture::finish();
#endif
#endif
#endif
        DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
#if defined(BO1_STRINGTABLE_HEADER_GATE)
        for(unsigned w:{0u,4u,12u,16u})require(!DB_StreamAliasSlotLookup(0,w)&&!DB_StreamExactLookup(0,w),"all header/envelope mappings detached before owner release");
#if defined(BO1_STRINGTABLE_CELLS_GATE)
        for(int i=0;i<payload.cells.count;++i){const auto w=static_cast<unsigned>(payload.cells.diskStart+8*std::uint64_t(i));require(!DB_StreamAliasSlotLookup(4,w)&&!DB_StreamExactLookup(4,w),"all cell mappings detached before owner release");}
#endif
#endif
        for(int i=0;i<f.table.count;++i)require(!DB_StreamAliasSlotLookup(4,unsigned(4*i)),"all table aliases detached before owner release");
#if defined(BO1_ASSET_DIRECTORY_GATE)
        for(int i=0;directory.present&&i<directory.count;++i){const auto w=static_cast<unsigned>(directory.diskStart+8*std::uint64_t(i));require(!DB_StreamAliasSlotLookup(4,w+4)&&!DB_StreamExactLookup(4,w),"all directory aliases and exact mappings detached before owner release");}
#endif
#endif
        DB_FreeXBlocks(blocks,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"logical baseline after each genuine cycle");++cycles;
#if defined(BO1_FRONTEND_INTERN_GATE)
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"inline_string_count\":%zu,\"raw_string_bytes\":%llu,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"unique_interned_strings\":%zu,\"pool_live_allocations\":%d,\"pool_live_buckets\":%d,\"pool_debug_refs\":%u,\"pool_released\":true,\"hash_free_list_restored\":true,\"all_aliases_match\":true,\"reset_and_release_pass\":true,\"strings_used_after_release\":[",asset,round,type,f.strings.size()-f.nulls,(unsigned long long)(f.end-52-f.table.diskBytes),r.Position(),f.end,(unsigned long long)f.diskEnd,(unsigned long long)f.nativeEnd,uniqueIds.size(),poolAllocations,poolBuckets,poolReferences);
        for(unsigned i=0;i<32;++i)std::printf("%s%d",i?",":"",diagnostics[i]);std::printf("]}\n");
#elif defined(BO1_STRINGTABLE_LOOKUP_GATE)
        auto cycleDisk=payload.indices.diskEnd,cycleNative=payload.indices.nativeEnd;
#if defined(BO1_RAWFILE_PAYLOAD_GATE)
        const auto&next=asset?br:ar;if(next.header.present){cycleDisk=next.payload.diskEnd;cycleNative=next.payload.nativeEnd;}
#endif
        unsigned inlineCells=0,offsetCells=0;std::uint64_t inlineBytes=0;for(const auto&e:payload.strings){if(e.offset)++offsetCells;else if(e.tag){++inlineCells;inlineBytes+=e.bytes;}}
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"name_bytes\":%llu,\"cell_count\":%d,\"inline_cell_strings\":%u,\"offset_cell_strings\":%u,\"inline_cell_bytes\":%llu,\"block0_native_after_pop\":0,\"block0_shadow_after_pop\":%u,\"every_cell_string_matches\":true,\"exact_initialized_targets\":true,\"actual_owned_slot_conversion\":true,\"provenance_cleared_before_reset\":true,\"actual_lookup_executed\":true,\"actual_casefold_queries_agree\":true,\"actual_hash_matches\":true,\"indices_loaded\":true,\"original_index_order_preserved\":true,\"scalar_stride_two\":true,\"scalar_loader_cleared\":true,\"all_aliases_match\":true,\"live_header_scope_verified\":true,\"prior_block4_preserved\":true,\"reset_and_release_pass\":true,\"row_queries\":%u,\"row_query_misses\":%u,\"row_query_disagreements\":%u,\"column_queries\":%u,\"column_query_disagreements\":%u}\n",asset,round,type,r.Position(),bytes.size(),(unsigned long long)cycleDisk,(unsigned long long)cycleNative,(unsigned long long)payload.name.bytes,payload.cells.count,inlineCells,offsetCells,(unsigned long long)inlineBytes,tempDiskCursor(),lastLookup.queries,lastLookup.rowMisses,lastLookup.rowDisagreements,lastLookup.queries,lastLookup.columnDisagreements);
#elif defined(BO1_STRINGTABLE_HASH_GATE)
        unsigned inlineCells=0,offsetCells=0;std::uint64_t inlineBytes=0;for(const auto&e:payload.strings){if(e.offset)++offsetCells;else if(e.tag){++inlineCells;inlineBytes+=e.bytes;}}
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"name_bytes\":%llu,\"cell_count\":%d,\"inline_cell_strings\":%u,\"offset_cell_strings\":%u,\"inline_cell_bytes\":%llu,\"block0_native_after_pop\":0,\"block0_shadow_after_pop\":%u,\"every_cell_string_matches\":true,\"exact_initialized_targets\":true,\"actual_owned_slot_conversion\":true,\"provenance_cleared_before_reset\":true,\"actual_hash_matches\":true,\"indices_loaded\":true,\"original_index_order_preserved\":true,\"scalar_stride_two\":true,\"scalar_loader_cleared\":true,\"all_aliases_match\":true,\"live_header_scope_verified\":true,\"prior_block4_preserved\":true,\"reset_and_release_pass\":true}\n",asset,round,type,r.Position(),bytes.size(),(unsigned long long)payload.indices.diskEnd,(unsigned long long)payload.indices.nativeEnd,(unsigned long long)payload.name.bytes,payload.cells.count,inlineCells,offsetCells,(unsigned long long)inlineBytes,tempDiskCursor());
#elif defined(BO1_STRINGTABLE_INDEX_GATE)
        unsigned inlineCells=0,offsetCells=0;std::uint64_t inlineBytes=0;for(const auto&e:payload.strings){if(e.offset)++offsetCells;else if(e.tag){++inlineCells;inlineBytes+=e.bytes;}}
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"name_bytes\":%llu,\"cell_count\":%d,\"inline_cell_strings\":%u,\"offset_cell_strings\":%u,\"inline_cell_bytes\":%llu,\"block0_native_after_pop\":0,\"block0_shadow_after_pop\":%u,\"every_cell_string_matches\":true,\"exact_initialized_targets\":true,\"actual_owned_slot_conversion\":true,\"provenance_cleared_before_reset\":true,\"indices_loaded\":true,\"original_index_order_preserved\":true,\"scalar_stride_two\":true,\"scalar_loader_cleared\":true,\"all_aliases_match\":true,\"live_header_scope_verified\":true,\"prior_block4_preserved\":true,\"reset_and_release_pass\":true}\n",asset,round,type,r.Position(),bytes.size(),(unsigned long long)payload.indices.diskEnd,(unsigned long long)payload.indices.nativeEnd,(unsigned long long)payload.name.bytes,payload.cells.count,inlineCells,offsetCells,(unsigned long long)inlineBytes,tempDiskCursor());
#elif defined(BO1_STRINGTABLE_STRINGS_GATE)
        unsigned inlineCells=0,offsetCells=0;std::uint64_t inlineBytes=0;for(const auto&e:payload.strings){if(e.offset)++offsetCells;else if(e.tag){++inlineCells;inlineBytes+=e.bytes;}}
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"name_bytes\":%llu,\"cell_count\":%d,\"inline_cell_strings\":%u,\"offset_cell_strings\":%u,\"inline_cell_bytes\":%llu,\"block0_native_after_pop\":0,\"block0_shadow_after_pop\":%u,\"every_cell_string_matches\":true,\"exact_initialized_targets\":true,\"actual_owned_slot_conversion\":true,\"provenance_cleared_before_reset\":true,\"index_tag_unresolved\":true,\"all_aliases_match\":true,\"live_header_scope_verified\":true,\"prior_block4_preserved\":true,\"reset_and_release_pass\":true}\n",asset,round,type,r.Position(),bytes.size(),(unsigned long long)payload.stringDiskEnd,(unsigned long long)payload.stringNativeEnd,(unsigned long long)payload.name.bytes,payload.cells.count,inlineCells,offsetCells,(unsigned long long)inlineBytes,tempDiskCursor());
#elif defined(BO1_STRINGTABLE_CELLS_GATE)
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"name_bytes\":%llu,\"name_source_start\":%zu,\"name_disk_start\":%llu,\"name_native_start\":%llu,\"cell_count\":%d,\"cell_source_start\":%zu,\"cell_disk_start\":%llu,\"cell_native_start\":%llu,\"cell_disk_bytes\":%llu,\"cell_native_bytes\":%llu,\"block0_native_after_pop\":0,\"block0_shadow_after_pop\":%u,\"all_cell_tags_hashes_match\":true,\"cell_padding_zero\":true,\"exact_cell_mappings\":true,\"all_aliases_match\":true,\"live_header_scope_verified\":true,\"prior_block4_preserved\":true,\"reset_and_release_pass\":true}\n",asset,round,type,r.Position(),bytes.size(),(unsigned long long)payload.cells.diskEnd,(unsigned long long)payload.cells.nativeEnd,(unsigned long long)payload.name.bytes,payload.nameSource,(unsigned long long)payload.name.diskStart,(unsigned long long)payload.name.nativeStart,payload.cells.count,payload.cellSource,(unsigned long long)payload.cells.diskStart,(unsigned long long)payload.cells.nativeStart,(unsigned long long)payload.cells.diskBytes,(unsigned long long)payload.cells.nativeBytes,tempDiskCursor());
#elif defined(BO1_STRINGTABLE_HEADER_GATE)
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"column_count\":%d,\"row_count\":%d,\"cell_count\":%llu,\"header_disk_bytes\":20,\"header_native_bytes\":%zu,\"block0_native_before_pop\":%llu,\"block0_native_after_pop\":0,\"block0_shadow_after_pop\":%llu,\"child_tags_preserved\":true,\"exact_header_mapping\":true,\"all_aliases_match\":true,\"prior_block4_preserved\":true,\"reset_and_release_pass\":true}\n",asset,round,type,r.Position(),bytes.size(),(unsigned long long)directory.diskEnd,(unsigned long long)directory.nativeEnd,header.columns,header.rows,(unsigned long long)header.cells,sizeof(StringTable),(unsigned long long)header.nativeEnd,(unsigned long long)tempDiskCursor());
#elif defined(BO1_ASSET_DIRECTORY_GATE)
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"inline_string_count\":%zu,\"raw_string_bytes\":%llu,\"directory_count\":%d,\"directory_disk_bytes\":%llu,\"directory_native_bytes\":%llu,\"directory_source_start\":%zu,\"directory_disk_start\":%llu,\"directory_native_start\":%llu,\"decoded_source_end\":%zu,\"source_consumed_bytes\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"directory_types_match\":true,\"header_tags_match\":true,\"padding_zero\":true,\"exact_element_mappings\":true,\"all_aliases_match\":true,\"reset_and_release_pass\":true}\n",asset,round,type,f.strings.size()-f.nulls,(unsigned long long)(f.end-52-f.table.diskBytes),directory.count,(unsigned long long)directory.diskBytes,(unsigned long long)directory.nativeBytes,f.end,(unsigned long long)directory.diskStart,(unsigned long long)directory.nativeStart,f.end+static_cast<std::size_t>(directory.diskBytes),r.Position(),(unsigned long long)directory.diskEnd,(unsigned long long)directory.nativeEnd);
#else
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"inline_string_count\":%zu,\"raw_string_bytes\":%llu,\"max_string_bytes\":%u,\"null_slots\":%u,\"empty_slots\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"native_padding_bytes\":%llu,\"per_string_lengths_match\":true,\"raw_bytes_match\":true,\"mappings_match\":true,\"all_aliases_match\":true,\"reset_and_release_pass\":true}\n",asset,round,type,f.strings.size()-f.nulls,(unsigned long long)(f.end-52-f.table.diskBytes),f.max,f.nulls,f.empty,r.Position(),f.end,(unsigned long long)f.diskEnd,(unsigned long long)f.nativeEnd,(unsigned long long)f.padding);
#endif
    }
    const char*schema=
#if defined(BO1_ENTRY_GENUINE_GATE)
        "BlackOpsARM.TASK-0033.entry-owned-genuine.v1";
#elif defined(BO1_RAWFILE_OWNER_GATE)
        "BlackOpsARM.TASK-0030.rawfile-owner.v1";
#elif defined(BO1_RAWFILE_PAYLOAD_GATE)
        "BlackOpsARM.TASK-0029.rawfile-payload.v1";
#elif defined(BO1_RAWFILE_HEADER_GATE)
        "BlackOpsARM.TASK-0028.rawfile-header.v1";
#elif defined(BO1_FRONTEND_INTERN_GATE)
        "BlackOpsARM.TASK-0017.intern-strings.v1";
#elif defined(BO1_STRINGTABLE_LOOKUP_GATE)
        "BlackOpsARM.TASK-0023.stringtable-lookup.v1";
#elif defined(BO1_STRINGTABLE_HASH_GATE)
        "BlackOpsARM.TASK-0022.stringtable-hash.v1";
#elif defined(BO1_STRINGTABLE_INDEX_GATE)
        "BlackOpsARM.TASK-0021.stringtable-indices.v1";
#elif defined(BO1_STRINGTABLE_STRINGS_GATE)
        "BlackOpsARM.TASK-0020.stringtable-strings.v1";
#elif defined(BO1_STRINGTABLE_CELLS_GATE)
        "BlackOpsARM.TASK-0019.stringtable-cells.v1";
#elif defined(BO1_STRINGTABLE_HEADER_GATE)
        "BlackOpsARM.TASK-0018.stringtable-header.v1";
#elif defined(BO1_ASSET_DIRECTORY_GATE)
        "BlackOpsARM.TASK-0016.asset-directory.v1";
#else
        "BlackOpsARM.TASK-0015.raw-strings.v1";
#endif
    std::printf("{\"schema\":\"%s\",\"pointer_bytes\":%zu,\"fixtures_passed\":%u,\"genuine_cycles_passed\":%u,\"logical_baseline_restored\":true}\n",schema,sizeof(void*),tested,cycles);
}
