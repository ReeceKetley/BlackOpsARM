// Actual Load_XStringCustom, with bounded borrowed decoded bytes. This finite
// adapter does not certify the engine's asynchronous/refill file loader.
#include "frontend_zone_fixture.h"
#include <platform/fastfile_assetlist.h>
#include <platform/fastfile_script_table.h>
#include <platform/fastfile_raw_string.h>
#include <platform/fastfile_inflate.h>
#include <database/db_expand.hpp>
#include <database/db_stream_load.h>
#include <zlib/zlib.h>
#include <algorithm>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <limits>
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
static void decode(const char*path,const char*out){Bo1InflatedFastFile f;require(Bo1_InflateFastFilePath(path,{},&f)==Bo1InflateResult::Ok,"complete genuine production decode");
    auto plan=preflight(f.decoded.data(),f.decoded.size(),f.decoded.size());const auto bytes=f.decoded.size(),capacity=f.decoded.capacity();const auto adler=adler32(1,f.decoded.data(),static_cast<unsigned>(bytes));
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
            Load_XStringCustom(&loaded);require(loaded==reinterpret_cast<const char*>(b[4].data+p.nativeStart),"actual raw leaf retains native pointer");
            require(!std::memcmp(loaded,r.Current()-p.bytes,static_cast<std::size_t>(p.bytes)),"each loaded byte including NUL matches validated source");
            value=reinterpret_cast<uintptr_t>(loaded);std::memcpy(slot,&value,sizeof(value));
#if BO1_NATIVE_64BIT
            for(unsigned d=0;d<p.bytes;++d)require(DB_StreamExactLookup(4,static_cast<unsigned>(p.diskStart+d))==b[4].data+p.nativeStart+d,"all loaded character offsets map exactly");
#endif
        }else require(!value,"null slot remains absent");
#if BO1_NATIVE_64BIT
        require(DB_StreamAliasSlotLookup(4,unsigned(4*entry.index))==reinterpret_cast<uintptr_t*>(slot),"table alias still names native owned pointer slot");
        require(DB_StreamWin32Pos(4)==p.diskEnd,"raw leaf shadow bytes plus zero disk alignment");
#endif
        require(DB_GetStreamPos()==b[4].data+p.nativeEnd,"raw leaf native cursor exact");
        if(measurements){if(comma)std::printf(",");comma=true;std::printf("{\"index\":%u,\"length\":%llu,\"adler32\":%lu}",entry.index,(unsigned long long)p.bytes,p.present?adler32(1,b[4].data+p.nativeStart,static_cast<unsigned>(p.bytes)):1ul);}
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
    require(argc==6&&!std::strcmp(argv[1],"gate"),"gate prefix1 decoded1 prefix2 decoded2");frontend_fixture::startup();DB_InitStreams(detached);const auto baseline=frontend_fixture::counts();
    auto a=readPrefix(argv[2]),b=readPrefix(argv[4]);auto ap=preflight(a.data(),a.size(),std::strtoull(argv[3],nullptr,10)),bp=preflight(b.data(),b.size(),std::strtoull(argv[5],nullptr,10));require(ap.end==a.size()&&bp.end==b.size(),"prefix stops exactly before asset directory");
    const auto tested=fixtures(ap.envelope.zone);unsigned cycles=0;
    for(unsigned round=0;round<3;++round)for(unsigned type=0;type<2;++type)for(unsigned asset=0;asset<2;++asset){auto&f=asset?bp:ap;auto&bytes=asset?b:a;auto zone=f.envelope.zone;zone.limits.allocType=type;frontend_fixture::Owner owner;owner.type=type;XBlock blocks[7]{};unsigned failed;
        require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),blocks,failed)==ZoneResult::Ok,"genuine actual allocator policy");DB_InitStreams(blocks);DecodedSource r(bytes.data()+52,bytes.size()-52);execute(f,r,blocks,asset&&round==0&&type==0);
        DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
        for(int i=0;i<f.table.count;++i)require(!DB_StreamAliasSlotLookup(4,unsigned(4*i)),"all table aliases detached before owner release");
#endif
        DB_FreeXBlocks(blocks,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"logical baseline after each genuine cycle");++cycles;
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"inline_string_count\":%zu,\"raw_string_bytes\":%llu,\"max_string_bytes\":%u,\"null_slots\":%u,\"empty_slots\":%u,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"native_padding_bytes\":%llu,\"per_string_lengths_match\":true,\"raw_bytes_match\":true,\"mappings_match\":true,\"all_aliases_match\":true,\"reset_and_release_pass\":true}\n",asset,round,type,f.strings.size()-f.nulls,(unsigned long long)(f.end-52-f.table.diskBytes),f.max,f.nulls,f.empty,r.Position(),f.end,(unsigned long long)f.diskEnd,(unsigned long long)f.nativeEnd,(unsigned long long)f.padding);
    }
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0015.raw-strings.v1\",\"pointer_bytes\":%zu,\"fixtures_passed\":%u,\"genuine_cycles_passed\":%u,\"logical_baseline_restored\":true}\n",sizeof(void*),tested,cycles);
}
