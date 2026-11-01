#include <cgame_mp/cg_main_mp.h>

bool CG_ValidateSavedBolts(const CGBoltSavedState &state)
{
    bool seen[512]{};
    for (const auto &key : state.slots)
    {
        if (key.number == -1)
        {
            if (key.currentUse || key.nextUse) return false;
            continue;
        }
        if (key.number < 1024 || key.number >= ENTNUM_CSC_END
            || seen[key.number - 1024]) return false;
        seen[key.number - 1024] = true;
    }
    return true;
}

bool CG_CaptureBolts(int localClientNum, CGBoltSavedState &state)
{
    if (localClientNum || !cg_fakeEntitiesArray) return false;
    CGBoltSavedState result{};
    result.cursor = g_BoltIndex[0];
    for (int slot = 0; slot < 10; ++slot)
    {
        const auto *cent = g_BoltEnt[0][slot];
        if (!cent) continue;
        // Compare addresses before reading a borrower: stale/non-pool pointers
        // must not be dereferenced, nor subtracted from an unrelated array.
        int index = -1;
        for (int i = 0; i < 512; ++i)
            if (cent == &cg_fakeEntitiesArray[i].cent) { index = i; break; }
        if (index < 0 || !cg_fakeEntitiesInuseArray[index] || 1024 + index >= g_entNumCscEnd
            || cent->nextState.number != 1024 + index || cent->nextState.eType != ET_SCRIPTMOVER) return false;
        result.slots[slot] = {1024 + index, cent->currentState.useCount, cent->nextState.lerp.useCount};
    }
    if (!CG_ValidateSavedBolts(result)) return false;
    state = result;
    return true;
}

bool CG_ApplyBolts(int localClientNum, const CGBoltSavedState &state)
{
    if (localClientNum || !cg_fakeEntitiesArray || !CG_ValidateSavedBolts(state)) return false;
    centity_s *resolved[10]{};
    for (int slot = 0; slot < 10; ++slot)
    {
        const auto &key = state.slots[slot];
        if (key.number == -1) continue;
        if (key.number >= g_entNumCscEnd || !cg_fakeEntitiesInuseArray[key.number - 1024]) return false;
        auto *cent = &cg_fakeEntitiesArray[key.number - 1024].cent;
        if (cent->nextState.number != key.number || cent->nextState.eType != ET_SCRIPTMOVER
            || cent->currentState.useCount != key.currentUse || cent->nextState.lerp.useCount != key.nextUse) return false;
        resolved[slot] = cent;
    }
    // Publish only after all owners resolve, before saved frames are released.
    for (int slot = 0; slot < 10; ++slot) g_BoltEnt[0][slot] = resolved[slot];
    g_BoltIndex[0] = state.cursor;
    return true;
}

void CG_ClearBoltReferences(int localClientNum)
{
    if (localClientNum) return;
    for (auto &cent : g_BoltEnt[0]) cent = nullptr;
    g_BoltIndex[0] = 0;
}
