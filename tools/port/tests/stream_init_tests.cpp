// TASK-0012: genuine header allocation -> production stream cursor lifetime.
// No payload copying, asset readers, pointer fixups, registry or zone init.
#include <platform/fastfile_zone.h>
#include <platform/bo1_native_width.h>
#include <database/db_memory.h>
#include <database/db_stream.h>
#include <universal/physicalmemory.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <cstdint>
#if defined(__ANDROID__)
#include <platform/android/zone_memory.h>
#include <platform/android/hunk_services.h>
#include <platform/android/virtual_memory.h>
#else
#include <Windows.h>
#endif
using namespace bo1::fastfile;
#if defined(__ANDROID__)
bool Bo1_TestZoneRegistrationFailure(){return false;} // TASK-0011 injection hook disabled here.
#endif
static void check(bool ok,const char*why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
// Persistent detached descriptor set: stream globals never point at expired locals.
static XBlock detached[7]{};
static unsigned initCalls;
static void init(XBlock*b){DB_InitStreams(b);++initCalls;}
struct Counts {std::uint64_t owners,count,bytes,reservations,low,high;};
static Counts counts(){
#if defined(__ANDROID__)
    return {bo1::android::ZoneMemoryCount(),bo1::android::ZoneMemoryAllocationCount(),bo1::android::ZoneMemoryRequestedBytes(),bo1::android::VirtualReservationCount(),0,0};
#else
    return {g_mem.prim[0].allocListCount+g_mem.prim[1].allocListCount,0,0,g_mem.buf?1u:0u,g_mem.prim[0].pos,g_mem.prim[1].pos};
#endif
}
static bool same(Counts a,Counts b){return a.owners==b.owners&&a.count==b.count&&a.bytes==b.bytes&&a.reservations==b.reservations&&a.low==b.low&&a.high==b.high;}
static void printCounts(Counts c){std::printf("{\"owners\":%llu,\"allocation_count\":%llu,\"requested_bytes\":%llu,\"reservations\":%llu,\"low\":%llu,\"high\":%llu}",
    (unsigned long long)c.owners,(unsigned long long)c.count,(unsigned long long)c.bytes,(unsigned long long)c.reservations,(unsigned long long)c.low,(unsigned long long)c.high);}
struct Owner {const char*name="genuine-stream-init";unsigned type{};int failIndex=-1;unsigned begins{};};
static bool begin(void*v,const ZonePlan&p){auto&c=*static_cast<Owner*>(v);++c.begins;
#if defined(__ANDROID__)
    if(bo1::android::ZoneMemoryHasOpenOwner()||bo1::android::ZoneMemoryContains(c.name,c.type))return false;
    bo1::android::ZoneMemoryBegin(c.name,c.type,static_cast<int>(p.firstFitCapacity));
#else
    if(g_mem.prim[c.type].allocName)return false;
    for(const auto&prim:g_mem.prim)for(unsigned i=0;i<prim.allocListCount;++i)if(prim.allocList[i].name&&!std::strcmp(prim.allocList[i].name,c.name))return false;
    PMem_BeginAlloc(c.name,c.type,TRACK_FASTFILE);
#endif
    return true;}
static unsigned char*allocate(void*v,unsigned n,unsigned i){auto&c=*static_cast<Owner*>(v);if(int(i)==c.failIndex)return nullptr;
    return DB_MemAlloc(n,(i==3||i==6)?2u:(i==0?0u:1u),c.type,c.name,0);}
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
    // Diagnostic-only cumulative int reset, same isolated reference as TASK-0011.
    extern int freeTot;freeTot=0;PMem_Free(c.name);
#endif
}
static ZoneBackend backend(Owner&c){return {&c,begin,allocate,end,release};}
static bool empty(const XBlock(&b)[7]){for(auto&x:b)if(x.data||x.size)return false;return true;}
static std::size_t page(){
#if defined(__ANDROID__)
    return bo1::android::VirtualPageSize();
#else
    SYSTEM_INFO s{};GetSystemInfo(&s);return s.dwPageSize;
#endif
}
static ZoneLimits limits(unsigned type){std::uint64_t pool=0x14800000u;
#if BO1_NATIVE_64BIT
    pool+=pool/2;
#endif
    return {pool,INT32_MAX-8192u,4096,type,0};}
