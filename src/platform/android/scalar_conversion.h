#pragma once
#include <cmath>
#include <cfenv>
#include <cstdint>
#include <limits>

namespace bo1::android {
// Scalar counterpart of CVTSD2SI's signed 32-bit result: honor the active
// floating-point rounding mode and return integer-indefinite on invalid input.
inline std::int32_t RoundedDoubleToInt32(double value) {
    const double rounded = std::rint(value);
    if (!std::isfinite(rounded) ||
        rounded < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        rounded > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
        std::feraiseexcept(FE_INVALID);
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(rounded);
}
}
