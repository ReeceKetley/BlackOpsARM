// Native process-lifetime recursive engine sections. SL/MT/DBHASH keep their
// existing PoolLock identity. The full Windows implementation is not overridden.
#include <platform/critical_section_services.h>
#include <platform/android/script_string_services.h>
#include <cstdio>
#include <cstdlib>
#if defined(__ANDROID__)
#include <pthread.h>
#include <cerrno>
#else
#include <Windows.h>
#endif

#if !(defined(__ANDROID__) && defined(__aarch64__)) && !(defined(_WIN32) && (defined(BO1_STRING_POOL_GATE) || defined(BO1_ASSET_ENTRY_GATE)))
#error "Script pool bridge is Android ARM64 or an explicit Windows pool gate only."
#endif

static_assert(sizeof(CriticalSection) == 4);
static_assert(CRITSECT_SCRIPT_STRING == 27 && CRITSECT_MEMORY_TREE == 28 && CRITSECT_DBHASH == 74);
static_assert(CRITSECT_COUNT == 75);
namespace {
[[noreturn]] void fail(const char *operation, int error) {
    std::fprintf(stderr, "Native critical sections: %s (%d)\n", operation, error);
    std::fflush(stderr);
    std::abort();
}
unsigned index(CriticalSection section) {
    const auto i = static_cast<unsigned>(section);
    if (i >= CRITSECT_COUNT) fail("invalid identifier", static_cast<int>(section));
    return i;
}
bool isPool(CriticalSection section) {
    return section == CRITSECT_SCRIPT_STRING || section == CRITSECT_MEMORY_TREE || section == CRITSECT_DBHASH;
}
bo1::script::PoolLock poolLock(CriticalSection section) {
    switch (section) {
    case CRITSECT_SCRIPT_STRING: return bo1::script::PoolLock::Strings;
    case CRITSECT_MEMORY_TREE: return bo1::script::PoolLock::MemoryTree;
    case CRITSECT_DBHASH: return bo1::script::PoolLock::Database;
    default:
        fail("invalid pool identifier", static_cast<int>(section));
    }
}
#if defined(__ANDROID__)
pthread_mutex_t sections[CRITSECT_COUNT];
pthread_once_t once = PTHREAD_ONCE_INIT;
void initialize() {
    pthread_mutexattr_t attributes;
    int result = pthread_mutexattr_init(&attributes);
    if (result) fail("mutexattr_init", result);
    result = pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    if (result) fail("mutexattr_settype", result);
    for (unsigned i = 0; i < CRITSECT_COUNT; ++i) {
        if (isPool(static_cast<CriticalSection>(i))) continue;
        result = pthread_mutex_init(&sections[i], &attributes);
        if (result) fail("mutex_init", result);
    }
    result = pthread_mutexattr_destroy(&attributes);
    if (result) fail("mutexattr_destroy", result);
}
#else
CRITICAL_SECTION sections[CRITSECT_COUNT];
INIT_ONCE once = INIT_ONCE_STATIC_INIT;
BOOL CALLBACK initialize(PINIT_ONCE, PVOID, PVOID *) {
    for (unsigned i = 0; i < CRITSECT_COUNT; ++i) {
        if (isPool(static_cast<CriticalSection>(i))) continue;
        if (!InitializeCriticalSectionEx(&sections[i], 0, 0))
            fail("InitializeCriticalSectionEx", static_cast<int>(GetLastError()));
    }
    return TRUE;
}
#endif
}

void __cdecl Sys_InitializeCriticalSections() {
    // Thread-safe/idempotent, with no implicit engine thread-role registration.
    bo1::script::InitializeServices();
#if defined(__ANDROID__)
    const int result = pthread_once(&once, initialize);
    if (result) fail("pthread_once", result);
#else
    if (!InitOnceExecuteOnce(&once, initialize, nullptr, nullptr))
        fail("InitOnceExecuteOnce", static_cast<int>(GetLastError()));
#endif
}
void __cdecl Sys_EnterCriticalSection(CriticalSection section) {
    const unsigned i = index(section);
    if (isPool(section)) { bo1::script::Lock(poolLock(section)); return; }
    Sys_InitializeCriticalSections();
#if defined(__ANDROID__)
    const int result = pthread_mutex_lock(&sections[i]);
    if (result) fail("mutex_lock", result);
#else
    EnterCriticalSection(&sections[i]);
#endif
}
bool __cdecl Sys_TryEnterCriticalSection(CriticalSection section) {
    const unsigned i = index(section);
    if (isPool(section)) return bo1::script::TryLock(poolLock(section));
    Sys_InitializeCriticalSections();
#if defined(__ANDROID__)
    const int result = pthread_mutex_trylock(&sections[i]);
    if (result == EBUSY) return false;
    if (result) fail("mutex_trylock", result);
    return true;
#else
    return TryEnterCriticalSection(&sections[i]) != FALSE;
#endif
}
void __cdecl Sys_LeaveCriticalSection(CriticalSection section) {
    const unsigned i = index(section);
    if (isPool(section)) { bo1::script::Unlock(poolLock(section)); return; }
    Sys_InitializeCriticalSections();
#if defined(__ANDROID__)
    const int result = pthread_mutex_unlock(&sections[i]);
    if (result) fail("mutex_unlock", result);
#else
    LeaveCriticalSection(&sections[i]);
#endif
}
bool __cdecl Sys_IsServerThread() {
    // Unset is false. Once registered, the actual server thread must remain
    // alive during role queries; the underlying service has no reset API.
    return bo1::script::IsServerThread();
}
