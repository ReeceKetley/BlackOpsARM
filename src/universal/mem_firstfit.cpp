#include "mem_firstfit.h"
#include "assertive.h"
#if defined(__ANDROID__)
#include <platform/android/hunk_services.h>
#else
#include <win32/win_common.h>
#endif
#include <cstdint>

static void FirstFit_Lock()
{
#if defined(__ANDROID__)
    bo1::android::LockFirstFit();
#else
    Sys_EnterCriticalSection(CRITSECT_MEMFIRSTFIT);
#endif
}
static void FirstFit_Unlock()
{
#if defined(__ANDROID__)
    bo1::android::UnlockFirstFit();
#else
    Sys_LeaveCriticalSection(CRITSECT_MEMFIRSTFIT);
#endif
}

static constexpr uintptr_t kFirstFitBusyTag = static_cast<uintptr_t>(static_cast<intptr_t>(-559038737));

HunkUser *__cdecl Hunk_FirstFitInit(
                void *buffer,
                unsigned int size,
                HU_ALLOCATION_SCHEME scheme,
                unsigned int flags,
                void *scheme_specific_data,
                const char *name,
                int type)
{
    (void)scheme_specific_data;
    FirstFitHunkUser *user = reinterpret_cast<FirstFitHunkUser *>(buffer);
    const unsigned int headerBytes = static_cast<unsigned int>(sizeof(FirstFitHunkUser));
    const unsigned int nodeBytes = static_cast<unsigned int>(sizeof(_firstfit_heapnode));

    if ( (flags & 2) == 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_firstfit.cpp",
                    49,
                    0,
                    "%s",
                    "(flags & HF_FROMBUFFER)!=0") )
    {
        __debugbreak();
    }
    if ( size <= headerBytes + nodeBytes
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_firstfit.cpp",
                    50,
                    0,
                    "%s",
                    "size>sizeof(FIRSTFIT_HUNKUSER)+sizeof(FIRSTFIT_HEAPNODE)") )
    {
        __debugbreak();
    }

    user->hunkUser.name = name;
    user->hunkUser.scheme = scheme;
    user->hunkUser.flags = flags;
    user->hunkUser.type = type;
    user->reserved = -1;
    user->size = size;
    user->free_blocks = reinterpret_cast<_firstfit_heapnode *>(
        reinterpret_cast<unsigned char *>(user) + headerBytes);
    user->free_blocks->next = nullptr;
    user->free_blocks->size = static_cast<int>(size - headerBytes);
    user->used = static_cast<int>(headerBytes);
    return &user->hunkUser;
}

void __cdecl Hunk_FirstFitReset(HunkUser *_user)
{
    FirstFitHunkUser *user = reinterpret_cast<FirstFitHunkUser *>(_user);
    const unsigned int headerBytes = static_cast<unsigned int>(sizeof(FirstFitHunkUser));
    user->free_blocks = reinterpret_cast<_firstfit_heapnode *>(
        reinterpret_cast<unsigned char *>(user) + headerBytes);
    user->free_blocks->next = nullptr;
    user->free_blocks->size = static_cast<int>(user->size - headerBytes);
    user->used = static_cast<int>(headerBytes);
}

void __cdecl Hunk_FirstFitDestroy(HunkUser *_user)
{
    FirstFitHunkUser *user = reinterpret_cast<FirstFitHunkUser *>(_user);
    user->hunkUser.scheme = HU_SCHEME_DEFAULT;
    user->hunkUser.flags = 0;
    user->hunkUser.name = nullptr;
    user->hunkUser.type = 0;
    user->size = 0;
    user->free_blocks = nullptr;
    user->reserved = 0;
    user->used = 0;
}

static uintptr_t FirstFit_AlignUser(uintptr_t linkAddr, int alignment)
{
    // Heapnode header, then optional pad, then back-pointer, then aligned user payload.
    const uintptr_t minPayload = linkAddr + sizeof(_firstfit_heapnode) + sizeof(void *);
#if BO1_NATIVE_64BIT
    if (alignment < static_cast<int>(alignof(_firstfit_heapnode))) alignment = alignof(_firstfit_heapnode);
#endif
    const uintptr_t mask = static_cast<uintptr_t>(alignment) - 1;
    return (minPayload + mask) & ~mask;
}

