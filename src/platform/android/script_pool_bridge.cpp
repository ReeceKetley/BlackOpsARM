// Bounded real SL/MT platform bridge: only the two production pool sections
// are supported. Unsupported sections fail explicitly, rather than appearing
// initialized or locked. The full Windows implementation is never overridden.
#include <platform/script_pool_services.h>
#include <platform/android/script_string_services.h>
#include <cstdio>
#include <cstdlib>

#if !(defined(__ANDROID__) && defined(__aarch64__)) && !(defined(_WIN32) && defined(BO1_STRING_POOL_GATE))
#error "Script pool bridge is Android ARM64 or an explicit Windows pool gate only."
#endif

static_assert(sizeof(CriticalSection) == 4);
static_assert(CRITSECT_SCRIPT_STRING == 27 && CRITSECT_MEMORY_TREE == 28);
namespace {
bo1::script::PoolLock poolLock(CriticalSection section) {
    switch (section) {
    case CRITSECT_SCRIPT_STRING: return bo1::script::PoolLock::Strings;
    case CRITSECT_MEMORY_TREE: return bo1::script::PoolLock::MemoryTree;
    default:
        std::fprintf(stderr, "Script pool bridge: unsupported critical section %d\n", static_cast<int>(section));
        std::fflush(stderr);
        std::abort();
    }
}
}

void __cdecl Sys_InitializeCriticalSections() {
    // Real process-lifetime recursive locks; initialization is thread-safe and
    // idempotent. No engine thread role is implicitly registered here.
    bo1::script::InitializeServices();
}
void __cdecl Sys_EnterCriticalSection(CriticalSection section) {
    bo1::script::Lock(poolLock(section));
}
bool __cdecl Sys_TryEnterCriticalSection(CriticalSection section) {
    return bo1::script::TryLock(poolLock(section));
}
void __cdecl Sys_LeaveCriticalSection(CriticalSection section) {
    bo1::script::Unlock(poolLock(section));
}
bool __cdecl Sys_IsServerThread() {
    // Unset is false. Once registered, the actual server thread must remain
    // alive during role queries; the underlying service has no reset API.
    return bo1::script::IsServerThread();
}
