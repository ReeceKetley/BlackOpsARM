#include "packed_vertex_decode.h"

namespace
{
constexpr std::uint32_t kComponentMask = 0x3FFu;
constexpr unsigned kComponentBits = 10u;

// Sign-extends a 10-bit two's complement field without relying on
// implementation-defined right shifts of negative values.
constexpr std::int32_t SignExtend10(std::uint32_t field)
{
    return static_cast<std::int32_t>(field ^ 0x200u) - 0x200;
}

constexpr float NormalizeSigned10(std::uint32_t field)
{
    const std::int32_t value = SignExtend10(field);
    const float normalized = static_cast<float>(value) / 511.0f;
    return normalized < -1.0f ? -1.0f : normalized;
}
} // namespace

std::array<float, 4> DecodeUDec3(std::uint32_t packed)
{
    return {
        static_cast<float>(packed & kComponentMask),
        static_cast<float>((packed >> kComponentBits) & kComponentMask),
        static_cast<float>((packed >> (kComponentBits * 2u)) & kComponentMask),
        1.0f
    };
}

std::array<float, 4> DecodeDec3N(std::uint32_t packed)
{
    return {
        NormalizeSigned10(packed & kComponentMask),
        NormalizeSigned10((packed >> kComponentBits) & kComponentMask),
        NormalizeSigned10((packed >> (kComponentBits * 2u)) & kComponentMask),
        1.0f
    };
}
