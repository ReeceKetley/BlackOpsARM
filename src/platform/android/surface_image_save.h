#pragma once
#include <platform/win32_compat/native_d3dx9_api.h>
#include <cstddef>

// Pixels point to the top row of borrowed BGRA/XRGB storage, with positive
// byte pitch. Storage must remain readable for the complete call.
HRESULT BO1_SaveSurfacePixels(const char *filename,D3DXIMAGE_FILEFORMAT fileFormat,
    D3DFORMAT pixelFormat,const void *pixels,UINT width,UINT height,std::ptrdiff_t pitch);
