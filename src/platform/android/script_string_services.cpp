#include "script_string_services.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>
#if defined(__ANDROID__) && defined(__aarch64__)
#include <pthread.h>
#include <cerrno>
#elif defined(_WIN32) && defined(BO1_SCRIPT_STRING_SERVICES_HOST_TEST)
#include <Windows.h>
#else
#error "Native script services support Android ARM64 or the explicit real Windows host probe."
#endif

namespace {
[[noreturn]] void fail(const char *operation, int error) {
    std::fprintf(stderr, "Script pool service failure: %s (%d)\n", operation, error);
    std::fflush(stderr);
    std::abort();
}
unsigned index(bo1::script::PoolLock lock) {
    if (lock == bo1::script::PoolLock::Strings) return 0;
    if (lock == bo1::script::PoolLock::MemoryTree) return 1;
    fail("invalid lock", static_cast<int>(lock));
}
std::atomic<unsigned> serverState{0}; // 0 unset, 1 publishing, 2 immutable identity
#if defined(__ANDROID__)
pthread_mutex_t sections[2];
pthread_once_t once = PTHREAD_ONCE_INIT;
pthread_t serverThread{};
void initialize() {
    pthread_mutexattr_t attributes;
    int result = pthread_mutexattr_init(&attributes);
    if (result) fail("mutexattr_init", result);
    result = pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    if (result) fail("mutexattr_settype", result);
    for (auto &section : sections) {
        result = pthread_mutex_init(&section, &attributes);
        if (result) fail("mutex_init", result);
    }
    result = pthread_mutexattr_destroy(&attributes);
    if (result) fail("mutexattr_destroy", result);
}
bool currentIsServer() { return pthread_equal(serverThread, pthread_self()) != 0; }
#else
CRITICAL_SECTION sections[2];
INIT_ONCE once = INIT_ONCE_STATIC_INIT;
DWORD serverThread{};
BOOL CALLBACK initialize(PINIT_ONCE, PVOID, PVOID *) {
    for (auto &section : sections)
        if (!InitializeCriticalSectionEx(&section, 0, 0)) fail("InitializeCriticalSectionEx", static_cast<int>(GetLastError()));
    return TRUE;
}
bool currentIsServer() { return serverThread == GetCurrentThreadId(); }
#endif
}
void bo1::script::InitializeServices() {
#if defined(__ANDROID__)
    const int result = pthread_once(&once, initialize);
    if (result) fail("pthread_once", result);
#else
    if (!InitOnceExecuteOnce(&once, initialize, nullptr, nullptr)) fail("InitOnceExecuteOnce", static_cast<int>(GetLastError()));
#endif
}
void bo1::script::Lock(PoolLock lock) {
    const unsigned i = index(lock);
    InitializeServices();
#if defined(__ANDROID__)
    const int result = pthread_mutex_lock(&sections[i]);
    if (result) fail("mutex_lock", result);
#else
    EnterCriticalSection(&sections[i]);
#endif
}
bool bo1::script::TryLock(PoolLock lock) {
    const unsigned i = index(lock);
    InitializeServices();
#if defined(__ANDROID__)
    const int result = pthread_mutex_trylock(&sections[i]);
    if (result == EBUSY) return false;
    if (result) fail("mutex_trylock", result);
    return true;
#else
    return TryEnterCriticalSection(&sections[i]) != FALSE;
#endif
}
void bo1::script::Unlock(PoolLock lock) {
    const unsigned i = index(lock);
    InitializeServices();
#if defined(__ANDROID__)
    const int result = pthread_mutex_unlock(&sections[i]);
    if (result) fail("mutex_unlock", result);
#else
    LeaveCriticalSection(&sections[i]);
#endif
}
void bo1::script::RegisterServerThread() {
    unsigned expected = 0;
    if (serverState.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
#if defined(__ANDROID__)
        serverThread = pthread_self();
#else
        serverThread = GetCurrentThreadId();
#endif
        serverState.store(2, std::memory_order_release);
        return;
    }
    while (serverState.load(std::memory_order_acquire) == 1) std::this_thread::yield();
    if (!currentIsServer()) fail("server thread already registered", 0);
}
bool bo1::script::IsServerThread() {
    return serverState.load(std::memory_order_acquire) == 2 && currentIsServer();
}
