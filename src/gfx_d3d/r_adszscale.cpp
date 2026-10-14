#include "r_adszscale.h"
#include "rb_state.h"
#include "r_foliage.h"

int activeAdsZScale;

float adsZScale[2][1] = { { 1.0 }, { 1.0 } };

#if BO1_NATIVE_64BIT && !defined(_WIN32)
// Two frame slots, each holding the sole local client. Keep the original storage.
static_assert(sizeof(adsZScale) / sizeof(adsZScale[0]) == 2);
static_assert(sizeof(adsZScale[0]) / sizeof(adsZScale[0][0]) == 1);
static_assert(sizeof(adsZScale) == 8);
#endif

void __cdecl R_StartADSZScaleFrame()
{
    ++activeAdsZScale;
}

void __cdecl R_SetADSZScale(unsigned int viewport, float pos)
{
#if BO1_NATIVE_64BIT && !defined(_WIN32)
    iassert(viewport < sizeof(adsZScale[0]) / sizeof(adsZScale[0][0]));
    adsZScale[(activeAdsZScale + 1) % 2][viewport] = pos;
#else
    adsZScale[viewport][(activeAdsZScale + 1) % 2] = pos;
#endif
}

void __cdecl R_SetADSZScaleConstants(unsigned int viewport, GfxCmdBufSourceState *source)
{
#if BO1_NATIVE_64BIT && !defined(_WIN32)
    iassert(viewport < sizeof(adsZScale[0]) / sizeof(adsZScale[0][0]));
#endif
    R_SetCodeConstant(source,
        CONST_SRC_CODE_ADSZSCALE,
#if BO1_NATIVE_64BIT && !defined(_WIN32)
        adsZScale[activeAdsZScale % 2][viewport],
#else
        adsZScale[viewport][activeAdsZScale % 2],
#endif
        0.0f,
        0.0f,
        0.0f
        );
}

