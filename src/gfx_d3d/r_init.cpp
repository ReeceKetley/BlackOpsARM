#include "r_init.h"
#if defined(__ANDROID__) && defined(BO1_NATIVE_FULLSCREEN_TRIANGLE) && BO1_NATIVE_FULLSCREEN_TRIANGLE
#include "platform/android/fullscreen_triangle.h"
#endif
#if defined(__ANDROID__) && defined(BO1_NATIVE_POST_FUSION_TRIAL) && BO1_NATIVE_POST_FUSION_TRIAL
#include "platform/android/post_fusion_trial.h"
#endif
#if defined(BO1_NATIVE_RENDER_CONFIG)
#include <qcommon/common.h>
#include <database/db_registry.h>
#include <gfx_d3d/r_dvars.h>
#include <gfx_d3d/r_rendercmds.h>
#endif
#if !defined(BO1_NATIVE_RENDER_GLOBALS)
#include <win32/win_main.h>
#include <qcommon/common.h>

#if !defined(__ANDROID__)
#include <dxerr.h>
#else
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <platform/android/renderer_window.h>
#include <platform/android/render_worker.h>
#include <platform/android/performance_gpu_overlay.h>
#include <platform/win32_compat/win32_time.h>
#endif
#include <d3d9.h>
#include "rb_backend.h"
#include "r_dvars.h"
#include <database/db_registry.h>
#include "rb_logfile.h"
#include "r_buffers.h"
#include "r_rendertarget.h"
#include "r_model_lighting.h"
#include "r_staticmodelcache.h"
#include "r_utils.h"
#include "r_rendercmds.h"
#include "rb_superflare.h"
#include "rb_sky.h"
#include "rb_corona.h"
#include <win32/win_net.h>
#include <qcommon/threads.h>
#include <win32/win_workercmds.h>
#include "r_draw_method.h"
#include "r_water.h"
#include "r_image.h"
#include "r_font.h"
#include "r_water_load_obj.h"
#include "r_light.h"
#include "r_fog.h"
#include "r_wind.h"
#include "r_water_sim.h"
#include "r_debug.h"
#include "r_caps.h"
#include "rb_state.h"
#include "r_scene.h"
#include "rb_stream.h"
#include "r_stream.h"
#include <universal/com_buildinfo.h>
#include "r_cmds.h"
#include "r_state.h"
#include "r_model.h"
#include "r_vertexstream2.h"
#include "r_foliage.h"
#include "r_skybox.h"
#include "r_cinematic.h"
#include "rb_imagetouch.h"
#include "r_setstate_d3d.h"
#include <cgame/cg_scr_main.h>
#include <universal/com_workercmds.h>
#include <mjpeg/yuv.h>
#include <win32/win_common.h>
#include "rb_resource.h"
#include "r_state_utils.h"
#include "r_hw_nvidia.h"

#endif

#if defined(__ANDROID__) && !defined(BO1_NATIVE_RENDER_GLOBALS)
namespace {
template<class F> auto R_SDLOwner(F operation, bool requireVideo = false) -> decltype(operation()) {
    using Result = decltype(operation());
    struct Call { F *operation; bool requireVideo; Result result{}; } call{&operation,requireVideo};
    if (!bo1::android::RunRendererOwnerTask([](void *opaque) {
        auto &call = *static_cast<Call *>(opaque);
        if (call.requireVideo && !bo1::android::PrepareRendererVideo()) return false;
        call.result = (*call.operation)();
        return true;
    }, &call)) {
        Com_Error(ERR_FATAL, "renderer SDL owner: %s", SDL_GetError());
        std::abort();
    }
    return call.result;
}
bo1::android::RendererWindowSpec R_SDLWindowSpec(const GfxWindowParms &parms) {
    return {Com_GetBuildOfficialNameR(), parms.x, parms.y, parms.displayWidth,
        parms.displayHeight, parms.hz, parms.fullscreen != 0, parms.borderless != 0, false};
}
void R_SDLWindowFailure(const char *operation) {
    Com_Error(ERR_FATAL, "renderer %s: %s", operation, SDL_GetError());
    std::abort();
}
}
#endif

#if defined(__ANDROID__) && !defined(BO1_NATIVE_RENDER_GLOBALS)
#define R_WSI_CALL(expression) R_SDLOwner([&] { return (expression); }, true)
#else
#define R_WSI_CALL(expression) (expression)
#endif

int g_destroy_window;

GfxConfiguration gfxCfg;
vidConfig_t vidConfig;
DxGlobals dx;
r_global_permanent_t rgp;
r_globals_t rg;
GfxMetrics gfxMetrics;
GfxGlobals r_glob;
GfxWorldDpvsStatic *g_worldDpvs;
GfxWorldDraw *g_worldDraw;
GfxWorldDpvsPlanes *g_worldDpvsPlanes;

bool g_allocateMinimalResources;
GfxAssets gfxAssets;

const dvar_t *r_mode;
const dvar_t *r_displayRefresh;

int g_disableRendering;

#if !defined(BO1_NATIVE_RENDER_GLOBALS)






void __cdecl R_SetColorMappings()
{
    GfxGammaRamp gammaRamp; // [esp+0h] [ebp-208h] BYREF

    if ( vidConfig.deviceSupportsGamma )
    {
        R_CalcGammaRamp(&gammaRamp);
        RB_SetGammaRamp(&gammaRamp);
    }
}

void __cdecl R_CalcGammaRamp(GfxGammaRamp *gammaRamp)
{
    float unitScaleValue; // [esp+8h] [ebp-30h]
    float v2; // [esp+Ch] [ebp-2Ch]
    float v3; // [esp+18h] [ebp-20h]
    unsigned __int16 adjustedColorValue; // [esp+28h] [ebp-10h]
    unsigned __int16 colorTableIndex; // [esp+2Ch] [ebp-Ch]
    float exponent; // [esp+30h] [ebp-8h]

    iassert(gammaRamp);
    iassert(r_gamma->current.value > 0);
    exponent = 1.0 / r_gamma->current.value;
    for (colorTableIndex = 0; colorTableIndex < 0x100u; ++colorTableIndex)
    {
        if (exponent == 1.0)
        {
            adjustedColorValue = 257 * colorTableIndex;
        }
        else
        {
            // KB decompilation fix (MP and zombies), SP 0x006b6e04..0x006b6e45: (float)i * (float)(1/255) (mulss
            // [0x00A0ACD0]), the double CRT pow of the two floats, then * 65535.0f rounded (fadd qword [0x009D3F88];
            // fistp at 0x006b6e45) - not i / 255.0, powf and a truncation.
            v2 = (float)colorTableIndex * 0.0039215689f;
            unitScaleValue = (float)pow((double)v2, (double)exponent);
            iassert(unitScaleValue >= 0 && unitScaleValue < 1 + 0.5f / 65535);
            adjustedColorValue = I_RoundBiasToInt((float)(unitScaleValue * 65535.0f));
        }
        gammaRamp->entries[colorTableIndex] = adjustedColorValue;
    }
}

#endif // normal renderer bodies before shared configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS) || defined(BO1_NATIVE_RENDER_CONFIG)
void __cdecl R_MakeDedicated(const GfxConfiguration *config)
{
    SetGfxConfig(config);
    if ( !r_loadForRenderer )
        R_RegisterDvars();
    Dvar_SetBool((dvar_s *)r_loadForRenderer, 0);
    Dvar_MakeLatchedValueCurrent((dvar_s *)r_loadForRenderer);
    DB_LoadGraphicsAssetsForPC();
}

void __cdecl SetGfxConfig(const GfxConfiguration *config)
{
    const char *unitScaleValue; // eax

    if ( !config && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 1192, 0, "%s", "config") )
        __debugbreak();
    if ( (!config->maxClientViews || config->maxClientViews > 4)
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    1193,
                    0,
                    "config->maxClientViews not in [1, GFX_MAX_CLIENT_VIEWS]\n\t%i not in [%i, %i]",
                    config->maxClientViews,
                    1,
                    4) )
    {
        __debugbreak();
    }
    if ( config->critSectCount != 75 )
    {
        unitScaleValue = va("%d != %d", config->critSectCount, 75);
        if ( !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                        1195,
                        0,
                        "%s\n\t%s",
                        "config->critSectCount == CRITSECT_COUNT",
                        unitScaleValue) )
            __debugbreak();
    }
    memcpy(&gfxCfg, config, sizeof(gfxCfg));
}

#endif // shared renderer configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS)
void __cdecl R_ReleaseForShutdownOrReset()
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_POST_FUSION_TRIAL) && BO1_NATIVE_POST_FUSION_TRIAL
    bo1::android::post_fusion_trial::Release();
#endif
#if defined(__ANDROID__) && defined(BO1_NATIVE_FULLSCREEN_TRIANGLE) && BO1_NATIVE_FULLSCREEN_TRIANGLE
    bo1::android::fullscreen_triangle::Release();
#endif
#if defined(__ANDROID__)
    bo1::android::performance::gpu::Release();
#endif
    IDirect3DSurface9 *v0; // [esp+0h] [ebp-24h]
    int i; // [esp+4h] [ebp-20h]
    int gpuIdx; // [esp+8h] [ebp-1Ch]
    IDirect3DSurface9 *v3; // [esp+Ch] [ebp-18h]
    IDirect3DSurface9 *var; // [esp+10h] [ebp-14h]
    IDirect3DSwapChain9 *varCopy; // [esp+14h] [ebp-10h]
    unsigned int fenceIter; // [esp+18h] [ebp-Ch]
    int windowIndex; // [esp+20h] [ebp-4h]

    for (windowIndex = 0; windowIndex < dx.windowCount; ++windowIndex)
    {
        do
        {
            if (r_logFile)
            {
                if (r_logFile->current.integer)
                    RB_LogPrint("dx.windows[windowIndex].swapChain->Release()\n");
            }
            varCopy = dx.windows[windowIndex].swapChain;
            dx.windows[windowIndex].swapChain = 0;
#if defined(__ANDROID__)
            iassert(varCopy);
            auto remaining=R_SDLOwner([&] { return varCopy->Release(); });
            iassert(!remaining);
#else
            R_ReleaseAndSetNULL<IDirect3DIndexBuffer9>(
                (IDirect3DSurface9 *)varCopy,
                "dx.windows[windowIndex].swapChain",
                "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                1374);
#endif
        } while (alwaysfails);
    }

    R_ShutdownRenderTargets();
    R_ShutdownModelLightingImage();
    R_ShutdownStaticModelCache();
    R_DestroyDynamicBuffers();
    R_DestroyParticleCloudBuffer();

    if (!g_allocateMinimalResources)
        R_ShutdownRenderBuffers();

    iassert(!gfxBuf.smodelCacheVb);

    if (dx.flushGpuQuery)
    {
        do
        {
            if (r_logFile && r_logFile->current.integer)
                RB_LogPrint("dx.flushGpuQuery->Release()\n");
#if defined(__ANDROID__)
            auto *query=dx.flushGpuQuery;
            dx.flushGpuQuery=nullptr;
            iassert(query);
            auto remaining=query->Release();
            iassert(!remaining);
#else
            var = (IDirect3DSurface9 *)dx.flushGpuQuery;
            dx.flushGpuQuery = 0;
            R_ReleaseAndSetNULL<IDirect3DIndexBuffer9>(
                var,
                "dx.flushGpuQuery",
                "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                1392);
#endif
        } while (alwaysfails);
    }
    for (fenceIter = 0; fenceIter < ARRAY_COUNT(dx.fencePool); ++fenceIter)
    {
        if (dx.fencePool[fenceIter])
        {
            do
            {
                if (r_logFile && r_logFile->current.integer)
                    RB_LogPrint("dx.fencePool[fenceIter]->Release()\n");
#if defined(__ANDROID__)
                auto *query=dx.fencePool[fenceIter];
                dx.fencePool[fenceIter]=nullptr;
                iassert(query);
                auto remaining=query->Release();
                iassert(!remaining);
#else
                v3 = (IDirect3DSurface9 *)dx.fencePool[fenceIter];
                dx.fencePool[fenceIter] = 0;
                R_ReleaseAndSetNULL<IDirect3DIndexBuffer9>(
                    v3,
                    "dx.fencePool[fenceIter]",
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    1398);
#endif
            } while (alwaysfails);
        }
    }

    for (gpuIdx = 0; gpuIdx < ARRAY_COUNT(dx.swapFence); ++gpuIdx)
    {
        if (dx.swapFence[gpuIdx])
            dx.swapFence[gpuIdx] = NULL;
    }

    RB_FreeSuperFlareQueries();
    RB_FreeSunSpriteQueries();
    RB_FreeCoronaSpriteQueries();
    for (i = 0; i < 4; ++i)
    {
        if (gfxAssets.pixelCountQuery[i])
        {
            do
            {
                if (r_logFile && r_logFile->current.integer)
                    RB_LogPrint("gfxAssets.pixelCountQuery[gpuIdx]->Release()\n");
#if defined(__ANDROID__)
                auto *query=gfxAssets.pixelCountQuery[i];
                gfxAssets.pixelCountQuery[i]=nullptr;
                iassert(query);
                auto remaining=query->Release();
                iassert(!remaining);
#else
                v0 = (IDirect3DSurface9 *)gfxAssets.pixelCountQuery[i];
                gfxAssets.pixelCountQuery[i] = 0;
                R_ReleaseAndSetNULL<IDirect3DIndexBuffer9>(
                    v0,
                    "gfxAssets.pixelCountQuery[gpuIdx]",
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    1426);
#endif
            } while (alwaysfails);
        }
    }
}

void __cdecl R_UpdateGpuSyncType()
{
    // k1: the PC exes only ever store 0 here, whatever r_gpuSync says (SP 0x006B78BA in R_CreateDevice and 0x006D78E7
    // after R_GpuSyncModified; MP 0x006D929C / 0x006F5DF9 are the only writes to dx.gpuSync), so RB_EndSceneRendering
    // never waits on the GPU fence. KB used r_gpuSync (default "adaptive"), which made the backend spin every frame.
    dx.gpuSync = 0;
}

char __cdecl R_CreateDevice(const GfxWindowParms *wndParms)
{
    const char *v1; // eax
    _D3DPRESENT_PARAMETERS_ d3dpp; // [esp+0h] [ebp-44h] BYREF
    HWND__ *hwnd; // [esp+38h] [ebp-Ch]
    HRESULT hr; // [esp+3Ch] [ebp-8h]
    unsigned int behavior; // [esp+40h] [ebp-4h]

    hwnd = 0;
    if (!wndParms
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2006, 0, "%s", "wndParms"))
    {
        __debugbreak();
    }
    if (dx.windowCount
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2008, 0, "%s", "dx.windowCount == 0"))
    {
        __debugbreak();
    }
    if (!wndParms->hwnd
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2009, 0, "%s", "wndParms->hwnd"))
    {
        __debugbreak();
    }
    hwnd = wndParms->hwnd;
    if (dx.device
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2017, 0, "%s", "dx.device == NULL"))
    {
        __debugbreak();
    }
    dx.depthStencilFormat = (D3DFORMAT)R_GetDepthStencilFormat(D3DFMT_A8R8G8B8);
    R_SetD3DPresentParameters(&d3dpp, wndParms);
    // r_multithreaded_device is latched; apply any pending value here, at the
    // one place behavior flags can actually change (device CREATION — a Reset keeps
    // the old flags, so the resize path must NOT make it current).
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_multithreaded_device);
    behavior = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    if (r_multithreaded_device->current.enabled)
        behavior |= D3DCREATE_MULTITHREADED;

    hr = R_CreateDeviceInternal(hwnd, behavior, &d3dpp);
    if (hr >= 0)
    {
        if (!dx.device
            && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2049, 1, "%s", "dx.device"))
        {
            __debugbreak();
        }
        return 1;
    }
    else
    {
        v1 = R_ErrorDescription(hr);
        Com_Printf(8, "Couldn't create a Direct3D device: %s\n", v1);
        return 0;
    }
}

