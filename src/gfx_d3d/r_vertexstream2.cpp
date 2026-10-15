#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
#include "gfx_vulkan/buffer_assets.h"
#include "gfx_vulkan/shader_assets.h"
#endif
#include "r_vertexstream2.h"
#include <qcommon/common.h>

#include <cstring>
#include "r_buffers.h"

IDirect3DVertexBuffer9 *stream2VertexBuffer;
unsigned int stream2AllocatedSize;

void __cdecl R_VertexStream2_Init(unsigned int stream2ByteSize)
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if(bo1::vulkan::shader_assets::Active()) {
        if(!stream2ByteSize)return;
        if(stream2AllocatedSize==stream2ByteSize) {
            auto record=bo1::vulkan::buffer_assets::Acquire(&stream2VertexBuffer);
            if(!record||record->gpu->Size()!=stream2ByteSize)Com_Error(ERR_FATAL,"Native stream2 allocation state mismatch");return;
        }
        if(stream2AllocatedSize)R_VertexStream2_Shutdown();
        if(!AllocateVertexStream2Buffer(stream2ByteSize)) {Com_Error(ERR_FATAL,"Native stream2 allocation failed");return;}
        stream2AllocatedSize=stream2ByteSize;R_VertexStream2_Clear();return;
    }
#endif
    float *vbPtr; // [esp+0h] [ebp-8h]

    Com_Printf(8, "vertex stream 2: --------------------------\n");
    Com_Printf(8, "vertex stream 2: needs %u bytes\n", stream2ByteSize);
    if ( stream2AllocatedSize == stream2ByteSize )
    {
        Com_Printf(8, "vertex stream 2: already allocated %u bytes\n", stream2AllocatedSize);
    }
    else if ( stream2ByteSize )
    {
        if ( stream2AllocatedSize )
            R_VertexStream2_Shutdown();
        if ( AllocateVertexStream2Buffer(stream2ByteSize) )
        {
            Com_Printf(8, "vertex stream 2: vertex buffer allocate ok\n");
            vbPtr = (float *)R_LockVertexBuffer(stream2VertexBuffer, 0, 0, 0);
            if ( vbPtr )
            {
                memset((unsigned __int8 *)vbPtr, 0, stream2ByteSize);
                R_UnlockVertexBuffer(stream2VertexBuffer);
            }
            stream2AllocatedSize = stream2ByteSize;
        }
        else
        {
            Com_Printf(8, "vertex stream 2: vertex buffer allocate failed\n");
        }
    }
    Com_Printf(8, "vertex stream 2: --------------------------\n");
}

bool __cdecl AllocateVertexStream2Buffer(unsigned int stream2ByteSize)
{
    R_AllocDynamicVertexBuffer(&stream2VertexBuffer, stream2ByteSize);
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if(bo1::vulkan::shader_assets::Active()) {
        auto record=bo1::vulkan::buffer_assets::Acquire(&stream2VertexBuffer);return record&&record->gpu->Handle()&&record->gpu->Size()==stream2ByteSize;
    }
#endif
    return stream2VertexBuffer != 0;
}

void __cdecl R_VertexStream2_Shutdown()
{
    if ( stream2AllocatedSize )
        FreeVertexStream2Buffer();
    stream2AllocatedSize = 0;
}

void FreeVertexStream2Buffer()
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if(bo1::vulkan::shader_assets::Active()) {bo1::vulkan::buffer_assets::Release(&stream2VertexBuffer);return;}
#endif
    R_FreeStaticVertexBuffer(stream2VertexBuffer);
}

unsigned __int8 *__cdecl R_VertexStream2_Lock(unsigned int *stream2LockedSize)
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if(bo1::vulkan::shader_assets::Active()) {
        if(!stream2LockedSize) {Com_Error(ERR_FATAL,"Native stream2 lock has no size output");return nullptr;}
        *stream2LockedSize=stream2AllocatedSize;if(!stream2AllocatedSize)return nullptr;
        std::string error;auto* mapped=static_cast<unsigned char*>(bo1::vulkan::buffer_assets::MapDiscard(&stream2VertexBuffer,error,true));
        if(!mapped)Com_Error(ERR_FATAL,"Native stream2 lock: %s",error.c_str());return mapped;
    }
#endif
    unsigned __int8 *result; // eax

    if ( stream2AllocatedSize )
    {
        result = (unsigned __int8 *)R_LockVertexBuffer(stream2VertexBuffer, 0, 0, 0);
        *stream2LockedSize = stream2AllocatedSize;
    }
    else
    {
        *stream2LockedSize = 0;
        return 0;
    }
    return result;
}

void __cdecl R_VertexStream2_Unlock()
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if(bo1::vulkan::shader_assets::Active()) {
        if(stream2AllocatedSize)R_FinishStaticVertexBufferFromOwner(&stream2VertexBuffer);return;
    }
#endif
    if ( stream2AllocatedSize )
        R_UnlockVertexBuffer(stream2VertexBuffer);
}

void __cdecl R_VertexStream2_Clear()
{
    unsigned __int8 *stream2Ptr; // [esp+0h] [ebp-8h]
    unsigned int stream2Size; // [esp+4h] [ebp-4h] BYREF

    stream2Ptr = R_VertexStream2_Lock(&stream2Size);
    if ( stream2Ptr )
        memset(stream2Ptr, 0, stream2Size);
    R_VertexStream2_Unlock();
}

IDirect3DVertexBuffer9 **__cdecl R_VertexStream2_GetVB()
{
    return &stream2VertexBuffer;
}

