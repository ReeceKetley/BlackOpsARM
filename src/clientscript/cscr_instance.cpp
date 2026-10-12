#include "cscr_instance.h"
#if !defined(BO1_STRING_POOL_GATE)
#include "cscr_variable.h"
#endif
#include "cscr_stringlist.h"

#if !defined(BO1_STRING_POOL_GATE)
scrVarDebugPub_t gScrVarDebugPubBuff[2];
#endif
scrStringDebugGlob_t gScrStringDebugGlobBuff[2];

#if !defined(BO1_STRING_POOL_GATE)
void __cdecl Scr_ClearScrVarDebugPub(scriptInstance_t inst)
{
    gScrVarDebugPub[inst] = 0;
}

void __cdecl Scr_ResetScrVarDebugPub(scriptInstance_t inst)
{
    gScrVarDebugPub[inst] = &gScrVarDebugPubBuff[inst];
}

#endif // unrelated variable-debug globals remain in normal engine builds

void __cdecl Scr_ClearScrStringDebugGlob(scriptInstance_t inst)
{
    gScrStringDebugGlob[inst] = 0;
}

void __cdecl Scr_ResetScrStringDebugGlob(scriptInstance_t inst)
{
    gScrStringDebugGlob[inst] = &gScrStringDebugGlobBuff[inst];
}

