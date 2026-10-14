#pragma once
#include <cstddef>
namespace bo1::android {
void *AllocateAligned(std::size_t bytes, std::size_t alignment);
void FreeAligned(void *memory);
}
