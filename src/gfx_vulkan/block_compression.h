#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
namespace bo1::vulkan {
enum class BlockCompression { BC1,BC2,BC3 };
// Little-endian block bytes, tightly packed XY block rows and successive Z
// slices. Handles the incomplete edge blocks of small/non-multiple-of-4 mips.
bool DecodeBlocks(BlockCompression,unsigned width,unsigned height,unsigned depth,
    const void* source,std::size_t sourceBytes,std::vector<unsigned char>& rgba,std::string& error);
}