void __cdecl R_SetD3DPresentParameters(_D3DPRESENT_PARAMETERS_ *d3dpp, const GfxWindowParms *wndParms)
{
    if ( !d3dpp && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 389, 0, "%s", "d3dpp") )
        __debugbreak();
    if ( !wndParms
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 390, 0, "%s", "wndParms") )
    {
        __debugbreak();
    }
    R_SetupAntiAliasing(wndParms);
    memset((unsigned __int8 *)d3dpp, 0, sizeof(_D3DPRESENT_PARAMETERS_));
    d3dpp->BackBufferHeight = wndParms->displayHeight;
    d3dpp->BackBufferWidth = wndParms->displayWidth;
    d3dpp->BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp->BackBufferCount = 1;
    d3dpp->MultiSampleType = dx.multiSampleType;
    d3dpp->MultiSampleQuality = dx.multiSampleQuality;
    d3dpp->SwapEffect = D3DSWAPEFFECT_DISCARD;
    // zombies: a headless client screenshots its back buffer after Present (r_screenshot.cpp); COPY keeps it
    // (COPY allows no multisampling: headless forces r_aaSamples 1)
    if ( Sys_IsHeadlessClient() && !wndParms->fullscreen && dx.multiSampleType == D3DMULTISAMPLE_NONE )
        d3dpp->SwapEffect = D3DSWAPEFFECT_COPY;
    d3dpp->EnableAutoDepthStencil = 0;
    d3dpp->AutoDepthStencilFormat = dx.depthStencilFormat;
    d3dpp->PresentationInterval = r_vsync->current.enabled ? 1 : 0x80000000;
    if ( !wndParms->hwnd
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 425, 0, "%s", "wndParms->hwnd") )
    {
        __debugbreak();
    }
    d3dpp->hDeviceWindow = wndParms->hwnd;
    d3dpp->Flags = 0;
    if ( wndParms->fullscreen )
    {
        d3dpp->Windowed = 0;
        d3dpp->FullScreen_RefreshRateInHz = wndParms->hz;
    }
    else
    {
        d3dpp->Windowed = 1;
        d3dpp->FullScreen_RefreshRateInHz = 0;
    }
}

void __cdecl R_SetupAntiAliasing(const GfxWindowParms *wndParms)
{
    _D3DMULTISAMPLE_TYPE multiSampleCount; // [esp+0h] [ebp-Ch]
    DWORD qualityLevels; // [esp+8h] [ebp-4h] BYREF

    if ( !wndParms
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 295, 0, "%s", "wndParms") )
    {
        __debugbreak();
    }
    if ( (wndParms->aaSamples < 1 || wndParms->aaSamples > 16)
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    309,
                    0,
                    "wndParms->aaSamples not in [1, 16]\n\t%i not in [%i, %i]",
                    wndParms->aaSamples,
                    1,
                    16) )
    {
        __debugbreak();
    }
    if ( r_reflectionProbeGenerate->current.enabled )
        multiSampleCount = D3DMULTISAMPLE_NONMASKABLE;
    else
        multiSampleCount = (_D3DMULTISAMPLE_TYPE)wndParms->aaSamples;
    while ( multiSampleCount > D3DMULTISAMPLE_NONMASKABLE )
    {
        dx.multiSampleType = multiSampleCount;
        //if ( ((int (__thiscall *)(IDirect3D9 *, IDirect3D9 *, unsigned int, int, int, bool, _D3DMULTISAMPLE_TYPE, unsigned int *))dx.d3d9->CheckDeviceMultiSampleType)(
        //             dx.d3d9,
        //             dx.d3d9,
        //             0,
        //             1,
        //             21,
        //             !wndParms->fullscreen,
        //             multiSampleCount,
        //             &qualityLevels) >= 0 )
        if (dx.d3d9->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, !wndParms->fullscreen, multiSampleCount, &qualityLevels) >= 0)
        {
            Com_Printf(8, "Using %ix anti-aliasing\n", multiSampleCount);
            dx.multiSampleQuality = 0;
            return;
        }

        //--multiSampleCount;
        multiSampleCount = (_D3DMULTISAMPLE_TYPE)((int)multiSampleCount - 1);
    }
    dx.multiSampleType = D3DMULTISAMPLE_NONE;
    dx.multiSampleQuality = 0;
}

HRESULT __cdecl R_CreateDeviceInternal(HWND__ *hwnd, unsigned int behavior, _D3DPRESENT_PARAMETERS_ *d3dpp)
{
    int DeviceType; // eax
    unsigned __int8 res; // [esp+3h] [ebp-21h] BYREF
    HRESULT getModeSuccessCode; // [esp+4h] [ebp-20h]
    _D3DDISPLAYMODE getModeResult; // [esp+8h] [ebp-1Ch] BYREF
    HRESULT hr; // [esp+1Ch] [ebp-8h]
    int attempt; // [esp+20h] [ebp-4h]

    Com_Printf(8, "Creating Direct3D device...\n");
    attempt = 0;
    while ( 1 )
    {
        dx.adapterNativeIsValid = R_GetMonitorDimensions(&dx.adapterNativeWidth, &dx.adapterNativeHeight);
        d3dpp->hDeviceWindow = 0;
        DeviceType = R_GetDeviceType();
        //hr = ((int (__thiscall *)(IDirect3D9 *, IDirect3D9 *, unsigned int, int, HWND__ *, unsigned int, _D3DPRESENT_PARAMETERS_ *, IDirect3DDevice9 **))dx.d3d9->CreateDevice)(
        //             dx.d3d9,
        //             dx.d3d9,
        //             dx.adapterIndex,
        //             DeviceType,
        //             hwnd,
        //             behavior,
        //             d3dpp,
        //             &dx.device);
        if ( Sys_IsHeadlessClient() ) // zombies: a 9Ex device (see R_PreCreateWindow); windowed, so no display mode
            hr = R_WSI_CALL(((IDirect3D9Ex *)dx.d3d9)->CreateDeviceEx(dx.adapterIndex, (D3DDEVTYPE)DeviceType, hwnd, behavior, d3dpp, NULL, (IDirect3DDevice9Ex **)&dx.device));
        else
        hr = R_WSI_CALL(dx.d3d9->CreateDevice(dx.adapterIndex, (D3DDEVTYPE)DeviceType, hwnd, behavior, d3dpp, &dx.device));
        if ( hr >= 0 )
            break;
        #if defined(__ANDROID__)
        bo1::win32::Sleep(0x64u);
#else
        Sleep(0x64u);
#endif
        if ( hr != -2005530520 )
            ++attempt;
        if ( attempt == 20 )
        {
            if ( !dx.adapterIndex )
                return hr;
            dx.adapterIndex = 0;
            return R_CreateDeviceInternal(hwnd, behavior, d3dpp);
        }
    }
    dx.nvStereoActivated = 0;
    dx.nvStereoHandle = 0;
#if !defined(__ANDROID__)
    if ( !NvAPI_Stereo_CreateHandleFromIUnknown(dx.device, &dx.nvStereoHandle) )
    {
        NvAPI_Stereo_IsActivated(dx.nvStereoHandle, &res);
        dx.nvStereoActivated = res != 0;
    }
#endif
    getModeSuccessCode = R_WSI_CALL(dx.d3d9->GetAdapterDisplayMode(dx.adapterIndex, &getModeResult));
    if ( getModeSuccessCode < 0 )
    {
        dx.adapterFullscreenWidth = d3dpp->BackBufferWidth;
        dx.adapterFullscreenHeight = d3dpp->BackBufferHeight;
    }
    else
    {
        dx.adapterFullscreenWidth = getModeResult.Width;
        dx.adapterFullscreenHeight = getModeResult.Height;
    }
    return hr;
}

bool __cdecl R_GetMonitorDimensions(int *width, int *height)
{
#if defined(__ANDROID__)
    if (!width || !height || !dx.d3d9) return false;
    auto monitor = R_SDLOwner([&] { return dx.d3d9->GetAdapterMonitor(dx.adapterIndex); });
    intptr_t encoded = reinterpret_cast<intptr_t>(monitor);
    bo1::android::RendererDisplay display{};
    if (encoded <= 0 || encoded - 1 > INT32_MAX || !bo1::android::GetRendererDisplay(static_cast<int>(encoded - 1), &display)) {
        *width = *height = 0; return false;
    }
    *width = display.width; *height = display.height; return true;
#else

    tagMONITORINFO mi; // [esp+0h] [ebp-2Ch] BYREF
    HMONITOR__ *adapterMonitor; // [esp+28h] [ebp-4h]

    adapterMonitor = dx.d3d9->GetAdapterMonitor(dx.adapterIndex);
    mi.cbSize = 40;
    if ( GetMonitorInfoA(adapterMonitor, &mi) )
    {
        *width = mi.rcMonitor.right - mi.rcMonitor.left;
        *height = mi.rcMonitor.bottom - mi.rcMonitor.top;
        return 1;
    }
    else
    {
        *width = GetSystemMetrics(0);
        *height = GetSystemMetrics(1);
        return *width > 0 && *height > 0;
    }

#endif
}

int __cdecl R_GetDeviceType()
{
    unsigned int v0; // eax
    _D3DADAPTER_IDENTIFIER9 id; // [esp+4h] [ebp-458h] BYREF
    unsigned int Adapter; // [esp+458h] [ebp-4h]

    dx.adapterIndex = 0;
    for ( Adapter = 0; ; ++Adapter )
    {
        v0 = R_WSI_CALL(dx.d3d9->GetAdapterCount());
        if ( Adapter >= v0 )
            break;
        if ( dx.d3d9->GetAdapterIdentifier(Adapter, 0, &id) >= 0 )
        {
            ;
            if (strstr(id.Description, "PerfHUD"))
            {
                dx.adapterIndex = Adapter;
                return 2;
            }
        }
    }
    return 1;
}

// borderless: BO1Zombies's own full-screen borderless mode (L11; neither BO1 exe has one). r_fullscreen 0 +
// r_borderless 1: a WS_POPUP window without a frame over the whole monitor at vid_xpos/vid_ypos (else the primary), the
// back buffer at the monitor's size (r_mode / r_customMode ignored), D3D windowed. A windowed device is not lost when
// the window loses focus (Alt+Tab, a click on another monitor) and the display mode never changes. r_fullscreen 1 is
// still exclusive full screen; r_fullscreen 0 + r_borderless 0 the normal window.
static bool s_borderlessWindow; // borderless: the game window is the borderless one now
static int s_borderlessDpiAware = -1; // borderless: -1 not tried, else the result of R_BorderlessMakeDpiAware

bool __cdecl R_IsBorderlessWindow()
{
    return s_borderlessWindow;
}

// borderless: what R_SetWndParms will pick (the headless client never goes exclusive, see there)
static bool R_BorderlessWanted()
{
    if ( !r_borderless || !r_borderless->current.enabled )
        return false;
    return !Dvar_GetBool("r_fullscreen") || Sys_IsHeadlessClient();
}

