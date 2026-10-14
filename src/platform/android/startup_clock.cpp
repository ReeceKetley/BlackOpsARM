#include <platform/win32_compat/win32_time.h>
#include <cstdint>
// Original startup/profiling bodies call the Win32 spelling at global scope.
bo1::win32::BOOL QueryPerformanceCounter(bo1::win32::LARGE_INTEGER *counter) {
    return bo1::win32::QueryPerformanceCounter(counter);
}

unsigned int Sys_MillisecondsRaw() {
    bo1::win32::LARGE_INTEGER counter{};
    bo1::win32::QueryPerformanceCounter(&counter);
    return static_cast<std::uint32_t>(counter.QuadPart / 1000000LL);
}
unsigned int Sys_Milliseconds() {
    // Match the reference: first call establishes the base; unsigned subtraction
    // preserves the original 32-bit wraparound contract.
    static const std::uint32_t base = Sys_MillisecondsRaw();
    return Sys_MillisecondsRaw() - base;
}
