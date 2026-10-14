#include <platform/android/server_thread.h>
#include <platform/android/thread_services.h>
#include <platform/android/assertion_services.h>
#include <platform/critical_section_services.h>
#include <universal/thread_data.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <new>

extern std::atomic<int> g_databaseStopServer;
namespace {
struct Proof { std::atomic<int> stage{},checks{},released{},lockFree{}; unsigned mainTid{}; };
Proof *proof;
int mode;
std::atomic<unsigned long long> entered{},exited{};
void Check(bool ok,const char *why) {
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);_exit(91);}
    proof->checks.fetch_add(1);
}
void Until(int stage) {
    for(int i=0;i<5000;++i){if(proof->stage.load()>=stage)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    _exit(92);
}
void Reporter(const char *) {
    std::thread observer([] {
        const bool acquired=Sys_TryEnterCriticalSection(CRITSECT_NETTHREAD_OVERRIDE);
        if(acquired)Sys_LeaveCriticalSection(CRITSECT_NETTHREAD_OVERRIDE);
        proof->lockFree.store(acquired);
    });observer.join();
}
void Callback(unsigned role) {
    Check(role==THREAD_CONTEXT_SERVER && Sys_GetThreadContext()==THREAD_CONTEXT_SERVER,"real server role");
    Check(Sys_IsServerThread() && !Sys_IsMainThread(),"actual script server identity");
    Check(Sys_GetCurrentThreadId()!=proof->mainTid,"actual separate gettid");
    Check(Sys_GetValue(1)==&va_info[role] && Sys_GetValue(2)==g_com_error[role]
        && Sys_GetValue(3)==&g_traceThreadInfo[role] && Sys_GetValue(4)==&g_cmd_args[0],"actual canonical native TLS");
    if(mode==2)throw std::runtime_error("bounded callback exception");
    Sys_InitServerEvents();
    Sys_ServerCompleted(); // Completion comes from this real installed callback.
    proof->stage.store(1);
    if(mode==1)for(;;)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    Check(Sys_WaitStartServer(5000),"canonical wake starts frame");
    Check(!Sys_WaitServer(0),"start clears completed manual gate");
    Sys_ResetServerNetworkCompletedEvent();
    Check(g_networkOverrideThread==Sys_GetCurrentThreadId(),"actual network override owner");
    proof->stage.store(2);
    Until(3);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    proof->released.store(1);
    Sys_SetServerNetworkCompletedEvent();
    Until(4);
    Check(Sys_WaitStartServer(0)==false,"reset wake remains clear");
    g_databaseStopServer.store(1);
    Sys_WakeServer();
    Check(!Sys_WaitStartServer(0),"database stop blocks wake admission");
    g_databaseStopServer.store(0);
    Sys_SleepServer();
    Check(!Sys_WaitStartServer(0),"sleep consumes original manual wake");
    // Deliberate return proves terminal retirement/fatal; never a restart claim.
}
void RunChild(int which) {
    mode=which;
    alarm(12);
    if(which==3){Sys_SpawnServerThread(Callback);_exit(93);}
    Sys_InitMainThread();proof->mainTid=Sys_GetCurrentThreadId();
    Check(!Sys_IsServerThread(),"unpublished server identity is false");
    if(which==4){
        bo1::android::SetAssertionReporter(Reporter);
        // Initialize real events; callback remains alive during this fault.
        mode=1;Check(Sys_SpawnServerThread(Callback)==1,"real fault spawn");Until(1);
        Sys_SetServerNetworkCompletedEvent();_exit(94);
    }
    // Test the reused actual owner, including manual/auto coalescing semantics.
    void *manual=nullptr,*automatic=nullptr;
    Sys_CreateEvent(true,false,&manual);Sys_CreateEvent(false,false,&automatic);
    Check(!Sys_WaitForSingleObjectTimeout(&manual,0),"manual initially clear");
    Sys_SetEvent(&manual);Check(Sys_WaitForSingleObjectTimeout(&manual,0) && Sys_WaitForSingleObjectTimeout(&manual,0),"manual remains signaled");
    Sys_ResetEvent(&manual);Check(!Sys_WaitForSingleObjectTimeout(&manual,2),"manual reset and timeout");
    Sys_SetEvent(&automatic);Sys_SetEvent(&automatic);
    Check(Sys_WaitForSingleObjectTimeout(&automatic,0) && !Sys_WaitForSingleObjectTimeout(&automatic,0),"auto signals coalesce and consume once");
    Sys_DestroyEvent(&manual);Sys_DestroyEvent(&automatic);
    Check(Sys_SpawnServerThread(Callback)==1,"actual asynchronous spawn");
    if(which==2)for(;;)pause();
    Until(1);Check(Sys_WaitServer(0) && Sys_WaitServer(0),"callback publishes manual completed");
    if(which==1){Sys_SpawnServerThread(Callback);_exit(95);}
    Sys_ResetServerAllowNetworkEvent();Check(!Sys_WaitForSingleObjectTimeout(&allowServerNetworkEvent,0),"network admission can reset");
    Sys_SetServerAllowNetworkEvent();Check(Sys_WaitForSingleObjectTimeout(&allowServerNetworkEvent,0),"network admission can signal");
    Sys_WakeServer();Until(2);
    Check(!Sys_WaitForSingleObjectTimeout(&serverNetworkCompletedEvent,0),"actual override clears network completion");
    proof->stage.store(3);
    Sys_InitServerEvents(); // Waits for the actual server to release override.
    Check(proof->released.load()==1 && g_networkOverrideThread==0,"reinitialization waited for override release");
    Check(!Sys_WaitServer(0) && !Sys_WaitForSingleObjectTimeout(&wakeServerEvent,0),"reinitialization clears wake/completed");
    Check(Sys_WaitForSingleObjectTimeout(&allowServerNetworkEvent,0) && Sys_WaitForSingleObjectTimeout(&serverNetworkCompletedEvent,0),"reinitialization publishes network gates");
    proof->stage.store(4);
    for(;;)pause(); // Actual entry's terminal reporter exits the whole child.
}
}
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
int main() {
    proof=new(mmap(nullptr,sizeof(Proof),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0))Proof;
    unsigned total=0;
    for(int which=0;which<5;++which) {
        proof->stage.store(0);proof->checks.store(0);proof->released.store(0);proof->lockFree.store(0);
        const pid_t child=fork();if(child<0)return 90;if(!child)RunChild(which);
        int status{};if(waitpid(child,&status,0)!=child || !WIFEXITED(status) || WEXITSTATUS(status)!=2)return 96;
        if(which==4 && !proof->lockFree.load())return 97;
        total+=proof->checks.load();
    }
    std::printf("PASS: %u actual native server event/thread lifecycle checks; five expected fatal children\n",total);
}