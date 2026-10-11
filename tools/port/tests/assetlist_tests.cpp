// Genuine envelope only. Finite validated reader is a fixture boundary, not
// the engine's Win32 asynchronous/refill DB_LoadXFileData implementation.
#include <platform/fastfile_assetlist.h>
#include <platform/fastfile_inflate.h>
#include <database/db_expand.hpp>
#include <database/db_stream.h>
#include <database/db_memory.h>
#include <universal/physicalmemory.h>
#include <zlib/zlib.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#if defined(__ANDROID__)
#include <platform/android/zone_memory.h>
#include <platform/android/hunk_services.h>
#include <platform/android/virtual_memory.h>
bool Bo1_TestZoneRegistrationFailure(){return false;}
#else
#include <Windows.h>
#endif
using namespace bo1::fastfile;
static void check(bool v,const char*why){if(!v){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static XBlock detached[7]{};
static XAssetList published{}; // Persistent alias destination until explicit reset.
struct Reader{const unsigned char*data{};std::size_t bytes{},position{};unsigned calls{};bool fail{};};
static Reader *reader;
struct StreamState {XBlock*blocks;unsigned char*pos;unsigned index,delay,stack;std::array<unsigned char*,7>cursors{};
    std::array<unsigned,7>shadow{};std::array<uintptr_t*,2>alias{};};
static StreamState state(){StreamState s{g_streamBlocks,g_streamPos,g_streamPosIndex,g_streamDelayIndex,g_streamPosStackIndex};
    for(unsigned i=0;i<7;++i){s.cursors[i]=g_streamPosArray[i];
#if BO1_NATIVE_64BIT
        s.shadow[i]=DB_StreamWin32Pos(i);
#endif
    }
#if BO1_NATIVE_64BIT
    s.alias={DB_StreamAliasSlotLookup(0,4),DB_StreamAliasSlotLookup(0,12)};
#endif
    return s;}
static bool sameState(const StreamState&a,const StreamState&b){return a.blocks==b.blocks&&a.pos==b.pos&&a.index==b.index&&a.delay==b.delay&&a.stack==b.stack&&a.cursors==b.cursors&&a.shadow==b.shadow&&a.alias==b.alias;}
void __cdecl DB_LoadXFileData(unsigned __int8*dst,int n){
    if(!reader||!dst||n!=16||reader->fail||reader->position>reader->bytes||std::size_t(n)>reader->bytes-reader->position)throw std::runtime_error("bounded envelope read unavailable");
    std::memcpy(dst,reader->data+reader->position,std::size_t(n));reader->position+=std::size_t(n);++reader->calls;
}
struct Counts{std::uint64_t owners,count,bytes,reservations,low,high;};
static Counts counts(){
#if defined(__ANDROID__)
    return {bo1::android::ZoneMemoryCount(),bo1::android::ZoneMemoryAllocationCount(),bo1::android::ZoneMemoryRequestedBytes(),bo1::android::VirtualReservationCount(),0,0};
#else
    return {g_mem.prim[0].allocListCount+g_mem.prim[1].allocListCount,0,0,g_mem.buf?1u:0u,g_mem.prim[0].pos,g_mem.prim[1].pos};
#endif
}
static bool same(Counts a,Counts b){return a.owners==b.owners&&a.count==b.count&&a.bytes==b.bytes&&a.reservations==b.reservations&&a.low==b.low&&a.high==b.high;}
static std::size_t page(){
#if defined(__ANDROID__)
    return bo1::android::VirtualPageSize();
#else
    SYSTEM_INFO s{};GetSystemInfo(&s);return s.dwPageSize;
#endif
}
static ZoneLimits limits(unsigned type=0){std::uint64_t pool=0x14800000u;
#if BO1_NATIVE_64BIT
    pool+=pool/2;
#endif
    return {pool,INT32_MAX-8192u,4096,type,0};}
struct Owner{const char*name="genuine-assetlist";unsigned type{};};
static bool begin(void*v,const ZonePlan&p){auto&c=*static_cast<Owner*>(v);
#if defined(__ANDROID__)
    if(bo1::android::ZoneMemoryHasOpenOwner()||bo1::android::ZoneMemoryContains(c.name,c.type))return false;
    bo1::android::ZoneMemoryBegin(c.name,c.type,static_cast<int>(p.firstFitCapacity));
#else
    if(g_mem.prim[c.type].allocName)return false;
    PMem_BeginAlloc(c.name,c.type,TRACK_FASTFILE);
#endif
    return true;}
static unsigned char*allocate(void*v,unsigned n,unsigned i){auto&c=*static_cast<Owner*>(v);return DB_MemAlloc(n,(i==3||i==6)?2u:(i==0?0u:1u),c.type,c.name,0);}
static void end(void*v){
#if defined(__ANDROID__)
    (void)v;bo1::android::ZoneMemoryEnd();
#else
    auto&c=*static_cast<Owner*>(v);PMem_EndAlloc(c.name,c.type);
#endif
}
static void release(void*v){auto&c=*static_cast<Owner*>(v);
#if defined(__ANDROID__)
    bo1::android::ZoneMemoryRelease(c.name,c.type);
#else
    extern int freeTot;freeTot=0;PMem_Free(c.name);
#endif
}
static std::array<unsigned char,52> readPrefix(const char*path){std::array<unsigned char,52>b{};std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(b.data()),b.size());check(f.gcount()==52&&f.peek()==EOF,"exact validated52-byte prefix artifact");return b;}
static void decode(const char*path,const char*out){Bo1InflatedFastFile file;check(Bo1_InflateFastFilePath(path,{},&file)==Bo1InflateResult::Ok,"genuine complete decode");check(file.decoded.size()>=52,"decoded envelope present");
    const auto sum=adler32(1,file.decoded.data(),static_cast<unsigned>(file.decoded.size()));const auto size=file.decoded.size(),capacity=file.decoded.capacity();
    std::ofstream f(out,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(file.decoded.data()),52);f.close();check(bool(f),"prefix output");
    std::printf("{\"decoded_bytes\":%zu,\"decoded_adler32\":%lu,\"vector_capacity_bytes\":%zu,\"prefix_hex\":\"",size,sum,capacity);
    for(unsigned i=0;i<52;++i)std::printf("%02x",file.decoded[i]);std::vector<std::uint8_t>().swap(file.decoded);
    std::printf("\",\"decoded_buffer_released\":true,\"envelope_offset\":36,\"envelope_bytes\":16}\n");}
static void expand(Reader&r){reader=&r;
#if BO1_NATIVE_64BIT
    DB_ExpandLoad_XAssetList(&published);
#else
    DB_LoadXFileData(reinterpret_cast<unsigned char*>(&published),16); // Actual Win32 caller's envelope branch.
#endif
    reader=nullptr;
}
static void verify(const AssetListDisk&d,Reader&r,XBlock(&b)[7]){check(r.calls==1&&r.position==16,"one exact16-byte side-buffer read");
    check(published.stringList.count==d.stringCount&&published.assetCount==d.assetCount,"signed fields unchanged");
#if BO1_NATIVE_64BIT
    auto expected=[](std::uint32_t tag)->uintptr_t{return tag==0xffffffffu?~uintptr_t(0):tag==0xfffffffeu?~uintptr_t(1):uintptr_t(tag);};
    check(reinterpret_cast<uintptr_t>(published.stringList.strings)==expected(d.stringsTag)&&reinterpret_cast<uintptr_t>(published.assets)==expected(d.assetsTag),"production pointer-tag widening");
    check(DB_StreamAliasSlotLookup(0,4)==reinterpret_cast<uintptr_t*>(&published.stringList.strings)&&DB_StreamAliasSlotLookup(0,12)==reinterpret_cast<uintptr_t*>(&published.assets),"aliases point to persistent final fields");
    for(unsigned i=0;i<7;++i)check(DB_StreamWin32Pos(i)==0&&!DB_StreamExactLookup(i,0),"no shadow commit for side buffer");
#else
    check(!std::memcmp(&published,&d,16),"Win32 exact raw envelope");
#endif
    check(g_streamBlocks==b&&g_streamPos==b[0].data&&g_streamPosIndex==0&&g_streamPosStackIndex==0&&g_streamDelayIndex==0,"no block cursor advance");
    for(unsigned i=1;i<7;++i)check(g_streamPosArray[i]==b[i].data,"other block positions unchanged");
}
static unsigned fixtures(){unsigned tests=0;const auto base=counts();std::array<unsigned char,55>bytes{};XFile f{};f.size=48;f.blockSize[4]=32;AssetListDisk d{2,1,3,1};
    g_streamDelayIndex=3; // Make unintended initializer/reset on rejection observable.
#if BO1_NATIVE_64BIT
    DB_StreamRegisterAliasSlot(0,4,reinterpret_cast<uintptr_t*>(&published.stringList.strings));
    DB_StreamRegisterAliasSlot(0,12,reinterpret_cast<uintptr_t*>(&published.assets));
#endif
    auto pack=[&]{std::memcpy(bytes.data()+1,&f,36);std::memcpy(bytes.data()+37,&d,16);};pack();
    auto expect=[&](std::size_t n,std::uint64_t decoded,AssetListResult wanted){AssetListPlan p;auto before=published;const auto streams=state();Reader r{bytes.data()+37,16};auto result=PlanAssetList(bytes.data()+1,n,decoded,limits(),page(),p);
        check(result==wanted,"controlled metadata fixture status");check(r.calls==0&&r.position==0&&same(base,counts())&&!std::memcmp(&before,&published,sizeof(before))&&sameState(streams,state()),"preflight does not publish/read/allocate/reset streams");
        if(result!=AssetListResult::Ok)check(p.minimumDiskBytes==0&&p.zone.decodedBytes==0,"failed plan empty");++tests;};
    for(unsigned n=0;n<52;++n)expect(n,84,AssetListResult::ShortPrefix);
    expect(52,51,AssetListResult::ShortPrefix);expect(52,85,AssetListResult::ZoneHeader);
    d.stringCount=-1;pack();expect(52,84,AssetListResult::NegativeCount);d.stringCount=INT32_MIN;pack();expect(52,84,AssetListResult::NegativeCount);
    d.stringCount=2;d.assetCount=-1;pack();expect(52,84,AssetListResult::NegativeCount);
    d.assetCount=INT32_MAX;pack();expect(52,84,AssetListResult::DiskFootprint);d.assetCount=3;d.stringCount=INT32_MAX;pack();expect(52,84,AssetListResult::DiskFootprint);
    d={2,1,3,1};pack();expect(52,84,AssetListResult::Ok);expect(54,84,AssetListResult::Ok); // Unaligned prefix; extra prefix bytes untouched.
    f.blockSize[4]=31;pack();expect(52,84,AssetListResult::DiskFootprint);f.blockSize[4]=32;f.size=47;pack();expect(52,83,AssetListResult::DiskFootprint);
    f.size=48;d={INT32_MAX,0,INT32_MAX,0};pack();expect(52,84,AssetListResult::Ok); // Absent arrays preserve production presence semantics.
    f.blockSize[4]=33554432;f.size=f.blockSize[4]+16;d={8388608,1,0,0};pack();
    expect(52,std::uint64_t(f.size)+36,BO1_NATIVE_64BIT?AssetListResult::NativeFootprint:AssetListResult::Ok);
    // Genuine production widening and registration for the entire tag domain class.
    for(auto tag:{0u,1u,0x80000000u,0xfffffffdu,0xfffffffeu,0xffffffffu}){std::array<unsigned char,20>data{};d={0,tag,0,tag};std::memcpy(data.data(),&d,16);Reader r{data.data(),20};DB_InitStreams(detached);expand(r);verify(d,r,detached);DB_InitStreams(detached);++tests;}
    // Explicit read fault before copying or destination mutation; detach resets private stream maps.
    Reader bad{bytes.data()+37,15};const auto before=published;DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
    DB_StreamRegisterAliasSlot(0,4,reinterpret_cast<uintptr_t*>(&published.stringList.strings));
    DB_StreamRegisterAliasSlot(0,12,reinterpret_cast<uintptr_t*>(&published.assets));
#endif
    const auto streams=state();bool threw=false;reader=&bad;
    try{
#if BO1_NATIVE_64BIT
        DB_ExpandLoad_XAssetList(&published);
#else
        DB_LoadXFileData(reinterpret_cast<unsigned char*>(&published),16);
#endif
    }catch(const std::runtime_error&){threw=true;}reader=nullptr;
    check(threw&&!std::memcmp(&before,&published,sizeof(before))&&bad.position==0&&bad.calls==0&&same(base,counts())&&sameState(streams,state()),"short finite source fails explicitly without publication or stream mutation");DB_InitStreams(detached);++tests;
    return tests;
}
int main(int argc,char**argv){check(argc>=2,"decode or gate command");if(!std::strcmp(argv[1],"decode")){check(argc==4,"decode input output");decode(argv[2],argv[3]);return 0;}
    check(argc==6&&!std::strcmp(argv[1],"gate"),"gate prefix1 decoded1 prefix2 decoded2");
#if defined(__ANDROID__)
    bo1::android::RegisterHunkMainThread();
#else
    PMem_Init();
#endif
    DB_InitStreams(detached);const auto base=counts();const auto tested=fixtures();auto prefix1=readPrefix(argv[2]),prefix2=readPrefix(argv[4]);unsigned cycles=0;
    for(unsigned round=0;round<3;++round)for(unsigned type=0;type<2;++type)for(unsigned asset=0;asset<2;++asset){auto&prefix=asset?prefix2:prefix1;auto decoded=std::strtoull(argv[asset?5:3],nullptr,10);AssetListPlan p;
        check(PlanAssetList(prefix.data(),52,decoded,limits(type),page(),p)==AssetListResult::Ok,"genuine checked envelope plan");XBlock b[7]{};Owner o;o.type=type;unsigned failed;
        check(AllocateZonePlan(p.zone,page(),{&o,begin,allocate,end,release},b,failed)==ZoneResult::Ok,"genuine checked blocks");DB_InitStreams(b);Reader r{prefix.data()+36,16};expand(r);verify(p.disk,r,b);
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"string_count\":%d,\"strings_tag\":%u,\"asset_count\":%d,\"assets_tag\":%u,\"native_strings_bits\":%llu,\"native_assets_bits\":%llu,\"minimum_disk_bytes\":%llu,\"minimum_native_bytes\":%llu,\"read_calls\":%u,\"consumed_bytes\":%zu,\"block_cursors_unchanged\":true,\"persistent_alias_slots\":true",
            asset,round,type,p.disk.stringCount,p.disk.stringsTag,p.disk.assetCount,p.disk.assetsTag,(unsigned long long)reinterpret_cast<uintptr_t>(published.stringList.strings),(unsigned long long)reinterpret_cast<uintptr_t>(published.assets),(unsigned long long)p.minimumDiskBytes,(unsigned long long)p.minimumNativeBytes,r.calls,r.position);
        DB_InitStreams(detached);
#if BO1_NATIVE_64BIT
        check(!DB_StreamAliasSlotLookup(0,4)&&!DB_StreamAliasSlotLookup(0,12),"aliases detached before output/owner reuse");
#endif
        published={};DB_FreeXBlocks(b,0);release(&o);check(same(base,counts()),"ownership baseline restored");++cycles;std::printf(",\"reset_and_release_pass\":true}\n");
    }
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0013.envelope.v1\",\"pointer_bytes\":%zu,\"envelope_native_bytes\":%zu,\"fixtures_passed\":%u,\"genuine_cycles_passed\":%u,\"logical_baseline_restored\":true,\"disk_bytes_per_read\":16}\n",sizeof(void*),sizeof(XAssetList),tested,cycles);
}