// borderless: 4K monitors often run at 200% scaling. A DPI-unaware process sees each one as 1920x1080 and the DWM
// stretches its window, so the borderless back buffer would hold a quarter of the monitor's pixels. The process
// becomes per-monitor DPI aware (v2, else v1, else system aware) before the game window exists, and only when that
// window is created borderless, so the r_fullscreen 0/1 paths keep their DPI-unaware behaviour. A window switched to
// borderless later by vid_restart keeps the awareness it was created with (R_BorderlessLogWindow prints it).
static void R_BorderlessMakeDpiAware()
{
#if defined(__ANDROID__)
    // Windows process DPI awareness has no Android counterpart. SDL drawable
    // dimensions and actual display DPI are queried instead.
    Com_Printf(8, "borderless: Android uses SDL drawable sizing\n");
#else

    typedef BOOL(WINAPI * SetProcessDpiAwarenessContextFn)(HANDLE value);
    typedef BOOL(WINAPI * SetProcessDPIAwareFn)();
    HMODULE user32;
    SetProcessDpiAwarenessContextFn setContext;
    SetProcessDPIAwareFn setAware;
    const char *how;

    if ( s_borderlessDpiAware >= 0 )
        return;
    s_borderlessDpiAware = 0;
    how = "none";
    user32 = GetModuleHandleA("user32.dll");
    setContext = user32 ? (SetProcessDpiAwarenessContextFn)GetProcAddress(user32, "SetProcessDpiAwarenessContext") : 0;
    setAware = user32 ? (SetProcessDPIAwareFn)GetProcAddress(user32, "SetProcessDPIAware") : 0;
    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE = -3 (Windows 10 1607+)
    if ( setContext && setContext((HANDLE)-4) )
        how = "per-monitor v2";
    else if ( setContext && setContext((HANDLE)-3) )
        how = "per-monitor";
    else if ( setAware && setAware() )
        how = "system";
    s_borderlessDpiAware = IsProcessDPIAware() ? 1 : 0;
    Com_Printf(8, "borderless: process DPI awareness set: %s (DPI aware %i)\n", how, s_borderlessDpiAware);

#endif
}

// borderless: the DPI awareness of a window (0 unaware, 1 system, 2 per monitor; -1 unknown before Windows 10 1607)
static int R_BorderlessWindowDpiAwareness(HWND hwnd)
{
#if defined(__ANDROID__)
    return -1;
#else

    typedef HANDLE(WINAPI * GetWindowDpiAwarenessContextFn)(HWND hwnd);
    typedef int(WINAPI * GetAwarenessFromDpiAwarenessContextFn)(HANDLE value);
    HMODULE user32;
    GetWindowDpiAwarenessContextFn getContext;
    GetAwarenessFromDpiAwarenessContextFn getAwareness;

    user32 = GetModuleHandleA("user32.dll");
    getContext = user32 ? (GetWindowDpiAwarenessContextFn)GetProcAddress(user32, "GetWindowDpiAwarenessContext") : 0;
    getAwareness = user32
        ? (GetAwarenessFromDpiAwarenessContextFn)GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext")
        : 0;
    if ( !getContext || !getAwareness )
        return -1;
    return getAwareness(getContext(hwnd));

#endif
}

// borderless: L21 - a borderless run behaves as if r_mode were the back buffer's (the monitor's) size. vidConfig
// already follows the window parms; this makes the r_mode dvar say the same, so the video settings menu shows the
// mode actually drawn and applying it from there does not ask for a different one. Set internally (current and
// latched together, so no vid_restart is queued), and only when the adapter lists that mode.
static void R_BorderlessSyncMode(int width, int height)
{
    char modeText[32];
    int modeIndex;

    if ( !r_mode )
        return;
    Com_sprintf(modeText, sizeof(modeText), "%ix%i", width, height);
    if ( !I_stricmp(Dvar_EnumToString(r_mode), modeText) )
        return;
    for ( modeIndex = 0; dx.resolutionNameTable[modeIndex]; ++modeIndex )
    {
        if ( !I_stricmp(dx.resolutionNameTable[modeIndex], modeText) )
        {
            Com_Printf(8, "borderless: r_mode %s -> %s (the back buffer's size)\n", Dvar_EnumToString(r_mode), modeText);
            Dvar_SetInt((dvar_s *)r_mode, modeIndex);
            return;
        }
    }
    Com_Printf(8, "borderless: r_mode %s kept: the adapter lists no %s mode\n", Dvar_EnumToString(r_mode), modeText);
}

// borderless: the window parms - the monitor at (vid_xpos, vid_ypos), else the primary; its full rect, no frame
static void R_BorderlessSetWndParms(GfxWindowParms *wndParms)
{
#if defined(__ANDROID__)
    bo1::android::RendererDisplay display{};
    if (!bo1::android::ChooseRendererDisplay(0, Dvar_GetInt("vid_xpos"), Dvar_GetInt("vid_ypos"), &display))
        R_SDLWindowFailure("borderless display selection");
    wndParms->displayWidth = wndParms->sceneWidth = display.width;
    wndParms->displayHeight = wndParms->sceneHeight = display.height;
    R_BorderlessSyncMode(display.width, display.height);
    wndParms->hz = display.refreshRate;
    wndParms->x = display.x; wndParms->y = display.y;
    wndParms->hwnd = 0; wndParms->aaSamples = r_aaSamples->current.integer;
#else

    POINT pt;
    MONITORINFO mi;

    pt.x = Dvar_GetInt("vid_xpos");
    pt.y = Dvar_GetInt("vid_ypos");
    mi.cbSize = sizeof(mi);
    if ( !GetMonitorInfoA(MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY), &mi) )
    {
        mi.rcMonitor.left = 0;
        mi.rcMonitor.top = 0;
        mi.rcMonitor.right = GetSystemMetrics(SM_CXSCREEN);
        mi.rcMonitor.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    wndParms->displayWidth = mi.rcMonitor.right - mi.rcMonitor.left;
    wndParms->displayHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
    wndParms->sceneWidth = wndParms->displayWidth;
    wndParms->sceneHeight = wndParms->displayHeight;
    R_BorderlessSyncMode(wndParms->displayWidth, wndParms->displayHeight);
    wndParms->hz = 60;
    wndParms->x = mi.rcMonitor.left;
    wndParms->y = mi.rcMonitor.top;
    wndParms->hwnd = 0;
    wndParms->aaSamples = r_aaSamples->current.integer;

#endif
}

// borderless: one line of evidence - the window's style, its client rect on screen against its monitor's rect, the back
// buffer, whether it is minimized, and the DPI awareness. Only for the borderless window.
void __cdecl R_BorderlessLogWindow(const char *when)
{
#if defined(__ANDROID__)
    if (!s_borderlessWindow || !dx.windows[0].hwnd) return;
    bo1::android::RendererWindowInfo window{};
    bo1::android::RendererDisplay display{};
    if (!bo1::android::InspectRendererWindow(reinterpret_cast<SDL_Window *>(dx.windows[0].hwnd), &window)
        || !bo1::android::GetRendererDisplay(window.displayIndex, &display)) {
        Com_Printf(8, "borderless %s: actual SDL query failed: %s\n", when, SDL_GetError()); return;
    }
    Com_Printf(8, "borderless %s: SDL flags 0x%08x logical (%i,%i) %ix%i drawable %ix%i display (%i,%i) %ix%i dpi %.1f back buffer %ix%i\n",
        when, window.flags, window.x, window.y, window.width, window.height,
        window.drawableWidth, window.drawableHeight, display.x, display.y, display.width,
        display.height, display.dpi, dx.windows[0].width, dx.windows[0].height);
#else

    HWND hwnd;
    RECT client;
    POINT origin;
    MONITORINFO mi;
    int match;

    hwnd = dx.windows[0].hwnd;
    if ( !s_borderlessWindow || !hwnd )
        return;
    GetClientRect(hwnd, &client);
    origin.x = 0;
    origin.y = 0;
    ClientToScreen(hwnd, &origin);
    mi.cbSize = sizeof(mi);
    if ( !GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi) )
        memset(&mi.rcMonitor, 0, sizeof(mi.rcMonitor));
    match = origin.x == mi.rcMonitor.left && origin.y == mi.rcMonitor.top
        && client.right - client.left == mi.rcMonitor.right - mi.rcMonitor.left
        && client.bottom - client.top == mi.rcMonitor.bottom - mi.rcMonitor.top
        && dx.windows[0].width == client.right - client.left && dx.windows[0].height == client.bottom - client.top;
    Com_Printf(
        8,
        "borderless: %s: style 0x%08x exstyle 0x%08x client (%i,%i) %ix%i monitor (%i,%i) %ix%i back buffer %ix%i "
        "match %i iconic %i foreground %i DPI aware process %i window %i\n",
        when,
        (unsigned int)GetWindowLongA(hwnd, GWL_STYLE),
        (unsigned int)GetWindowLongA(hwnd, GWL_EXSTYLE),
        origin.x,
        origin.y,
        client.right - client.left,
        client.bottom - client.top,
        mi.rcMonitor.left,
        mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        dx.windows[0].width,
        dx.windows[0].height,
        match,
        IsIconic(hwnd) ? 1 : 0,
        GetForegroundWindow() == hwnd ? 1 : 0,
        IsProcessDPIAware() ? 1 : 0,
        R_BorderlessWindowDpiAwareness(hwnd));

#endif
}

void __cdecl R_CheckResizeWindow()
{
    if ( Dvar_HasLatchedValue(r_aspectRatio)
        || Dvar_HasLatchedValue(r_mode)
        || Dvar_HasLatchedValue(r_fullscreen)
        || Dvar_HasLatchedValue(r_aaSamples)
        || Dvar_HasLatchedValue(r_displayRefresh)
        || Dvar_HasLatchedValue(r_vsync)
        || Dvar_HasLatchedValue(r_fullscreen)
        || Dvar_HasLatchedValue(r_borderless) ) // borderless: vid_restart applies a changed r_borderless
    {
        dx.resizeWindow = 1;
        while ( dx.resizeWindow )
            NET_Sleep(5u);
        Sys_WaitD3DDeviceOKEvent();
    }
}

void __cdecl R_ResizeWindow()
{
    HWND hwnd; // [esp+0h] [ebp-44h]
    DWORD exStyle; // [esp+4h] [ebp-40h]
    unsigned int style; // [esp+8h] [ebp-3Ch]
    GfxWindowParms wndParms; // [esp+Ch] [ebp-38h] BYREF
    tagRECT rc; // [esp+34h] [ebp-10h] BYREF

    Dvar_MakeLatchedValueCurrent((dvar_s*)r_aspectRatio);
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_mode);
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_fullscreen);
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_aaSamples);
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_vsync);
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_fullscreen);
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_displayRefresh);
    Dvar_MakeLatchedValueCurrent((dvar_s*)r_borderless); // borderless
    R_SetWndParms(&wndParms);
    R_StoreWindowSettings(&wndParms);
    hwnd = dx.windows[0].hwnd;
    if (!dx.windows[0].hwnd
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2739, 0, "%s", "hwnd"))
    {
        __debugbreak();
    }
    R_SetWndParms(&wndParms);
    dx.windows[0].width = wndParms.displayWidth;
    dx.windows[0].height = wndParms.displayHeight;
#if defined(__ANDROID__)
    if (!bo1::android::ConfigureRendererWindow(reinterpret_cast<SDL_Window *>(hwnd), R_SDLWindowSpec(wndParms)))
        R_SDLWindowFailure("window resize");
#else
    if (wndParms.borderless)
    {
        // borderless: WS_POPUP, no frame, not topmost (HWND_NOTOPMOST below, as the window)
        exStyle = 0;
        style = WS_POPUP;
    }
    else if (wndParms.fullscreen)
    {
        exStyle = 8;
        style = 0x80000000;
    }
    else
    {
        exStyle = 0;
        style = 13107200;
    }
    rc.left = 0;
    rc.right = wndParms.displayWidth;
    rc.top = 0;
    rc.bottom = wndParms.displayHeight;
    AdjustWindowRectEx(&rc, style, 0, exStyle);
    SetWindowLongA(hwnd, -16, style);
    SetWindowPos(
        hwnd,
        reinterpret_cast<HWND>(static_cast<intptr_t>(wndParms.fullscreen - 2)),
        wndParms.x,
        wndParms.y,
        rc.right - rc.left,
        rc.bottom - rc.top,
        0x60u);
#endif
    s_borderlessWindow = wndParms.borderless; // borderless
    if (wndParms.borderless)
    {
        Com_Printf(
            8,
            "Resizing %i x %i borderless window at (%i, %i)\n",
            wndParms.displayWidth,
            wndParms.displayHeight,
            wndParms.x,
            wndParms.y);
        R_BorderlessLogWindow("resized");
    }
    else if (wndParms.fullscreen)
        Com_Printf(
            8,
            "Resizing %i x %i fullscreen at (%i, %i)\n",
            wndParms.displayWidth,
            wndParms.displayHeight,
            wndParms.x,
            wndParms.y);
    else
        Com_Printf(
            8,
            "Resizing %i x %i window at (%i, %i)\n",
            wndParms.displayWidth,
            wndParms.displayHeight,
            wndParms.x,
            wndParms.y);
}

