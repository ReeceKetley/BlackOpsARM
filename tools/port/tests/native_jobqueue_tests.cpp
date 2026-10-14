#include <platform/startup_services_api.h>
#include <platform/android/tl_memory.h>
#include <platform/android/command_line_services.h>
#include <tl/tl_system.h>
#include <tl/jobqueue/jobqueue_all.h>
#include <universal/com_workercmds.h>
#include <qcommon/threads.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" void __cyg_profile_func_enter(void *, void *) {}
extern "C" void __cyg_profile_func_exit(void *, void *) {}
void check(bool good,const char *name){if(!good){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}std::printf("PASS %s\n",name);}
std::atomic<unsigned> total{},background{},initialized{};
std::atomic<bool> valid{true};
unsigned freedA{},freedB{};
void *allocateA(unsigned size,unsigned alignment,unsigned){void *pointer=nullptr;return posix_memalign(&pointer,alignment,size)?nullptr:pointer;}
void freeA(void *pointer){++freedA;std::free(pointer);}
void freeB(void *pointer){++freedB;std::free(pointer);}
struct Payload {unsigned value;unsigned char data[128];};
void initialize(int id){if(Sys_GetThreadContext()!=id+1)valid=false;initialized.fetch_add(1);}
int consume(jqBatch *batch) {
    auto *data=static_cast<Payload *>(jqLockData(batch));
    for(auto byte:data->data)if(byte!=static_cast<unsigned char>(data->value))valid=false;
    total.fetch_add(data->value);
    if(!Sys_IsMainThread())background.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    jqUnlockData(batch);return 0;
}
int main(int argc,char **argv) {
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(20));std::_Exit(2);}).detach();
    if(argc!=1)return 3;
    bo1::android::InstallEngineCommandLine(argv[0],"+set bo1_headless 1");
    Sys_InitializeCriticalSections();Sys_InitMainThread();
    const auto baseline=bo1::android::TLAllocationCount();
    void *memory=tlMemAlloc(64,128,0);check(memory&&reinterpret_cast<std::uintptr_t>(memory)%128==0,"actual aligned TL allocation");
    std::memset(memory,0xa5,64);memory=tlMemRealloc(memory,128,128,0);
    check(memory&&static_cast<unsigned char *>(memory)[63]==0xa5,"owned aligned realloc preserves data");tlMemFree(memory);
    check(bo1::android::TLAllocationCount()==baseline,"TL allocation ownership returns to baseline");
    tlSystemCallbacks providerA{},providerB{};
    providerA.MemAlloc=allocateA;providerA.MemFree=freeA;
    providerB.MemAlloc=allocateA;providerB.MemFree=freeB;
    tlSetSystemCallbacks(&providerA);memory=tlMemAlloc(64,128,0);std::memset(memory,0x7e,64);
    tlSetSystemCallbacks(&providerB);memory=tlMemRealloc(memory,128,128,0);
    check(memory&&static_cast<unsigned char *>(memory)[63]==0x7e,"realloc retains original provider after callback change");
    tlMemFree(memory);check(freedA==2&&freedB==0,"original allocation provider performs both frees");
    tlSystemCallbacks empty{};tlSetSystemCallbacks(&empty);
    jqInit();jqSetBatchDataHeapSize(65536,64);jqSetWorkerInitFunction(initialize);
    jqModule module{"native_owned_data",JQ_WORKER_GENERIC,consume};
    jqWorkerCmd command{&module,sizeof(Payload),0,0,nullptr,nullptr,0};
    for(int cycle=0;cycle<2;++cycle) {
        jqEnableWorkers(0x0d);jqStart();
        check(jqGetNumWorkers()==3,"real sparse processor worker configuration");
        Sys_InitWorkerThreadContext();
        check(jqWorkers[1].ThreadId && jqWorkers[2].ThreadId
            && jqWorkers[1].ThreadId!=jqWorkers[2].ThreadId,"renderer worker contexts have distinct live native owners");
        for(unsigned i=1;i<=24;++i){Payload data{i};std::memset(data.data,static_cast<unsigned char>(i),sizeof(data.data));Sys_AddWorkerCmdInternal(&command,reinterpret_cast<unsigned char *>(&data),nullptr);}
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!background && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
        check(background>0,"actual background worker execution");
        Sys_WaitWorkerCmdInternal(&command);
        check(!jqPoll(&module.Group)&&!jqPoll(nullptr),"actual completion and distinct queue counts drained");
        check(jqGetBatchDataAvailable()==65536,"out of line worker data freed by real heap");
        jqStop();check(jqGetNumWorkers()==0,"workers retired and joined");
    }
    check(valid&&initialized==4&&total==600,"native TLS and exact copied job payloads across restart");
    jqShutdown();check(bo1::android::TLAllocationCount()==baseline,"queue nodes heap workers and bitmaps released");
}
