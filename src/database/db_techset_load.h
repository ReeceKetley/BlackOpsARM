#pragma once
#include "db_techset_types.h"
#include "db_assetlist_types.h"
extern MaterialTechnique *varMaterialTechnique;
extern MaterialTechnique **varMaterialTechniquePtr;
extern MaterialTechniqueSet *varMaterialTechniqueSet;
extern MaterialTechniqueSet **varMaterialTechniqueSetPtr;
void __cdecl Load_MaterialTechnique(bool);
XAsset *__cdecl AllocLoad_FxElemVisStateSample();
void __cdecl Load_MaterialTechniqueSetAsset(XAssetHeader*);
void __cdecl Load_MaterialTechniqueSetPtr(bool);
