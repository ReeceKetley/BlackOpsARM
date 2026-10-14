#pragma once
#include <cstdint>
#include <cstring>
#include <cfenv>
#include <limits>
#if !defined(__ANDROID__) || !defined(__aarch64__)
#error Native float snapping requires Android ARM64
#endif
namespace bo1::android {
inline std::int32_t SnapFloatToInt32(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    if ((bits & 0x7f800000u) == 0x7f800000u) {
        std::feraiseexcept(FE_INVALID);
        return std::numeric_limits<std::int32_t>::min();
    }
    // FRINTX obeys FPCR rounding mode and records inexact, matching CVTSS2SI.
    float rounded;
    asm volatile("frintx %s0, %s1" : "=w"(rounded) : "w"(value));
    if (rounded >= 2147483648.0f || rounded < -2147483648.0f) {
        std::feraiseexcept(FE_INVALID);
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(rounded);
}
}
