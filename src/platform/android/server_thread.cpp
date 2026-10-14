#include "server_thread.h"
#include "thread_services.h"
#include "script_string_services.h"
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <platform/critical_section_services.h>

void Sys_Error(char *,...);
void *wakeServerEvent;
void *allowServerNetworkEvent;
void *serverNetworkCompletedEvent;
unsigned int g_networkOverrideThread;

namespace {
enum class State { Unspawned, Creating, Running, Terminal };
std::atomic<State> state{State::Unspawned};
std::atomic<bool> terminalRequested{},terminalParked{};
std::mutex parkMutex;
std::condition_variable parkCondition;
// Events and the real endless callback are process-owned. Never destroy or
// recycle them while the server loop can publish completion or await network.
void (*callback)(unsigned int);
[[noreturn]] void fail(const char *reason,int error=0) {
    Sys_Error(const_cast<char *>("Native server thread: %s (%d)"),reason,error);
    std::abort(); // Fatal reporter must never permit failed publication.
}
void *entry(void *) {
    Sys_InitThread(THREAD_CONTEXT_SERVER); // Real va/error/trace/command TLS.
    bo1::script::RegisterServerThread();
    const int named=pthread_setname_np(pthread_self(),"SERVER");
    if(named)fail("thread naming",named);
    (void)Sys_GetCurrentThreadId();
    state.store(State::Running,std::memory_order_release);
    try { callback(THREAD_CONTEXT_SERVER); }
    catch (...) {
        bo1::android::RetireThreadRole();
        state.store(State::Terminal,std::memory_order_release);
        fail("server callback threw");
    }
    bo1::android::RetireThreadRole();
    state.store(State::Terminal,std::memory_order_release);
    fail("original server callback unexpectedly returned");
}
}
unsigned int Sys_GetCurrentThreadId() {
    const long tid=syscall(SYS_gettid);
    if(tid<=0 || static_cast<unsigned long>(tid)>UINT_MAX)fail("gettid",errno);
    return static_cast<unsigned int>(tid);
}
char Sys_SpawnServerThread(void (*function)(unsigned int)) {
    if(!Sys_IsMainThread() || !function || terminalRequested.load(std::memory_order_acquire))fail("invalid spawn owner/callback or sealed terminal admission");
    State expected=State::Unspawned;
    if(!state.compare_exchange_strong(expected,State::Creating,std::memory_order_acq_rel))
        fail("duplicate/restarted server spawn");
    Sys_CreateEvent(true,false,&wakeServerEvent);
    Sys_CreateEvent(true,false,&serverCompletedEvent);
    Sys_CreateEvent(true,true,&allowServerNetworkEvent);
    Sys_CreateEvent(true,true,&serverNetworkCompletedEvent);
    callback=function;
    pthread_attr_t attributes;
    int error=pthread_attr_init(&attributes);
    if(error)fail("thread attributes",error);
    error=pthread_attr_setdetachstate(&attributes,PTHREAD_CREATE_DETACHED);
    if(error) { pthread_attr_destroy(&attributes); fail("detached process owner",error); }
    pthread_t worker;
    error=pthread_create(&worker,&attributes,entry,nullptr);
    const int destroyed=pthread_attr_destroy(&attributes);
    if(error)fail("thread creation",error);
    if(destroyed)fail("thread attribute retirement",destroyed);
    // Match Windows resume semantics: callback starts asynchronously and only
    // its real protocol may signal ServerCompleted. This does not certify TLS
    // initialization or map readiness at the moment this call returns.
    return 1;
}
void bo1::android::ServerTerminalCheckpoint() {
    if(!terminalRequested.load(std::memory_order_acquire))return;
    if(!Sys_IsServerThread())fail("terminal checkpoint requires actual server owner");
    // No synthetic completion: this checkpoint is only the next loop boundary
    // reached after the canonical callback has published real completion.
    if(!Sys_WaitServer(0))fail("terminal checkpoint before actual server completion");
    Sys_EnterCriticalSection(CRITSECT_NETTHREAD_OVERRIDE);
    const bool clear=g_networkOverrideThread==0;
    Sys_LeaveCriticalSection(CRITSECT_NETTHREAD_OVERRIDE);
    if(!clear)fail("terminal checkpoint retains network override");
    std::unique_lock<std::mutex> lock(parkMutex);
    terminalParked.store(true,std::memory_order_release);
    parkCondition.notify_all();
    // No event destruction, TLS retirement, join, resume or reinit is certified.
    // Terminal process exit retires this parked OS thread and its owned events.
    parkCondition.wait(lock,[]{return false;});
    std::abort();
}
void bo1::android::ParkServerForTerminalShutdown() {
    if(!Sys_IsMainThread())fail("terminal park requires main owner");
    terminalRequested.store(true,std::memory_order_release);
    const auto observed=state.load(std::memory_order_acquire);
    if(observed==State::Unspawned)return; // Seals admission, no thread fabricated.
    if(observed==State::Terminal)fail("terminal park after callback failure");
    // Main-only spawn publication finishes before a subsequent main request;
    // Creating here means the real OS thread has not reached its entry yet.
    if(!wakeServerEvent || !allowServerNetworkEvent)fail("terminal request before spawn publication");
    Sys_SetEvent(&wakeServerEvent);
    Sys_SetEvent(&allowServerNetworkEvent);
    std::unique_lock<std::mutex> lock(parkMutex);
    const bool acknowledged=parkCondition.wait_for(lock,std::chrono::seconds(5),[]{
        return terminalParked.load(std::memory_order_acquire);
    });
    lock.unlock();
    if(!acknowledged)fail("server terminal park acknowledgment timeout");
}
bool bo1::android::ServerTerminalShutdownEstablished() {
    return terminalRequested.load(std::memory_order_acquire)
        && (state.load(std::memory_order_acquire)==State::Unspawned
            || terminalParked.load(std::memory_order_acquire));
}
