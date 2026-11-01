#pragma once

void __cdecl CG_InitBolt(unsigned int localClientNum);
unsigned int __cdecl CG_AddBolt(
                int localClientNum,
                int targetEntityNum,
                unsigned int weapon,
                const float *position,
                const float *normal,
                unsigned __int8 tagIndex,
                int physUserBody);
void __cdecl CG_UpdateBolt(int localClientNum);

#if defined(__ANDROID__)
// Borrowed fake-entity identities only; entity/network checkpoints own the data.
struct CGBoltSavedState
{
    unsigned int cursor = 0;
    struct Key { int number = -1, currentUse = 0, nextUse = 0; } slots[10];
};
bool CG_ValidateSavedBolts(const CGBoltSavedState &state);
bool CG_CaptureBolts(int localClientNum, CGBoltSavedState &state);
bool CG_ApplyBolts(int localClientNum, const CGBoltSavedState &state);
void CG_ClearBoltReferences(int localClientNum);
#endif
