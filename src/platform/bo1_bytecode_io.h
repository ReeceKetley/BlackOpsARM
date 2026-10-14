#pragma once

#include <bit>
#include <cstring>
#include <type_traits>

// Script instructions own packed bytes, rather than aligned scalar objects.
// Preserve their little-endian widths; code addresses remain Narrow32 handles.
namespace bo1::bytecode {
static_assert(std::endian::native == std::endian::little,
              "The canonical script bytecode uses little-endian operands");

template <typename T>
inline constexpr bool Scalar =
    std::is_arithmetic_v<T> && !std::is_same_v<T, bool> &&
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);

template <typename T>
T Load(const void *bytes)
{
    static_assert(Scalar<T>, "Use a fixed-width scalar, never a native pointer");
    T value;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

template <typename T>
void Store(void *bytes, T value)
{
    static_assert(Scalar<T>, "Use a fixed-width scalar, never a native pointer");
    std::memcpy(bytes, &value, sizeof(value));
}
} // namespace bo1::bytecode
