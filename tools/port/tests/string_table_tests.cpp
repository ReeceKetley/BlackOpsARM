// Actual production pointer-table leaf, with explicit finite-reader fixture.
// Stops before raw strings, interning, asset arrays or full async/refill loader.
#include "frontend_zone_fixture.h"
#include <platform/fastfile_assetlist.h>
#include <platform/fastfile_script_table.h>
#include <platform/fastfile_inflate.h>
#include <database/db_expand.hpp>
#include <zlib/zlib.h>
#include <array>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <limits>
using namespace bo1::fastfile;
using frontend_fixture::require;
#if defined(__ANDROID__)
bool Bo1_TestZoneRegistrationFailure(){return false;}
#endif
static XBlock detached[7]{};
struct Reader{const unsigned char*data{};std::size_t bytes{},position{};unsigned calls{};bool fail{};};
static Reader *reader;
void __cdecl DB_LoadXFileData(unsigned __int8*dst,int n){
    if(!reader||!dst||n<0||reader->fail||reader->position>reader->bytes||std::size_t(n)>reader->bytes-reader->position)throw std::runtime_error("bounded table reader unavailable");
    std::memcpy(dst,reader->data+reader->position,std::size_t(n));reader->position+=std::size_t(n);++reader->calls;
}
static uintptr_t widened(std::uint32_t tag){
#if BO1_NATIVE_64BIT
    return tag==0xffffffffu?~uintptr_t(0):tag==0xfffffffeu?~uintptr_t(1):uintptr_t(tag);
#else
    return tag;
#endif
}
static ScriptTablePlan plan(std::int32_t n,std::uint32_t tag,std::uint64_t remaining,const ZonePlan&zone,std::uint64_t w=0,std::uint64_t native=0){
    ScriptTablePlan p;require(PlanScriptTable(n,tag,remaining,zone.file.blockSize[4],zone.logical[4],w,native,p)==ScriptTableResult::Ok,"checked production table plan");return p;}
static void execute(const ScriptTablePlan&p,Reader&r){
    if(!p.present)return;
    DB_PushStreamPos(4);void*out=DB_AllocStreamPos(3);require(out==g_streamBlocks[4].data+p.nativeStart,"actual native alignment vs plan");reader=&r;
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_Ptr32Array(out,p.count);
#else
    if(p.diskBytes){DB_LoadXFileData(static_cast<unsigned char*>(out),static_cast<int>(p.diskBytes));
        DB_IncStreamPos(static_cast<int>(p.diskBytes));} // Original Win32 Load_Stream branch.
#endif
    reader=nullptr;DB_PopStreamPos();
}
static void verify(const ScriptTablePlan&p,const unsigned char*tags,Reader&r,XBlock(&b)[7]){
    require(r.position==p.diskBytes&&r.calls==(p.diskBytes?1u:0u),"exact table-only read, no read when absent/empty");
    require(g_streamPosIndex==0&&g_streamPos==b[0].data&&g_streamPosStackIndex==0,"pop restores block zero");
    require(g_streamPosArray[4]==b[4].data+p.nativeEnd,"native block4 cursor footprint");
    for(unsigned i=1;i<7;++i)if(i!=4)require(g_streamPosArray[i]==b[i].data,"other cursors unchanged");
#if BO1_NATIVE_64BIT
    require(DB_StreamWin32Pos(4)==p.diskEnd,"shadow alignment/commit footprint");
#endif
    for(int i=0;p.present&&i<p.count;++i){std::uint32_t tag;std::memcpy(&tag,tags+4*std::size_t(i),4);uintptr_t actual;
        auto *slot=b[4].data+p.nativeStart+sizeof(void*)*std::size_t(i);std::memcpy(&actual,slot,sizeof(actual));require(actual==widened(tag),"every actual widened table tag");
#if BO1_NATIVE_64BIT
        const auto w=static_cast<unsigned>(p.diskStart+4*std::uint64_t(i));require(DB_StreamAliasSlotLookup(4,w)==reinterpret_cast<uintptr_t*>(slot),"every alias points to actual owned native slot");
        require(DB_StreamExactLookup(4,w)==slot,"element starts map exactly");
        for(unsigned d=1;d<4;++d)require(!DB_StreamExactLookup(4,w+d),"expanded element interior is not exact");
#endif
    }
#if BO1_NATIVE_64BIT
    if(p.present&&p.count)require(!DB_StreamExactLookup(4,static_cast<unsigned>(p.diskEnd)),"endpoint is not committed interior");
#endif
}
struct Frame{std::vector<unsigned char>bytes;AssetListPlan envelope;};
static Frame readFrame(const char*path,std::uint64_t decoded){std::ifstream in(path,std::ios::binary);std::array<unsigned char,52>first{};in.read(reinterpret_cast<char*>(first.data()),52);require(in.gcount()==52,"validated frame header");
    Frame f;require(PlanAssetList(first.data(),52,decoded,frontend_fixture::limits(),frontend_fixture::page(),f.envelope)==AssetListResult::Ok,"genuine envelope metadata");
    auto p=plan(f.envelope.disk.stringCount,f.envelope.disk.stringsTag,decoded-52,f.envelope.zone);f.bytes.resize(52+static_cast<std::size_t>(p.diskBytes));std::memcpy(f.bytes.data(),first.data(),52);
    if(p.diskBytes)in.read(reinterpret_cast<char*>(f.bytes.data()+52),static_cast<std::streamsize>(p.diskBytes));require(bool(in)&&in.peek()==EOF,"exact validated table prefix");return f;}
