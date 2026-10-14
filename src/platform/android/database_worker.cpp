#include "database_worker.h"
#include "thread_services.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace {
struct Worker {
    std::mutex mutex;
    std::condition_variable condition;
    std::thread thread;
    bool ready{};
    bool retired{};
    bool joined{};
};
// One-shot, process-owned state. No destruction while the original endless
// callback is running. A finite callback may retire and be genuinely joined.
Worker *worker;
std::atomic<bool> claimed{};
[[noreturn]] void fail(const char *why) {
    std::fprintf(stderr, "Native database worker: %s\n", why);
    std::abort();
}
}
char Sys_SpawnDatabaseThread(void (*function)(unsigned int)) {
    if (!Sys_IsMainThread() || !function || claimed.exchange(true)) fail("invalid/duplicate spawn");
    Sys_CreateEvent(false, false, &wakeDatabaseEvent);
    Sys_CreateEvent(true, true, &databaseCompletedEvent);
    Sys_CreateEvent(true, true, &databaseCompletedEvent2);
    Sys_CreateEvent(true, true, &resumedDatabaseEvent);
    try {
        worker = new Worker;
        worker->thread = std::thread([function] {
            Sys_InitThread(THREAD_CONTEXT_DATABASE);
            {
                std::lock_guard<std::mutex> lock(worker->mutex);
                worker->ready = true;
                worker->condition.notify_all();
            }
            function(THREAD_CONTEXT_DATABASE);
            bo1::android::RetireThreadRole();
            {
                std::lock_guard<std::mutex> lock(worker->mutex);
                worker->retired = true;
                worker->condition.notify_all();
            }
        });
    } catch (...) { fail("thread creation"); }
    std::unique_lock<std::mutex> lock(worker->mutex);
    if (!worker->condition.wait_for(lock, std::chrono::seconds(5), [] { return worker->ready; }))
        fail("TLS initialization timeout");
    std::fputs("DATABASE worker TLS initialized: context=13\n", stderr);
    return 1;
}
void bo1::android::JoinRetiredDatabaseThread() {
    if (!Sys_IsMainThread() || !worker) fail("join without main-owned worker");
    {
        std::unique_lock<std::mutex> lock(worker->mutex);
        if (worker->joined) fail("duplicate join");
        if (!worker->condition.wait_for(lock, std::chrono::seconds(5), [] { return worker->retired; }))
            fail("join requires callback retirement");
        worker->joined = true;
    }
    worker->thread.join();
}
