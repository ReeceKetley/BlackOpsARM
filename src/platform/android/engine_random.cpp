#include "engine_random.h"
#include <cstdint>
#include <limits>

namespace bo1::android {
static_assert(std::numeric_limits<std::uint32_t>::digits == 32);
static_assert(std::numeric_limits<unsigned int>::digits == 32);
namespace { thread_local std::uint32_t state = 1; }
void CrtSrand(unsigned int seed) { state = seed; }
int CrtRand()
{
    // Unsigned arithmetic deliberately wraps at 32 bits, matching UCRT.
    state = state * UINT32_C(214013) + UINT32_C(2531011);
    return static_cast<int>((state >> 16) & UINT32_C(0x7fff));
}
}