void __cdecl R_StoreWindowSettings(const GfxWindowParms *wndParms)
{
    const char *unitScaleValue; // eax
    bool v2; // [esp+0h] [ebp-3Ch]
    int monitorHeight; // [esp+2Ch] [ebp-10h]
    int monitorWidth; // [esp+30h] [ebp-Ch]
    int aspectHeight; // [esp+38h] [ebp-4h]

    if ( !r_aspectRatio
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 476, 0, "%s", "r_aspectRatio") )
    {
        __debugbreak();
    }
    vidConfig.sceneWidth = wndParms->sceneWidth;
    vidConfig.sceneHeight = wndParms->sceneHeight;
    vidConfig.displayWidth = wndParms->displayWidth;
    vidConfig.displayHeight = wndParms->displayHeight;
    vidConfig.displayFrequency = wndParms->hz;
    vidConfig.isFullscreen = wndParms->fullscreen;
    switch ( r_aspectRatio->current.integer )
    {
        case 0:
            if ( vidConfig.isFullscreen && dx.adapterNativeIsValid )
            {
                monitorWidth = dx.adapterNativeWidth;
                monitorHeight = dx.adapterNativeHeight;
            }
            else
            {
                monitorWidth = vidConfig.displayWidth;
                monitorHeight = vidConfig.displayHeight;
            }
            // KB decompilation fix (MP and zombies): round, not truncate (SP 0x006b696f: fld; fadd qword [0x009D3F88]; fistp).
            // With auto aspect at 1280x768 / 1600x960 / 800x480 the exe picks 16:10 where the truncation picked 16:9.
            aspectHeight = I_RoundBiasToInt((float)((float)((float)monitorHeight * 16.0) / (float)monitorWidth));
            if ( aspectHeight == 10 )
            {
                vidConfig.aspectRatioWindow = 1.6f;
            }
            else if ( aspectHeight >= 10 )
            {
                vidConfig.aspectRatioWindow = (1.0f + (1.0f / 3.0f));
            }
            else
            {
                //vidConfig.aspectRatioWindow = FLOAT_1_7777778;
                vidConfig.aspectRatioWindow = 1.7777778f;
            }
            break;
        case 1:
            vidConfig.aspectRatioWindow = (1.0f + (1.0f / 3.0f));
            break;
        case 2:
            vidConfig.aspectRatioWindow = 1.6f;
            break;
        case 3:
            vidConfig.aspectRatioWindow = 1.7777778f;
            break;
        default:
            unitScaleValue = va("unhandled case, aspectRatio = %i\n", r_aspectRatio->current.integer);
            if ( !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 521, 1, unitScaleValue) )
                __debugbreak();
            break;
    }
    if ( !com_wideScreen
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 537, 0, "%s", "com_wideScreen") )
    {
        __debugbreak();
    }
    if ( vidConfig.aspectRatioWindow == 1.3333334 )
        Dvar_SetBool((dvar_s *)com_wideScreen, 0);
    else
        Dvar_SetBool((dvar_s *)com_wideScreen, 1);
    vidConfig.aspectRatioScenePixel = (double)vidConfig.sceneHeight
                                                                    * vidConfig.aspectRatioWindow
                                                                    / (double)vidConfig.sceneWidth;
    if ( vidConfig.isFullscreen )
        vidConfig.aspectRatioDisplayPixel = (float)((float)dx.adapterFullscreenHeight * vidConfig.aspectRatioWindow)
                                                                            / (float)dx.adapterFullscreenWidth;
    else
        vidConfig.aspectRatioDisplayPixel = 1.0f;
    vidConfig.isToolMode = 0;
    v2 = r_reflectionProbeGenerate && r_reflectionProbeGenerate->current.enabled;
    vidConfig.isToolMode = v2;
}

void __cdecl R_SetWndParms(GfxWindowParms *wndParms)
{
    const char *resolutionString; // [esp+0h] [ebp-Ch]
    int refreshRate; // [esp+4h] [ebp-8h] BYREF
    const char *refreshRateString; // [esp+8h] [ebp-4h]

    wndParms->fullscreen = Dvar_GetBool("r_fullscreen");
    if ( Sys_IsHeadlessClient() ) // zombies: a headless client never goes fullscreen (that changes the display mode)
        wndParms->fullscreen = 0;
    // borderless: r_fullscreen 0 + r_borderless 1 - the monitor's rect; r_mode, r_customMode and the clamp below are unused
    wndParms->borderless = !wndParms->fullscreen && r_borderless && r_borderless->current.enabled;
    if ( wndParms->borderless )
    {
        R_BorderlessSetWndParms(wndParms);
        return;
    }
    if ( wndParms->fullscreen || !R_SetCustomResolution(wndParms) )
    {
        resolutionString = Dvar_EnumToString(r_mode);
        sscanf(resolutionString, "%ix%i", &wndParms->displayWidth, &wndParms->displayHeight);
    }
    // zombies HARNESS ONLY: no monitor clamp for the headless client (see R_SetCustomResolution).
    if ( !wndParms->fullscreen && !Sys_IsHeadlessClient() )
    {
        if ( !dx.adapterNativeIsValid )
            dx.adapterNativeIsValid = R_GetMonitorDimensions(&dx.adapterNativeWidth, &dx.adapterNativeHeight);
        if ( wndParms->displayWidth > dx.adapterNativeWidth || wndParms->displayHeight > dx.adapterNativeHeight )
        {
            wndParms->displayWidth = dx.adapterNativeWidth;
            wndParms->displayHeight = dx.adapterNativeHeight;
        }
    }
    wndParms->sceneWidth = wndParms->displayWidth;
    wndParms->sceneHeight = wndParms->displayHeight;
    if ( wndParms->fullscreen )
    {
        refreshRateString = Dvar_EnumToString(r_displayRefresh);
        sscanf(refreshRateString, "%i Hz", &refreshRate);
        wndParms->hz = (int)R_ClosestRefreshRateForMode(wndParms->displayWidth, wndParms->displayHeight, refreshRate);
    }
    else
    {
        wndParms->hz = 60;
    }
    wndParms->x = Dvar_GetInt("vid_xpos");
    wndParms->y = Dvar_GetInt("vid_ypos");
    wndParms->hwnd = 0;
    wndParms->aaSamples = r_aaSamples->current.integer;
}

int __cdecl R_ClosestRefreshRateForMode(unsigned int width, unsigned int height, int refreshRate)
{
#if defined(__ANDROID__)
    if (!width || !height || width > INT32_MAX || height > INT32_MAX || refreshRate < 0 || !dx.displayModeCount || dx.displayModeCount > 256)
        R_FatalInitError("Invalid refresh-rate search parameters");
    int bot=0, top=static_cast<int>(dx.displayModeCount)-1;
    while (bot <= top) {
        int mid=bot+(top-bot)/2;
        const auto &mode=dx.displayModes[mid];
        int comparison=mode.Width < width ? -1 : mode.Width > width ? 1
            : mode.Height < height ? -1 : mode.Height > height ? 1
            : mode.RefreshRate < static_cast<unsigned>(refreshRate) ? -1 : mode.RefreshRate > static_cast<unsigned>(refreshRate) ? 1 : 0;
        if (!comparison) return refreshRate;
        if(comparison >= 0) top=mid-1; else bot=mid+1;
    }
    if (top >= 0 && dx.displayModes[top].Width == width && dx.displayModes[top].Height == height) return dx.displayModes[top].RefreshRate;
    if (bot < static_cast<int>(dx.displayModeCount) && dx.displayModes[bot].Width == width && dx.displayModes[bot].Height == height) return dx.displayModes[bot].RefreshRate;
    R_FatalInitError("Requested fullscreen resolution is absent from actual display modes");
    std::abort();
#else

    const char *v4; // eax
    int top; // [esp+0h] [ebp-10h]
    int bot; // [esp+4h] [ebp-Ch]
    int comparison; // [esp+8h] [ebp-8h]
    int mid; // [esp+Ch] [ebp-4h]

    bot = 0;
    top = dx.displayModeCount - 1;
    while ( bot <= top )
    {
        mid = (bot + top) / 2;
        comparison = static_cast<int>(dx.displayModes[mid].Width) - static_cast<int>(width);
        if ( !comparison )
        {
            comparison = static_cast<int>(dx.displayModes[mid].Height) - static_cast<int>(height);
            if ( !comparison )
            {
                comparison = static_cast<int>(dx.displayModes[mid].RefreshRate) - refreshRate;
                if ( !comparison )
                    return refreshRate;
            }
        }
        if ( comparison >= 0 )
            top = mid - 1;
        else
            bot = mid + 1;
    }
    if ( top < 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    2327,
                    0,
                    "%s\n\t(top) = %i",
                    "(top >= 0)",
                    top) )
    {
        __debugbreak();
    }
    if ( top != bot - 1
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2328, 0, "%s", "top == bot - 1") )
    {
        __debugbreak();
    }
    if ( dx.displayModes[top].Width == width && dx.displayModes[top].Height == height )
        return dx.displayModes[top].RefreshRate;
    if ( dx.displayModes[bot].Width != width || dx.displayModes[bot].Height != height )
    {
        v4 = va(
                     "%i = (%i %i), %i = (%i %i), want (%i %i)",
                     top,
                     dx.displayModes[top].Width,
                     dx.displayModes[bot].Height,
                     bot,
                     dx.displayModes[bot].Width,
                     dx.displayModes[bot].Height,
                     width,
                     height);
        if ( !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                        2331,
                        0,
                        "%s\n\t%s",
                        "dx.displayModes[bot].Width == width && dx.displayModes[bot].Height == height",
                        v4) )
            __debugbreak();
    }
    return dx.displayModes[bot].RefreshRate;

#endif
}

bool __cdecl R_SetCustomResolution(GfxWindowParms *wndParms)
{
    int monitorHeight; // [esp+0h] [ebp-8h] BYREF
    int monitorWidth; // [esp+4h] [ebp-4h] BYREF

    if ( sscanf(r_customMode->current.string, "%ix%i", &wndParms->displayWidth, &wndParms->displayHeight) != 2 )
        return 0;
    // zombies HARNESS ONLY: the headless client draws on a private desktop and only reads its back buffer, so it
    // may use a mode larger than this PC's monitor (x6 c31: HUD screenshots at 3840x2160). Normal runs keep
    // the retail monitor check.
    if ( Sys_IsHeadlessClient() )
        return 1;
    return !R_GetMonitorDimensions(&monitorWidth, &monitorHeight)
            || wndParms->displayWidth <= monitorWidth && wndParms->displayHeight <= monitorHeight;
}

#endif // normal renderer bodies before shared configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS) || defined(BO1_NATIVE_RENDER_CONFIG)
void __cdecl R_AllocateMinimalResources()
{
    g_allocateMinimalResources = 1;
}

void __cdecl R_SetIsMultiplayer(bool isMp)
{
    r_glob.isMultiplayer = isMp;
}

#endif // shared renderer configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS)
void __cdecl R_InitThreads()
{
    if ( r_glob.isRenderingRemoteUpdate
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    3198,
                    0,
                    "%s",
                    "!r_glob.isRenderingRemoteUpdate") )
    {
        __debugbreak();
    }
    Com_Printf(8, "Trying SMP acceleration...\n");
    R_InitRenderThread();
    R_InitWorkerThreads();
    Com_Printf(8, "...succeeded.\n");
}

void __cdecl R_Init()
{

    Com_Printf(8, "----- R_Init -----\n");
    r_glob.remoteScreenLastSceneResolveTarget = 0;
    Swap_Init();
    R_Register();
    R_InitGlobalStructs();
    R_InitDrawMethod();
    R_InitGraphicsApi();
    RB_RegisterBackendAssets();
    R_InitWater();
    dx.sunSpriteSamples = RB_CalcSunSpriteSamples();
    if ( !dx.sunSpriteSamples )
    {
        Com_Printf(8, "Sun sprite occlusion query calibration failed; reverting to low-quality sun visibility test");
        RB_FreeSunSpriteQueries();
    }
}

void R_InitGraphicsApi()
{
    GfxWindowParms wndParms; // [esp+4h] [ebp-28h] BYREF

    iassert((dx.device != NULL) == (dx.d3d9 != NULL));

    if ( dx.device )
    {
        R_InitSystems();
    }
    else
    {
        if ( R_BorderlessWanted() ) // borderless: DPI aware before the adapter / monitor choice and the window
            R_BorderlessMakeDpiAware();
#if defined(__ANDROID__)
        if (!R_PreCreateWindow()) { R_FatalInitError("Couldn't initialize actual SDL/DXVK renderer"); std::abort(); }
#else
        R_PreCreateWindow();
#endif
        while ( 1 )
        {
            R_SetWndParms(&wndParms);
            if ( R_CreateGameWindow(&wndParms) )
                break;
            if ( !R_ReduceWindowSettings() )
                R_FatalInitError("Couldn't initialize renderer");
        }
    }
}

void R_InitSystems()
{
    R_InitImages();
    Material_Init();
    R_InitFonts();
    R_InitLoadWater();
    R_InitLightDefs();
    R_ClearFogs();
    R_InitWind();
    R_InitWaterSimulation();
    R_InitDebug();
    rg.registered = 1;
}

char __cdecl R_PreCreateWindow()
{
    _D3DADAPTER_IDENTIFIER9 adapterId; // [esp+4h] [ebp-458h] BYREF
#if defined(__ANDROID__)
    if (!bo1::android::PrepareRendererVideo()) { Com_Printf(8, "SDL video unavailable: %s\n", SDL_GetError()); return 0; }
#endif

    if ( dx.d3d9 )
    {
        if ( !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                        1774,
                        1,
                        "D3D re-initialized before being shutdown") )
            __debugbreak();
    }
    else
    {
        Com_Printf(8, "Getting Direct3D 9 interface...\n");
        if ( Sys_IsHeadlessClient() )
        {
            // zombies: a headless client's window is on a desktop that is never the input desktop. Plain Direct3D 9
            // refuses that desktop (GetDeviceCaps D3DERR_NOTAVAILABLE, CreateDevice D3DERR_INVALIDCALL); Direct3D 9Ex
            // renders there and Present returns S_PRESENT_OCCLUDED (MEASURED with build\probe\d3dprobe.cpp, notes/x5-client-report.md).
            // 9Ex has no managed pool: r_image.cpp makes those textures dynamic default-pool ones in this mode.
            IDirect3D9Ex *d3d9ex = NULL;
            HRESULT hrEx = R_WSI_CALL(Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex));
            if ( hrEx < 0 )
                Com_Printf(8, "bo1_headless_client: Direct3DCreate9Ex failed: %s (0x%08x)\n", R_ErrorDescription(hrEx), hrEx);
            dx.d3d9 = d3d9ex;
        }
        else
        dx.d3d9 = R_WSI_CALL(Direct3DCreate9(0x20u));
        if ( !dx.d3d9 )
        {
            Com_Printf(8, "Direct3D 9 failed to initialize\n");
            return 0;
        }
    }
    dx.adapterIndex = R_ChooseAdapter();
    R_StoreDirect3DCaps(dx.adapterIndex);
    R_EnumDisplayModes(dx.adapterIndex);
    dx.nvInitialized = 0;
    if ( !dx.d3d9->GetAdapterIdentifier(dx.adapterIndex, 0, &adapterId) )
    {
        dx.vendorId = adapterId.VendorId;
#if !defined(__ANDROID__)
        if ( adapterId.VendorId == 4318 )
            dx.nvInitialized = NvAPI_Initialize() == 0;
#endif
    }
    return 1;
}

