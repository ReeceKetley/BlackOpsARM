#include "l_memory.h"
#include <universal/com_memory.h>
#if defined(__ANDROID__)
#include <cstddef>
#include <cstring>
#include <climits>
#include <cerrno>
static constexpr unsigned nativePrefix = alignof(std::max_align_t);
static_assert(nativePrefix >= 4);
#endif

unsigned int *__cdecl GetMemory(unsigned int size)
{
    unsigned int *ptr; // [esp+4h] [ebp-4h]

#if defined(__ANDROID__)
    if (size > static_cast<unsigned>(INT_MAX) - nativePrefix) { errno = ENOMEM; return nullptr; }
    auto *base = reinterpret_cast<unsigned char *>(Z_Malloc(static_cast<int>(size + nativePrefix), "GetMemory", 11));
    if (!base) return nullptr;
    const unsigned magic = 305419896;
    std::memcpy(base + nativePrefix - sizeof(magic), &magic, sizeof(magic));
    return reinterpret_cast<unsigned int *>(base + nativePrefix);
#else
    ptr = Z_Malloc(size + 4, "GetMemory", 11);
    if ( !ptr )
        return 0;
    *ptr = 305419896;
    return ptr + 1;
#endif
}

unsigned __int8 *__cdecl GetClearedMemory(unsigned int size)
{
    unsigned __int8 *ptr; // [esp+0h] [ebp-4h]

    ptr = (unsigned __int8 *)GetMemory(size);
#if defined(__ANDROID__)
    if (!ptr) return nullptr;
#endif
    memset(ptr, 0, size);
    return ptr;
}

void __cdecl FreeMemory(char *ptr)
{
    if ( *((unsigned int *)ptr - 1) == 305419896 )
#if defined(__ANDROID__)
        Z_Free(ptr - nativePrefix, 11);
#else
        Z_Free(ptr - 4, 11);
#endif
}

