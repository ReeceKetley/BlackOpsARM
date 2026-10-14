#include <platform/android/cpu_profile.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <unistd.h>
using namespace bo1::android;
static void check(bool ok,const char *what){if(!ok){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}std::printf("PASS %s\n",what);}
int main(){
    std::atomic<unsigned> tid{};std::atomic<bool> burn{},done{},stop{};
    std::thread worker([&]{
        pthread_setname_np(pthread_self(),"cpu-owner-test");tid.store(gettid());
        while(!burn.load())std::this_thread::yield();
        const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(150);
        volatile unsigned long long value=1;
        while(std::chrono::steady_clock::now()<end)value=value*1664525+1013904223;
        done=true;while(!stop.load())std::this_thread::yield();
    });
    while(!tid.load())std::this_thread::yield();
    unsigned ids[256];std::size_t count=0;
    check(EnumerateCpuThreads(ids,256,&count),"actual process task enumeration");
    bool found=false;for(std::size_t i=0;i<count;++i)found|=ids[i]==tid;
    check(found,"snapshot contains real worker");
    auto *owner=OpenCpuThread(tid);check(owner!=nullptr,"owned live task descriptor");
    char name[40];check(CpuThreadName(owner,name,sizeof(name)) && !strcmp(name,"cpu-owner-test"),"actual kernel task name");
    std::uint64_t before=0,after=0,p0=0,p1=0,current=0;
    check(ReadCpuThread(owner,&before) && ProcessCpuNanoseconds(&p0) && CurrentCpuNanoseconds(&current),"real task process and current CPU clocks");
    burn=true;while(!done.load())std::this_thread::yield();
    check(ReadCpuThread(owner,&after) && ProcessCpuNanoseconds(&p1) && after>before && p1>p0,"CPU work produces monotonic nonzero measured deltas");
    stop=true;worker.join();after=123;
    check(!ReadCpuThread(owner,&after) && after==123,"exited task rejects sampling without publishing fabricated zero");
    CloseCpuThread(owner);
    check(!OpenCpuThread(0),"invalid task identity rejected");
    std::puts("PASS native CPU profile ownership and timing");
}
