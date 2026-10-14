#include "stream_worker.h"
#include "thread_services.h"
#include <atomic>
#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace {
struct Event {
    std::mutex mutex;
    std::condition_variable condition;
    bool manual;
    bool signaled;
    unsigned waiters{};
    std::uint64_t pulseEpoch{};
    Event(bool manualReset, bool initial) : manual(manualReset), signaled(initial) {}
};
// Handles and the original endless worker are process-owned. No shutdown or
// reinitialization certification: events must never be destroyed with waiters.
std::atomic<bool> spawned{};
[[noreturn]] void fail(const char *reason) {
    std::fprintf(stderr, "Native stream worker: %s\n", reason);
    std::abort();
}
Event &get(void **handle) {
    if (!handle || !*handle) fail("uninitialized event");
    return *static_cast<Event *>(*handle);
}
}
void Sys_CreateEvent(bool manualReset, bool initialState, void **handle) {
    if (!handle || *handle) fail("event already initialized");
    try { *handle = new Event(manualReset, initialState); }
    catch (...) { fail("event allocation"); }
}
void Sys_SetEvent(void **handle) {
    auto &event = get(handle);
    std::lock_guard<std::mutex> lock(event.mutex);
    event.signaled = true; // Multiple signals coalesce, matching Windows.
    if (event.manual) event.condition.notify_all();
    else event.condition.notify_one();
}
void Sys_ResetEvent(void **handle) {
    auto &event = get(handle);
    std::lock_guard<std::mutex> lock(event.mutex);
    event.signaled = false;
}
bool Sys_WaitForSingleObjectTimeout(void **handle, unsigned int milliseconds) {
    auto &event = get(handle);
    std::unique_lock<std::mutex> lock(event.mutex);
    const auto epoch=event.pulseEpoch;
    ++event.waiters;
    bool ready;
    if (milliseconds == 0xffffffffu)
        {event.condition.wait(lock, [&] { return event.signaled || event.pulseEpoch!=epoch; });ready=true;}
    else ready=event.condition.wait_for(lock, std::chrono::milliseconds(milliseconds),
                                      [&] { return event.signaled || event.pulseEpoch!=epoch; });
    --event.waiters;
    if(!ready)return false;
    if (!event.manual) event.signaled = false;
    return true;
}
void Sys_DestroyEvent(void **handle) {
    auto &event=get(handle);
    std::unique_lock<std::mutex> lock(event.mutex);
    if(event.waiters)fail("event destruction with waiters");
    *handle=nullptr;lock.unlock();delete &event;
}
void Sys_PulseManualEvent(void **handle) {
    auto &event=get(handle);
    std::lock_guard<std::mutex> lock(event.mutex);
    if(!event.manual)fail("manual pulse requested for auto-reset event");
    event.signaled=false;
    ++event.pulseEpoch;event.condition.notify_all();
}
void Sys_WaitForSingleObject(void **handle) {
    (void)Sys_WaitForSingleObjectTimeout(handle, 0xffffffffu);
}
char Sys_SpawnStreamThread(void (*function)(unsigned int)) {
    if (!Sys_IsMainThread() || !function || spawned.exchange(true)) fail("invalid/duplicate spawn");
    Sys_CreateEvent(true, false, &sndInitializedEvent);
    Sys_CreateEvent(true, false, &streamCompletedEvent);
    Sys_CreateEvent(false, false, &streamEvent);
    try {
        std::thread([function] {
            Sys_InitThread(THREAD_CONTEXT_STREAM);
            std::fprintf(stderr, "STREAM worker TLS ready: context=%d\n", Sys_GetThreadContext());
            function(THREAD_CONTEXT_STREAM);
            bo1::android::RetireThreadRole();
            fail("original stream callback unexpectedly returned");
        }).detach();
    } catch (...) { fail("thread creation"); }
    // The genuine callback must reach Sys_StreamSleep before returning success.
    // Sound stays uninitialized until its real owner signals it.
    if (!Sys_WaitForSingleObjectTimeout(&streamCompletedEvent, 5000)) fail("worker startup timeout");
    std::fputs("STREAM worker parked at real sound-initialization gate\n", stderr);
    return 1;
}
