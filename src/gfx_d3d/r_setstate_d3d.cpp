#include "r_setstate_d3d.h"
#include "r_init.h"
#if defined(__ANDROID__)
#include <qcommon/common.h>
#include <cstdlib>
#include <thread>
#endif

void __cdecl R_FinishGpuFence()
{
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
