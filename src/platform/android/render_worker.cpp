#include "render_worker.h"
#include "thread_services.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
namespace {
struct Worker {
    std::mutex mutex;
    std::condition_variable condition;
    std::thread thread;
    bool ready{},retired{},joined{};
};
Worker *worker;
std::atomic<bool> claimed{};
[[noreturn]] void fail(const char *reason) {
    std::fprintf(stderr,"Native render worker: %s\n",reason);std::abort();
}
}
char Sys_SpawnRenderThread(void (*function)(unsigned int)) {
    if(!Sys_IsMainThread() || !function || claimed.exchange(true))fail("invalid or duplicate spawn");
    // Exact original event modes/states. These are synchronization state, not
    // proof of graphics registration or successful device creation.
    Sys_CreateEvent(false,false,&renderPausedEvent);
    Sys_CreateEvent(true,true,&renderCompletedEvent);
    Sys_CreateEvent(true,false,&resourcesFlushedEvent);
    Sys_CreateEvent(true,false,&resourcesQueuedEvent);
    Sys_CreateEvent(true,true,&rendererRunningEvent);
    Sys_CreateEvent(false,false,&backendEvent[1]);
    Sys_CreateEvent(true,false,&backendEvent[0]);
    Sys_CreateEvent(true,true,&updateSpotLightEffectEvent);
    Sys_CreateEvent(true,true,&updateEffectsEvent);
    Sys_CreateEvent(true,true,&d3dDeviceOKEvent);
    Sys_CreateEvent(true,false,&d3dDeviceHardStartEvent);
    Sys_CreateEvent(true,false,&d3dShutdownEvent);
    Sys_CreateEvent(true,true,&d3dDeviceWinMessageEvent);
    Sys_CreateEvent(true,false,&win32QuitEvent);
    Sys_CreateEvent(true,false,&win32ScriptDebuggerDrawEvent);
    Sys_CreateEvent(true,false,&rgRegisteredEvent);
    Sys_CreateEvent(true,false,&renderEvent);
    try {
        worker=new Worker;
        worker->thread=std::thread([function] {
            Sys_InitThread(THREAD_CONTEXT_BACKEND);
            if(pthread_setname_np(pthread_self(),"bo1-render"))fail("thread naming failed");
            {std::lock_guard<std::mutex> lock(worker->mutex);worker->ready=true;worker->condition.notify_all();}
            function(THREAD_CONTEXT_BACKEND);
            bo1::android::RetireThreadRole();
            {std::lock_guard<std::mutex> lock(worker->mutex);worker->retired=true;worker->condition.notify_all();}
        });
    } catch (...) {fail("thread creation failed");}
    std::unique_lock<std::mutex> lock(worker->mutex);
    if(!worker->condition.wait_for(lock,std::chrono::seconds(5),[]{return worker->ready;}))fail("TLS startup timeout");
    // Native OS scheduling is used. Windows priority/ideal-CPU hints have no
    // exact equivalent and do not establish graphics readiness.
    return 1;
}
bool bo1::android::WaitForRenderThreadRetirement(unsigned milliseconds) {
    if(!Sys_IsMainThread() || !worker)fail("invalid retirement wait owner");
    std::unique_lock<std::mutex> lock(worker->mutex);
    return worker->condition.wait_for(lock,std::chrono::milliseconds(milliseconds),[]{return worker->retired;});
}
void bo1::android::JoinRetiredRenderThread() {
    if(!Sys_IsMainThread() || !worker)fail("invalid join owner");
    {std::lock_guard<std::mutex> lock(worker->mutex);
        if(!worker->retired || worker->joined)fail("callback not retired or already joined");
        worker->joined=true;}
    worker->thread.join();
    // Original event handles stay process-owned. Join does not reset role/state.
}

bool bo1::android::HasSpawnedRenderThread() { return claimed.load(std::memory_order_acquire); }
