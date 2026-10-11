#pragma once
#include "mem_userhunk.h"

struct _fixed_heapnode { _fixed_heapnode *next; };
struct HunkUserFixed
{
    HunkUser hunkUser;
    unsigned int size;
    unsigned int blockSize;
    _fixed_heapnode *freeHead;
};
HunkUser *__cdecl Hunk_FixedInit(void *buffer, int size, HU_ALLOCATION_SCHEME scheme,
    unsigned int flags, void *scheme_specific_data, const char *name, int type);
void __cdecl Hunk_FixedReset(HunkUser *user);
void __cdecl Hunk_FixedDestroy(HunkUser *user);
void *__cdecl Hunk_FixedAlloc(HunkUser *user, int size, int alignment, const char *name);
void __cdecl Hunk_FixedFree(HunkUser *user, void *ptr);
