#include <platform/android/scheduling_services.h>
#include <platform/android/thread_services.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>

// Model a kernel cpuset restriction at the syscall boundary. TLS ownership,
// affinity reads, the underlying successful affinity write and thread exit
// remain real. No production scheduling hooks or fake readbacks are used.
static std::atomic<pid_t> restrictedTid{};
static std::atomic<unsigned> requests{};
static bool restrictAffinity,rejectAffinity;
static cpu_set_t permitted;
static std::atomic<unsigned> profileEntered{},profileExited{};
extern "C" void __cyg_profile_func_enter(void *,void *){profileEntered.fetch_add(1);}
extern "C" void __cyg_profile_func_exit(void *,void *){profileExited.fetch_add(1);}
extern "C" int __real_sched_setaffinity(pid_t,size_t,const cpu_set_t *);
extern "C" int __wrap_sched_setaffinity(pid_t tid,size_t size,const cpu_set_t *wanted) {
    if(restrictedTid.load()==(tid?tid:gettid())) {
        requests.fetch_add(1);
        if(rejectAffinity){errno=EPERM;return -1;}
        if(restrictAffinity){cpu_set_t selected{};CPU_AND(&selected,wanted,&permitted);return __real_sched_setaffinity(tid,sizeof(selected),&selected);}
    }
    return __real_sched_setaffinity(tid,size,wanted);
}
static void Check(bool value,const char *reason){if(!value){std::fprintf(stderr,"FAIL: %s\n",reason);std::_Exit(91);}}
int main(int argc,char **argv) {
    alarm(15);
    restrictAffinity=argc==2&&!std::strcmp(argv[1],"--restrict-affinity");
    rejectAffinity=argc==2&&!std::strcmp(argv[1],"--reject-affinity");
    Sys_InitMainThread();
    if(argc==2&&!std::strcmp(argv[1],"--wrong-owner")) {
        bo1::android::RetireSchedulingThread(THREAD_CONTEXT_SERVER);
        return 92;
    }
    std::atomic<bool> ready{},retire{};
    std::thread worker([&]{
        Sys_InitThread(THREAD_CONTEXT_SERVER);ready.store(true);
        while(!retire.load())std::this_thread::yield();
        cpu_set_t effective{};Check(!sched_getaffinity(0,sizeof(effective),&effective),"real effective affinity read");
        CPU_ZERO(&permitted);for(int cpu=0;cpu<CPU_SETSIZE;++cpu)if(CPU_ISSET(cpu,&effective)){CPU_SET(cpu,&permitted);break;}
        if(restrictAffinity||rejectAffinity){
            Check(!__real_sched_setaffinity(0,sizeof(permitted),&permitted),"real engine pin before retirement");
            restrictedTid.store(gettid());
        }
        bo1::android::RetireThreadRole();
        if(restrictAffinity){cpu_set_t actual{};Check(!sched_getaffinity(0,sizeof(actual),&actual)&&CPU_EQUAL(&actual,&permitted),"kernel-confirmed restricted mask until thread exit");}
    });
    while(!ready.load())std::this_thread::yield();
    bo1::android::SchedulingState before{};Check(bo1::android::ReadSchedulingState(THREAD_CONTEXT_SERVER,&before),"real registered owner");
    if(restrictAffinity||rejectAffinity)Check(CPU_COUNT(&before.inheritedAffinity)>1,"restriction requires multi-CPU inheritance");
    retire.store(true);worker.join();
    bo1::android::SchedulingState after{};Check(!bo1::android::ReadSchedulingState(THREAD_CONTEXT_SERVER,&after),"joined owner has no stale scheduling slot");
    Check(!restrictAffinity||requests.load()==1,"one successful inherited-mask restore, no old engine-pin rollback");
    Check(profileEntered.load()&&profileExited.load(),"canonical instrumented thread initialization executed");
    std::printf("PASS: real scheduling owner retired and joined; restricted=%i restoreRequests=%u\n",restrictAffinity,requests.load());
}