void __cdecl R_StoreDirect3DCaps(unsigned int adapterIndex)
{
    unsigned int MaxUserClipPlanes; // [esp+0h] [ebp-138h]
    unsigned int MaxTextureHeight; // [esp+4h] [ebp-134h]
    _D3DCAPS9 caps; // [esp+8h] [ebp-130h] BYREF

    R_GetDirect3DCaps(adapterIndex, &caps);
    R_PickRenderer(&caps);
    if ( (int)caps.MaxTextureHeight < (int)caps.MaxTextureWidth )
        MaxTextureHeight = caps.MaxTextureHeight;
    else
        MaxTextureHeight = caps.MaxTextureWidth;
    vidConfig.maxTextureSize = MaxTextureHeight;
    vidConfig.maxTextureMaps = 16;
    vidConfig.deviceSupportsGamma = (caps.Caps2 & 0x20000) != 0;
    if ( Sys_IsHeadlessClient() ) // zombies: a headless client never sets a gamma ramp (it belongs to the user's display)
        vidConfig.deviceSupportsGamma = 0;
    if ( (int)caps.MaxUserClipPlanes < 6 )
        MaxUserClipPlanes = caps.MaxUserClipPlanes;
    else
        MaxUserClipPlanes = 6;
    gfxMetrics.maxClipPlanes = MaxUserClipPlanes;
    gfxMetrics.hasAnisotropicMinFilter = (caps.TextureFilterCaps & 0x400) != 0;
    gfxMetrics.hasAnisotropicMagFilter = (caps.TextureFilterCaps & 0x4000000) != 0;
    gfxMetrics.maxAnisotropy = caps.MaxAnisotropy;
    gfxMetrics.slopeScaleDepthBias = (caps.RasterCaps & 0x2000000) != 0;
    gfxMetrics.canMipCubemaps = (caps.TextureCaps & 0x10000) != 0;
    gfxMetrics.hasTransparencyMsaa = R_CheckTransparencyMsaa(adapterIndex);
    R_SetShadowmapFormats_DX(adapterIndex);
}

void __cdecl R_GetDirect3DCaps(unsigned int adapterIndex, _D3DCAPS9 *caps)
{
    const char *v2; // eax
    const char *v3; // eax
    int hr; // [esp+0h] [ebp-8h]
    int attempt; // [esp+4h] [ebp-4h]

    if ( !dx.d3d9 && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 606, 0, "%s", "dx.d3d9") )
        __debugbreak();
    attempt = 0;
    while ( 1 )
    {
        hr = dx.d3d9->GetDeviceCaps(adapterIndex, D3DDEVTYPE_HAL, caps);
        if ( hr >= 0 )
            break;
        #if defined(__ANDROID__)
        bo1::win32::Sleep(0x64u);
#else
        Sleep(0x64u);
#endif
        if ( ++attempt == 20 )
        {
            v2 = R_ErrorDescription(hr);
            v3 = va("GetDeviceCaps failed: %s", v2);
            R_FatalInitError(v3);
        }
    }
}

void __cdecl R_PickRenderer(_D3DCAPS9 *caps)
{
    Com_Printf(
        8,
        "Pixel shader version is %i.%i\n",
        BYTE1(caps->PixelShaderVersion),
        (unsigned __int8)caps->PixelShaderVersion);
    Com_Printf(
        8,
        "Vertex shader version is %i.%i\n",
        BYTE1(caps->VertexShaderVersion),
        (unsigned __int8)caps->VertexShaderVersion);
    R_CheckDxCaps(caps);
}

bool __cdecl R_CheckTransparencyMsaa(unsigned int adapterIndex)
{
    return r_aaSamples->current.integer != 1
            && dx.d3d9->CheckDeviceFormat(
                     adapterIndex,
                     D3DDEVTYPE_HAL,
                     D3DFMT_X8R8G8B8,
                     0,
                     D3DRTYPE_SURFACE,
                     (_D3DFORMAT)1094800211) == 0;
}

void __cdecl R_SetShadowmapFormats_DX(unsigned int adapterIndex)
{
    _D3DFORMAT colorFormat; // [esp+0h] [ebp-30h]
    unsigned int formatIndex; // [esp+8h] [ebp-28h]
    _D3DFORMAT depthFormat; // [esp+Ch] [ebp-24h]
    _D3DFORMAT formats[4][2]; // [esp+10h] [ebp-20h]

    *(_QWORD *)&formats[0][0] = 0x170000004BLL;
    *(_QWORD *)&formats[1][0] = 0x170000004BLL;
    *(_QWORD *)&formats[2][0] = 0x160000004BLL;
    *(_QWORD *)&formats[3][0] = 0x150000004BLL;

    if ( NV_UseShadowNullColorRenderTarget() )
        formats[0][0] = (_D3DFORMAT)1280070990;

    for ( formatIndex = 0; formatIndex < 4; ++formatIndex )
    {
        depthFormat = formats[formatIndex][0];
        colorFormat = formats[formatIndex][1];
        if ( (!NV_UseShadowNullColorRenderTarget()
             || colorFormat != 1280070990
             || !(dx.d3d9->CheckDeviceFormat(adapterIndex, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 1, (D3DRESOURCETYPE)1, (D3DFORMAT)1280070990))
            && !dx.d3d9->CheckDepthStencilMatch(
                        adapterIndex,
                        D3DDEVTYPE_HAL,
                        D3DFMT_X8R8G8B8,
                        colorFormat,
                        depthFormat)
            && !dx.d3d9->CheckDeviceFormat(
                        adapterIndex,
                        D3DDEVTYPE_HAL,
                        D3DFMT_X8R8G8B8,
                        2u,
                        D3DRTYPE_TEXTURE,
                        depthFormat) ))
        {
            gfxMetrics.shadowmapFormatPrimary = depthFormat;
            gfxMetrics.shadowmapFormatSecondary = colorFormat;
            gfxMetrics.shadowmapBuildTechType = 2;
            gfxMetrics.hasHardwareShadowmap = 1;
            gfxMetrics.shadowmapSamplerState = 98;
            Dvar_SetBoolByName("ui_showShadowOptions", 1);
            return;
        }
    }
    gfxMetrics.shadowmapFormatPrimary = D3DFMT_R32F;
    gfxMetrics.shadowmapFormatSecondary = D3DFMT_D24X8;
    gfxMetrics.shadowmapBuildTechType = 3;
    gfxMetrics.hasHardwareShadowmap = 0;
    gfxMetrics.shadowmapSamplerState = 97;
    Dvar_SetBool((dvar_s *)sm_enable, 0);
    Dvar_SetBoolByName("ui_showShadowOptions", 0);
}

void __cdecl R_EnumDisplayModes(unsigned int adapterIndex)
{
    const char *unitScaleValue; // eax
    int v2; // eax
    int v3; // eax
    int refreshRateCount; // [esp+0h] [ebp-C30h]
    int resolutionCount; // [esp+4h] [ebp-C2Ch]
    int availableResolutions[256][2]; // [esp+8h] [ebp-C28h] BYREF
    HRESULT hr; // [esp+80Ch] [ebp-424h]
    unsigned int modeCountReported; // [esp+810h] [ebp-420h]
    unsigned int modeIndex; // [esp+814h] [ebp-41Ch]
    int resolutionIndex; // [esp+818h] [ebp-418h]
    int defaultRefreshRateIndex; // [esp+81Ch] [ebp-414h]
    int availableRefreshRates[257]; // [esp+820h] [ebp-410h] BYREF
    char *modeText; // [esp+C24h] [ebp-Ch]
    int refreshRateIndex; // [esp+C28h] [ebp-8h]
    int defaultResolutionIndex; // [esp+C2Ch] [ebp-4h]

    modeCountReported = R_WSI_CALL(dx.d3d9->GetAdapterModeCount(adapterIndex, D3DFMT_X8R8G8B8));
#if defined(__ANDROID__)
    _D3DDISPLAYMODE currentDisplayMode{};
    HRESULT currentDisplayModeResult = R_WSI_CALL(dx.d3d9->GetAdapterDisplayMode(adapterIndex, &currentDisplayMode));
    Com_Printf(8, "Android D3D display: adapter %u enumerated %u current hr 0x%08x %ux%u refresh %u format %u fullscreen %i\n",
        adapterIndex, modeCountReported, static_cast<unsigned>(currentDisplayModeResult), currentDisplayMode.Width,
        currentDisplayMode.Height, currentDisplayMode.RefreshRate, static_cast<unsigned>(currentDisplayMode.Format), Dvar_GetBool("r_fullscreen"));
    R_SDLOwner([&] {
        const int displays = SDL_GetNumVideoDisplays();
        const intptr_t monitor = reinterpret_cast<intptr_t>(dx.d3d9->GetAdapterMonitor(adapterIndex));
        const int display = monitor > 0 && monitor <= displays ? static_cast<int>(monitor - 1) : -1;
        Com_Printf(8, "Android SDL display: driver %s count %i selected %i\n", SDL_GetCurrentVideoDriver(), displays, display);
        if (display >= 0) {
            SDL_DisplayMode current{}, desktop{};
            const int currentResult = SDL_GetCurrentDisplayMode(display, &current);
            const int desktopResult = SDL_GetDesktopDisplayMode(display, &desktop);
            const int modes = SDL_GetNumDisplayModes(display);
            Com_Printf(8, "Android SDL current: result %i %ix%i refresh %i format %s bpp %i; desktop result %i %ix%i refresh %i format %s bpp %i; modes %i\n",
                currentResult, current.w, current.h, current.refresh_rate, SDL_GetPixelFormatName(current.format), SDL_BITSPERPIXEL(current.format),
                desktopResult, desktop.w, desktop.h, desktop.refresh_rate, SDL_GetPixelFormatName(desktop.format), SDL_BITSPERPIXEL(desktop.format), modes);
            for (int index = 0; index < modes && index < 8; ++index) {
                SDL_DisplayMode mode{};
                const int result = SDL_GetDisplayMode(display, index, &mode);
                Com_Printf(8, "Android SDL mode %i: result %i %ix%i refresh %i format %s bpp %i\n", index, result,
                    mode.w, mode.h, mode.refresh_rate, SDL_GetPixelFormatName(mode.format), SDL_BITSPERPIXEL(mode.format));
            }
        }
        return true;
    }, true);
#endif
    dx.displayModeCount = 0;
    for ( modeIndex = 0; modeIndex < modeCountReported && dx.displayModeCount < 0x100; ++modeIndex )
    {
        hr = R_WSI_CALL(dx.d3d9->EnumAdapterModes(
                     adapterIndex,
                     D3DFMT_X8R8G8B8,
                     modeIndex,
                     &dx.displayModes[dx.displayModeCount]));
        if ( hr >= 0 )
        {
#if defined(__ANDROID__)
            const auto &mode=dx.displayModes[dx.displayModeCount];
            if (!mode.Width || !mode.Height || mode.Width > INT32_MAX || mode.Height > INT32_MAX || mode.RefreshRate > INT32_MAX)
                R_FatalInitError("DXVK reported an invalid display mode");
#endif
#if !defined(__ANDROID__)
            if ( !dx.displayModes[dx.displayModeCount].RefreshRate )
                dx.displayModes[dx.displayModeCount].RefreshRate = 60;
#else
            if (modeIndex < 8)
                Com_Printf(8, "Android D3D mode %u: %ux%u refresh %u format %u\n", modeIndex, mode.Width, mode.Height, mode.RefreshRate, static_cast<unsigned>(mode.Format));
#endif
            ++dx.displayModeCount;
        }
    }
#if defined(__ANDROID__)
    qsort(dx.displayModes, dx.displayModeCount, sizeof(dx.displayModes[0]), [](const void *a, const void *b) {
        const auto &left=*static_cast<const _D3DDISPLAYMODE *>(a);
        const auto &right=*static_cast<const _D3DDISPLAYMODE *>(b);
        if(left.Width != right.Width) return left.Width < right.Width ? -1 : 1;
        if(left.Height != right.Height) return left.Height < right.Height ? -1 : 1;
        return left.RefreshRate < right.RefreshRate ? -1 : left.RefreshRate > right.RefreshRate ? 1 : 0;
    });
#else
    qsort(dx.displayModes, dx.displayModeCount, sizeof(*(dx.displayModes)), (int (__cdecl *)(const void *, const void *))R_CompareDisplayModes);
#endif
    resolutionCount = 0;
    refreshRateCount = 0;
    for ( modeIndex = 0; modeIndex < dx.displayModeCount; ++modeIndex )
    {
        resolutionCount = R_AddValidResolution(
                                                dx.displayModes[modeIndex].Width,
                                                (int)dx.displayModes[modeIndex].Height,
                                                resolutionCount,
                                                availableResolutions);
        refreshRateCount = R_AddValidRefreshRate(
                                                 (int)dx.displayModes[modeIndex].RefreshRate,
                                                 refreshRateCount,
                                                 availableRefreshRates);
    }
#if defined(__ANDROID__)
    if (!resolutionCount && !Dvar_GetBool("r_fullscreen") && currentDisplayModeResult >= 0 &&
        currentDisplayMode.Width >= 800 && currentDisplayMode.Width <= INT32_MAX &&
        currentDisplayMode.Height >= 600 && currentDisplayMode.Height <= INT32_MAX &&
        currentDisplayMode.RefreshRate > 0 && currentDisplayMode.RefreshRate <= INT32_MAX &&
        currentDisplayMode.Format == D3DFMT_X8R8G8B8) {
        // Android may expose a 16-bit SurfaceView display mode before the real
        // Vulkan window is created. DXVK's 32-bit exclusive-mode enumeration
        // can then be empty. Its genuine current mode still sizes a windowed
        // renderer; never insert it into the exclusive display-mode table.
        resolutionCount = R_AddValidResolution(static_cast<int>(currentDisplayMode.Width),
            static_cast<int>(currentDisplayMode.Height), resolutionCount, availableResolutions);
        refreshRateCount = R_AddValidRefreshRate(static_cast<int>(currentDisplayMode.RefreshRate),
            refreshRateCount, availableRefreshRates);
        Com_Printf(8, "Android windowed resolution uses actual D3D current display %ux%u refresh %u; exclusive modes remain %u\n",
            currentDisplayMode.Width, currentDisplayMode.Height, currentDisplayMode.RefreshRate, dx.displayModeCount);
    }
#endif
    modeText = dx.modeText;
    if ( !resolutionCount )
    {
        unitScaleValue = va("No valid resolutions of %i x %i or above found", 800, 600);
        R_FatalInitError(unitScaleValue);
    }
    defaultResolutionIndex = 0;
    for ( resolutionIndex = 0; resolutionIndex < resolutionCount; ++resolutionIndex )
    {
        dx.resolutionNameTable[resolutionIndex] = modeText;
        #if defined(__ANDROID__)
        size_t remaining=sizeof(dx.modeText)-static_cast<size_t>(modeText-dx.modeText);
        v2 = std::snprintf(modeText, remaining, "%ix%i", availableResolutions[resolutionIndex][0], availableResolutions[resolutionIndex][1]);
        if (v2 < 0 || static_cast<size_t>(v2) >= remaining) R_FatalInitError("Display mode text exceeds actual storage");
#else
        v2 = sprintf(modeText, "%ix%i", availableResolutions[resolutionIndex][0], availableResolutions[resolutionIndex][1]);
#endif
        modeText += v2 + 1;
        if ( availableResolutions[defaultResolutionIndex][0] < 640 || availableResolutions[defaultResolutionIndex][1] < 480 )
            defaultResolutionIndex = resolutionIndex;
    }
    dx.resolutionNameTable[resolutionIndex] = 0;
    r_mode = _Dvar_RegisterEnum(
                         "r_mode",
                         dx.resolutionNameTable,
                         defaultResolutionIndex,
                         0x21u,
                         "Direct X resolution mode");
#if defined(__ANDROID__)
    qsort(availableRefreshRates, refreshRateCount, sizeof(availableRefreshRates[0]), [](const void *a, const void *b) {
        int left=*static_cast<const int *>(a), right=*static_cast<const int *>(b);
        return left < right ? -1 : left > right ? 1 : 0;
    });
#else
    qsort(availableRefreshRates, refreshRateCount, sizeof(*(availableRefreshRates)), (int (__cdecl *)(const void *, const void *))R_CompareRefreshRates);
#endif
    defaultRefreshRateIndex = 0;
    for ( refreshRateIndex = 0; refreshRateIndex < refreshRateCount; ++refreshRateIndex )
    {
        dx.refreshRateNameTable[refreshRateIndex] = modeText;
        #if defined(__ANDROID__)
        size_t remaining=sizeof(dx.modeText)-static_cast<size_t>(modeText-dx.modeText);
        v3 = std::snprintf(modeText, remaining, "%i Hz", availableRefreshRates[refreshRateIndex]);
        if (v3 < 0 || static_cast<size_t>(v3) >= remaining) R_FatalInitError("Display mode text exceeds actual storage");
#else
        v3 = sprintf(modeText, "%i Hz", availableRefreshRates[refreshRateIndex]);
#endif
        modeText += v3 + 1;
        if ( availableRefreshRates[defaultRefreshRateIndex] < 60 )
            defaultRefreshRateIndex = refreshRateIndex;
    }
    dx.refreshRateNameTable[refreshRateIndex] = 0;
    r_displayRefresh = _Dvar_RegisterEnum(
                                             "r_displayRefresh",
                                             dx.refreshRateNameTable,
                                             defaultRefreshRateIndex,
                                             0x221u,
                                             "Refresh rate");
}

