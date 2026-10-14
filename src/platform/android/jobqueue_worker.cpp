#include "jobqueue_worker.h"
#include "thread_services.h"
#include <tl/jobqueue/jobqueue_all.h>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include <thread>
#include <unistd.h>
namespace {
struct Thread {
    jqWorker *worker;
    ThreadContext_t role;
    std::mutex mutex;
    std::condition_variable condition;
    bool ready{},retired{};
    unsigned nativeId{};
    std::thread thread;
};
thread_local Thread *currentWorker;
[[noreturn]] void fail(const char *why){std::fprintf(stderr,"Native job worker: %s\n",why);std::abort();}
}
void *bo1::android::SpawnJobWorker(jqWorker *worker,unsigned (*function)(jqWorker *)) {
    if(!Sys_IsMainThread() || !worker || !function || worker->WorkerID<1 || worker->WorkerID>8)
        fail("invalid spawn");
    auto *handle=new Thread{worker,static_cast<ThreadContext_t>(worker->WorkerID+1)};
    try {
        handle->thread=std::thread([handle,function] {
            currentWorker=handle;Sys_InitThread(handle->role);
            function(handle->worker);
            RetireThreadRole();
            std::lock_guard<std::mutex> lock(handle->mutex);handle->retired=true;handle->condition.notify_all();
        });
    } catch(...){delete handle;fail("thread creation failed");}
    std::unique_lock<std::mutex> lock(handle->mutex);
    if(!handle->condition.wait_for(lock,std::chrono::seconds(5),[&]{return handle->ready;}))fail("worker initialization timeout");
    return handle;
}
void bo1::android::JobWorkerReady(jqWorker *worker) {
    if(!currentWorker || currentWorker->worker!=worker)fail("ready without actual worker owner");
    std::lock_guard<std::mutex> lock(currentWorker->mutex);
    if(Sys_GetThreadContext()!=currentWorker->role || worker->ThreadId!=static_cast<unsigned>(gettid()))
        fail("ready worker identity mismatch");
    currentWorker->nativeId=static_cast<unsigned>(gettid());
    currentWorker->ready=true;currentWorker->condition.notify_all();
}
void __cdecl Sys_InitWorkerThreadContext() {
    // Native workers bind their own TLS before publishing ready. Unlike the
    // Windows threadId table, role queries use that owning TLS binding.
    // Validate the original renderer's two dedicated queue owners here.
    if(!Sys_IsMainThread() || !jqWorkers || jqGetNumWorkers()<3)fail("worker context initialization without workers");
    for(unsigned index=1;index<=2;++index) {
        auto *worker=&jqWorkers[index];
        if(worker->WorkerID!=index || worker->Processor!=(1u<<(index+1)) || !worker->Thread)
            fail("unexpected renderer worker configuration");
        auto *handle=static_cast<Thread *>(worker->Thread);
        std::lock_guard<std::mutex> lock(handle->mutex);
        if(handle->worker!=worker || handle->role!=static_cast<ThreadContext_t>(index+1)
            || !handle->ready || handle->retired || !handle->nativeId || worker->ThreadId!=handle->nativeId)
            fail("renderer worker context has no live native owner");
    }
}
void bo1::android::JoinJobWorker(void *opaque) {
    if(!Sys_IsMainThread() || !opaque)fail("invalid join");
    auto *handle=static_cast<Thread *>(opaque);
    handle->thread.join();
    if(!handle->retired)fail("joined worker failed to retire TLS");
    ResetJoinedWorkerRole(handle->role);delete handle;
}
void SetThreadName(unsigned int id,const char *name) {
    if(!name || (id!=0xffffffffu && id!=static_cast<unsigned>(gettid())))fail("thread name requires current native owner");
    char shortName[16];std::snprintf(shortName,sizeof(shortName),"%s",name);
    if(pthread_setname_np(pthread_self(),shortName))fail("cannot set native thread name");
}
