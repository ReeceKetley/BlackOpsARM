#include "mem_fixed.h"
#include "assertive.h"
#include <cstdint>

static bool Fixed_Require(bool condition, const char *message)
{
    if (!condition && !Assert_MyHandler(__FILE__, __LINE__, 0, "%s", message))
        __debugbreak();
    return condition;
}

HunkUser *__cdecl Hunk_FixedInit(void *buffer, int size, HU_ALLOCATION_SCHEME scheme,
    unsigned int flags, void *scheme_specific_data, const char *name, int type)
{
    if (!Fixed_Require(buffer && scheme_specific_data && (flags & 2), "fixed hunk requires a buffer and block size")
        || !Fixed_Require(size > static_cast<int>(sizeof(HunkUserFixed) + sizeof(_fixed_heapnode)), "fixed hunk buffer is too small"))
        return nullptr;
    const int requested = *static_cast<const int *>(scheme_specific_data);
    if (!Fixed_Require(requested > 0 && (requested & (requested - 1)) == 0, "fixed block size must be a positive power of two"))
        return nullptr;
    HunkUserFixed *user = static_cast<HunkUserFixed *>(buffer);
    user->hunkUser = { scheme, flags, name, type };
    user->size = size;
    user->blockSize = requested < sizeof(_fixed_heapnode) ? sizeof(_fixed_heapnode) : requested;
    Hunk_FixedReset(&user->hunkUser);
    return &user->hunkUser;
}

void __cdecl Hunk_FixedReset(HunkUser *base)
{
    HunkUserFixed *user = reinterpret_cast<HunkUserFixed *>(base);
    const size_t stride = user->blockSize + sizeof(_fixed_heapnode);
    const size_t count = (user->size - sizeof(HunkUserFixed)) / stride;
    char *data = reinterpret_cast<char *>(user + 1);
    user->freeHead = count ? reinterpret_cast<_fixed_heapnode *>(data) : nullptr;
    for (size_t index = 0; index < count; ++index)
    {
        _fixed_heapnode *node = reinterpret_cast<_fixed_heapnode *>(data + index * stride);
        node->next = index + 1 < count ? reinterpret_cast<_fixed_heapnode *>(data + (index + 1) * stride) : nullptr;
    }
}

void __cdecl Hunk_FixedDestroy(HunkUser *base)
{
    HunkUserFixed *user = reinterpret_cast<HunkUserFixed *>(base);
    user->hunkUser = { HU_SCHEME_DEFAULT, 0, nullptr, 0 };
    user->size = 0;
    user->blockSize = 0;
    user->freeHead = nullptr;
}

void *__cdecl Hunk_FixedAlloc(HunkUser *base, int size, int alignment, const char *name)
{
    (void)alignment;
    (void)name;
    HunkUserFixed *user = reinterpret_cast<HunkUserFixed *>(base);
    if (!Fixed_Require(size >= 0 && static_cast<unsigned int>(size) <= user->blockSize, "allocation exceeds fixed block size"))
        return nullptr;
    _fixed_heapnode *node = user->freeHead;
    if (node)
        user->freeHead = node->next;
    return node;
}

void __cdecl Hunk_FixedFree(HunkUser *base, void *ptr)
{
    if (!ptr)
        return;
    HunkUserFixed *user = reinterpret_cast<HunkUserFixed *>(base);
    const uintptr_t first = reinterpret_cast<uintptr_t>(user + 1);
    const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    const size_t stride = user->blockSize + sizeof(_fixed_heapnode);
    const size_t count = (user->size - sizeof(HunkUserFixed)) / stride;
    if (!Fixed_Require(address >= first && address < first + count * stride && (address - first) % stride == 0,
        "invalid pointer passed into fixed free"))
        return;
    _fixed_heapnode *node = static_cast<_fixed_heapnode *>(ptr);
    node->next = user->freeHead;
    user->freeHead = node;
}
