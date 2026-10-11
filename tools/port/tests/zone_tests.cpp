#include <database/db_zone_types.h>
#include <database/db_memory.h>
#include <database/db_stream.h>
#include <platform/android/zone_memory.h>
#include <platform/android/zone_heap.h>
#include <platform/fastfile_reader.h>
#include <universal/physicalmemory.h>
#include "hunk_main_thread_test.h"
#include "hunk_test_checks.h"
#include <cstring>
#include <thread>
#if defined(__ANDROID__)
#include <platform/android/virtual_memory.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
static void aborts(const char *exe, const char *scenario) {
    pid_t p=fork(); CHECK(p>=0);
    if(!p){execl(exe,exe,scenario,static_cast<char *>(nullptr));_exit(99);}
    int s; CHECK(waitpid(p,&s,0)==p); CHECK(WIFSIGNALED(s)&&WTERMSIG(s)==SIGABRT);
}
#endif
static unsigned expanded(unsigned n)
{
#if BO1_NATIVE_64BIT
    return n ? static_cast<unsigned>(static_cast<unsigned long long>(n) * (n>=0x2000000 ? 135 : n>=0x100000 ? 190 : 200)/100)+65536 : 0;
#else
    return n;
#endif
}
static void init(XBlock *blocks)
{
    DB_InitStreams(blocks);
    CHECK(g_streamBlocks==blocks && g_streamPos==blocks[0].data);
    CHECK(g_streamPosIndex==0 && g_streamDelayIndex==0 && g_streamPosStackIndex==0 && g_streamPosArray[0]==nullptr);
    for(unsigned i=1;i<7;++i) CHECK(g_streamPosArray[i]==blocks[i].data);
#if BO1_NATIVE_64BIT
    for(unsigned i=0;i<7;++i) CHECK(DB_StreamWin32Pos(i)==0 && DB_StreamAliasSlotLookup(i,4)==nullptr && DB_StreamExactLookup(i,4)==nullptr);
#endif
}
int main(int argc, char **argv)
{
    using namespace bo1::android;
    RegisterHunkTestMainThread();
#if defined(__ANDROID__)
    if(argc==2) {
        const char *s=argv[1];
        if(!std::strcmp(s,"end")) ZoneMemoryEnd();
        else if(!std::strcmp(s,"unknown")) ZoneMemoryRelease("unknown",0);
        else if(!std::strcmp(s,"outside")) _PMem_AllocNamed(1,4096,4,0,"no-zone",TRACK_FASTFILE);
        else if(!std::strcmp(s,"type")) ZoneMemoryBegin("bad-type",2,65536);
        else if(!std::strcmp(s,"open")) {ZoneMemoryBegin("open",0,65536);ZoneMemoryRelease("open",0);}
        else if(!std::strcmp(s,"align")) {ZoneMemoryBegin("bad-align",0,65536);_PMem_AllocNamed(1,3,4,0,"bad-align",TRACK_FASTFILE);}
        else if(!std::strcmp(s,"full")) {ZoneMemoryBegin("full",0,65536);unsigned sizes[7]={65536,0,0,0,0,0,0};XBlock b[7]{};DB_AllocXBlocks(sizes,"full",b,0,0);}
        else if(!std::strcmp(s,"worker")) {std::thread worker([]{ZoneMemoryBegin("worker",0,65536);});worker.join();}
        return 99;
    }
    const auto reservations=VirtualReservationCount();
#else
    (void)argc; (void)argv;
#endif
    CHECK(sizeof(XFile)==36 && sizeof(Bo1FastFilePreamble)==48);
    // Logical decompressed fixture, not raw bytes from a genuine .ff.
    Bo1FastFilePreamble fixture{}; std::memcpy(fixture.magic,BO1_FF_MAGIC_UNSIGNED,8);
    fixture.version=473; fixture.size=0x12345678;
    for(unsigned i=0;i<7;++i) fixture.blockSize[i]=4097+i*1003;
    XFile file{}; std::memcpy(&file,&fixture.size,sizeof(file));
    const XFile disk=file;
    XBlock blocks[7]{};
    ZoneMemoryBegin("fixture-all-seven",0,2*1024*1024);
    DB_AllocXBlocks(file.blockSize,"fixture-all-seven",blocks,0,0);
    CHECK(std::memcmp(&file,&disk,sizeof(file))==0 && ZoneMemoryAllocationCount()==7);
    std::size_t bytes=0;
    for(unsigned i=0;i<7;++i) {
        CHECK(blocks[i].size==expanded(file.blockSize[i]) && blocks[i].data);
        CHECK(reinterpret_cast<std::uintptr_t>(blocks[i].data)%4096==0);
#if defined(__ANDROID__)
        CHECK(reinterpret_cast<std::uintptr_t>(blocks[i].data)>UINT32_MAX);
#endif
        bytes+=blocks[i].size+(i==3||i==6 ? 0 : 15);
        for(unsigned j=0;j<blocks[i].size;++j) CHECK(blocks[i].data[j]==0);
        blocks[i].data[blocks[i].size-1]=static_cast<unsigned char>(i+1);
    }
    CHECK(ZoneMemoryRequestedBytes()==bytes); ZoneMemoryEnd();
    init(blocks);
    DB_IncStreamPos(3); CHECK(DB_GetStreamPos()==blocks[0].data+3);
    for(unsigned i=1;i<7;++i) {
        DB_PushStreamPos(i); CHECK(DB_GetStreamPos()==blocks[i].data && g_streamPosStackIndex==1);
        auto *aligned=DB_AllocStreamPos(3);
        CHECK(reinterpret_cast<std::uintptr_t>(aligned)%sizeof(void*)==0);
        DB_IncStreamPos(17); DB_PopStreamPos();
        CHECK(g_streamPosIndex==0 && DB_GetStreamPos()==blocks[0].data+3 && g_streamPosStackIndex==0);
        CHECK(g_streamPosArray[i]==aligned+17 && blocks[i].data[blocks[i].size-1]==i+1);
    }
    const void **alias=DB_InsertPointer(); *alias=blocks[6].data;
    CHECK(*alias==blocks[6].data && g_streamPosIndex==0 && g_streamPosStackIndex==0);
#if BO1_NATIVE_64BIT
    DB_StreamRegisterAliasSlot(4,4,reinterpret_cast<std::uintptr_t *>(const_cast<void **>(alias)));
    CHECK(DB_StreamAliasSlotLookup(4,4));
#endif
#if BO1_NATIVE_64BIT
    DB_SetExpandElem(4,8); DB_SetExpandWin32Size(4); // Pending state from the preceding zone.
#endif
    init(blocks); // Native drift/alias/segment state resets, no old-zone pointers.
#if BO1_NATIVE_64BIT
    DB_SetExpandWin32Size(8); DB_IncStreamPos(16);
    CHECK(DB_StreamExactLookup(0,0)==blocks[0].data);
    CHECK(DB_StreamExactLookup(0,4)==nullptr); // No stale 4->8 element stride.
#endif
    XBlock empty[7]{}; init(empty); // Detach globals before freeing ownership.
    DB_FreeXBlocks(blocks,0);
    for(const auto &block:blocks) CHECK(!block.data && !block.size);
    CHECK(ZoneMemoryCount()==1); ZoneMemoryRelease("fixture-all-seven",0);
    CHECK(ZoneMemoryCount()==0 && ZoneMemoryAllocationCount()==0 && ZoneMemoryRequestedBytes()==0);
    unsigned zeros[7]{}; DB_AllocXBlocks(zeros,"all-zero",blocks,0,0);
    CHECK(ZoneMemoryCount()==0); for(const auto &block:blocks) CHECK(!block.data && !block.size);
    // Zero and sparse blocks plus high allocation-type metadata.
    unsigned sparse[7]={1,0,37,0,0,0,0};
    ZoneMemoryBegin("fixture-sparse",1,1024*1024);
    DB_AllocXBlocks(sparse,"fixture-sparse",blocks,1,0); ZoneMemoryEnd();
    CHECK(blocks[0].size==expanded(1) && blocks[2].size==expanded(37));
    for(unsigned i:{1u,3u,4u,5u,6u}) CHECK(!blocks[i].data && !blocks[i].size);
    init(blocks); init(empty); DB_FreeXBlocks(blocks,0); ZoneMemoryRelease("fixture-sparse",1);
    // Actual alternate Z branch; physical PMem blocks must be zero in this mode.
    DB_AllocXBlocks(sparse,"fixture-z",blocks,0,0x40000000);
    CHECK(ZoneAllocationCount()==2); init(blocks); init(empty);
    DB_FreeXBlocks(blocks,0x40000000); CHECK(ZoneAllocationCount()==0 && ZoneAllocatedBytes()==0);
    // Existing native expansion factor transitions; no huge disk fixture is allocated.
    for(unsigned size:{0xfffffu,0x100000u,0x1ffffffu,0x2000000u}) {
        unsigned sizes[7]={size,0,0,0,0,0,0};
        ZoneMemoryBegin("factor-boundary",0,70*1024*1024);
        DB_AllocXBlocks(sizes,"factor-boundary",blocks,0,0); ZoneMemoryEnd();
        CHECK(blocks[0].size==expanded(size)); init(blocks); init(empty);
        DB_FreeXBlocks(blocks,0); ZoneMemoryRelease("factor-boundary",0);
    }
#if defined(__ANDROID__)
    for(const char *scenario:{"end","unknown","outside","type","open","align","full","worker"}) aborts(argv[0],scenario);
    CHECK(VirtualReservationCount()==reservations);
    Bo1_Diag("Native zone gate diagnostic"); Bo1_DiagStack("zone gate");
#endif
    CHECK(ZoneMemoryCount()==0 && ZoneMemoryAllocationCount()==0 && ZoneMemoryRequestedBytes()==0);
    std::puts("PASS actual zone bootstrap: 7 blocks/all memory types, unchanged 36-byte XFile, native size factors, zero/sparse/Z paths, aligned pointers, real first-fit named lifetime, stream init/cursors/alias reset, explicit invalid ownership failures");
}
