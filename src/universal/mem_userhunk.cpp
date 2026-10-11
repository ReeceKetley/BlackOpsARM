#include "mem_userhunk.h"
#if defined(__ANDROID__)
#include "memory_services.h"
#include "com_errors.h"
#include "mem_firstfit.h"
#include "mem_fixed.h"
#include <qcommon/mem_track.h>
#include <platform/android/virtual_memory.h>
#include <platform/android/hunk_services.h>
#include <cstring>
#include <cstddef>
// Debug/telemetry services remain external; the default allocator does not call them.
void __cdecl Com_Error(errorParm_t code, const char *format, ...);
void Bo1_Diag(const char *format, ...);
void Bo1_DiagStack(const char *context);
#else
#include <database/db_stream.h>
#include "assertive.h"
#include <qcommon/threads.h>
#include "mem_firstfit.h"
#include "com_memory.h"
#include <qcommon/mem_track.h>
#include "q_shared.h"
#include <qcommon/common.h>
#include "mem_firstfit.h"
#include "mem_fixed.h"

#endif

static bool Hunk_IsMainThread()
{
#if defined(__ANDROID__)
    return bo1::android::IsHunkMainThread();
#else
    return Sys_IsMainThread();
#endif
}

HunkUser *g_DebugHunkUser;

struct ALLOCATION_SCHEME_FUNCTIONS // sizeof=0x14
{
    HunkUser *(__cdecl *Init)(void *, int, HU_ALLOCATION_SCHEME, unsigned int, void *, const char *, int);
    void (__cdecl *Reset)(HunkUser *);
    void (__cdecl *Destroy)(HunkUser *);
    void *(__cdecl *Alloc)(HunkUser *, int, int, const char *);
    void (__cdecl *Free)(HunkUser *, void *);
};

// Typed adapters preserve each allocator's API without incompatible function-pointer calls.
ALLOCATION_SCHEME_FUNCTIONS g_HunkUserAllocationSchemeMap[4] =
{
  {
    [](void *b, int s, HU_ALLOCATION_SCHEME h, unsigned f, void *d, const char *n, int t) { return Hunk_UserDefaultInit(static_cast<unsigned char *>(b), s, h, f, d, n, t); },
    [](HunkUser *u) { Hunk_UserDefaultReset(reinterpret_cast<HunkUserDefault *>(u)); },
    [](HunkUser *u) { Hunk_UserDefaultDestroy(reinterpret_cast<HunkUserDefault *>(u)); },
    [](HunkUser *u, int s, int a, const char *n) { return Hunk_UserDefaultAlloc(reinterpret_cast<HunkUserDefault *>(u), static_cast<unsigned int>(s), a, n); },
    Hunk_UserDefaultFree
  },
  {
    Hunk_UserDebugInit, Hunk_UserDebugReset, Hunk_UserDebugDestroy,
    [](HunkUser *u, int s, int a, const char *) { return Hunk_UserDebugAlloc(u, s, a); },
    Hunk_UserDebugFree
  },
  {
    [](void *b, int s, HU_ALLOCATION_SCHEME h, unsigned f, void *d, const char *n, int t) { return Hunk_FirstFitInit(b, static_cast<unsigned int>(s), h, f, d, n, t); },
    Hunk_FirstFitReset, Hunk_FirstFitDestroy,
    [](HunkUser *u, int s, int a, const char *) { return Hunk_FirstFitAlloc(u, s, a); },
    Hunk_FirstFitFree
  },
  { Hunk_FixedInit, Hunk_FixedReset, Hunk_FixedDestroy, Hunk_FixedAlloc, Hunk_FixedFree }
};

static psize_int Hunk_PageSize()
{
#if defined(__ANDROID__)
    return static_cast<psize_int>(bo1::android::VirtualPageSize());
#else
    return 4096;
#endif
}


HunkUser *__cdecl Hunk_UserDebugInit(
                void *buffer,
                int size,
                HU_ALLOCATION_SCHEME scheme,
                unsigned int flags,
                void *scheme_specific_data,
                const char *name,
                int type)
{
    (void)scheme_specific_data;
    HunkUserDebug *user = reinterpret_cast<HunkUserDebug *>(buffer);
    const int headerBytes = static_cast<int>(sizeof(HunkUserDebug));

    if ( scheme != HU_SCHEME_DEBUG
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    109,
                    0,
                    "%s",
                    "scheme==HU_SCHEME_DEBUG") )
    {
        __debugbreak();
    }
    if ( !Hunk_IsMainThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    110,
                    0,
                    "%s",
                    "Sys_IsMainThread()") )
    {
        __debugbreak();
    }
    user->hunkUser.scheme = scheme;
    user->hunkUser.flags = flags;
    user->hunkUser.name = name;
    user->hunkUser.type = type;
    user->firstFit = Hunk_FirstFitInit(
        static_cast<unsigned char *>(buffer) + headerBytes,
        static_cast<unsigned int>(size - headerBytes),
        HU_SCHEME_FIRSTFIT,
        flags,
        nullptr,
        name,
        type);
    return &user->hunkUser;
}

