#pragma once
#include <d3d9.h>
#include <cstdint>
#include <cstring>
#include <limits>

namespace bo1::android {
// Caller owns at least width*height*bytesPerPixel output bytes.
// Capture the game's surface; Android has no Win32 desktop/monitor coordinates.
inline bool CaptureBackBuffer(IDirect3DDevice9 *device, IDirect3DSwapChain9 *chain,
    int x, int y, int width, int height, int bytesPerPixel, unsigned char *output) {
    if (!device || !chain || !output || x<0 || y<0 || width<=0 || height<=0 ||
        (bytesPerPixel!=3 && bytesPerPixel!=4)) return false;
    struct Surface {
        IDirect3DSurface9 *value{};
        ~Surface() { if(value) value->Release(); }
    } back, resolved, copy;
    if (FAILED(chain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&back.value))) return false;
    D3DSURFACE_DESC desc{};
    if (FAILED(back.value->GetDesc(&desc))) return false;
    if (desc.Format!=D3DFMT_A8R8G8B8 && desc.Format!=D3DFMT_X8R8G8B8) return false;
    if (std::uint64_t(x)+width>desc.Width || std::uint64_t(y)+height>desc.Height ||
        std::uint64_t(x)+width>INT32_MAX || std::uint64_t(y)+height>INT32_MAX) return false;
    const std::size_t rowBytes=std::size_t(width)*bytesPerPixel;
    if (rowBytes>std::numeric_limits<std::size_t>::max()/std::size_t(height)) return false;
    IDirect3DSurface9 *source=back.value;
    if(desc.MultiSampleType!=D3DMULTISAMPLE_NONE) {
        if(FAILED(device->CreateRenderTarget(desc.Width,desc.Height,desc.Format,
            D3DMULTISAMPLE_NONE,0,FALSE,&resolved.value,nullptr))) return false;
        if(FAILED(device->StretchRect(back.value,nullptr,resolved.value,nullptr,D3DTEXF_NONE))) return false;
        source=resolved.value;
    }
    if(FAILED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,
        D3DPOOL_SYSTEMMEM,&copy.value,nullptr))) return false;
    if(FAILED(device->GetRenderTargetData(source,copy.value))) return false;
    const RECT rect{x,y,x+width,y+height};
    D3DLOCKED_RECT locked{};
    if(FAILED(copy.value->LockRect(&locked,&rect,D3DLOCK_READONLY))) return false;
    if(!locked.pBits || locked.Pitch<0 || std::uint64_t(locked.Pitch)<std::uint64_t(width)*4) {
        copy.value->UnlockRect(); return false;
    }
    const auto *pixels=static_cast<const unsigned char *>(locked.pBits);
    for(int row=0;row<height;++row) {
        auto *destination=output+std::size_t(row)*rowBytes;
        const auto *input=pixels+std::size_t(row)*std::size_t(locked.Pitch);
        if(bytesPerPixel==4) std::memcpy(destination,input,rowBytes);
        else for(int column=0;column<width;++column)
            std::memcpy(destination+std::size_t(column)*3,input+std::size_t(column)*4,3);
    }
    return SUCCEEDED(copy.value->UnlockRect());
}
}
