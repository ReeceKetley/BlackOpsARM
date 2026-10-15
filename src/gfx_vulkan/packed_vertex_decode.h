#pragma once
#include <array>
#include <cstdint>

// Standalone decoders for the D3D9 10:10:10:2 packed vertex types. Standard C++
// only: no allocation and no engine dependencies.
//
// Both formats keep X in bits 0..9, Y in bits 10..19 and Z in bits 20..29 of the
// 32-bit word. Per the Microsoft D3DDECLTYPE definitions the two W bits (30..31)
// are not part of the expanded value; W is always 1.0f.

// D3DDECLTYPE_UDEC3: unsigned 10:10:10, expanded to (x, y, z, 1) with each of
// x, y, z an unsigned integer value in [0, 1023] converted to float unchanged.
std::array<float, 4> DecodeUDec3(std::uint32_t packed);

// D3DDECLTYPE_DEC3N: signed normalized 10:10:10 (two's complement, range
// [-512, 511]), expanded to (x, y, z, 1). Each component is value / 511 with the
// most negative value -512 clamped to -1.0f, so the range is [-1, 1].
std::array<float, 4> DecodeDec3N(std::uint32_t packed);
