#include "r_draw_sunshadow.h"
#include "r_state_utils.h"
#include "r_wind.h"
#include "r_state.h"
#include "r_foliage.h"
#include "r_dvars.h"
#include "rb_draw3d.h"
#include "rb_spotshadow.h"
#if defined(__ANDROID__)
#include "gfx_vulkan/material_dispatch.h"
#include <android/log.h>
#endif
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
#include "r_sunshadow.h"
#include "gfx_vulkan/pass_recording.h"
#include "gfx_vulkan/shader_assets.h"
#include <string>
#include <vector>
namespace {
// Render thread: whether this frame's far partition received the cached static
// casters (restored before the near partition's clear).
bool s_sunShadowFarRestored;
VkRect2D R_SunShadowFarRect(const GfxBackEndData *data)
{
    const GfxViewport &v = data->sunShadow.partition[1].viewport;
    return VkRect2D{ { v.x, v.y + (int)dx.sunShadowmapSize }, { (unsigned)v.width, (unsigned)v.height } };
}
void R_DrawSunShadowSurfsOfKind(GfxCmdBufContext context, const GfxDrawSurfListInfo *drawList, bool dynamic)
{
    thread_local std::vector<GfxDrawSurf> surfs;
    surfs.clear();
    for (unsigned i = 0; i < drawList->drawSurfCount; ++i)
        if ((drawList->drawSurfs[i].fields.surfType >= SF_BMODEL) == dynamic)
            surfs.push_back(drawList->drawSurfs[i]);
    if (surfs.empty())
        return;
    GfxDrawSurfListInfo filtered = *drawList;
    filtered.drawSurfs = surfs.data();
    filtered.drawSurfCount = (unsigned)surfs.size();
    R_DrawSurfs(context, 0, &filtered);
}
}
#endif

void    R_DrawSunShadowMap(
                const GfxViewInfo *viewInfo,
                unsigned int partitionIndex,
                GfxCmdBuf *cmdBuf)
{
    float x; // [esp+20h] [ebp-1ABCh]
    const GfxBackEndData *data; // [esp+2Ch] [ebp-1AB0h]
    GfxCmdBufSourceState state; // [esp+30h] [ebp-1AACh] BYREF

    R_InitCmdBufSourceState(&state, &viewInfo->input, 0);
    R_SetWindShaderConstants(&state);
    R_SetRenderTargetSize(&state, R_RENDERTARGET_SHADOWMAP_SUN);

    data = viewInfo->input.data;
    x = data->sunShadow.partition[partitionIndex].shadowViewParms.projectionMatrix.m[2][2];

    R_UpdateCodeConstant(
        &state,
        CONST_SRC_CODE_SHADOWMAP_POLYGON_OFFSET,
        (sm_polygonOffsetBias->current.value * 0.25) * x,
        sm_polygonOffsetScale->current.value,
        0.0,
        0.0);

    R_SetViewportValues(&state, 0, partitionIndex * dx.sunShadowmapSize, dx.sunShadowmapSize, dx.sunShadowmapSize);

    SunShadowMapCallbackUserData userData;
    userData.partition = &data->sunShadow.partition[partitionIndex];
    userData.drawList = &viewInfo->drawList[DRAWLIST_SUN_SHADOW_MAP0 + partitionIndex];
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    userData.data = data;
#endif
    
    R_DrawCall(
        R_DrawSunShadowMapCallback, 
        &userData,
        &state, 
        viewInfo, 
        (GfxDrawSurfListInfo*)&viewInfo->drawList[DRAWLIST_SUN_SHADOW_MAP0 + partitionIndex], // de-const cast
        &data->sunShadow.partition[partitionIndex].shadowViewParms,
        cmdBuf, 
        0);
}

