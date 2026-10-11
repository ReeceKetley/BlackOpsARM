#include "win32_time.h"
#include "win32_errors.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sched.h>
#include <time.h>
#include <unistd.h>

namespace bo1::win32 {
namespace {
[[noreturn]] void timeFailure(const char* operation)
{
    std::fprintf(stderr, "win32_compat: %s failed (errno=%d)\n", operation, errno);
    std::abort(); // Sleep has no error return; never pretend a failed sleep worked.
}
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* frequency)
{
    if (!frequency) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    frequency->QuadPart = 1000000000LL; // nanosecond clock timebase, fixed for all threads
    return 1; // Successful calls leave this thread's last-error value untouched.
}

void Sleep(DWORD milliseconds)
{
    if (milliseconds == 0) {
        if (sched_yield() != 0)
            timeFailure("sched_yield");
        return;
    }
    if (milliseconds == INFINITE) {
        for (;;) // POSIX signals must not turn an indefinite sleep into success.
            pause();
    }
    timespec remaining{static_cast<time_t>(milliseconds / 1000u),
                       static_cast<long>(milliseconds % 1000u) * 1000000L};
    timespec request = remaining;
    while (nanosleep(&request, &remaining) != 0) {
        if (errno != EINTR)
            timeFailure("nanosleep");
        request = remaining;
    }
}
}
