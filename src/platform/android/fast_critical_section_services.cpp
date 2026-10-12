#include <platform/fast_critical_section.h>
#include <cstdio>
#include <cstdlib>
#if defined(__ANDROID__) && defined(__aarch64__)
#include <sched.h>
#include <cerrno>
#elif !defined(_WIN32) || !defined(BO1_FAST_CRIT_SECT_NATIVE_HOST_TEST)
#error "Native FastCriticalSection services require Android ARM64 or the explicit Windows host probe."
#endif

void bo1::fcs::YieldThread() {
#if defined(__ANDROID__)
    if (sched_yield() != 0) {
        std::fprintf(stderr, "Native FastCriticalSection scheduler yield failed (%d)\n", errno);
        std::abort();
    }
#else
    // The reference NET_Sleep(0) reaches the genuine Windows Sleep(0).
    Sleep(0);
#endif
}
