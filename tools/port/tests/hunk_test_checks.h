#pragma once
#include <cstdio>
#include <cstdlib>
#if defined(__ANDROID__)
#include <platform/android/virtual_memory.h>
inline std::size_t HunkTestReservationCount() { return bo1::android::VirtualReservationCount(); }
#else
std::size_t HunkTestReservationCount();
#endif
inline void HunkCheck(bool value, int line, const char *expr)
{
    if (!value) { std::fprintf(stderr, "FAIL %d: %s\n", line, expr); std::abort(); }
}
#define CHECK(c) HunkCheck((c), __LINE__, #c)