void *__cdecl Hunk_FirstFitAlloc(HunkUser *_user, int size, int alignment)
{
    FirstFitHunkUser *user = reinterpret_cast<FirstFitHunkUser *>(_user);
    _firstfit_heapnode *free_link;
    _firstfit_heapnode *last;
    int adj_size;

#if BO1_NATIVE_64BIT
    iassert(size >= 0 && size <= INT32_MAX - 8192);
    iassert(alignment > 0 && alignment <= 4096 && !(alignment & (alignment - 1)));
#endif
    if ( alignment <= 0
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_firstfit.cpp", 90, 0, "%s", "alignment>0") )
    {
        __debugbreak();
    }
    FirstFit_Lock();
    free_link = user->free_blocks;
    last = nullptr;
    while ( 1 )
    {
        if ( !free_link )
            goto LABEL_16;
        {
            const uintptr_t linkAddr = reinterpret_cast<uintptr_t>(free_link);
            const uintptr_t aligned = FirstFit_AlignUser(linkAddr, alignment);
#if BO1_NATIVE_64BIT
            // The next heap header and payload back-pointer need native alignment,
            // even when the requested payload size/alignment is only one byte.
            const uintptr_t end = (aligned + static_cast<uintptr_t>(size) + alignof(_firstfit_heapnode) - 1)
                & ~(static_cast<uintptr_t>(alignof(_firstfit_heapnode)) - 1);
            adj_size = static_cast<int>(end - linkAddr);
#else
            adj_size = static_cast<int>(aligned - linkAddr) + size;
#endif
        }
        if ( free_link->size >= adj_size )
            break;
        last = free_link;
        free_link = free_link->next;
    }
    if ( !free_link->next && (free_link->size - adj_size) <= 1024 )
    {
LABEL_16:
        FirstFit_Unlock();
        return nullptr;
    }
    if ( (free_link->size - adj_size) > 1024 )
    {
        const uintptr_t linkAddr = reinterpret_cast<uintptr_t>(free_link);
        const uintptr_t aligned = FirstFit_AlignUser(linkAddr, alignment);
#if BO1_NATIVE_64BIT
        auto *split = reinterpret_cast<_firstfit_heapnode *>(linkAddr + static_cast<uintptr_t>(adj_size));
#else
        auto *split = reinterpret_cast<_firstfit_heapnode *>(aligned + static_cast<uintptr_t>(size));
#endif
        split->next = free_link->next;
        split->size = free_link->size - adj_size;
        free_link->next = split;
        free_link->size = adj_size;
    }
    if ( last )
        last->next = free_link->next;
    else
        user->free_blocks = free_link->next;

    {
        const uintptr_t linkAddr = reinterpret_cast<uintptr_t>(free_link);
        const uintptr_t aligned = FirstFit_AlignUser(linkAddr, alignment);
        free_link->next = reinterpret_cast<_firstfit_heapnode *>(kFirstFitBusyTag);
        *reinterpret_cast<_firstfit_heapnode **>(aligned - sizeof(void *)) = free_link;
        user->used += free_link->size;
        FirstFit_Unlock();
        return reinterpret_cast<void *>(aligned);
    }
}

void __cdecl Hunk_FirstFitFree(HunkUser *_user, void *ptr)
{
    FirstFitHunkUser *user = reinterpret_cast<FirstFitHunkUser *>(_user);
    _firstfit_heapnode *free_link;
    _firstfit_heapnode *scan;
    _firstfit_heapnode *last;

    if ( !ptr )
        return;

    FirstFit_Lock();
    last = nullptr;
    scan = user->free_blocks;
#if BO1_NATIVE_64BIT
    const uintptr_t first = reinterpret_cast<uintptr_t>(user) + sizeof(FirstFitHunkUser);
    const uintptr_t end = reinterpret_cast<uintptr_t>(user) + user->size;
    const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    iassert(address >= first + sizeof(_firstfit_heapnode) + sizeof(void *) && address < end);
    iassert(address % alignof(void *) == 0);
#endif
    free_link = *reinterpret_cast<_firstfit_heapnode **>(static_cast<unsigned char *>(ptr) - sizeof(void *));
#if BO1_NATIVE_64BIT
    const uintptr_t linkAddress = reinterpret_cast<uintptr_t>(free_link);
    iassert(linkAddress >= first && linkAddress <= end - sizeof(_firstfit_heapnode));
    iassert(linkAddress % alignof(_firstfit_heapnode) == 0);
#endif
    if ( free_link->next == reinterpret_cast<_firstfit_heapnode *>(kFirstFitBusyTag) )
    {
        user->used -= free_link->size;
        if ( !user->free_blocks
            && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_firstfit.cpp",
                        178,
                        0,
                        "%s",
                        "hunk->free_blocks!=NULL") )
        {
            __debugbreak();
        }
        while ( scan )
        {
            if ( scan > free_link )
            {
                if ( last )
                {
                    free_link->next = scan;
                    last->next = free_link;
                }
                else
                {
                    free_link->next = user->free_blocks;
                    user->free_blocks = free_link;
                }
                if ( last && reinterpret_cast<_firstfit_heapnode *>(
                                 reinterpret_cast<char *>(last) + last->size) == free_link )
                {
                    last->size += free_link->size;
                    last->next = free_link->next;
                    free_link = last;
                }
                if ( free_link->next
                    && reinterpret_cast<_firstfit_heapnode *>(
                           reinterpret_cast<char *>(free_link) + free_link->size) == free_link->next )
                {
                    free_link->size += free_link->next->size;
                    free_link->next = free_link->next->next;
                }
                goto LABEL_24;
            }
            last = scan;
            scan = scan->next;
        }
        if ( !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_firstfit.cpp",
                        220,
                        0,
                        "%s\n\t%s",
                        "0",
                        "invalid pointer passed into first fit free\n") )
            __debugbreak();
    }
    else if ( !Assert_MyHandler(
                             "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_firstfit.cpp",
                             173,
                             0,
                             "%s\n\t%s",
                             "0",
                             "buffer overrun or underrun or illegal ptr detected") )
    {
        __debugbreak();
    }
LABEL_24:
    FirstFit_Unlock();
}
