#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
#include "gfx_vulkan/buffer_assets.h"
#include "gfx_vulkan/shader_assets.h"
#endif
#include "r_xsurface_optimize.h"
#include <qcommon/threads.h>
#include "rb_resource.h"
#include <xanim/xmodel_utils.h>
#include "r_buffers.h"
#include "r_init.h"

void __cdecl XModelOptimizeCallback(XModel *data)
{
    _XModelOptimize(data);
}

#if BO1_NATIVE_64BIT && !defined(_WIN32)
// Resource actions invoke void(void *); preserve the canonical typed payload.
static void __cdecl XModelOptimizeCallbackResource(void *data)
{
    XModelOptimizeCallback(static_cast<XModel *>(data));
}
#endif

void __cdecl XModelOptimize(XModel *model)
{
    if ( XModelBad(model)
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_xsurface_optimize.cpp",
                    118,
                    0,
                    "%s",
                    "!XModelBad( model )") )
    {
        __debugbreak();
    }
    if ( Sys_IsRenderThread() )
    {
        _XModelOptimize(model);
    }
    else
    {
#if BO1_NATIVE_64BIT && !defined(_WIN32)
        RB_Resource_CallbackParam(XModelOptimizeCallbackResource, model);
#else
        RB_Resource_CallbackParam((void (__cdecl *)(void *))XModelOptimizeCallback, model);
#endif
        RB_Resource_Flush();
    }
}

void __cdecl _XModelOptimize(XModel *model)
{
    int lodIndex; // [esp+0h] [ebp-14h]
    int surfIndex; // [esp+4h] [ebp-10h]
    int surfCount; // [esp+8h] [ebp-Ch]
    int lodCount; // [esp+Ch] [ebp-8h]
    XSurface *surfaces; // [esp+10h] [ebp-4h] BYREF

    lodCount = XModelGetNumLods(model);
    for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex )
    {
        surfCount = XModelGetSurfaces(model, &surfaces, lodIndex);
        for ( surfIndex = 0; surfIndex < surfCount; ++surfIndex )
            XSurfaceOptimize(model, &surfaces[surfIndex]);
    }
}

void __cdecl XSurfaceOptimize(XModel *model, XSurface *surf)
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active()) {
        if (!surf || surf->indexBuffer || !surf->triIndices || !surf->triCount) {Com_Error(ERR_FATAL,"Invalid native index surface");return;}
        if (!bo1::vulkan::buffer_assets::Acquire(&surf->indexBuffer)) {
            const std::size_t bytes=std::size_t(surf->triCount)*6,padded=(bytes+31)&~std::size_t(31);std::string error;
            if (padded>0x200000 || !bo1::vulkan::buffer_assets::LoadPacked(&surf->indexBuffer,surf->triIndices,bytes,padded,bo1::vulkan::buffer_assets::Kind::Index16,error)) {
                Com_Error(ERR_FATAL,"Native index surface load: %s",error.c_str());return;
            }
        }
        if (!(surf->flags&0x80))XSurfaceOptimizeRigid(model,surf);return;
    }
#endif
    void *indexBuffer; // [esp+0h] [ebp-Ch]
    int rawIndexBytes; // [esp+4h] [ebp-8h]
    signed int indexBytes; // [esp+8h] [ebp-4h]

    if ( surf->indexBuffer )
        goto LABEL_10;
    if ( !surf->triIndices
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_xsurface_optimize.cpp",
                    71,
                    0,
                    "%s",
                    "surf->triIndices") )
    {
        __debugbreak();
    }
    rawIndexBytes = 6 * surf->triCount;
    indexBytes = (rawIndexBytes + 31) & 0xFFFFFFE0;
    if ( indexBytes > 0x200000
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_xsurface_optimize.cpp",
                    75,
                    0,
                    "%s",
                    "indexBytes <= MATERIAL_MAX_INDEX_BUFFER_SIZE") )
    {
        __debugbreak();
    }
    indexBuffer = R_AllocStaticIndexBuffer(&surf->indexBuffer, indexBytes);
    if ( indexBuffer )
    {
        Com_Memcpy(indexBuffer, surf->triIndices, rawIndexBytes);
        memset((unsigned __int8 *)indexBuffer + rawIndexBytes, 0, indexBytes - rawIndexBytes);
        R_FinishStaticIndexBuffer(surf->indexBuffer);
LABEL_10:
        if ( (surf->flags & 0x80) == 0 )
            XSurfaceOptimizeRigid(model, surf);
    }
}

void __cdecl XSurfaceOptimizeRigid(XModel *model, XSurface *surface)
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active()) {
        if (!surface || (surface->flags&0x80) || surface->vb0 || !surface->verts0 || !surface->vertCount) {Com_Error(ERR_FATAL,"Invalid native rigid surface");return;}
        if (!bo1::vulkan::buffer_assets::Acquire(&surface->vb0)) {
            const std::size_t bytes=std::size_t(surface->vertCount)*32;std::string error;
            if (!bo1::vulkan::buffer_assets::LoadPacked(&surface->vb0,surface->verts0,bytes,bytes,bo1::vulkan::buffer_assets::Kind::Vertex,error))
                Com_Error(ERR_FATAL,"Native rigid surface load: %s",error.c_str());
        }
        return;
    }
