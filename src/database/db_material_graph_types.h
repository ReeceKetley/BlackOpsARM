#pragma once
#include <platform/bo1_native_width.h>
#include <cstdint>
#include <cstddef>
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
#if !defined(_WIN32)
using DWORD = std::uint32_t;
#else
using DWORD = unsigned long;
#endif

struct MaterialStreamRouting // sizeof=0x2
{                                                                             // XREF: MaterialVertexStreamRouting/r
                                                                                // ?Load_BuildVertexDecl@@YAXPAPAUMaterialVertexDeclaration@@@Z/r ...
        unsigned __int8 source;                         // XREF: Material_LoadPassVertexDecl+26A/w
                                                                                // Material_LoadPassVertexDecl+277/w
        unsigned __int8 dest;                             // XREF: Material_LoadPassVertexDecl+281/w
};

struct MaterialVertexStreamRouting // sizeof=0x68
{                                                                             // XREF: MaterialVertexDeclaration/r
        MaterialStreamRouting data[16];
        IDirect3DVertexDeclaration9 *decl[18];
};

struct MaterialVertexDeclaration // sizeof=0x6C
{                                                                             // XREF: $54435CF730F84DB67694F69169881761/r
        unsigned __int8 streamCount;
        bool hasOptionalSource;
        bool isLoaded;
        // padding byte
        MaterialVertexStreamRouting routing;
};

struct __declspec(align(4)) GfxVertexShaderLoadDef // sizeof=0x8
{                                                                             // XREF: MaterialVertexShaderProgram/r
        DWORD *program;
        unsigned __int16 programSize;
        // padding byte
        // padding byte
};

struct MaterialVertexShaderProgram // sizeof=0xC
{                                                                             // XREF: MaterialVertexShader/r
        IDirect3DVertexShader9 *vs;
        GfxVertexShaderLoadDef loadDef;
};

struct MaterialVertexShader // sizeof=0x10
{
        const char *name;
        MaterialVertexShaderProgram prog;
};

struct __declspec(align(4)) GfxPixelShaderLoadDef // sizeof=0x8
{                                                                             // XREF: MaterialPixelShaderProgram/r
        DWORD *program;
        unsigned __int16 programSize;
        // padding byte
        // padding byte
};

struct MaterialPixelShaderProgram // sizeof=0xC
{                                                                             // XREF: MaterialPixelShader/r
        IDirect3DPixelShader9 *ps;
        GfxPixelShaderLoadDef loadDef;
};

struct MaterialPixelShader // sizeof=0x10
{
        const char *name;
        MaterialPixelShaderProgram prog;
};

struct MaterialArgumentCodeConst // sizeof=0x4
{                                                                             // XREF: MaterialArgumentDef/r
                                                                                // GfxShaderConstantState::<unnamed_type_fields>/r
        unsigned __int16 index;
        unsigned __int8 firstRow;
        unsigned __int8 rowCount;
};

union MaterialArgumentDef // sizeof=0x4
{                                                                             // XREF: MaterialShaderArgument/r
        const float *literalConst;
        MaterialArgumentCodeConst codeConst;
        unsigned int codeSampler;
        unsigned int nameHash;
};

struct MaterialShaderArgument // sizeof=0x8
{                                                                             // XREF: Material_LoadPass/r
                                                                                // Material_ParseShaderArguments/r
        unsigned __int16 type;
        unsigned __int16 dest;
        MaterialArgumentDef u;
};

struct MaterialPass // sizeof=0x14
{                                                                             // XREF: MaterialTechnique/r
                                                                                // Material_LoadTechnique/r
        MaterialVertexDeclaration *vertexDecl;
                                                                                // XREF: Material_LoadTechnique+3A6/r
        MaterialVertexShader *vertexShader;
        //$DC376F3ECA4A0877231069DAD65BD3EF ___u2;
        union //$DC376F3ECA4A0877231069DAD65BD3EF // sizeof=0x4
        {                                                                             // XREF: MaterialPass/r
            MaterialPixelShader *pixelShader;
            MaterialPixelShader *localPixelShader;
        };
        unsigned __int8 perPrimArgCount;
        unsigned __int8 perObjArgCount;
        unsigned __int8 stableArgCount;
        unsigned __int8 customSamplerFlags;
        //$852269A6FD668DA4A9FB8DA442445F23 ___u7;
        union //$852269A6FD668DA4A9FB8DA442445F23 // sizeof=0x4
        {                                                                             // XREF: MaterialPass/r
            MaterialShaderArgument *localArgs;
            MaterialShaderArgument *args;
        };
};

struct MaterialTechnique // sizeof=0x1C
{
        const char *name;
        unsigned __int16 flags;
        unsigned __int16 passCount;
        MaterialPass passArray[1];
};


static_assert(sizeof(MaterialStreamRouting)==2);
static_assert(sizeof(MaterialArgumentCodeConst)==4);
static_assert(sizeof(MaterialArgumentDef)==(BO1_NATIVE_64BIT?8:4));
static_assert(sizeof(MaterialShaderArgument)==(BO1_NATIVE_64BIT?16:8));
static_assert(offsetof(MaterialShaderArgument,u)==(BO1_NATIVE_64BIT?8:4));
static_assert(sizeof(MaterialVertexDeclaration)==(BO1_NATIVE_64BIT?184:108));
static_assert(offsetof(MaterialVertexDeclaration,routing)==(BO1_NATIVE_64BIT?8:4));
static_assert(offsetof(MaterialVertexStreamRouting,decl)==32);
static_assert(sizeof(GfxVertexShaderLoadDef)==(BO1_NATIVE_64BIT?16:8));
static_assert(sizeof(GfxPixelShaderLoadDef)==(BO1_NATIVE_64BIT?16:8));
static_assert(sizeof(MaterialVertexShaderProgram)==(BO1_NATIVE_64BIT?24:12));
static_assert(sizeof(MaterialPixelShaderProgram)==(BO1_NATIVE_64BIT?24:12));
static_assert(sizeof(MaterialVertexShader)==(BO1_NATIVE_64BIT?32:16));
static_assert(sizeof(MaterialPixelShader)==(BO1_NATIVE_64BIT?32:16));
static_assert(sizeof(MaterialPass)==(BO1_NATIVE_64BIT?40:20));
static_assert(offsetof(MaterialPass,vertexShader)==sizeof(void*));
static_assert(offsetof(MaterialPass,pixelShader)==2*sizeof(void*));
static_assert(offsetof(MaterialPass,perPrimArgCount)==3*sizeof(void*));
static_assert(offsetof(MaterialPass,args)==(BO1_NATIVE_64BIT?32:16));
static_assert(offsetof(MaterialTechnique,passArray)==(BO1_NATIVE_64BIT?16:8));
static_assert(sizeof(MaterialTechnique)==(BO1_NATIVE_64BIT?56:28));
