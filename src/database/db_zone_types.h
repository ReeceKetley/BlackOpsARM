#pragma once
#include <cstddef>
#include <cstdint>
#include <universal/q_shared.h>

// Runtime block pointers are native-width; FastFile sizes remain 32-bit.
struct XBlock
{
    unsigned __int8 *data;
    unsigned int size;
};

struct XFile
{
    unsigned int size;
    unsigned int externalSize;
    unsigned int blockSize[7];
};
static_assert(sizeof(unsigned int) == 4 && sizeof(XFile) == 36);
static_assert(offsetof(XFile, blockSize) == 8);
static_assert(offsetof(XBlock, size) == sizeof(void *));