int __cdecl R_CompareDisplayModes(unsigned int *e0, unsigned int *e1)
{
    int delta; // [esp+0h] [ebp-Ch]

    delta = *e0 - *e1;
    if ( *e0 == *e1 )
    {
        delta = e0[1] - e1[1];
        if ( !delta )
            return e0[2] - e1[2];
    }
    return delta;
}

int __cdecl R_AddValidRefreshRate(int refreshRate, int rateCount, int *availableRefreshRates)
{
#if defined(__ANDROID__)
    if (!availableRefreshRates || rateCount < 0 || rateCount > 256 || refreshRate < 0) R_FatalInitError("Invalid refresh table bounds");
#endif
    int rateIndex; // [esp+4h] [ebp-4h]
    int rateIndexa; // [esp+4h] [ebp-4h]

    for ( rateIndex = 0; rateIndex < rateCount; ++rateIndex )
    {
        if ( availableRefreshRates[rateIndex] == refreshRate )
            return rateCount;
    }
#if defined(__ANDROID__)
    if (rateCount == 256) R_FatalInitError("Refresh table is full");
#endif
    for ( rateIndexa = rateCount; rateIndexa > 0 && availableRefreshRates[rateIndexa - 1] > refreshRate; --rateIndexa )
        availableRefreshRates[rateIndexa] = availableRefreshRates[rateIndexa - 1];
    availableRefreshRates[rateIndexa] = refreshRate;
    return rateCount + 1;
}

int __cdecl R_CompareRefreshRates(unsigned int *e0, unsigned int *e1)
{
    return *e0 - *e1;
}

int __cdecl R_AddValidResolution(int width, int height, int resolutionCount, int (*availableResolutions)[2])
{
#if defined(__ANDROID__)
    if (!availableResolutions || resolutionCount < 0 || resolutionCount > 256) R_FatalInitError("Invalid resolution table bounds");
    if (resolutionCount && availableResolutions[resolutionCount-1][0] == width && availableResolutions[resolutionCount-1][1] == height) return resolutionCount;
    if (width < 800 || height < 600) return resolutionCount;
    if (resolutionCount == 256) R_FatalInitError("Resolution table is full");
    availableResolutions[resolutionCount][0]=width; availableResolutions[resolutionCount][1]=height;
    return resolutionCount+1;
#else

    if ( resolutionCount < 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    1483,
                    0,
                    "%s\n\t(resolutionCount) = %i",
                    "(resolutionCount >= 0)",
                    resolutionCount) )
    {
        __debugbreak();
    }
    if ( resolutionCount > 0
        && (*availableResolutions)[2 * resolutionCount - 2] == width
        && (*availableResolutions)[2 * resolutionCount - 1] == height )
    {
        return resolutionCount;
    }
    if ( width < 800 || height < 600 )
        return resolutionCount;
    (*availableResolutions)[2 * resolutionCount] = width;
    (*availableResolutions)[2 * resolutionCount + 1] = height;
    return resolutionCount + 1;

#endif
}

unsigned int __cdecl R_ChooseAdapter()
{
    unsigned int foundAdapterIndex; // [esp+14h] [ebp-470h]
    unsigned int adapterIndex; // [esp+1Ch] [ebp-468h]
    HMONITOR__ *desiredMonitor; // [esp+20h] [ebp-464h]
    unsigned int adapterCount; // [esp+24h] [ebp-460h]
    _D3DADAPTER_IDENTIFIER9 id; // [esp+2Ch] [ebp-458h] BYREF

    desiredMonitor = R_ChooseMonitor();
    foundAdapterIndex = 0;
    adapterCount = R_WSI_CALL(dx.d3d9->GetAdapterCount());
    for ( adapterIndex = 0; adapterIndex < adapterCount; ++adapterIndex )
    {
        if ( desiredMonitor )
        {
            if ( R_WSI_CALL(dx.d3d9->GetAdapterMonitor(adapterIndex)) != desiredMonitor )
                continue;
            foundAdapterIndex = adapterIndex;
        }
        if ( dx.d3d9->GetAdapterIdentifier(adapterIndex, 0, &id) >= 0
            && !strcmp(id.Description, "NVIDIA NVPerfHUD") )
        {
            return adapterIndex;
        }
    }
    return foundAdapterIndex;
}

struct GfxEnumMonitors // sizeof=0x8
{                                       // XREF: R_ChooseMonitor/r
    int monitorIndex;                   // XREF: R_ChooseMonitor+23/w
    HMONITOR__ *foundMonitor;           // XREF: R_ChooseMonitor+26/w
};

HMONITOR__ *__cdecl R_ChooseMonitor()
{
#if defined(__ANDROID__)
    bo1::android::RendererDisplay display{};
    int preferred = Dvar_GetBool("r_fullscreen") ? r_monitor->current.integer : 0;
    if (!bo1::android::ChooseRendererDisplay(preferred, Dvar_GetInt("vid_xpos"), Dvar_GetInt("vid_ypos"), &display))
        R_SDLWindowFailure("display selection");
    return reinterpret_cast<HMONITOR__ *>(static_cast<intptr_t>(display.index) + 1);
#else

    POINT pt; // [esp+0h] [ebp-10h]
    GfxEnumMonitors enumData; // [esp+8h] [ebp-8h] BYREF

    if ( Dvar_GetBool("r_fullscreen") )
    {
        enumData.monitorIndex = r_monitor->current.integer;
        enumData.foundMonitor = 0;
        EnumDisplayMonitors(0, 0, (MONITORENUMPROC)R_MonitorEnumCallback, (LPARAM)&enumData);
        if ( enumData.foundMonitor )
            return enumData.foundMonitor;
    }
    pt.x = Dvar_GetInt("vid_xpos");
    pt.y = Dvar_GetInt("vid_ypos");
    return MonitorFromPoint(pt, 1u);

#endif
}

int __stdcall R_MonitorEnumCallback(HMONITOR__ *monitorHandle, HDC__ *hdc, tagRECT *rect, LPARAM userData)
{
    GfxEnumMonitors *data = reinterpret_cast<GfxEnumMonitors *>(userData);
    if (--data->monitorIndex)
    {
        return 1;
    }
    else
    {
        data->foundMonitor = monitorHandle;
        return 0;
    }
}

char __cdecl R_CreateGameWindow(GfxWindowParms *wndParms)
{
    // zombies: the one place the game window is created; headless (a dedicated server) must never get here. The
    // headless client may: tools\headless.ps1 -Client starts it on the private desktop WinSta0\bo1_headless, so
    // its window is never on the user's desktop (see win_main.cpp)
    if ( Sys_IsHeadless() && !Sys_IsHeadlessClient() )
        Com_Error(ERR_FATAL, "bo1_headless: the renderer tried to create the game window");
    if ( Sys_IsHeadlessClient() && wndParms->fullscreen )
        Com_Error(ERR_FATAL, "bo1_headless_client: the renderer tried to create a fullscreen window");
    if ( !R_CreateWindow(wndParms) )
        return 0;

    if ( !R_InitHardware(wndParms) ) {
#if defined(__ANDROID__)
        // Failed hardware initialization may own render resources and an implicit
        // swapchain before R_FinishAttachingToWindow publishes windowCount.
        if (dx.device) {
            R_ReleaseForShutdownOrReset();
            if (dx.windows[0].swapChain) {
                auto *chain=dx.windows[0].swapChain; dx.windows[0].swapChain=nullptr;
                R_SDLOwner([&] { return chain->Release(); });
            }
            R_SDLOwner([&] { return dx.device->Release(); }); dx.device=nullptr;
        }
        if (!bo1::android::DestroyRendererWindow(reinterpret_cast<SDL_Window *>(wndParms->hwnd))) R_SDLWindowFailure("failed window retirement");
        wndParms->hwnd=nullptr;
#endif
        return 0;
    }

    dx.targetWindowIndex = 0;
#if defined(__ANDROID__)
    if (!bo1::android::ShowRendererWindow(reinterpret_cast<SDL_Window *>(wndParms->hwnd))) R_SDLWindowFailure("show window");
#else
    ShowWindow(wndParms->hwnd, 5);
#endif
    R_BorderlessLogWindow("created"); // borderless
    return 1;
}

char __cdecl R_InitHardware(const GfxWindowParms *wndParms)
{
    unsigned int nvInitialized; // ecx
    int v3; // [esp+0h] [ebp-3Ch]
    unsigned int numAFRGroups; // [esp+4h] [ebp-38h]
    int count; // [esp+14h] [ebp-28h]
    NV_GET_CURRENT_SLI_STATE sliState; // [esp+18h] [ebp-24h] BYREF
    bool failed; // [esp+3Bh] [ebp-1h]

    if ( !R_CreateDevice(wndParms) )
        return 0;
    if ( useFastFile->current.enabled )
        DB_LoadGraphicsAssetsForPC();
    R_UpdateGpuSyncType();
    dx.gpuCount = 1;
#if defined(BO1_MULTI_GPU) && !defined(__ANDROID__)
    if ( r_multiGpu->current.enabled )
    {
        if ( dx.vendorId == 4318 )
        {
            nvInitialized = dx.nvInitialized;
            if ( dx.nvInitialized )
            {
                sliState.version = 65564;
                if ( !NvAPI_D3D_GetCurrentSLIState(dx.device, &sliState) )
                {
                    if ( sliState.numAFRGroups >= 4 )
                    {
                        numAFRGroups = 4;
                    }
                    else
                    {
                        nvInitialized = sliState.numAFRGroups;
                        numAFRGroups = sliState.numAFRGroups;
                    }
                    dx.gpuCount = numAFRGroups;
                }
            }
        }
        if ( dx.vendorId == 4098 )
        {
            count = AtiQueryMgpuCount((void *)nvInitialized);
            if ( count < 1 )
                count = 1;
            if ( count >= 4 )
                v3 = 4;
            else
                v3 = count;
            dx.gpuCount = v3;
        }
    }
#endif
    Com_Printf(8, "Using %d GPU(s).\n", dx.gpuCount);
    R_StoreWindowSettings(wndParms);
    RB_InitSceneViewport();
    //BLOPS_NULLSUB();
    failed = R_CreateForInitOrReset() == 0;
    if ( failed )
        return 0;
    Com_Printf(8, "Setting initial state...\n");
    RB_SetInitialState();
    R_InitGamma();
    R_InitScene();
    R_InitSystems();
    //BLOPS_NULLSUB();
    R_FinishAttachingToWindow(wndParms);
    return 1;
}