void __cdecl R_DrawSunShadowMapCallback(const void *userData, GfxCmdBufContext context, GfxCmdBufContext prepassContext)
{
    const GfxDrawSurfListInfo *drawList; // [esp+34h] [ebp-8h]
    const GfxSunShadowPartition *partition; // [esp+38h] [ebp-4h]

    const SunShadowMapCallbackUserData *data = (const SunShadowMapCallbackUserData *)userData;

    partition = data->partition;
    drawList = data->drawList;

    R_SetRenderTarget(context, R_RENDERTARGET_SHADOWMAP_SUN);
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    // Cached far partition: its static casters' depth is copied in, then only
    // the near partition is cleared and dynamic casters are drawn over it.
    const unsigned long long cacheKey = bo1::vulkan::shader_assets::Active() ? R_SunShadowCacheKey(data->data) : 0;
    if ( !partition->partitionIndex )
    {
        s_sunShadowFarRestored = false;
        if ( cacheKey )
        {
            std::string error;
            auto scope = bo1::vulkan::pass_recording::Current();
            if ( !scope || !scope->RestoreSunShadowCache(context.state, R_SunShadowFarRect(data->data), cacheKey, s_sunShadowFarRestored, error) )
                Com_Error(ERR_FATAL, "Native sun shadow cache restore: %s", error.c_str());
        }
        if ( s_sunShadowFarRestored )
        {
            const GfxViewport nearRegion{ 0, 0, (int)dx.sunShadowmapSize, (int)dx.sunShadowmapSize };
            R_ClearScreen(context.state->prim.device, 3u, shadowmapClearColor, 1.0, 0, &nearRegion);
        }
        else
            R_ClearScreen(context.state->prim.device, 3u, shadowmapClearColor, 1.0, 0, 0);
    }
#else
    if ( !partition->partitionIndex )
        R_ClearScreen(context.state->prim.device, 3u, shadowmapClearColor, 1.0, 0, 0);
#endif
    R_HW_EnableScissor(
        context.state->prim.device,
        partition->viewport.x,
        partition->viewport.y + partition->partitionIndex * dx.sunShadowmapSize,
        partition->viewport.width,
        partition->viewport.height);
#if defined(__ANDROID__)
    // Diagnostic: per-partition caster composition and the draws it issues.
    static unsigned diagnosticFrame;
    const bool report = partition->partitionIndex < 2 && (diagnosticFrame++ % 200) < 2;
    const unsigned drawsBefore = bo1::vulkan::material_dispatch::ReadProfile().draws;
#endif
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if ( partition->partitionIndex == 1 && cacheKey )
    {
        if ( !s_sunShadowFarRestored )
        {
            // Rebuild: static casters, keep them, then the dynamic ones on top.
            R_DrawSunShadowSurfsOfKind(context, drawList, false);
            std::string error;
            auto scope = bo1::vulkan::pass_recording::Current();
            if ( !scope || !scope->StoreSunShadowCache(context.state, R_SunShadowFarRect(data->data), cacheKey, error) )
                Com_Error(ERR_FATAL, "Native sun shadow cache store: %s", error.c_str());
        }
        R_DrawSunShadowSurfsOfKind(context, drawList, true);
    }
    else
        R_DrawSurfs(context, 0, drawList);
#else
    R_DrawSurfs(context, 0, drawList);
#endif
#if defined(__ANDROID__)
    if (report)
    {
        unsigned byType[16]{};
        for (unsigned i = 0; i < drawList->drawSurfCount; ++i)
            ++byType[drawList->drawSurfs[i].fields.surfType & 15];
        __android_log_print(ANDROID_LOG_INFO, "BO1NativeVulkan",
            "sun_shadow_partition index=%u restored=%d entries=%u draws=%u bsp=%u bsp_pretess=%u smodel=%u smodel_pretess=%u smodel_cached=%u smodel_skinned=%u bmodel=%u xmodel_rigid=%u xmodel_rigid_skinned=%u xmodel_skinned=%u other=%u",
            partition->partitionIndex,
#if defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
            (int)s_sunShadowFarRestored,
#else
            0,
#endif
            drawList->drawSurfCount,
            bo1::vulkan::material_dispatch::ReadProfile().draws - drawsBefore,
            byType[0], byType[1], byType[2], byType[3], byType[4], byType[5], byType[6], byType[7], byType[8], byType[9],
            byType[10] + byType[11] + byType[12] + byType[13] + byType[14] + byType[15]);
    }
#endif
    R_HW_DisableScissor(context.state->prim.device);
}

