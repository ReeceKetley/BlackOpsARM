#pragma once
#include <platform/win32_compat/win32_types.h>
#include <cstdint>
#include <cerrno>
#include <ctime>
#include <limits>

namespace bo1::android {
inline bool ReadFileTime(bo1::win32::FILETIME *output) {
    if (!output) { errno = EINVAL; return false; }
    timespec now{};
    if (::clock_gettime(CLOCK_REALTIME, &now)) return false;
    constexpr std::int64_t epochOffset = 11644473600LL;
    constexpr std::uint64_t scale = 10000000;
    constexpr auto maxSeconds = std::numeric_limits<std::uint64_t>::max() / scale;
    if (now.tv_sec < -epochOffset || now.tv_sec > static_cast<std::int64_t>(maxSeconds)-epochOffset) {
        errno = ERANGE; return false;
    }
    const auto seconds = static_cast<std::uint64_t>(now.tv_sec + epochOffset);
    const auto fraction = static_cast<std::uint64_t>(now.tv_nsec / 100);
    const auto base = seconds * scale;
    if (fraction > std::numeric_limits<std::uint64_t>::max()-base) { errno=ERANGE; return false; }
    const std::uint64_t ticks = base + fraction;
    *output = {static_cast<std::uint32_t>(ticks), static_cast<std::uint32_t>(ticks >> 32)};
    return true;
}
}
