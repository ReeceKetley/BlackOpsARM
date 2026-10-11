#pragma once
// Standalone fixture infrastructure, never a shipping runtime replacement.
// Uses actual Windows PMem or native named Hunk ownership and production DB_MemAlloc.
#include <platform/fastfile_zone.h>
#include <database/db_memory.h>
#include <database/db_stream.h>
#include <universal/physicalmemory.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
#if defined(__ANDROID__)
#include <platform/android/zone_memory.h>
#include <platform/android/hunk_services.h>
#include <platform/android/virtual_memory.h>
#else
#include <Windows.h>
extern int freeTot;
#endif
namespace frontend_fixture {
inline void require(bool v,const char*why){if(!v){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
struct Counts{std::uint64_t owners,count,bytes,reservations,low,high;
    bool operator==(const Counts&b)const{return owners==b.owners&&count==b.count&&bytes==b.bytes&&reservations==b.reservations&&low==b.low&&high==b.high;}};
inline Counts counts(){
#if defined(__ANDROID__)
    return {bo1::android::ZoneMemoryCount(),bo1::android::ZoneMemoryAllocationCount(),bo1::android::ZoneMemoryRequestedBytes(),bo1::android::VirtualReservationCount(),0,0};
#else
    return {g_mem.prim[0].allocListCount+g_mem.prim[1].allocListCount,0,0,g_mem.buf?1u:0u,g_mem.prim[0].pos,g_mem.prim[1].pos};
#endif
}
struct StreamState{XBlock*blocks;unsigned char*pos;unsigned index,delay,stack;
    std::array<unsigned char*,7>cursors{};std::array<unsigned,7>shadow{};std::array<uintptr_t*,2>aliases{};
    bool operator==(const StreamState&b)const{return blocks==b.blocks&&pos==b.pos&&index==b.index&&delay==b.delay&&stack==b.stack&&cursors==b.cursors&&shadow==b.shadow&&aliases==b.aliases;}};
inline StreamState streamState(){StreamState s{g_streamBlocks,g_streamPos,g_streamPosIndex,g_streamDelayIndex,g_streamPosStackIndex};
    for(unsigned i=0;i<7;++i){s.cursors[i]=g_streamPosArray[i];
#if BO1_NATIVE_64BIT
        s.shadow[i]=DB_StreamWin32Pos(i);
#endif
    }
#if BO1_NATIVE_64BIT
    s.aliases={DB_StreamAliasSlotLookup(4,0),DB_StreamAliasSlotLookup(4,4)};
#endif
    return s;}
inline std::size_t page(){
#if defined(__ANDROID__)
    return bo1::android::VirtualPageSize();
#else
    SYSTEM_INFO s{};GetSystemInfo(&s);return s.dwPageSize;
#endif
}
inline bo1::fastfile::ZoneLimits limits(unsigned type=0){std::uint64_t pool=0x14800000u;
#if BO1_NATIVE_64BIT
    pool+=pool/2;
#endif
    return {pool,INT32_MAX-8192u,4096,type,0};}
inline void startup(){
#if defined(__ANDROID__)
    bo1::android::RegisterHunkMainThread();
#else
    PMem_Init();
#endif
}
struct Owner{const char*name="frontend-genuine-zone";unsigned type{};};
inline bool begin(void*v,const bo1::fastfile::ZonePlan&p){auto&c=*static_cast<Owner*>(v);
#if defined(__ANDROID__)
    if(bo1::android::ZoneMemoryHasOpenOwner()||bo1::android::ZoneMemoryContains(c.name,c.type))return false;
    bo1::android::ZoneMemoryBegin(c.name,c.type,static_cast<int>(p.firstFitCapacity));
#else
    if(g_mem.prim[c.type].allocName)return false;
    for(const auto&prim:g_mem.prim)for(unsigned i=0;i<prim.allocListCount;++i)if(prim.allocList[i].name&&!std::strcmp(prim.allocList[i].name,c.name))return false;
    PMem_BeginAlloc(c.name,c.type,TRACK_FASTFILE);
#endif
    return true;}
inline unsigned char*allocate(void*v,unsigned n,unsigned i){auto&c=*static_cast<Owner*>(v);return DB_MemAlloc(n,(i==3||i==6)?2u:(i==0?0u:1u),c.type,c.name,0);}
inline void end(void*v){
#if defined(__ANDROID__)
    (void)v;bo1::android::ZoneMemoryEnd();
#else
    auto&c=*static_cast<Owner*>(v);PMem_EndAlloc(c.name,c.type);
#endif
}
inline void release(void*v){auto&c=*static_cast<Owner*>(v);
#if defined(__ANDROID__)
    bo1::android::ZoneMemoryRelease(c.name,c.type);
#else
    ::freeTot=0; // Only reset the legacy cumulative diagnostic, never ownership.
    PMem_Free(c.name);
#endif
}
inline bo1::fastfile::ZoneBackend backend(Owner&c){return {&c,begin,allocate,end,release};}
}
