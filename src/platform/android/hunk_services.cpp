#include "hunk_services.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <unistd.h>
#if !defined(__ANDROID__) || !defined(__aarch64__)
#error Android ARM64 hunk services only
#endif
namespace {
std::atomic<pid_t> mainThread;
std::recursive_mutex firstFitMutex;
}
void bo1::android::RegisterHunkMainThread()
{
    const pid_t current = gettid();
    pid_t expected = 0;
    if (!mainThread.compare_exchange_strong(expected, current, std::memory_order_acq_rel)
        && expected != current)
    {
        std::fputs("Hunk main thread already registered by another thread\n", stderr);
        std::abort();
    }
}
bool bo1::android::IsHunkMainThread() { return mainThread.load(std::memory_order_acquire) == gettid(); }
void bo1::android::LockFirstFit() { firstFitMutex.lock(); }
void bo1::android::UnlockFirstFit() { firstFitMutex.unlock(); }
