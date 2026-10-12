#pragma once
#include <cstddef>

struct RawFile // sizeof=0xC
{                                       // XREF: XAssetPoolEntry<RawFile>/r
    const char *name;
    int len;
    const char *buffer;
};

static_assert(sizeof(int) == 4, "RawFile length retains four bytes");
static_assert(offsetof(RawFile, name) == 0);
static_assert(offsetof(RawFile, len) == sizeof(void *));
static_assert(offsetof(RawFile, buffer) == (sizeof(void *) == 8 ? 16 : 8));
static_assert(sizeof(RawFile) == (sizeof(void *) == 8 ? 24 : 12));