void R_InitGamma()
{
    Dvar_SetModified(r_gamma);
}

void __cdecl R_FinishAttachingToWindow(const GfxWindowParms *wndParms)
{
    iassert((dx.windowCount >= 0 && dx.windowCount < ((1 / (((((0) ? (0x7fffffff) : (-0x7fffffff)) * (((0) & ~1) == 0))) / (0x7fffffff)) == 1) ? 5 : 1)));
    iassert(dx.windows[dx.windowCount].swapChain);

    dx.windows[dx.windowCount].hwnd = wndParms->hwnd;
    dx.windows[dx.windowCount].width = wndParms->displayWidth;
    dx.windows[dx.windowCount].height = wndParms->displayHeight;
    dx.windowCount++;
}

char __cdecl R_CreateForInitOrReset()
{
    const char *v0; // eax
    int gpuIdx; // [esp+8h] [ebp-10h]
    int hr; // [esp+Ch] [ebp-Ch]
    unsigned int fenceIter; // [esp+10h] [ebp-8h]

    Com_Printf(8, "Initializing render targets...\n");
    R_InitRenderTargets();
    RB_StreamInit();
    R_StreamInit();
    if ( !g_allocateMinimalResources )
    {
        R_InitRenderBuffers();
        R_InitModelLightingImage();
        Com_Printf(8, "Initializing static model cache...\n");
        R_InitStaticModelCache();
    }
    Com_Printf(8, "Initializing dynamic buffers...\n");
    R_CreateDynamicBuffers();
    if ( !g_allocateMinimalResources )
    {
        Com_Printf(8, "Initializing particle cloud buffer...\n");
        R_CreateParticleCloudBuffer();
    }

    Com_Printf(8, "Creating Direct3D queries...\n");
    dx.nextFence = 0;


    //hr = dx.device->CreateQuery(D3DQUERYTYPE_EVENT, &dx.gpuSyncDelay);
    hr = dx.device->CreateQuery(D3DQUERYTYPE_EVENT, &dx.flushGpuQuery);

    if ( hr >= 0 )
    {
        for (fenceIter = 0; fenceIter < 8; ++fenceIter)
        {
            hr = dx.device->CreateQuery(D3DQUERYTYPE_EVENT, &dx.fencePool[fenceIter]);
            iassert(hr >= 0); // ADD
        }
            
        if ( !g_allocateMinimalResources )
        {
            RB_AllocSunSpriteQueries();
            RB_AllocCoronaSpriteQueries();
            RB_AllocSuperFlareQueries();
            for ( gpuIdx = 0; gpuIdx < 4; ++gpuIdx )
                gfxAssets.pixelCountQuery[gpuIdx] = RB_HW_AllocOcclusionQuery();
        }
        return 1;
    }
    else
    {
        v0 = R_ErrorDescription(hr);
        Com_Printf(8, "Event query creation failed: %s (0x%08x)\n", v0, hr);
        return 0;
    }
}

IDirect3DQuery9 *__cdecl RB_HW_AllocOcclusionQuery()
{
    const char *v0; // eax
    int hr; // [esp+0h] [ebp-8h]
    IDirect3DQuery9 *query; // [esp+4h] [ebp-4h] BYREF


    hr = dx.device->CreateQuery(D3DQUERYTYPE_OCCLUSION, &query);
    //hr = ((int (__thiscall *)(IDirect3DDevice9 *, IDirect3DDevice9 *, int, IDirect3DQuery9 **))dx.device->CreateQuery)(
    //             dx.device,
    //             dx.device,
    //             9,
    //             &query);
    if ( hr >= 0 )
        return query;
    v0 = R_ErrorDescription(hr);
    Com_Printf(8, "Occlusion query creation failed: %s (0x%08x)\n", v0, hr);
    return 0;
}

char __cdecl R_CreateWindow(GfxWindowParms *wndParms)
{
#if defined(__ANDROID__)
    if (!wndParms || wndParms->hwnd) {
        Com_Error(ERR_FATAL, "renderer: invalid window creation state"); std::abort();
    }
    // The Windows private desktop has no Android equivalent. Do not expose a
    // supposedly headless client window on the user's actual Activity.
    if (Sys_IsHeadless()) {
        Com_Error(ERR_FATAL, "renderer: Android headless window creation is unsupported"); std::abort();
    }
    SDL_Window *window{};
    auto spec = R_SDLWindowSpec(*wndParms);
    if (!bo1::android::CreateRendererWindow(spec, &window)) {
        Com_Printf(8, "Couldn't create SDL Vulkan window: %s\n", SDL_GetError()); return 0;
    }
    bo1::android::RendererWindowInfo info{};
    if (!bo1::android::InspectRendererWindow(window, &info)) {
        if (!bo1::android::DestroyRendererWindow(window)) R_SDLWindowFailure("invalid drawable cleanup");
        Com_Printf(8, "SDL window has no usable drawable: %s\n", SDL_GetError()); return 0;
    }
    wndParms->displayWidth=info.drawableWidth; wndParms->displayHeight=info.drawableHeight;
    wndParms->hwnd = reinterpret_cast<HWND__ *>(window);
    s_borderlessWindow = wndParms->borderless;
    Com_Printf(8, "Game SDL Vulkan window successfully created.\n");
    return 1;
#else

    const char *BuildOfficialNameR; // eax
    int x; // [esp-20h] [ebp-3Ch]
    int y; // [esp-1Ch] [ebp-38h]
    int v5; // [esp-18h] [ebp-34h]
    int v6; // [esp-14h] [ebp-30h]
    DWORD exStyle; // [esp+0h] [ebp-1Ch]
    DWORD style; // [esp+4h] [ebp-18h]
    HINSTANCE__ *hinst; // [esp+8h] [ebp-14h]
    tagRECT rc; // [esp+Ch] [ebp-10h] BYREF

    if (!wndParms
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 2230, 0, "%s", "wndParms"))
    {
        __debugbreak();
    }
    if (wndParms->hwnd
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
            2231,
            0,
            "%s",
            "wndParms->hwnd == NULL"))
    {
        __debugbreak();
    }
    if (wndParms->borderless)
    {
        // borderless: a visible WS_POPUP window without a frame, exactly the monitor's rect; not topmost
        Com_Printf(
            8,
            "Attempting %i x %i borderless window at (%i, %i)\n",
            wndParms->displayWidth,
            wndParms->displayHeight,
            wndParms->x,
            wndParms->y);
        exStyle = 0;
        style = WS_POPUP | WS_VISIBLE;
    }
    else if (wndParms->fullscreen)
    {
        Com_Printf(
            8,
            "Attempting %i x %i fullscreen with 32 bpp at %i hz\n",
            wndParms->displayWidth,
            wndParms->displayHeight,
            wndParms->hz);
        exStyle = 8;
        style = 0x80000000;
    }
    else
    {
        Com_Printf(
            8,
            "Attempting %i x %i window at (%i, %i)\n",
            wndParms->displayWidth,
            wndParms->displayHeight,
            wndParms->x,
            wndParms->y);
        exStyle = 0;
        style = 281542656;
    }
    rc.left = 0;
    rc.right = wndParms->displayWidth;
    rc.top = 0;
    rc.bottom = wndParms->displayHeight;
    AdjustWindowRectEx(&rc, style, 0, exStyle);
    hinst = GetModuleHandleA(0);
    v6 = rc.bottom - rc.top;
    v5 = rc.right - rc.left;
    y = wndParms->y;
    x = wndParms->x;
    BuildOfficialNameR = Com_GetBuildOfficialNameR();
    wndParms->hwnd = CreateWindowExA(exStyle, "CoDBlackOps", BuildOfficialNameR, style, x, y, v5, v6, 0, 0, hinst, 0);
    if (wndParms->hwnd)
    {
        if (!wndParms->fullscreen)
        {
            SetWindowPos(wndParms->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, 3u);
            if ( !Sys_IsHeadlessClient() ) // zombies: a headless client never changes focus
                SetFocus(wndParms->hwnd);
        }
        s_borderlessWindow = wndParms->borderless; // borderless
        Com_Printf(8, "Game window successfully created.\n");
        return 1;
    }
    else
    {
        Com_Printf(8, "Couldn't create a window.\n");
        return 0;
    }

#endif
}

char __cdecl R_ReduceWindowSettings()
{
    if ( r_aaSamples->current.integer <= 1 )
    {
        if ( r_displayRefresh->current.integer <= 0 || vidConfig.displayFrequency <= 0x3C )
        {
            if ( r_mode->current.integer <= 0 || vidConfig.displayWidth <= 0x280 && vidConfig.displayHeight <= 0x1E0 )
            {
                return 0;
            }
            else
            {
                Dvar_SetInt((dvar_s *)r_mode, r_mode->current.integer - 1);
                return 1;
            }
        }
        else
        {
            Dvar_SetInt((dvar_s *)r_displayRefresh, r_displayRefresh->current.integer - 1);
            return 1;
        }
    }
    else
    {
        Dvar_SetInt((dvar_s *)r_aaSamples, r_aaSamples->current.integer - 1);
        return 1;
    }
}

void R_Register()
{
    R_RegisterDvars();
    R_RegisterCmds();
    //BLOPS_NULLSUB();
}

void R_InitGlobalStructs()
{
    memset((unsigned __int8 *)&rg, 0, sizeof(rg));
    memset((unsigned __int8 *)&rgp, 0, sizeof(rgp));
    RB_InitBackendGlobalStructs();
    g_drawConsts.identityPlacement.base.quat[0] = 0.0f;
    g_drawConsts.identityPlacement.base.quat[1] = 0.0f;
    g_drawConsts.identityPlacement.base.quat[2] = 0.0f;
    g_drawConsts.identityPlacement.base.quat[3] = 1.0f;
    g_drawConsts.identityPlacement.base.origin[0] = 0.0f;
    g_drawConsts.identityPlacement.base.origin[1] = 0.0f;
    g_drawConsts.identityPlacement.base.origin[2] = 0.0f;
    g_drawConsts.identityPlacement.scale = 1.0f;
    MatrixIdentity44(rg.identityViewParms.viewMatrix.m);
    MatrixIdentity44(rg.identityViewParms.projectionMatrix.m);
    MatrixIdentity44(rg.identityViewParms.viewProjectionMatrix.m);
    MatrixIdentity44(rg.identityViewParms.inverseViewProjectionMatrix.m);
}

void __cdecl R_ShutdownStreams()
{
    if ( dx.device )
        R_ClearAllStreamSources(&gfxCmdBufState.prim);
}

void __cdecl R_ShutdownInternal()
{

    if ( rg.registered )
    {
        R_SyncRenderThread();
        rg.registered = 0;
        Sys_ClearRGRegisteredEvent();
        r_glob.startedRenderThread = 0;
        R_StreamSyncThenFlush(0);
        R_StreamShutdown();
        R_ShutdownStreams();
        R_ShutdownMaterialUsage();
        R_ShutdownDebug();
        R_SaveLightVisHistory();
        R_ShutdownLightDefs();
        R_ShutdownWorld();
        R_ShutdownModels();
        if ( !useFastFile->current.enabled )
        {
            R_ShutdownLoadWater();
            R_ShutdownFonts();
            Material_Shutdown();
            R_ShutdownImages();
        }
        R_VertexStream2_Shutdown();
        R_FoliageShutdown();
        R_ClearSkyboxModel();
        R_ResetModelLighting();
        rgp.world = 0;
        R_UnlockSkinnedCache();
        R_FlushStaticModelCache();
        if ( g_destroy_window )
        {
            R_ShutdownDirect3D();
            R_ShutdownRenderCommands();
        }
        R_UnregisterCmds();
    }
}

void R_ShutdownDirect3D()
{
    R_ShutdownDirect3DInternal();
}

void R_ShutdownDirect3DInternal()
{
#if defined(__ANDROID__)
    R_FreeWaterSimulationBuffers();
    if (useFastFile->current.enabled) R_UnloadGraphicsAssets();
    R_Cinematic_Shutdown();
    R_ReleaseForShutdownOrReset();
    if (dx.device) { R_SDLOwner([&] { return dx.device->Release(); }); dx.device = nullptr; }
    dx.nvStereoHandle = nullptr; dx.nvStereoActivated = false; dx.nvInitialized = false;
    if (dx.d3d9) { R_SDLOwner([&] { return dx.d3d9->Release(); }); dx.d3d9 = nullptr; }
    while (dx.windowCount) {
        auto &window = dx.windows[dx.windowCount - 1];
        if (!bo1::android::DestroyRendererWindow(reinterpret_cast<SDL_Window *>(window.hwnd))) R_SDLWindowFailure("window destruction");
        window.hwnd = nullptr; --dx.windowCount;
    }
    if (!bo1::android::RetireRendererVideo()) R_SDLWindowFailure("video retirement");
#else

    R_FreeWaterSimulationBuffers();
    if (useFastFile->current.enabled)
        R_UnloadGraphicsAssets();
    R_Cinematic_Shutdown();
    R_ReleaseForShutdownOrReset();
    while (dx.windowCount)
    {
        if (!dx.windows[--dx.windowCount].hwnd
            && !Assert_MyHandler(
                "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                3029,
                0,
                "%s",
                "dx.windows[dx.windowCount].hwnd"))
        {
            __debugbreak();
        }
        if (IsWindow(dx.windows[dx.windowCount].hwnd))
            DestroyWindow(dx.windows[dx.windowCount].hwnd);
        dx.windows[dx.windowCount].hwnd = 0;
    }
    if (dx.device)
    {
        NvAPI_Stereo_DestroyHandle(dx.nvStereoHandle);
        dx.nvStereoHandle = 0;
        //((void(__thiscall *)(IDirect3DDevice9 *, IDirect3DDevice9 *))dx.device->Release)(dx.device, dx.device);
        dx.device->Release();
        dx.device = 0;
    }
    if (dx.d3d9)
    {
        //dx.d3d9->Release(dx.d3d9);
        dx.d3d9->Release();
        dx.d3d9 = 0;
    }

#endif
}