void __cdecl Hunk_UserDebugReset(HunkUser *_user)
{
#if defined(__ANDROID__)
    iassert(Hunk_IsMainThread());
#endif
    Hunk_FirstFitReset(reinterpret_cast<HunkUserDebug *>(_user)->firstFit);
}

void __cdecl Hunk_UserDebugDestroy(HunkUser *_user)
{
    if ( _user->scheme != HU_SCHEME_DEBUG
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    136,
                    0,
                    "%s",
                    "user->hunkUser.scheme==HU_SCHEME_DEBUG") )
    {
        __debugbreak();
    }
    if ( !Hunk_IsMainThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    137,
                    0,
                    "%s",
                    "Sys_IsMainThread()") )
    {
        __debugbreak();
    }
    Z_VirtualFree(_user, _user->type);
}

void *__cdecl Hunk_UserDebugAlloc(HunkUser *_user, int size, int alignment)
{
    if ( !_user
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp", 155, 0, "%s", "user") )
    {
        __debugbreak();
    }
    if ( _user->scheme != HU_SCHEME_DEBUG
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    156,
                    0,
                    "%s",
                    "user->hunkUser.scheme==HU_SCHEME_DEBUG") )
    {
        __debugbreak();
    }
    if ( !Hunk_IsMainThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    157,
                    0,
                    "%s",
                    "Sys_IsMainThread()") )
    {
        __debugbreak();
    }
    if ( (alignment & (alignment - 1)) != 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    158,
                    0,
                    "%s",
                    "!(alignment & (alignment - 1))") )
    {
        __debugbreak();
    }
    if ( alignment > 4096
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    159,
                    0,
                    "%s",
                    "alignment <= HUNK_MAX_ALIGNMENT") )
    {
        __debugbreak();
    }
    HunkUser *firstFit = reinterpret_cast<HunkUserDebug *>(_user)->firstFit;
    void *result = Hunk_FirstFitAlloc(firstFit, size, alignment);
    if (!result)
    {
        const FirstFitHunkUser *heap = reinterpret_cast<const FirstFitHunkUser *>(firstFit);
        Com_Error(ERR_FATAL, "Hunk_UserDebugAlloc: out of memory (%d requested, %d used, %u capacity)",
            size, heap->used, heap->size);
    }
    return result;
}

void __cdecl Hunk_UserDebugFree(HunkUser *_user, void *ptr)
{
    if ( _user->scheme != HU_SCHEME_DEBUG
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    168,
                    0,
                    "%s",
                    "_user->scheme==HU_SCHEME_DEBUG") )
    {
        __debugbreak();
    }
    if ( !Hunk_IsMainThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    169,
                    0,
                    "%s",
                    "Sys_IsMainThread()") )
    {
        __debugbreak();
    }
    Hunk_FirstFitFree(reinterpret_cast<HunkUserDebug *>(_user)->firstFit, ptr);
}

HunkUser *__cdecl Hunk_UserDefaultInit(
                unsigned __int8 *buffer,
                int size,
                HU_ALLOCATION_SCHEME scheme,
                unsigned int flags,
                void *scheme_specific_data,
                const char *name,
                int type)
{
    HunkUserDefault *user = (HunkUserDefault *)buffer;
    const psize_int headerBytes = offsetof(HunkUserDefault, buf);

    if ( scheme
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    179,
                    0,
                    "%s",
                    "scheme==HU_SCHEME_DEFAULT") )
    {
        __debugbreak();
    }
    memset(buffer, 0, (size_t)headerBytes);
    user->end = (psize_int)&buffer[size];
    user->pos = ((psize_int)(buffer + headerBytes) + 31) & ~(psize_int)31;
    if ( (user->pos & 0x1F) != 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    187,
                    0,
                    "%s\n\t(user->pos) = %i",
                    "(!(user->pos & 31))",
                    (int)user->pos) )
    {
        __debugbreak();
    }
    if ( (flags & 2) != 0 )
        user->locked = user->pos + (psize_int)size;
    else
        user->locked = (user->pos + Hunk_PageSize() - 1) & ~(Hunk_PageSize() - 1);
    user->maxSize = size;
    user->current = user;
    user->next = 0;
    user->hunkUser.scheme = scheme;
    user->hunkUser.flags = flags;
    user->hunkUser.name = name;
    user->hunkUser.type = type;
    if ( user->next
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp", 200, 0, "%s", "!user->next") )
    {
        __debugbreak();
    }
    return (HunkUser *)buffer;
}