static void decode(const char*path,const char*out){Bo1InflatedFastFile f;require(Bo1_InflateFastFilePath(path,{},&f)==Bo1InflateResult::Ok,"genuine complete decode");AssetListPlan e;
    require(PlanAssetList(f.decoded.data(),f.decoded.size(),f.decoded.size(),frontend_fixture::limits(),frontend_fixture::page(),e)==AssetListResult::Ok,"complete decoded envelope");auto p=plan(e.disk.stringCount,e.disk.stringsTag,f.decoded.size()-52,e.zone);
    const auto bytes=f.decoded.size(),capacity=f.decoded.capacity();const auto adler=adler32(1,f.decoded.data(),static_cast<unsigned>(bytes));const auto prefix=52+static_cast<std::size_t>(p.diskBytes);
    std::ofstream file(out,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<char*>(f.decoded.data()),prefix);file.close();require(bool(file),"prefix output");
    std::printf("{\"decoded_bytes\":%zu,\"decoded_adler32\":%lu,\"vector_capacity_bytes\":%zu,\"prefix_hex\":\"",bytes,adler,capacity);for(std::size_t i=0;i<prefix;++i)std::printf("%02x",f.decoded[i]);
    std::vector<std::uint8_t>().swap(f.decoded);std::printf("\",\"decoded_buffer_released\":true}\n");}
static unsigned fixtures(const ZonePlan&zone){unsigned tests=0;const auto baseline=frontend_fixture::counts();g_streamDelayIndex=3;
#if BO1_NATIVE_64BIT
    static uintptr_t marker[2]{};DB_StreamRegisterAliasSlot(4,0,&marker[0]);DB_StreamRegisterAliasSlot(4,4,&marker[1]);
#endif
    auto rejected=[&](std::int32_t n,std::uint32_t tag,std::uint64_t source,std::uint64_t dc,std::uint64_t nc,std::uint64_t w,std::uint64_t native,ScriptTableResult expected){
    const auto streams=frontend_fixture::streamState();ScriptTablePlan p;
    require(PlanScriptTable(n,tag,source,dc,nc,w,native,p)==expected,"controlled plan status");require(frontend_fixture::counts()==baseline&&frontend_fixture::streamState()==streams,"preflight no allocation/cursor/shadow/alias/delay change");if(expected!=ScriptTableResult::Ok)require(p.diskBytes==0&&!p.present,"failed plan empty");++tests;
    std::printf("{\"fixture\":\"preflight_%u\",\"status\":\"%s\",\"no_mutation\":true}\n",tests,ScriptTableResultName(expected));};
    rejected(-1,1,100,100,100,0,0,ScriptTableResult::NegativeCount);rejected(INT32_MIN,0,0,0,0,0,0,ScriptTableResult::NegativeCount);
    rejected(0,0,0,0,0,0,0,ScriptTableResult::Ok);rejected(INT32_MAX,0,0,0,0,0,0,ScriptTableResult::Ok);
    rejected(0,1,0,0,0,0,0,ScriptTableResult::NativeBounds);rejected(1,1,3,100,100,0,0,ScriptTableResult::SourceBounds);
    rejected(1,1,4,3,100,0,0,ScriptTableResult::DiskBounds);rejected(1,1,4,4,sizeof(void*)-1,0,0,ScriptTableResult::NativeBounds);
    rejected(1,1,4,4,sizeof(void*),0,0,ScriptTableResult::Ok);rejected(0,1,0,4,8,1,1,ScriptTableResult::Ok);
    rejected(0,1,0,3,8,1,1,ScriptTableResult::DiskBounds);rejected(0,1,0,4,sizeof(void*)-1,1,1,ScriptTableResult::NativeBounds);
    rejected(1,0,0,4,4,5,0,ScriptTableResult::DiskBounds);rejected(1,0,0,4,4,0,5,ScriptTableResult::NativeBounds);
    rejected(0,1,0,UINT64_MAX,UINT64_MAX,UINT64_MAX,0,ScriptTableResult::DiskBounds);rejected(0,1,0,UINT64_MAX,UINT64_MAX,0,UINT64_MAX,ScriptTableResult::NativeBounds);
#if BO1_NATIVE_64BIT
    rejected((1<<20)+1,1,UINT64_MAX,UINT64_MAX,UINT64_MAX,0,0,ScriptTableResult::HelperDomain);
#else
    rejected(INT32_MAX,1,UINT64_MAX,UINT64_MAX,UINT64_MAX,0,0,ScriptTableResult::CommitOverflow);
#endif
    const std::array<std::uint32_t,6>tags={0,1,0x80000000u,0xfffffffdu,0xfffffffeu,0xffffffffu};
    frontend_fixture::Owner owner;XBlock b[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),b,failed)==ZoneResult::Ok,"genuine fixture owner");
    for(unsigned offset:{0u,1u})for(int count:{0,1,2,6}){DB_InitStreams(b);
        if(offset){DB_PushStreamPos(4);DB_IncStreamPos(1);DB_PopStreamPos();}
        auto p=plan(count,1,4*tags.size(),zone,offset,offset);Reader r{reinterpret_cast<const unsigned char*>(tags.data()),4*tags.size()};execute(p,r);verify(p,r.data,r,b);++tests;}
    // Explicit pre-copy reader throw happens AFTER alignment, so reset/release
    // restores lifetime; no claim that production post-read exceptions roll back.
    DB_InitStreams(b);auto p=plan(2,1,8,zone);Reader bad{reinterpret_cast<const unsigned char*>(tags.data()),7};bool threw=false;
    std::array<unsigned char,16>before{};std::memcpy(before.data(),b[4].data,before.size());const auto allocated=frontend_fixture::counts();const auto streams=frontend_fixture::streamState();
    try{execute(p,bad);}catch(const std::runtime_error&){threw=true;}reader=nullptr;require(threw&&bad.position==0&&bad.calls==0&&g_streamPosStackIndex==1,"controlled short read before table writes");++tests;
    require(!std::memcmp(before.data(),b[4].data,before.size())&&frontend_fixture::counts()==allocated,"short read leaves bytes/owner unchanged before reset");