void R_UnloadGraphicsAssets()
{
    DB_ShutdownXAssets();
}

void __cdecl R_Shutdown(int destroyWindow)
{
#if defined(__ANDROID__)
    if (!bo1::android::HasSpawnedRenderThread())
    {
        // Dedicated startup owns database assets but creates no renderer/event
        // consumer. Never send a teardown request to an absent worker.
        if (!Sys_IsMainThread() || rg.registered || r_glob.startedRenderThread
            || dx.device || dx.d3d9 || dx.windowCount || dx.nvStereoHandle)
        {
            Com_Error(ERR_FATAL, "Renderer resources exist without an actual shutdown worker");
            std::abort();
        }
        g_destroy_window = destroyWindow;
        R_ShutdownInternal();
        return;
    }
#endif
    if ( Sys_QueryD3DShutdownEvent()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    3389,
                    0,
                    "%s",
                    "Sys_QueryD3DShutdownEvent() == qfalse") )
    {
        __debugbreak();
    }
    g_destroy_window = destroyWindow;
    Sys_SetD3DShutdownEvent();
    while ( Sys_QueryD3DShutdownEvent() )
        NET_Sleep(1u);
}



void __cdecl R_BeginRegistrationInternal()
{
    if ( rg.registered
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 3418, 0, "%s", "!rg.registered") )
    {
        __debugbreak();
    }
    R_Init();
    if ( (!dx.d3d9 || !dx.device)
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    3431,
                    0,
                    "%s",
                    "dx.d3d9 && dx.device") )
    {
        __debugbreak();
    }
    Sys_WaitRGRegisteredEvent();
    if ( !rg.registered
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    3435,
                    1,
                    "%s",
                    "rg.registered == true") )
    {
        __debugbreak();
    }
}

void __cdecl R_BeginRegistration(vidConfig_t *vidConfigOut)
{
    Sys_SetRGRegisteredEvent();
    while ( Sys_QueryRGRegisteredEvent() )
        NET_Sleep(0);
    if ( !vidConfigOut
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 3491, 0, "%s", "vidConfigOut") )
    {
        __debugbreak();
    }
    memcpy(vidConfigOut, &vidConfig, sizeof(vidConfig_t));
    if ( r_glob.startedRenderThread
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp",
                    3495,
                    0,
                    "%s",
                    "!r_glob.startedRenderThread") )
    {
        __debugbreak();
    }
    r_glob.startedRenderThread = 1;
    //BLOPS_NULLSUB();
}

void __cdecl R_EndRegistration()
{
    if ( !rg.registered
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 3505, 0, "%s", "rg.registered") )
    {
        __debugbreak();
    }
    //BLOPS_NULLSUB();
    if ( !useFastFile->current.enabled )
    {
        R_SyncRenderThread();
        RB_TouchAllImages();
    }
}

#endif // normal renderer bodies before shared configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS) || defined(BO1_NATIVE_RENDER_CONFIG)
void __cdecl R_TrackStatistics(trStatistics_t *stats)
{
    rg.globalstats = stats;
}

#endif // shared renderer configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS)
void __cdecl R_SyncGpu()
{
    R_FinishGpuFence();
}

void __cdecl R_UpdateTeamColors(const float *color_allies, const float *color_axis)
{
    Byte4PackRgba(color_allies, (unsigned __int8 *)&rg.color_allies);
    Byte4PackRgba(color_axis, (unsigned __int8 *)&rg.color_axis);
}

#endif // normal renderer bodies before shared configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS) || defined(BO1_NATIVE_RENDER_CONFIG)
void __cdecl R_ConfigureRenderer(const GfxConfiguration *config)
{
    SetGfxConfig(config);
    R_InitRenderCommands();
}

#endif // shared renderer configuration
#if !defined(BO1_NATIVE_RENDER_GLOBALS)
char __cdecl R_ReleaseLostDeviceAssets2()
{

    while ( !R_FinishedFrontendWorkerCmds() )
        NET_Sleep(1u);
    yuv_lost_device();
    Com_Printf(8, " - R_Cinematic_BeginLostDevice()...\n");
    R_Cinematic_BeginLostDevice();
    Com_Printf(8, "DB_BeginRecoverLostDevice()...\n");
    while ( !Sys_IsDatabaseReady() )
    {
        if ( r_glob.isRenderingRemoteUpdate )
            Sys_LeaveCriticalSection(CRITSECT_DBHASH);
        RB_Resource_Update(5);
        NET_Sleep(1u);
        if ( r_glob.isRenderingRemoteUpdate )
            Sys_EnterCriticalSection(CRITSECT_DBHASH);
    }
    DB_BeginRecoverLostDevice();
    Com_Printf(8, "R_ResetModelLighting()...\n");
    R_ResetModelLighting();
    Com_Printf(8, "R_ReleaseAllModels()...\n");
    R_ReleaseAllModels();
    Com_Printf(8, "R_ReleaseLostImages()...\n");
    R_ReleaseLostImages();
    Com_Printf(8, "R_ReleaseWorld()...\n");
    R_ReleaseWorld();
    return 1;
}

char __cdecl R_RecoverLostDevice()
{
    if ( !dx.device
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 3951, 0, "%s", "dx.device") )
    {
        __debugbreak();
    }
    Com_Printf(8, "Recovering lost device...\n");
    Com_Printf(8, "R_ResetDevice()...\n");
    if ( !R_ResetDevice() )
        return 0;
    yuv_recover_device();
    Com_Printf(8, "R_ReloadWorld()...\n");
    R_ReloadWorld();
    Com_Printf(8, "R_ReloadLostImages()...\n");
    R_ReloadLostImages();
    Com_Printf(8, "R_OptimizeAllModels()...\n");
    R_OptimizeAllModels();
    Com_Printf(8, "DB_EndRecoverLostDevice()...\n");
    DB_EndRecoverLostDevice();
    Com_Printf(8, "R_Cinematic_EndLostDevice()...\n");
    R_Cinematic_EndLostDevice();
    Com_Printf(8, "R_AbortRenderCommands()...\n");
    R_AbortRenderCommands();
    Com_Printf(8, "R_PushRemoteScreenUpdate()...\n");
    Com_Printf(8, "Finished recovering lost device.\n");
    return 1;
}

char __cdecl R_ResetDevice()
{
    _D3DPRESENT_PARAMETERS_ d3dpp; // [esp+0h] [ebp-74h] BYREF
    GfxWindowParms wndParms; // [esp+3Ch] [ebp-38h] BYREF
    tagRECT rc; // [esp+64h] [ebp-10h] BYREF

    wndParms.hwnd = dx.windows[0].hwnd;
    wndParms.x = 0;
    wndParms.y = 0;
    wndParms.displayWidth = dx.windows[0].width;
    wndParms.displayHeight = dx.windows[0].height;
    wndParms.sceneWidth = dx.windows[0].width;
    wndParms.sceneHeight = dx.windows[0].height;
    wndParms.hz = vidConfig.displayFrequency;
    wndParms.fullscreen = vidConfig.isFullscreen != 0;
    wndParms.aaSamples = r_aaSamples->current.integer;
    wndParms.borderless = s_borderlessWindow; // borderless
    R_SetD3DPresentParameters(&d3dpp, &wndParms);
    if (d3dpp.Windowed)
        Com_Printf(8, "Resetting %i x %i window.\n", d3dpp.BackBufferWidth, d3dpp.BackBufferHeight);
    else
        Com_Printf(8, "Resetting %i x %i fullscreen.\n", d3dpp.BackBufferWidth, d3dpp.BackBufferHeight);
    if (R_WSI_CALL(dx.device->Reset(&d3dpp)))
        return 0;
#if defined(__ANDROID__)
    if (!bo1::android::ConfigureRendererWindow(reinterpret_cast<SDL_Window *>(wndParms.hwnd), R_SDLWindowSpec(wndParms)))
        R_SDLWindowFailure("window reset sizing");
#else
    if (wndParms.fullscreen)
    {
        SetWindowPos(wndParms.hwnd, (HWND)(/*HWND_MESSAGE*/-3 | 0x2), 0, 0, 0, 0, 3u);
    }
    else if (wndParms.borderless)
    {
        // borderless: no frame to add - the window stays exactly the back buffer's (the monitor's) size, not topmost
        SetWindowPos(wndParms.hwnd, HWND_NOTOPMOST, 0, 0, wndParms.displayWidth, wndParms.displayHeight, 0x62u);
    }
    else
    {
        rc.left = 0;
        rc.right = wndParms.displayWidth;
        rc.top = 0;
        rc.bottom = wndParms.displayHeight;
        AdjustWindowRectEx(&rc, 0xC80000u, 0, 0);
        SetWindowPos(wndParms.hwnd, HWND_NOTOPMOST, wndParms.x, wndParms.y, rc.right - rc.left, rc.bottom - rc.top, 0x62u);
    }
#endif
    if (!R_CreateForInitOrReset())
        R_FatalInitError("Couldn't reinitialize after a lost Direct3D device");
    R_BorderlessLogWindow("reset"); // borderless
    R_InitCmdBufSourceState(&gfxCmdBufSourceState, &gfxCmdBufInput, 0);
    R_InitCmdBufState(&gfxCmdBufState);
    RB_InitSceneViewport();
    R_SetRenderTargetSize(&gfxCmdBufSourceState, R_RENDERTARGET_FRAME_BUFFER);
    R_SetRenderTarget(gfxCmdBufContext, R_RENDERTARGET_FRAME_BUFFER);
    return 1;
}

#endif // normal reset before shared renderer error cleanup
#if !defined(BO1_NATIVE_RENDER_GLOBALS) || defined(BO1_NATIVE_RENDER_CONFIG)
#if defined(BO1_NATIVE_RENDER_CONFIG)
#include <win32/win_main.h>
#include <qcommon/threads.h>
#include <gfx_d3d/rb_logfile.h>
#endif
void __cdecl R_ComErrorCleanup()
{
    const char *v1; // eax
    int hr; // [esp+4h] [ebp-4h]

    if (!Sys_IsMainThread()
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 4028, 0, "%s", "Sys_IsMainThread()"))
    {
        __debugbreak();
    }
    R_AbortRenderCommands();
    R_SyncRenderThread();
    if (dx.inScene)
    {
        if (r_logFile && r_logFile->current.integer)
            RB_LogPrint("dx.device->EndScene()\n");
        hr = dx.device->EndScene();
        if (hr < 0)
        {
            ++g_disableRendering;
            v1 = R_ErrorDescription(hr);
            Com_Error(
                ERR_FATAL,
                "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp (%i) dx.device->EndScene() failed: %s\n",
                4036,
                v1);
        }
        dx.inScene = 0;
    }
}

// Stereo query shares the configuration/error-cleanup selection.
bool __cdecl R_StereoActivated()
{
    return dx.nvStereoActivated;
}


#endif

#if defined(BO1_NATIVE_RENDER_GLOBALS)
#include <qcommon/common.h>
#include <win32/win_main.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#endif
void __cdecl    R_FatalInitError(const char *msg)
{
    Com_Printf(8, "********** DirectX returned an unrecoverable error code during initialization    **********\n");
    Com_Printf(8, "********** Initialization also happens while playing if DirectX loses a device **********\n");
    Com_Printf(8, "********** Consult the readme for how to continue from this problem                        **********\n");
    Com_Printf(8, "\n%s\n", msg);
    Sys_DirectXFatalError();
#if defined(__ANDROID__)
    std::abort(); // Do not publish renderer state if the fatal callback returns.
#endif
}

const char *__cdecl R_ErrorDescription(HRESULT hr)
{
#if defined(__ANDROID__)
    thread_local char text[48];
    static_assert(sizeof(hr)==sizeof(std::uint32_t));
    std::snprintf(text,sizeof(text),"Direct3D HRESULT 0x%08x",static_cast<std::uint32_t>(hr));
    return text;
#else
    return DXGetErrorDescriptionA(hr);
#endif
}

void __cdecl R_UnloadWorld()
{
    if ( !useFastFile->current.enabled
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_init.cpp", 3408, 0, "%s", "IsFastFileLoad()") )
    {
        __debugbreak();
    }
    if ( rgp.world )
        Sys_Error((char*)"Cannot unload bsp while it is in use");
}

void __cdecl    R_FatalLockError(HRESULT hr)
{
    const char *unitScaleValue; // eax

    Com_Printf(8, "********** DirectX failed a call to lock a vertex buffer or an index buffer **********\n");
    unitScaleValue = R_ErrorDescription(hr);
    Com_Printf(8, "********** error information:    %s\n", unitScaleValue);
    Sys_DirectXFatalError();
#if defined(__ANDROID__)
    std::abort(); // Do not publish renderer state if the fatal callback returns.
#endif
}