void __cdecl Hunk_UserDefaultReset(HunkUserDefault *_user)
{
    char *pos; // [esp+0h] [ebp-8h]
    const psize_int headerBytes = offsetof(HunkUserDefault, buf);

    if (_user->hunkUser.scheme
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            210,
            0,
            "%s",
            "user->hunkUser.scheme==HU_SCHEME_DEFAULT"))
    {
        __debugbreak();
    }
    if (_user->next)
    {
        //Hunk_UserDefaultDestroy(&_user->next->hunkUser);
        Hunk_UserDefaultDestroy(_user->next);
        _user->current = _user;
        _user->next = 0;
    }
    if ((_user->hunkUser.flags & 2) != 0)
    {
        _user->pos = (psize_int)_user->buf;
        _user->locked = (psize_int)_user->maxSize + _user->pos;
    }
    else
    {
        // First page boundary after the hunk base (base is page-aligned from VirtualReserve).
        pos = (char *)(((psize_int)_user + Hunk_PageSize()) & ~(Hunk_PageSize() - 1));
        if (pos != (char *)((_user->pos + Hunk_PageSize() - 1) & ~(Hunk_PageSize() - 1)))
        {
            if ((psize_int)(_user->pos - (psize_int)pos) <= 0
                && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    229,
                    0,
                    "%s",
                    "user->pos - pos > 0"))
            {
                __debugbreak();
            }
            Z_VirtualDecommit(pos, (int)(_user->pos - (psize_int)pos), _user->hunkUser.type);
        }
        _user->pos = (psize_int)_user->buf;
        _user->locked = (_user->pos + Hunk_PageSize() - 1) & ~(Hunk_PageSize() - 1);
#if defined(__ANDROID__)
        const psize_int clearEnd = _user->maxSize < Hunk_PageSize() ? _user->maxSize : Hunk_PageSize();
        memset(_user->buf, 0, static_cast<size_t>(clearEnd - headerBytes));
#else
        memset(_user->buf, 0, (size_t)(4096 - headerBytes));
#endif
    }
}

void __cdecl Hunk_UserDefaultDestroy(HunkUserDefault *_user)
{
    HunkUserDefault *current; // [esp+4h] [ebp-8h]
    HunkUserDefault *newCurrent; // [esp+8h] [ebp-4h]

    if (_user->hunkUser.scheme
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            251,
            0,
            "%s",
            "user->hunkUser.scheme==HU_SCHEME_DEFAULT"))
    {
        __debugbreak();
    }
    if ((_user->hunkUser.flags & 8) != 0)
        track_userhunk_freerange(_user->buf, _user->maxSize);
    if ((_user->hunkUser.flags & 2) == 0)
    {
        for (current = _user->next; current; current = newCurrent)
        {
            newCurrent = current->next;
            Z_VirtualFree(current, _user->hunkUser.type);
        }
        Z_VirtualFree(_user, _user->hunkUser.type);
    }
}

