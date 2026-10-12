#pragma once
#include "db_material_graph_types.h"
#include "db_stream_load.h"
#include "db_expand.hpp"
#include "db_techset_load.h"
extern GfxVertexShaderLoadDef *varGfxVertexShaderLoadDef;
extern GfxPixelShaderLoadDef *varGfxPixelShaderLoadDef;
extern MaterialVertexShaderProgram *varMaterialVertexShaderProgram;
extern MaterialVertexShader *varMaterialVertexShader;
extern MaterialVertexShader **varMaterialVertexShaderPtr;
extern MaterialPixelShaderProgram *varMaterialPixelShaderProgram;
extern MaterialPixelShader *varMaterialPixelShader;
extern MaterialPixelShader **varMaterialPixelShaderPtr;
extern MaterialVertexDeclaration *varMaterialVertexDeclaration;
extern MaterialArgumentCodeConst *varMaterialArgumentCodeConst;
extern MaterialShaderArgument *varMaterialShaderArgument;
extern MaterialArgumentDef *varMaterialArgumentDef;
extern MaterialVertexDeclaration **varMaterialPass;
extern void *varDWORD;
extern void *varuint;
extern float *varfloat;
void __cdecl Load_DWORDArray(bool,int);
void __cdecl Load_floatArray(bool,int);
void __cdecl Load_uint(bool);
void __cdecl Load_GfxVertexShaderLoadDef(bool);
void __cdecl Load_GfxPixelShaderLoadDef(bool);
void __cdecl Load_MaterialVertexShaderProgram(bool);
void __cdecl Load_MaterialPixelShaderProgram(bool);
void __cdecl Load_MaterialVertexShader(bool);
void __cdecl Load_MaterialVertexShaderPtr(bool);
void __cdecl Load_MaterialPixelShader(bool);
void __cdecl Load_MaterialPixelShaderPtr(bool);
void __cdecl Load_MaterialVertexDeclaration(bool);
void __cdecl Load_MaterialArgumentCodeConst(bool);
void __cdecl Load_MaterialArgumentDef(bool);
void __cdecl Load_MaterialShaderArgument(bool);
void __cdecl Load_MaterialShaderArgumentArray(bool,int);
void __cdecl Load_MaterialPass(bool);
void __cdecl Load_MaterialPassArray(bool,int);
void __cdecl Load_CreateMaterialPixelShader(GfxPixelShaderLoadDef*,MaterialPixelShader*);
void __cdecl Load_CreateMaterialVertexShader(GfxVertexShaderLoadDef*,MaterialVertexShader*);
void __cdecl Load_BuildVertexDecl(MaterialVertexDeclaration**);
