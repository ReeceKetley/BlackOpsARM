#include "mem_largelocal.h"
#include <qcommon/threads.h>
#include "assertive.h"
#if defined(__ANDROID__)
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <mutex>
static std::mutex largeLocalMutex;
[[noreturn]] static void LargeLocalNativeFailure(const char *reason) {
    std::fprintf(stderr, "Native LargeLocal failure: %s\n", reason);
    std::abort();
}
#endif

int g_largeLocalPos;
#if defined(__ANDROID__)
alignas(std::max_align_t) unsigned __int8 g_largeLocalBuf[524288];
#else
unsigned __int8 g_largeLocalBuf[524288];
#endif
int g_largeLocalRightPos = 524288;


int __cdecl LargeLocalBegin(int size)
{
#if defined(__ANDROID__)
    std::lock_guard<std::mutex> lock(largeLocalMutex);
#endif
    int startPos; // [esp+0h] [ebp-4h]
    unsigned int sizea; // [esp+Ch] [ebp+8h]

    if ( !Sys_IsMainThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    35,
                    0,
                    "%s",
                    "Sys_IsMainThread()") )
    {
        __debugbreak();
    }
    sizea = LargeLocalRoundSize(size);
    startPos = g_largeLocalPos;
#if defined(__ANDROID__)
    if (sizea > static_cast<unsigned>(sizeof(g_largeLocalBuf) - g_largeLocalPos)
        || sizea > static_cast<unsigned>(g_largeLocalRightPos - g_largeLocalPos))
        LargeLocalNativeFailure("left allocation exceeds arena or overlaps right owner");
#endif
    g_largeLocalPos += sizea;
    if ( g_largeLocalPos > 0x80000
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    43,
                    0,
                    "%s",
                    "g_largeLocalPos <= static_cast< int >( sizeof( g_largeLocalBuf ) )") )
    {
        __debugbreak();
    }
    if ( g_largeLocalPos > g_largeLocalRightPos
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    45,
                    0,
                    "%s",
                    "g_largeLocalPos <= g_largeLocalRightPos") )
    {
        __debugbreak();
    }
    return startPos;
}

unsigned int __cdecl LargeLocalRoundSize(int size)
{
#if defined(__ANDROID__)
    constexpr unsigned alignment = alignof(std::max_align_t);
    static_assert((alignment & (alignment - 1)) == 0);
    if (size < 0 || size > static_cast<int>(sizeof(g_largeLocalBuf))) LargeLocalNativeFailure("invalid size");
    return (static_cast<unsigned>(size) + alignment - 1) & ~(alignment - 1);
#else
    return (size + 3) & 0xFFFFFFFC;
#endif
}

void __cdecl LargeLocalEnd(int startPos)
{
#if defined(__ANDROID__)
    std::lock_guard<std::mutex> lock(largeLocalMutex);
    if (startPos < 0 || startPos > g_largeLocalPos) LargeLocalNativeFailure("invalid left rollback");
#endif
    if ( !Sys_IsMainThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    59,
                    0,
                    "%s",
                    "Sys_IsMainThread()") )
    {
        __debugbreak();
    }
    g_largeLocalPos = startPos;
    if ( startPos > g_largeLocalRightPos
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    64,
                    0,
                    "%s",
                    "g_largeLocalPos <= g_largeLocalRightPos") )
    {
        __debugbreak();
    }
}

unsigned __int8 *__cdecl LargeLocalGetBuf(int startPos, int size)
{
#if defined(__ANDROID__)
    std::lock_guard<std::mutex> lock(largeLocalMutex);
    const unsigned rounded = LargeLocalRoundSize(size);
    if (startPos < 0 || startPos > static_cast<int>(sizeof(g_largeLocalBuf))) LargeLocalNativeFailure("invalid buffer offset");
    if (Sys_IsMainThread()) {
        if (startPos > g_largeLocalPos || rounded > static_cast<unsigned>(g_largeLocalPos - startPos))
            LargeLocalNativeFailure("left buffer outside allocated range");
    } else if (startPos < static_cast<int>(rounded) || startPos - static_cast<int>(rounded) < g_largeLocalRightPos)
        LargeLocalNativeFailure("right buffer outside allocated range");
#endif
    int startIndex; // [esp+0h] [ebp-4h]

    if ( !Sys_IsMainThread()
        && !Sys_IsServerThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    121,
                    0,
                    "%s",
                    "Sys_IsMainThread() || Sys_IsServerThread()") )
    {
        __debugbreak();
    }
    if ( Sys_IsMainThread() )
        return &g_largeLocalBuf[startPos];
    startIndex = startPos - LargeLocalRoundSize(size);
    if ( startIndex < 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    134,
                    0,
                    "%s",
                    "startIndex >= 0") )
    {
        __debugbreak();
    }
    return &g_largeLocalBuf[startIndex];
}

