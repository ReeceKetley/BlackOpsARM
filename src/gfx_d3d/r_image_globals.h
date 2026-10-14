#pragma once
#include "r_material.h"
#include <cstddef>
// Canonical declaration moved from r_image.cpp; one owner, unchanged fields.
struct ImageGlobals
{
    GfxImage *imageHashTable[2048];
    int picmip;
    int picmipBump;
    int picmipSpec;
    CardMemory totalMemory;
};
extern ImageGlobals imageGlobals;
static_assert(offsetof(ImageGlobals,picmip)==sizeof(void *)*2048);
static_assert(offsetof(ImageGlobals,totalMemory)==sizeof(void *)*2048+12);
static_assert(sizeof(ImageGlobals)==(sizeof(void *)==8?0x4018:0x2014));
