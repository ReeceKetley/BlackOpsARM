#include "r_staticmodel.h"
#include "r_bsp.h"
#include "r_init.h"
#if defined(__ANDROID__)
#include <qcommon/common.h>
#include <cstdlib>
#include <climits>
#endif

int __cdecl R_StaticModelGetMemoryUsage(XModel *model, int *modelCount)
{
#if defined(__ANDROID__)
    if (!modelCount) { Com_Error(ERR_FATAL,"Static model memory query has no count output"); std::abort(); }
    *modelCount=0;
    const GfxWorld *world=rgp.world;
    if (!world) return 0;
    if (!model || world->dpvsPlanes.cellCount < 0 || (world->dpvsPlanes.cellCount && !world->cells)
        || (world->dpvs.smodelCount && !world->dpvs.smodelDrawInsts)) {
        Com_Error(ERR_FATAL,"Invalid static model world/query ownership"); std::abort();
    }
    int usage=0, matches=0;
    const int instanceBytes=R_StaticModelGetMemoryUsageInst();
    for (int cellIndex=0;cellIndex<world->dpvsPlanes.cellCount;++cellIndex) {
        const GfxCell &cell=world->cells[cellIndex];
        if (cell.aabbTreeCount < 0 || (cell.aabbTreeCount && !cell.aabbTree)) {
            Com_Error(ERR_FATAL,"Invalid static model cell tree ownership"); std::abort();
        }
        if (!cell.aabbTreeCount) continue; // An actual empty cell has no root tree.
        const GfxAabbTree &tree=*cell.aabbTree;
        if (tree.smodelIndexCount && !tree.smodelIndexes) {
            Com_Error(ERR_FATAL,"Static model root tree has no index array"); std::abort();
        }
        for (unsigned iter=0;iter<tree.smodelIndexCount;++iter) {
            unsigned index=tree.smodelIndexes[iter];
            if (index>=world->dpvs.smodelCount) {
                Com_Error(ERR_FATAL,"Static model index exceeds actual world count"); std::abort();
            }
            if (world->dpvs.smodelDrawInsts[index].model==model) {
                if (matches==INT_MAX || usage>INT_MAX-instanceBytes) {
                    Com_Error(ERR_FATAL,"Static model memory query exceeds its integer output capacity"); std::abort();
                }
                usage+=instanceBytes; ++matches;
            }
        }
    }
    *modelCount=matches;
    return usage;
#else

    int cellIndex; // [esp+0h] [ebp-14h]
    GfxAabbTree *tree; // [esp+8h] [ebp-Ch]
    int smodelIndexIter; // [esp+Ch] [ebp-8h]
    int usage; // [esp+10h] [ebp-4h]

    usage = 0;
    *modelCount = 0;
    if ( !rgp.world )
        return 0;
    for ( cellIndex = 0; cellIndex < rgp.world->dpvsPlanes.cellCount; ++cellIndex )
    {
        tree = rgp.world->cells[cellIndex].aabbTree;
        for ( smodelIndexIter = 0; smodelIndexIter < tree->smodelIndexCount; ++smodelIndexIter )
        {
            if ( rgp.world->dpvs.smodelDrawInsts[tree->smodelIndexes[smodelIndexIter]].model == model )
            {
                usage += R_StaticModelGetMemoryUsageInst();
                ++*modelCount;
            }
        }
    }
    return usage;

#endif
}

int __cdecl R_StaticModelGetMemoryUsageInst()
{
#if defined(__ANDROID__)
    static_assert(sizeof(GfxStaticModelDrawInst)<=INT_MAX);
    return static_cast<int>(sizeof(GfxStaticModelDrawInst));
#else
    return 76;
#endif
}

