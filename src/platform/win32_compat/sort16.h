#pragma once
#include <cstddef>
#include <cstdint>

namespace bo1::compat {
using Sort16Compare = int (*)(std::int16_t, std::int16_t, const void*) noexcept;
// Original executable qsort/shortsort ordering, restricted to a signed16 domain.
// The caller owns a live writable array; callbacks cannot modify it or throw.
// No globals, allocation or added equal-key tie-break. Invalid input is untouched.
bool Sort16(std::int16_t* values, std::size_t count,
            Sort16Compare compare, const void* context) noexcept;
}