void *__cdecl Hunk_UserDefaultAlloc(HunkUserDefault *user, unsigned int size, int alignment, const char *name)
{
    const char *v4; // eax
    psize_int range;
    psize_int pos;
    psize_int v8;
    HunkUserDefault *current;
    HunkUserDefault *newCurrent;
    psize_int alignmenta;
    const unsigned int headerBytes = (unsigned int)offsetof(HunkUserDefault, buf);

    if (!user
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp", 288, 0, "%s", "user"))
    {
        __debugbreak();
    }
    if (user->hunkUser.scheme
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            289,
            0,
            "%s",
            "user->hunkUser.scheme==HU_SCHEME_DEFAULT"))
    {
        __debugbreak();
    }
    if (size > (unsigned int)user->maxSize - headerBytes)
    {
        v4 = va("size: %d, maxSize: %d", size, user->maxSize - (int)headerBytes);
        if (!Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            290,
            0,
            "%s\n\t%s",
            "static_cast< uint >( size ) <= user->maxSize - offsetof( HunkUserDefault, buf )",
            v4))
            __debugbreak();
    }
    if ((alignment & (alignment - 1)) != 0
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            291,
            0,
            "%s",
            "!(alignment & (alignment - 1))"))
    {
        __debugbreak();
    }
    if (alignment > 4096
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            292,
            0,
            "%s",
            "alignment <= HUNK_MAX_ALIGNMENT"))
    {
        __debugbreak();
    }
    alignmenta = (psize_int)alignment - 1;
    for (current = user->current; ; current = newCurrent)
    {
        pos = current->pos;
        v8 = (pos % (alignmenta + 1)) ? (~alignmenta & (alignmenta + pos)) : current->pos;
        if ((psize_int)size + v8 <= current->end)
            break;
        if ((user->hunkUser.flags & 4) != 0 || (user->hunkUser.flags & 2) != 0)
        {
            Bo1_Diag("Hunk_UserAlloc OOM: name=%s type=%d flags=%u size=%u maxSize=%d pos=%llu end=%llu",
                     user->hunkUser.name ? user->hunkUser.name : "?", user->hunkUser.type, user->hunkUser.flags, size,
                     user->maxSize, (unsigned long long)current->pos, (unsigned long long)current->end);
            Bo1_DiagStack("HunkOOM");
            Com_Error(ERR_FATAL, "Hunk_UserAlloc: out of memory");
        }
        newCurrent = (HunkUserDefault *)Hunk_UserCreate(
            user->maxSize,
            user->hunkUser.scheme,
            user->hunkUser.flags,
            0,
            user->hunkUser.name,
            user->hunkUser.type);
        user->current = newCurrent;
        current->next = newCurrent;
    }
    current->pos = (psize_int)size + v8;
    if (current->pos > current->locked)
    {
#if defined(__ANDROID__)
        range = ((current->pos + Hunk_PageSize() - 1) & ~(Hunk_PageSize() - 1)) - current->locked;
#else
        range = ((current->pos + 4096) & ~(psize_int)4095) - current->locked;
#endif
        if (range <= 0
            && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp", 323, 0, "%s", "range > 0"))
        {
            __debugbreak();
        }
        if (range % Hunk_PageSize()
            && !Assert_MyHandler(
                "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                324,
                0,
                "%s",
                "range % PAGE_SIZE == 0"))
        {
            __debugbreak();
        }
        Z_VirtualCommit((char *)current->locked, (int)range, user->hunkUser.type);
        current->locked += range;
    }
    if ((user->hunkUser.flags & 8) != 0)
    {
        if (name)
            track_userhunk_alloc(size, (int)v8, name, user->hunkUser.type);
        else
            track_userhunk_alloc(size, (int)v8, user->hunkUser.name, user->hunkUser.type);
    }
    return (void *)v8;
}

void __cdecl Hunk_UserDefaultFree(HunkUser *user, void *ptr)
{
    if ( user->scheme
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    341,
                    0,
                    "%s",
                    "user->scheme==HU_SCHEME_DEFAULT") )
    {
        __debugbreak();
    }
    if ( (user->flags & 8) != 0 )
        track_userhunk_free(ptr);
}

void __cdecl Hunk_UserStartup()
{
    iassert(!g_DebugHunkUser);
    // Both script VMs keep developer lookup metadata. Shangri-La's client
    // scripts exhaust the original Win32 23 MB arena; use 48 MB on both hosts.
    g_DebugHunkUser = Hunk_UserCreate(0x3000000, HU_SCHEME_DEBUG, 0, 0, "Hunk_InitDebugMemory", 0);
}

void __cdecl Hunk_UserShutdown()
{
    if ( !g_DebugHunkUser
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    366,
                    0,
                    "%s",
                    "g_DebugHunkUser") )
    {
        __debugbreak();
    }
    Hunk_UserDestroy(g_DebugHunkUser);
    g_DebugHunkUser = 0;
}

HunkUser *__cdecl Hunk_UserCreateFromBuffer(
                void *buffer,
                int size,
                HU_ALLOCATION_SCHEME scheme,
                unsigned int flags,
                void *scheme_specific_data,
                const char *name,
                int type)
{
#if defined(__ANDROID__)
    if (!(buffer && size > static_cast<int>(offsetof(HunkUserDefault, buf) + 31) && !(flags & 8) && (scheme == HU_SCHEME_DEFAULT || scheme == HU_SCHEME_FIRSTFIT) && (reinterpret_cast<uintptr_t>(buffer) % alignof(HunkUserDefault)) == 0)) Com_Error(ERR_FATAL, "Android HunkUser: invalid or unsupported configuration");
#endif
    if ( scheme == HU_SCHEME_DEBUG
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
                    374,
                    0,
                    "%s",
                    "scheme!=HU_SCHEME_DEBUG") )
    {
        __debugbreak();
    }
    return g_HunkUserAllocationSchemeMap[scheme].Init(buffer, size, scheme, flags | 2, scheme_specific_data, name, type);
}

