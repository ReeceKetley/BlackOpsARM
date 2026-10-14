#include "occlusion_worker.h"
#include "thread_services.h"
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
[[noreturn]] void fail(const char *message) {
    std::fprintf(stderr,"Native occlusion worker: %s\n",message); std::abort();
}
}
void Sys_SpawnOcclusion(void (*function)(unsigned int)) {
    if(!Sys_IsMainThread() || !function) fail("main-owned callback required");
    if(worker) {
        if(!worker->joined) fail("previous worker must retire and join before reuse");
        bo1::android::ResetJoinedWorkerRole(THREAD_CONTEXT_OCCLUSION);
        Sys_ResetEvent(&occlusionEvent);
        worker->ready=worker->retired=worker->joined=false;
    } else {
        worker=new Worker;
        Sys_CreateEvent(false,false,&occlusionEvent);
    }
    try {
        worker->thread=std::thread([function] {
            Sys_InitThread(THREAD_CONTEXT_OCCLUSION);
            if(pthread_setname_np(pthread_self(),"bo1-occlusion")) fail("thread naming");
            { std::lock_guard<std::mutex> lock(worker->mutex); worker->ready=true; worker->condition.notify_all(); }
            function(THREAD_CONTEXT_OCCLUSION);
            bo1::android::RetireThreadRole();
            { std::lock_guard<std::mutex> lock(worker->mutex); worker->retired=true; worker->condition.notify_all(); }
        });
    } catch(...) { fail("thread creation"); }
    std::unique_lock<std::mutex> lock(worker->mutex);
    if(!worker->condition.wait_for(lock,std::chrono::seconds(5),[]{return worker->ready;})) fail("TLS startup timeout");
}
bool bo1::android::WaitForOcclusionThreadRetirement(unsigned milliseconds) {
    if(!worker) return false;
    std::unique_lock<std::mutex> lock(worker->mutex);
    return worker->condition.wait_for(lock,std::chrono::milliseconds(milliseconds),[]{return worker->retired;});
}
void bo1::android::JoinRetiredOcclusionThread() {
    if(!Sys_IsMainThread() || !worker) fail("main-owned worker required for join");
    {
        std::unique_lock<std::mutex> lock(worker->mutex);
        if(worker->joined) fail("duplicate join");
        if(!worker->condition.wait_for(lock,std::chrono::seconds(5),[]{return worker->retired;})) fail("callback did not retire");
    }
    worker->thread.join();
    worker->joined=true;
}
