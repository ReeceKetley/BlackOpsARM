#pragma once
#include <cstddef>
struct MaterialTechniqueSet;
// Canonical declaration moved from r_material.cpp; one owner, unchanged fields.
struct $5803DA4EE1143C7EE9D14DA3FE11ED2E
{
    int techniqueSetCount;
    MaterialTechniqueSet *techniqueSetHashTable[2048];
};
extern $5803DA4EE1143C7EE9D14DA3FE11ED2E materialGlobals;
static_assert(offsetof($5803DA4EE1143C7EE9D14DA3FE11ED2E,techniqueSetHashTable)==sizeof(void *));
static_assert(sizeof($5803DA4EE1143C7EE9D14DA3FE11ED2E)==sizeof(void *)*2049);