HunkUser *__cdecl Hunk_UserCreate(
    int size,
    HU_ALLOCATION_SCHEME scheme,
    unsigned int flags,
    void *scheme_specific_data,
    const char *name,
    int type)
{
    char *buffer; // [esp+0h] [ebp-4h]

#if defined(__ANDROID__)
    if (!(size > static_cast<int>(offsetof(HunkUserDefault, buf) + 31) && !(flags & 8) && (scheme == HU_SCHEME_DEFAULT || scheme == HU_SCHEME_DEBUG))) Com_Error(ERR_FATAL, "Android HunkUser: invalid or unsupported configuration");
#endif
    if ((flags & 2) != 0
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            381,
            0,
            "%s",
            "(flags & HF_FROMBUFFER)==0"))
    {
        __debugbreak();
    }
    if ((unsigned int)scheme >= HU_SCHEME_FIRSTFIT
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            382,
            0,
            "%s",
            "scheme == HU_SCHEME_DEFAULT || scheme == HU_SCHEME_DEBUG"))
    {
        __debugbreak();
    }
    if (size % 4096
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_userhunk.cpp",
            383,
            0,
            "%s\n\t(size) = %i",
            "(!(size % (4 * 1024)))",
            size))
    {
        __debugbreak();
    }
    buffer = (char *)Z_VirtualReserve(size);
    if (scheme == HU_SCHEME_DEBUG)
    {
        Z_VirtualCommit(buffer, size, type);
        return g_HunkUserAllocationSchemeMap[1].Init(buffer, size, HU_ALLOCATION_SCHEME::HU_SCHEME_DEBUG, flags | 2, scheme_specific_data, name, type);
    }
    else
    {
        // Commit at least the HunkUserDefault header (Win32 was 44 / 0x2C).
        Z_VirtualCommit(buffer, (int)((offsetof(HunkUserDefault, buf) + 31) & ~31u), type);
        return g_HunkUserAllocationSchemeMap[scheme].Init(buffer, size, scheme, flags, scheme_specific_data, name, type);
    }
}

void *__cdecl Hunk_UserAlloc(HunkUser *user, int size, int alignment, const char *name)
{
#if defined(__ANDROID__)
    if (!user || (user->scheme != HU_SCHEME_DEFAULT && user->scheme != HU_SCHEME_DEBUG && user->scheme != HU_SCHEME_FIRSTFIT) || size < 0 || alignment <= 0 || alignment > 4096 || (alignment & (alignment - 1)))
        Com_Error(ERR_FATAL, "Android HunkUser: invalid allocation");
#endif
    return g_HunkUserAllocationSchemeMap[user->scheme].Alloc(user, size, alignment, name);
}

void __cdecl Hunk_UserFree(HunkUser *user, void *ptr)
{
    g_HunkUserAllocationSchemeMap[user->scheme].Free(user, ptr);
}

void __cdecl Hunk_UserReset(HunkUser *user)
{
    g_HunkUserAllocationSchemeMap[user->scheme].Reset(user);
}

void __cdecl Hunk_UserDestroy(HunkUser *user)
{
    g_HunkUserAllocationSchemeMap[user->scheme].Destroy(user);
}

void __cdecl Hunk_UserSetPos(HunkUser *_user, unsigned char *pos)
{
    iassert(_user->scheme == HU_SCHEME_DEFAULT);

    HunkUserDefault *user = (HunkUserDefault *)_user;
    iassert((user->hunkUser.flags & HF_FIXEDSIZE) != 0);
    iassert(pos >= user->buf);
    iassert((psize_int)pos <= user->pos);

    user->pos = (psize_int)pos;
}

char *__cdecl Hunk_CopyString(HunkUser *user, const char *in)
{
    char v3; // [esp+3h] [ebp-21h]
    char *v4; // [esp+8h] [ebp-1Ch]
    const char *v5; // [esp+Ch] [ebp-18h]
    char *out; // [esp+20h] [ebp-4h]

    out = (char *)Hunk_UserAlloc(user, strlen(in) + 1, 1, 0);
    v5 = in;
    v4 = out;
    do
    {
        v3 = *v5;
        *v4++ = *v5++;
    }
    while ( v3 );
    return out;
}

