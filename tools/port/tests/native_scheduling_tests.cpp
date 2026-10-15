#include <platform/android/scheduling_services.h>
#include <platform/android/thread_services.h>
#include <universal/thread_data.h>
#include <atomic>
#include <array>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
std::atomic<unsigned> checks{},ready{};
std::atomic<bool> finish{};
std::atomic<unsigned long long> entered{},exited{};
constexpr std::array<ThreadContext_t,4> roles={THREAD_CONTEXT_SERVER,THREAD_CONTEXT_BACKEND,THREAD_CONTEXT_WORKER0,THREAD_CONTEXT_WORKER1};
void Check(bool ok,const char *reason){if(!ok){std::fprintf(stderr,"FAIL: %s\n",reason);_exit(91);}checks.fetch_add(1);}
void WaitReady(){for(int i=0;i<5000;++i){if(ready.load()==4)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}_exit(92);}
bo1::android::SchedulingState Observe(ThreadContext_t role){
    bo1::android::SchedulingState state{};Check(bo1::android::ReadSchedulingState(role,&state),"actual live scheduling role");
    // The kernel's effective mask is the requested mask limited to active
    // (non-paused) CPUs; it must be a non-empty subset of the observed request.
    cpu_set_t kernel{},outside{};Check(!sched_getaffinity(state.tid,sizeof(kernel),&kernel) && CPU_COUNT(&kernel),"actual effective kernel affinity read");
    CPU_XOR(&outside,&kernel,&state.currentAffinity);CPU_AND(&outside,&outside,&kernel);
    Check(!CPU_COUNT(&outside),"effective kernel affinity lies within reported requested affinity");
    errno=0;const int nice=getpriority(PRIO_PROCESS,state.tid);Check(!errno && nice==state.currentNice,"reported nice equals actual kernel state");return state;
}
void Worker(ThreadContext_t role){
    Sys_InitThread(role);Check(Sys_GetThreadContext()==role && Sys_GetValue(2)==g_com_error[role],"real TLS owner admitted scheduling slot");
    ready.fetch_add(1);while(!finish.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));
    bo1::android::RetireThreadRole();
}
void ExpectedForkFailure(){const pid_t child=fork();Check(child>=0,"foreign process control fork");if(!child){Sys_ModApplyPriority(2);_exit(93);}int status{};Check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==2,"inherited foreign process cannot target parent's live TIDs");}
}
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
int main(int argc,char **argv){
    alarm(15);bool pin2=argc==2 && !std::strcmp(argv[1],"--pin2");
    const bool denyNice=argc==2 && !std::strcmp(argv[1],"--deny-nice");
    if(denyNice){rlimit limit{};Check(!getrlimit(RLIMIT_NICE,&limit),"actual priority resource limit");limit.rlim_cur=0;Check(!setrlimit(RLIMIT_NICE,&limit),"actual permission constraint in isolated process");}
    if(pin2){cpu_set_t allowed{},selected{};Check(!sched_getaffinity(0,sizeof(allowed),&allowed),"actual inherited cpuset read");CPU_ZERO(&selected);int count=0;for(int cpu=0;cpu<CPU_SETSIZE && count<2;++cpu)if(CPU_ISSET(cpu,&allowed)){CPU_SET(cpu,&selected);++count;}
        Check(count==2 && !sched_setaffinity(0,sizeof(selected),&selected),"genuine two-CPU fixture inheritance");}
    Sys_InitMainThread();ExpectedForkFailure();
    const auto initial=Observe(THREAD_CONTEXT_MAIN);Check(!initial.affinityChanged && !initial.niceChanged,"actual inherited defaults captured");
    if(denyNice){
        Check(initial.inheritedNice>-20,"actual inherited nice has room for a boost");
        Sys_ModApplyPriority(1);const auto denied=Observe(THREAD_CONTEXT_MAIN);
        Check((denied.lastError==EACCES || denied.lastError==EPERM) && !denied.niceChanged && denied.currentNice==denied.inheritedNice,"real denied request is not falsely marked applied");
        Sys_ModApplyPriority(1);const auto cached=Observe(THREAD_CONTEXT_MAIN);Check(cached.lastError==denied.lastError && cached.currentNice==denied.currentNice,"failed-request cache preserves error and actual state");
        Sys_ModApplyPriority(0);const auto reset=Observe(THREAD_CONTEXT_MAIN);Check(!reset.lastError && !reset.niceChanged,"mode0 retains genuine inherited priority after denial");
        std::printf("PASS: %u actual priority-denial checks with lowered RLIMIT_NICE\n",checks.load());return 0;
    }
    std::array<std::thread,4> workers;for(unsigned i=0;i<roles.size();++i)workers[i]=std::thread(Worker,roles[i]);WaitReady();
    if(pin2){
        Win_LockThreadAffinity();Win_LockThreadAffinity();
        const auto main=Observe(THREAD_CONTEXT_MAIN),backend=Observe(THREAD_CONTEXT_BACKEND);
        Check(CPU_COUNT(&main.currentAffinity)==1 && CPU_COUNT(&backend.currentAffinity)==1 && !CPU_EQUAL(&main.currentAffinity,&backend.currentAffinity),"original two-CPU main/backend lock mapping actually applied");
        Win_UnlockThreadAffinity();const auto nested=Observe(THREAD_CONTEXT_MAIN);Check(CPU_COUNT(&nested.currentAffinity)==1,"nested lock remains until matching final unlock");
        Win_UnlockThreadAffinity();Check(!Observe(THREAD_CONTEXT_MAIN).affinityChanged && !Observe(THREAD_CONTEXT_BACKEND).affinityChanged,"final unlock restores actual inherited two-CPU masks");
    }else{
        Sys_ModApplyPCore(1);const auto server=Observe(THREAD_CONTEXT_SERVER);
        Check(!Observe(THREAD_CONTEXT_MAIN).affinityChanged && !Observe(THREAD_CONTEXT_BACKEND).affinityChanged,"server-only pcore mode preserves other inherited owners");
        Sys_ModApplyPCore(2);const auto main=Observe(THREAD_CONTEXT_MAIN);
        if(main.affinityChanged){Check(CPU_COUNT(&main.currentAffinity)<CPU_COUNT(&main.inheritedAffinity) && Sys_ModOnPCore()==1,"actual allowed fastest-capacity placement observed on main");}
        else Check(Sys_ModOnPCore()>=-1 && Sys_ModOnPCore()<=1,"truthful actual/unknown topology observation");
        for(auto role:roles)(void)Observe(role);
        Sys_ModApplyPriority(1);const auto boosted=Observe(THREAD_CONTEXT_MAIN);
        std::printf("ACTUAL main inherited nice=%d boosted nice=%d error=%d fastest-affinity-changed=%d\n",boosted.inheritedNice,boosted.currentNice,boosted.lastError,boosted.affinityChanged);
        Check(boosted.currentNice<=boosted.inheritedNice,"real priority request cannot report a lower nice priority");
        if(!boosted.lastError && boosted.inheritedNice>-20)Check(boosted.currentNice<boosted.inheritedNice,"kernel confirmed successful boost changes actual nice");
        if(boosted.lastError==EACCES || boosted.lastError==EPERM)Check(boosted.currentNice==boosted.inheritedNice,"denied privilege retains actual inherited nice");
        Sys_ModApplyPriority(1);Check(Observe(THREAD_CONTEXT_MAIN).currentNice==boosted.currentNice,"cached failed/successful request retains truthful actual state");
        Sys_ModApplyPriority(0);Sys_ModApplyPCore(0);
        const auto restored=Observe(THREAD_CONTEXT_MAIN);Check(!restored.affinityChanged && !restored.niceChanged,"mode0 restores actual inherited defaults");
        for(auto role:roles){const auto state=Observe(role);Check(!state.affinityChanged && !state.niceChanged,"worker inherited defaults restored");}
        Win_LockThreadAffinity();Win_UnlockThreadAffinity();Check(!Observe(THREAD_CONTEXT_MAIN).affinityChanged,"original large-cpuset profiling policy restores defaults");
        (void)server;
    }
    finish.store(true);for(auto &worker:workers)worker.join();
    for(auto role:roles){bo1::android::SchedulingState state{};Check(!bo1::android::ReadSchedulingState(role,&state),"retired/joined role cannot expose stale TID");}
    std::printf("PASS: %u actual native scheduling ownership/affinity/priority checks%s\n",checks.load(),pin2?" (two-CPU inherited cpuset)":"");
}
