#pragma once

// Tick units are platform-specific: Windows retains TSC; Android uses monotonic
// nanoseconds. Reports must use the matching frequency before they are ported.
#if defined(__ANDROID__)
#include <platform/android/profile_services.h>
#else
#include <intrin.h>
#if defined(_M_X64) || defined(_WIN64) || defined(BO1_PORT_X64)
#include <Windows.h>
#endif
inline unsigned long long BO1_ProfileTicks() { return __rdtsc(); }
inline unsigned long BO1_ProfileThreadId()
{
#if defined(_M_X64) || defined(_WIN64) || defined(BO1_PORT_X64)
    return GetCurrentThreadId();
#else
    return __readfsdword(0x24);
#endif
}
inline long BO1_ProfileAdd(volatile long *value, long n) { return _InterlockedExchangeAdd(value, n); }
inline long BO1_ProfileLoad(const volatile long *value) { return *value; }
#endif
