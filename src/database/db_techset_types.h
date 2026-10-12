#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>
struct MaterialTechnique;
struct MaterialTechniqueSet // sizeof=0x210
{                                                                             // XREF: XAssetPoolEntry<MaterialTechniqueSet>/r
        const char *name;
        unsigned __int8 worldVertFormat;
        unsigned __int8 unused[1];
        unsigned __int16 techsetFlags;
        MaterialTechnique *techniques[130];
};
#if !BO1_NATIVE_64BIT
static_assert(sizeof(MaterialTechniqueSet) == 528);
#endif

static_assert(offsetof(MaterialTechniqueSet, name)==0);
static_assert(offsetof(MaterialTechniqueSet, worldVertFormat)==sizeof(void*));
static_assert(offsetof(MaterialTechniqueSet, techsetFlags)==sizeof(void*)+2);
static_assert(offsetof(MaterialTechniqueSet, techniques)==(BO1_NATIVE_64BIT?16:8));
static_assert(sizeof(MaterialTechniqueSet)==(BO1_NATIVE_64BIT?1056:528));
