#pragma once
#include <ctime>
#include <bit>
#include <cstdint>
#include <cerrno>

namespace bo1::android {
inline std::int64_t WallTime64() {
    return static_cast<std::int64_t>(std::time(nullptr));
}

inline std::tm *BreakDownTime32(std::int32_t timestamp, bool local, std::tm &output) {
    // Retain MS CRT's pre-epoch rejection and untouched output on failure.
    if (timestamp < 0) { errno = EINVAL; return nullptr; }
    const std::time_t source = timestamp;
    return local ? ::localtime_r(&source, &output) : ::gmtime_r(&source, &output);
}

inline std::int32_t WallTime32() {
    // Preserve the engine's existing Windows low-32-bit time contract.
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(std::time(nullptr)));
}
}
