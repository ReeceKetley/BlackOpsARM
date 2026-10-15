#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
#include "gfx_vulkan/geometry_bindings.h"
#include "gfx_vulkan/shader_assets.h"
#include <qcommon/common.h>
#endif
#include "r_draw_model_util.h"
#include "rb_shade.h"

void __cdecl R_SetStreamsForXModelSurface(
                const XSurface *localSurf,
                GfxCmdBufPrimState *primState,
                GfxCmdBufPrimState *prepassPrimState)
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active()) {
        std::string error;
        if (!localSurf || localSurf->vb0 || !bo1::vulkan::geometry_bindings::SetVertex(primState,&localSurf->vb0,0,32,error)
            || (prepassPrimState && !bo1::vulkan::geometry_bindings::SetVertex(prepassPrimState,&localSurf->vb0,0,32,error))) {
            Com_Error(ERR_FATAL,"Native model stream binding: %s",error.c_str());return;
        }
        primState->streams[0].vb=nullptr;primState->streams[0].offset=0;primState->streams[0].stride=32;
        primState->streams[1]={};primState->streams[2]={};
        if(prepassPrimState) {
            prepassPrimState->streams[0].vb=nullptr;prepassPrimState->streams[0].offset=0;prepassPrimState->streams[0].stride=32;
            prepassPrimState->streams[1]={};prepassPrimState->streams[2]={};
        }
        return;
    }
#endif
    IDirect3DVertexBuffer9 *vb0; // [esp+0h] [ebp-4h]

    vb0 = localSurf->vb0;
    if ( prepassPrimState )
        R_SetStreamSource(prepassPrimState, vb0, 0, 0x20u);
    R_SetStreamSource(primState, vb0, 0, 0x20u);
}

