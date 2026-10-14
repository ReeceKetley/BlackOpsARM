#include "thread_services.h"
#include <universal/thread_data.h>
#if defined(__ANDROID__) && defined(BO1_NATIVE_SCHEDULING_SERVICES)
#include "scheduling_services.h"
#endif
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#if defined(__ANDROID__) && defined(__aarch64__)
#include "hunk_services.h"
#elif !defined(_WIN32) || !defined(BO1_NATIVE_THREAD_SERVICES_HOST_TEST)
#error "Native thread services require Android ARM64 or the explicit Windows host probe."
#endif

namespace {
[[noreturn]] void fail(const char *reason, int detail) {
    std::fprintf(stderr, "Native thread services: %s (%d)\n", reason, detail);
    std::fflush(stderr);
    std::abort();
}
// Never compare OS thread IDs after a thread exits: IDs can be reused. A
// one-shot role claim and owning TLS token establish the full native identity.
std::array<std::atomic<bool>, THREAD_CONTEXT_COUNT> claimed{};
void *values[THREAD_CONTEXT_COUNT][5]{};
struct Binding {
    int role = THREAD_CONTEXT_INVALID;
    bool retired = false;
    ~Binding() {
        if (role != THREAD_CONTEXT_INVALID && role != THREAD_CONTEXT_MAIN)
            fail("worker exited without retirement", role);
    }
};
thread_local Binding binding;
int current() {
    if (binding.retired) fail("retired thread", binding.role);
    if (binding.role == THREAD_CONTEXT_INVALID) fail("unregistered thread", -1);
    return binding.role;
}
void slot(int index) {
    if (index < 0 || index >= 5) fail("invalid TLS slot", index);
}
bool isRole(int role) {
    return !binding.retired && binding.role == role;
}
}

void bo1::android::RegisterThreadRole(ThreadContext_t role) {
    if (role != THREAD_CONTEXT_MAIN && role != THREAD_CONTEXT_BACKEND && role != THREAD_CONTEXT_DATABASE
        && role != THREAD_CONTEXT_STREAM && role != THREAD_CONTEXT_OCCLUSION && role != THREAD_CONTEXT_SERVER
        && !(role>=THREAD_CONTEXT_WORKER0 && role<=THREAD_CONTEXT_WORKER7))
        fail("unsupported role", static_cast<int>(role));
    if (binding.retired) fail("retired role cannot rebind", static_cast<int>(role));
    if (binding.role != THREAD_CONTEXT_INVALID) {
        if (binding.role != role) fail("thread role switch", static_cast<int>(role));
        Com_InitThreadData(role);
        return;
    }
    bool expected = false;
    if (!claimed[role].compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        fail("role already claimed", static_cast<int>(role));
    binding.role = role;
#if defined(__ANDROID__)
    if (role == THREAD_CONTEXT_MAIN) {
        RegisterHunkMainThread();
        if (!IsHunkMainThread()) fail("hunk main identity mismatch", role);
    }
#endif
    Com_InitThreadData(role);
#if defined(__ANDROID__) && defined(BO1_NATIVE_SCHEDULING_SERVICES)
    bo1::android::RegisterSchedulingThread(role);
#endif
}
void bo1::android::RetireThreadRole() {
    const int role = current();
    if (role == THREAD_CONTEXT_MAIN) fail("main retirement unsupported", role);
#if defined(__ANDROID__) && defined(BO1_NATIVE_SCHEDULING_SERVICES)
    bo1::android::RetireSchedulingThread(static_cast<ThreadContext_t>(role));
#endif
    for (auto &value : values[role]) value = nullptr;
    binding.retired = true;
    binding.role = THREAD_CONTEXT_INVALID;
    // claimed deliberately remains true: no worker restart/quiescence proof.
}
void __cdecl Sys_InitMainThread() { bo1::android::RegisterThreadRole(THREAD_CONTEXT_MAIN); }
void bo1::android::ResetJoinedWorkerRole(ThreadContext_t role) {
    if(!Sys_IsMainThread() || (role!=THREAD_CONTEXT_OCCLUSION && (role<THREAD_CONTEXT_WORKER0 || role>THREAD_CONTEXT_WORKER7)))
        fail("invalid joined worker reset",role);
    for(auto value:values[role])if(value)fail("worker TLS not retired",role);
    if(!claimed[role].exchange(false,std::memory_order_acq_rel))fail("unclaimed worker reset",role);
}
void __cdecl Sys_InitThread(int context) { bo1::android::RegisterThreadRole(static_cast<ThreadContext_t>(context)); }
int __cdecl Sys_GetThreadContext() { return current(); }
bool __cdecl Sys_IsMainThread() { return isRole(THREAD_CONTEXT_MAIN); }
bool __cdecl Sys_IsRenderThread() { return isRole(THREAD_CONTEXT_BACKEND); }
bool __cdecl Sys_IsDatabaseThread() { return isRole(THREAD_CONTEXT_DATABASE); }
bool __cdecl Sys_IsStreamThread() { return isRole(THREAD_CONTEXT_STREAM); }
void __cdecl Sys_SetValue(int index, void *data) { slot(index); values[current()][index] = data; }
void *__cdecl Sys_GetValue(int index) { slot(index); return values[current()][index]; }
