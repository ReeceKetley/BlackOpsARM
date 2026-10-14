#pragma once
// SDK-compatible declarations consumed by the original material loader.
// These declare dependencies; they do not supply shader compilation/reflection.
#include <d3d9.h>
DEFINE_GUID(IID_ID3DXBuffer, 0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0x0d, 0x98, 0x9c, 0x3a, 0x01, 0x02)

struct ID3DXBuffer : IUnknown {
    virtual void *STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
};
// Only the inherited buffer interface is consumed here. Additional constant-
// table methods must be declared before adding callers of those SDK methods.
struct ID3DXConstantTable : ID3DXBuffer {};
using LPD3DXCONSTANTTABLE = ID3DXConstantTable *;
enum D3DXINCLUDE_TYPE { D3DXINC_LOCAL = 0, D3DXINC_SYSTEM = 1, D3DXINC_FORCE_DWORD = 0x7fffffff };
struct ID3DXInclude {
    virtual HRESULT STDMETHODCALLTYPE Open(D3DXINCLUDE_TYPE type, const char *filename,
        const void *parent, const void **data, UINT *bytes) = 0;
    virtual HRESULT STDMETHODCALLTYPE Close(const void *data) = 0;
};
struct D3DXMACRO { const char *Name; const char *Definition; };
struct _D3DXSEMANTIC { UINT Usage; UINT UsageIndex; };
// DirectX SDK image-save ABI; native JPEG/TGA service owns the implementation.
enum D3DXIMAGE_FILEFORMAT {
    D3DXIFF_BMP=0, D3DXIFF_JPG=1, D3DXIFF_TGA=2, D3DXIFF_PNG=3,
    D3DXIFF_DDS=4, D3DXIFF_PPM=5, D3DXIFF_DIB=6, D3DXIFF_HDR=7,
    D3DXIFF_PFM=8, D3DXIFF_FORCE_DWORD=0x7fffffff
};
struct _D3DXSHADER_CONSTANTTABLE {
    DWORD Size, Creator, Version, Constants, ConstantInfo, Flags, Target;
};
struct _D3DXSHADER_CONSTANTINFO {
    DWORD Name;
    WORD RegisterSet, RegisterIndex, RegisterCount, Reserved;
    DWORD TypeInfo, DefaultValue;
};
static_assert(sizeof(_D3DXSEMANTIC) == 8);
static_assert(sizeof(_D3DXSHADER_CONSTANTTABLE) == 28);
static_assert(sizeof(_D3DXSHADER_CONSTANTINFO) == 20);

extern "C" {
HRESULT WINAPI D3DXSaveSurfaceToFileA(const char *filename,
    D3DXIMAGE_FILEFORMAT format, IDirect3DSurface9 *surface,
    const PALETTEENTRY *palette, const RECT *rect);
HRESULT WINAPI D3DXCreateBuffer(DWORD bytes, ID3DXBuffer **buffer);
HRESULT WINAPI D3DXCompileShader(const char *source, UINT length,
    const D3DXMACRO *defines, ID3DXInclude *include, const char *entry,
    const char *profile, DWORD flags, ID3DXBuffer **shader,
    ID3DXBuffer **diagnostics, ID3DXConstantTable **constants);
HRESULT WINAPI D3DXGetShaderConstantTable(const DWORD *shader, ID3DXConstantTable **constants);
HRESULT WINAPI D3DXGetShaderInputSemantics(const DWORD *shader, _D3DXSEMANTIC *semantics, UINT *count);
HRESULT WINAPI D3DXGetShaderOutputSemantics(const DWORD *shader, _D3DXSEMANTIC *semantics, UINT *count);
}