static XFile read(const char*path){XFile f{};std::ifstream in(path,std::ios::binary);in.read(reinterpret_cast<char*>(&f),sizeof(f));check(in.gcount()==sizeof(f)&&in.peek()==EOF,"exact validated 36-byte header");return f;}
static void cursors(XBlock(&b)[7]){
    check(g_streamBlocks==b&&g_streamPos==b[0].data&&DB_GetStreamPos()==b[0].data,"block zero current cursor");
    check(g_streamPosIndex==0&&g_streamDelayIndex==0&&g_streamPosStackIndex==0&&g_streamPosArray[0]==nullptr,"initial indexes and inactive current slot");
    for(unsigned i=0;i<7;++i){if(i)check(g_streamPosArray[i]==b[i].data,"all seven initial positions including zero slots");
#if BO1_NATIVE_64BIT
        check(DB_StreamWin32Pos(i)==0,"shadow cursor zero");check(!DB_StreamExactLookup(i,0)&&!DB_StreamAliasSlotLookup(i,0),"no stale commit/alias maps");
        check(DB_StreamMapLookup(i,0)==b[i].data,"initial drift identity");
        if(b[i].data)check(DB_StreamMapLookup(i,8)==b[i].data+8,"no stale nonzero drift");
#endif
    }
}
static void switches(XBlock(&b)[7]){for(unsigned i=0;i<7;++i){DB_PushStreamPos(i);check(g_streamPosIndex==i&&DB_GetStreamPos()==b[i].data&&g_streamPosStackIndex==1,"push including null slots, no dereference");DB_PopStreamPos();check(g_streamPosIndex==0&&g_streamPos==b[0].data&&g_streamPosStackIndex==0,"pop restores block zero");}}
static void resetFixture(XBlock(&b)[7]){
#if BO1_NATIVE_64BIT
    // Cursor arithmetic on an owned block; no payload is read or written.
    DB_SetExpandElem(4,8);DB_SetExpandWin32Size(8);DB_IncStreamPos(16);
    check(DB_StreamExactLookup(0,4)==b[0].data+8&&DB_StreamWin32Pos(0)==8,"observable expanded commit fixture");
    DB_StreamRegisterAliasSlot(0,0,reinterpret_cast<uintptr_t*>(b[0].data));
    check(DB_StreamAliasSlotLookup(0,0)==reinterpret_cast<uintptr_t*>(b[0].data),"alias fixture registered");
    DB_SetExpandElem(4,8);DB_SetExpandWin32Size(4);
#else
    DB_IncStreamPos(16);
#endif
    DB_PushStreamPos(4);g_streamDelayIndex=4;check(g_streamPosStackIndex==1,"active stack before reinit");
    init(b);cursors(b);
    DB_IncStreamPos(16);
#if BO1_NATIVE_64BIT
    check(DB_StreamWin32Pos(0)==16&&DB_StreamExactLookup(0,4)==b[0].data+4,"pending size AND stride reset");
    DB_SetExpandElem(4,8);DB_SetExpandWin32Size(4);
#endif
    init(b);cursors(b);
#if BO1_NATIVE_64BIT
    // Unequal sizes make a stale element stride observable at an interior offset.
    DB_SetExpandWin32Size(8);DB_IncStreamPos(16);
    check(DB_StreamWin32Pos(0)==8&&!DB_StreamExactLookup(0,4),"pending element stride reset independently");
    DB_StreamRegisterAliasSlot(0,0,reinterpret_cast<uintptr_t*>(b[0].data));
    DB_SetExpandElem(4,8);DB_SetExpandWin32Size(4);
#endif
    DB_PushStreamPos(4);g_streamDelayIndex=4;
}
int main(int argc,char**argv){check(argc==5,"usage: stream_init header1 decodedBytes1 header2 decodedBytes2");
#if defined(__ANDROID__)
    bo1::android::RegisterHunkMainThread();
#else
    PMem_Init(); // Retained production pool precedes the logical ownership baseline.
#endif
    init(detached);cursors(detached);const auto baseline=counts();unsigned passed=0,rejections=0,failures=0;
    XFile files[2]={read(argv[1]),read(argv[3])};const std::uint64_t bytes[2]={std::strtoull(argv[2],nullptr,10),std::strtoull(argv[4],nullptr,10)};
    // Invalid metadata and tampering never reach the backend or stream initializer.
    for(unsigned asset=0;asset<2;++asset){ZonePlan p{};auto l=limits(0);check(PlanZoneHeader(&files[asset],36,bytes[asset],l,page(),p)==ZoneResult::Ok,"genuine plan");
        Owner o;XBlock b[7]{};unsigned fail=99;const auto before=initCalls;
        check(PlanZoneHeader(&files[asset],35,bytes[asset],l,page(),p)==ZoneResult::ShortHeader,"truncated preflight");++rejections;
        check(PlanZoneHeader(&files[asset],36,bytes[asset]+1,l,page(),p)==ZoneResult::SizeMismatch,"inconsistent decoded length");++rejections;
        check(PlanZoneHeader(&files[asset],36,bytes[asset],l,page(),p)==ZoneResult::Ok,"restore genuine plan");++p.logical[0];
        check(AllocateZonePlan(p,page(),backend(o),b,fail)==ZoneResult::InconsistentPlan,"tampered descriptor preflight");++rejections;
        check(o.begins==0&&initCalls==before&&empty(b)&&same(counts(),baseline),"rejection before allocation/init");
        check(PlanZoneHeader(&files[asset],36,bytes[asset],l,page(),p)==ZoneResult::Ok,"partial fixture plan");
        for(unsigned i=0;i<7;++i)if(p.logical[i]){o.failIndex=int(i);check(AllocateZonePlan(p,page(),backend(o),b,fail)==ZoneResult::AllocationFailed&&fail==i,"controlled genuine partial failure");
            check(initCalls==before&&empty(b)&&same(counts(),baseline),"partial failure never publishes streams and restores owner");++failures;}
    }
    // Alternate assets, low/high PMem policy and repeated lifetimes; one owner at a time.
    for(unsigned round=0;round<3;++round)for(unsigned type=0;type<2;++type)for(unsigned asset=0;asset<2;++asset){
        ZonePlan p{};check(PlanZoneHeader(&files[asset],36,bytes[asset],limits(type),page(),p)==ZoneResult::Ok,"genuine plan");Owner o;o.type=type;XBlock b[7]{};unsigned fail=99;
        check(AllocateZonePlan(p,page(),backend(o),b,fail)==ZoneResult::Ok,"production checked allocation");
        for(unsigned i=0;i<7;++i)check(b[i].size==p.logical[i]&&bool(b[i].data)==bool(p.logical[i]),"logical blocks and zero slots");
        const auto allocated=counts();init(b);cursors(b);switches(b);init(b);cursors(b);resetFixture(b);
        // Detach while ownership is live. Actual DB_FreeXBlocks clears descriptors;
        // named release then frees native mapping or rewinds retained Windows PMem.
        init(detached);cursors(detached);DB_FreeXBlocks(b,0);check(empty(b)&&same(counts(),allocated),"descriptor clear does not release owner");
        release(&o);check(same(counts(),baseline),"named release restores logical baseline");cursors(detached);++passed;
        std::printf("{\"asset_index\":%u,\"round\":%u,\"alloc_type\":%u,\"disk_sizes\":[",asset,round,type);
        for(unsigned i=0;i<7;++i)std::printf("%s%u",i?",":"",p.file.blockSize[i]);std::printf("],\"logical_sizes\":[");
        for(unsigned i=0;i<7;++i)std::printf("%s%u",i?",":"",p.logical[i]);std::printf("],\"allocated\":");printCounts(allocated);
        std::printf(",\"seven_cursors_pass\":true,\"reset_pass\":true,\"detached_before_release\":true,\"logical_baseline_restored\":true}\n");
    }
    check(initCalls==1+passed*5,"initializer only after successful allocation or explicit detach");
    std::printf("{\"schema\":\"BlackOpsARM.TASK-0012.stream-init.v1\",\"pointer_bytes\":%zu,\"native64\":%s,\"cycles_passed\":%u,\"preflight_rejections\":%u,\"partial_failures\":%u,\"init_calls\":%u,\"baseline\":",sizeof(void*),BO1_NATIVE_64BIT?"true":"false",passed,rejections,failures,initCalls);
    printCounts(baseline);std::printf(",\"final\":");printCounts(counts());std::printf(",\"payload_bytes_read\":0,\"payload_bytes_written\":0}\n");
}
