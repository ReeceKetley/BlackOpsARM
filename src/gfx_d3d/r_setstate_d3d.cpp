#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
#include "gfx_vulkan/event_fences.h"
#include "gfx_vulkan/shader_assets.h"
#endif
#include "r_setstate_d3d.h"
#include "r_init.h"
#if defined(__ANDROID__)
#include <qcommon/common.h>
#include <cstdlib>
#include <thread>
#endif

void __cdecl R_FinishGpuFence()
{
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if(bo1::vulkan::shader_assets::Active()) {
        std::string error;if(!bo1::vulkan::event_fences::WaitQueue(error)) {Com_Error(ERR_FATAL,"Native GPU finish: %s",error.c_str());std::abort();}return;
    }
#endif
#if defined(__ANDROID__)
    if (!dx.device || !dx.flushGpuQuery) { Com_Error(ERR_FATAL,"GPU fence wait requires the actual device/query"); std::abort(); }
    for (;;) {
        HRESULT result=dx.flushGpuQuery->GetData(nullptr,0,D3DGETDATA_FLUSH);
        if (result==S_OK) return;
        if (result!=S_FALSE) { Com_Error(ERR_FATAL,"GPU fence GetData failed: %s",R_ErrorDescription(result)); std::abort(); }
        std::this_thread::yield();
    }
#else

    ///while ( LODWORD(dx.gpuSyncDelay)
    ///         && (*(int (__stdcall **)(unsigned int, unsigned int, unsigned int, int))(*(unsigned int *)LODWORD(dx.gpuSyncDelay) + 28))(
    ///                    dx.gpuSyncDelay,
    ///                    0,
    ///                    0,
    ///                    1) == 1 )
    ///    ;

    //while (dx.gpuSyncDelay->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
    while (dx.flushGpuQuery->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
    {
        /* spin until GPU reaches the fence */
    }
#endif
}