LargeLocal::LargeLocal(int sizeParam)
{
    if ( !Sys_IsMainThread()
        && !Sys_IsServerThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    149,
                    0,
                    "%s",
                    "Sys_IsMainThread() || Sys_IsServerThread()") )
    {
        __debugbreak();
    }
    if ( Sys_IsMainThread() )
        this->startPos = LargeLocalBegin(sizeParam);
    else
        this->startPos = LargeLocalBeginRight(sizeParam);
    this->size = sizeParam;
}

int __cdecl LargeLocalBeginRight(int size)
{
#if defined(__ANDROID__)
    std::lock_guard<std::mutex> lock(largeLocalMutex);
#endif
    int startPos; // [esp+0h] [ebp-4h]
    unsigned int sizea; // [esp+Ch] [ebp+8h]

    if ( !Sys_IsServerThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    80,
                    0,
                    "%s",
                    "Sys_IsServerThread()") )
    {
        __debugbreak();
    }
    sizea = LargeLocalRoundSize(size);
#if defined(__ANDROID__)
    if (sizea >= static_cast<unsigned>(g_largeLocalRightPos - g_largeLocalPos))
        LargeLocalNativeFailure("right allocation overlaps left owner");
#endif
    if ( (int)(g_largeLocalRightPos - sizea) <= g_largeLocalPos
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    84,
                    0,
                    "%s",
                    "g_largeLocalRightPos - size > g_largeLocalPos") )
    {
        __debugbreak();
    }
    startPos = g_largeLocalRightPos;
    g_largeLocalRightPos -= sizea;
    return startPos;
}

LargeLocal::~LargeLocal()
{
    if ( !Sys_IsMainThread()
        && !Sys_IsServerThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    175,
                    0,
                    "%s",
                    "Sys_IsMainThread() || Sys_IsServerThread()") )
    {
        __debugbreak();
    }
    if ( Sys_IsMainThread() )
        LargeLocalEnd(this->startPos);
    else
        LargeLocalEndRight(this->startPos);
}

void __cdecl LargeLocalEndRight(int startPos)
{
#if defined(__ANDROID__)
    std::lock_guard<std::mutex> lock(largeLocalMutex);
    if (startPos < g_largeLocalRightPos || startPos > static_cast<int>(sizeof(g_largeLocalBuf)))
        LargeLocalNativeFailure("invalid right rollback");
#endif
    if ( !Sys_IsServerThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    100,
                    0,
                    "%s",
                    "Sys_IsServerThread()") )
    {
        __debugbreak();
    }
    g_largeLocalRightPos = startPos;
    if ( startPos <= g_largeLocalPos
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    104,
                    0,
                    "%s",
                    "g_largeLocalRightPos > g_largeLocalPos") )
    {
        __debugbreak();
    }
}

unsigned __int8 *LargeLocal::GetBuf()
{
    if ( !Sys_IsMainThread()
        && !Sys_IsServerThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    199,
                    0,
                    "%s",
                    "Sys_IsMainThread() || Sys_IsServerThread()") )
    {
        __debugbreak();
    }
    return LargeLocalGetBuf(this->startPos, this->size);
}

void __cdecl LargeLocalReset()
{
#if defined(__ANDROID__)
    std::lock_guard<std::mutex> lock(largeLocalMutex);
#endif
    if ( !Sys_IsMainThread()
        && !Sys_IsServerThread()
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\universal\\mem_largelocal.cpp",
                    216,
                    0,
                    "%s",
                    "Sys_IsMainThread() || Sys_IsServerThread()") )
    {
        __debugbreak();
    }
    g_largeLocalPos = 0;
    g_largeLocalRightPos = 0x80000;
}