#endif
    IDirect3DVertexBuffer9 *vb; // [esp+0h] [ebp-10h] BYREF
    HRESULT hr; // [esp+4h] [ebp-Ch]
    void *vertexBuffer; // [esp+8h] [ebp-8h] BYREF
    int vertexBytes; // [esp+Ch] [ebp-4h]

    if ( (surface->flags & 0x80) != 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_xsurface_optimize.cpp",
                    22,
                    0,
                    "%s",
                    "!(surface->flags & XSURFACE_FLAG_DEFORMED)") )
    {
        __debugbreak();
    }
    if ( !surface->vb0 )
    {
        vertexBytes = 32 * surface->vertCount;
        hr = dx.device->CreateVertexBuffer(vertexBytes, 8u, 0, D3DPOOL_DEFAULT, &vb, 0);
        if ( hr >= 0 )
        {
            //hr = ((int (__thiscall *)(IDirect3DVertexBuffer9 *, IDirect3DVertexBuffer9 *, unsigned int, unsigned int, void **, unsigned int))vb->Lock)(
            //             vb,
            //             vb,
            //             0,
            //             0,
            //             &vertexBuffer,
            //             0);
            hr = vb->Lock(0, 0, &vertexBuffer, 0);
            if ( hr >= 0 )
            {
                surface->vb0 = vb;
                memcpy((unsigned __int8 *)vertexBuffer, (unsigned __int8 *)surface->verts0, vertexBytes);
                R_FinishStaticVertexBuffer(vb);
            }
            else
            {
                vb->Release();
            }
        }
    }
}

void __cdecl XModelUnoptimizeCallback(XModel *data)
{
    XModelUnoptimize(data);
}

#if BO1_NATIVE_64BIT && !defined(_WIN32)
// Resource actions invoke void(void *); preserve the canonical typed payload.
static void __cdecl XModelUnoptimizeCallbackResource(void *data)
{
    XModelUnoptimizeCallback(static_cast<XModel *>(data));
}
#endif

void __cdecl XModelUnoptimize(XModel *model)
{
    int lodIndex; // [esp+0h] [ebp-14h]
    int surfIndex; // [esp+4h] [ebp-10h]
    int surfCount; // [esp+8h] [ebp-Ch]
    int lodCount; // [esp+Ch] [ebp-8h]
    XSurface *surfaces; // [esp+10h] [ebp-4h] BYREF

    if ( Sys_IsRenderThread() )
    {
        if ( XModelBad(model)
            && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_xsurface_optimize.cpp",
                        179,
                        0,
                        "%s",
                        "!XModelBad( model )") )
        {
            __debugbreak();
        }
        lodCount = XModelGetNumLods(model);
        for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex )
        {
            surfCount = XModelGetSurfaces(model, &surfaces, lodIndex);
            for ( surfIndex = 0; surfIndex < surfCount; ++surfIndex )
                XSurfaceUnoptimize(&surfaces[surfIndex]);
        }
    }
    else
    {
#if BO1_NATIVE_64BIT && !defined(_WIN32)
        RB_Resource_CallbackParam(XModelUnoptimizeCallbackResource, model);
#else
        RB_Resource_CallbackParam((void (__cdecl *)(void *))XModelUnoptimizeCallback, model);
#endif
        RB_Resource_Flush();
    }
}

void __cdecl XSurfaceUnoptimize(XSurface *surf)
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active()) {
        if (!surf) {Com_Error(ERR_FATAL,"Invalid native surface unload");return;}
        bo1::vulkan::buffer_assets::Release(&surf->indexBuffer);
        if (!(surf->flags&0x80))bo1::vulkan::buffer_assets::Release(&surf->vb0);return;
    }
#endif
    if ( surf->indexBuffer )
    {
        R_FreeStaticIndexBuffer(surf->indexBuffer);
        surf->indexBuffer = 0;
    }
    if ( (surf->flags & 0x80) == 0 )
    {
        if ( surf->vb0 )
            XSurfaceUnoptimizeRigid(surf);
    }
}

void __cdecl XSurfaceUnoptimizeRigid(XSurface *surf)
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active()) {
        if (!surf || (surf->flags&0x80)) {Com_Error(ERR_FATAL,"Invalid native rigid unload");return;}
        bo1::vulkan::buffer_assets::Release(&surf->vb0);return;
    }
#endif
    if ( (surf->flags & 0x80) != 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_xsurface_optimize.cpp",
                    56,
                    0,
                    "%s",
                    "!(surf->flags & XSURFACE_FLAG_DEFORMED)") )
    {
        __debugbreak();
    }
    if ( !surf->vb0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_xsurface_optimize.cpp",
                    57,
                    0,
                    "%s",
                    "surf->vb0") )
    {
        __debugbreak();
    }
    R_FreeStaticVertexBuffer(surf->vb0);
    surf->vb0 = 0;
}

