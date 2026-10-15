#pragma once
#include "r_rendercmds.h"
#include "rb_state.h"

struct SunShadowMapCallbackUserData
{
    const GfxSunShadowPartition *partition;
    const GfxDrawSurfListInfo *drawList;
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    const GfxBackEndData *data; // native far-partition cache key and viewport
#endif
};

void    R_DrawSunShadowMap(
                const GfxViewInfo *viewInfo,
                unsigned int partitionIndex,
                GfxCmdBuf *cmdBuf);
void __cdecl R_DrawSunShadowMapCallback(const void *userData, GfxCmdBufContext context, GfxCmdBufContext prepassContext);