#if BO1_NATIVE_64BIT
    require(DB_StreamAliasSlotLookup(4,0)==streams.aliases[0]&&DB_StreamAliasSlotLookup(4,4)==streams.aliases[1]&&DB_StreamWin32Pos(4)==p.diskStart,"short read no partial aliases/commit before reset");
#else
    (void)streams;
#endif
    DB_InitStreams(detached);DB_FreeXBlocks(b,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"fixture release baseline");return tests;}
int main(int argc,char**argv){require(argc>=2,"decode/gate");if(!std::strcmp(argv[1],"decode")){require(argc==4,"decode input output");decode(argv[2],argv[3]);return 0;}
    require(argc==6&&!std::strcmp(argv[1],"gate"),"gate prefix1 decoded1 prefix2 decoded2");frontend_fixture::startup();DB_InitStreams(detached);const auto baseline=frontend_fixture::counts();
    auto smallFrame=readFrame(argv[2],std::strtoull(argv[3],nullptr,10));auto largeFrame=readFrame(argv[4],std::strtoull(argv[5],nullptr,10));const auto tested=fixtures(smallFrame.envelope.zone);unsigned cycles=0;
    for(unsigned round=0;round<3;++round)for(unsigned type=0;type<2;++type)for(unsigned asset=0;asset<2;++asset){auto&f=asset?largeFrame:smallFrame;auto zone=f.envelope.zone;zone.limits.allocType=type;
        frontend_fixture::Owner owner;owner.type=type;XBlock b[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),b,failed)==ZoneResult::Ok,"genuine owner");DB_InitStreams(b);
        auto p=plan(f.envelope.disk.stringCount,f.envelope.disk.stringsTag,f.bytes.size()-52,zone);Reader r{f.bytes.data()+52,f.bytes.size()-52};execute(p,r);verify(p,r.data,r,b);
        DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
        for(int i=0;i<p.count;++i)require(!DB_StreamAliasSlotLookup(4,unsigned(4*i)),"all table aliases cleared before owner release");
#endif
        DB_FreeXBlocks(b,0);frontend_fixture::release(&owner);require(frontend_fixture::counts()==baseline,"genuine owner baseline restored");++cycles;
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"string_count\":%d,\"table_disk_bytes\":%llu,\"table_native_bytes\":%llu,\"source_consumed_bytes\":%zu,\"decoded_source_end\":%zu,\"block4_disk_cursor\":%llu,\"block4_native_cursor\":%llu,\"all_tags_match\":true,\"all_aliases_match\":true,\"exact_element_mappings\":true,\"reset_and_release_pass\":true}\n",asset,round,type,p.count,(unsigned long long)p.diskBytes,(unsigned long long)p.nativeBytes,r.position,52+r.position,(unsigned long long)p.diskEnd,(unsigned long long)p.nativeEnd);
    }
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0014.string-table.v1\",\"pointer_bytes\":%zu,\"fixtures_passed\":%u,\"genuine_cycles_passed\":%u,\"logical_baseline_restored\":true}\n",sizeof(void*),tested,cycles);
}
