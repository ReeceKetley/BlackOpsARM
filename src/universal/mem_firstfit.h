#pragma once
#include "mem_userhunk.h"

struct _firstfit_heapnode
{
    _firstfit_heapnode *next;
    int size;
};

// Native layout (Win32 packed the same fields into HunkUser[2] via uint32 overlays).
struct FirstFitHunkUser
{
    HunkUser hunkUser;
    unsigned int size;
    _firstfit_heapnode *free_blocks;
    int reserved;
    int used;
};

HunkUser *__cdecl Hunk_FirstFitInit(
                void *buffer,
                unsigned int size,
                HU_ALLOCATION_SCHEME scheme,
                unsigned int flags,
                void *scheme_specific_data,
                const char *name,
                int type);
void __cdecl Hunk_FirstFitReset(HunkUser *_user);
void __cdecl Hunk_FirstFitDestroy(HunkUser *_user);
void *__cdecl Hunk_FirstFitAlloc(HunkUser *_user, int size, int alignment);
void __cdecl Hunk_FirstFitFree(HunkUser *_user, void *ptr);
