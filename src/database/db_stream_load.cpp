#include <platform/bo1_native_width.h>
#if defined(BO1_NATIVE_ZONE_LOAD)
#include "db_native_zone_load.h"
#endif
#include "db_stream_load.h"
#include "db_stream.h"
#include <universal/assertive.h>
#include <string.h>
#include <cstdint>
#include <cstdio>
#if BO1_NATIVE_64BIT
#include <universal/com_errors.h>
void __cdecl Com_Error(errorParm_t code, const char *format, ...);
#endif
#if defined(BO1_RAW_STRING_GATE)
void DB_LoadXFileData(unsigned __int8 *, int);
int DB_LoadXFileDataNullTerminated(unsigned __int8 *);
#if defined(BO1_FRONTEND_INTERN_GATE)
#include <clientscript/cscr_stringlist.h>
#endif
#else
#include "db_file_load.h"
#include <clientscript/cscr_stringlist.h>
#endif

#if BO1_NATIVE_64BIT
extern int g_dbTopAssetType;
#endif

StreamDelayInfo g_streamDelayArray[4096];

void __cdecl Load_Stream(bool atStreamStart, unsigned __int8 *ptr, int size)
{
#if BO1_NATIVE_64BIT
    // Expand follow-ups can see ptr==pos on Load_Stream(0, ...); Win32 never does (asserts).
    // Do not promote to a read — that overwrites already-expanded pointer fields with file bytes.
    if (!atStreamStart && ptr == DB_GetStreamPos())
    {
        static int s_logged;
        if (s_logged < 8)
        {
            ++s_logged;
        }
    }
    else
#endif
    if (atStreamStart != (ptr == DB_GetStreamPos())
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_stream_load.cpp",
            21,
            0,
            "%s",
            "atStreamStart == (ptr == DB_GetStreamPos())"))
    {
        __debugbreak();
    }
    if (atStreamStart && size)
    {
#if BO1_NATIVE_64BIT
        DB_StreamPrepareWrite();
        try
        {
#endif
        if (g_streamPosIndex - 1 < 3)
        {
            if (g_streamPosIndex == 1)
            {
                memset(ptr, 0, size);
            }
            else
            {
                if (g_streamDelayIndex >= 0x1000
                    && !Assert_MyHandler(
                        "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_stream_load.cpp",
                        36,
                        0,
                        "g_streamDelayIndex doesn't index ARRAY_COUNT( g_streamDelayArray )\n\t%i not in [0, %i)",
                        g_streamDelayIndex,
                        4096))
                {
                    __debugbreak();
                }
                g_streamDelayArray[g_streamDelayIndex].ptr = ptr;
                g_streamDelayArray[g_streamDelayIndex++].size = size;
            }
        }
        else
        {
            DB_LoadXFileData(ptr, size);
        }
#if BO1_NATIVE_64BIT
        }
        catch (...)
        {
            DB_StreamCancelWrite(true); // Direct reads may have changed owned bytes.
            throw;
        }
#endif
        DB_IncStreamPos(size);
    }
}

void __cdecl Load_DelayStream()
{
    unsigned int index; // [esp+4h] [ebp-8h]

    for (index = 0; index < g_streamDelayIndex; ++index)
        DB_LoadXFileData((unsigned __int8 *)g_streamDelayArray[index].ptr, g_streamDelayArray[index].size);
}

void __cdecl DB_ConvertOffsetToAlias(void *field)
{
    // FastFiles store 32-bit offsets in pointer slots. Read the low 32 bits, then
    // write a host pointer (32-bit on Win32, 64-bit on x64/ARM64).
    auto *data = reinterpret_cast<uintptr_t *>(field);
    const unsigned int offset = static_cast<unsigned int>(*data);

    if ( (!offset || offset >= 0xFFFFFFFE)
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_stream_load.cpp",
                    67,
                    0,
                    "%s\n\t(offset) = %i",
                    "(offset && (offset != 0xFFFFFFFF) && (offset != 0xFFFFFFFE))",
                    offset) )
    {
        __debugbreak();
    }
#if BO1_NATIVE_64BIT
    {
        const unsigned int blockIndex = (offset - 1) >> 29;
        const unsigned int within = (offset - 1) & 0x1FFFFFFFu;
        if (blockIndex == 0)
        {
            uintptr_t value{};
            const auto result = DB_StreamResolveAliasValue(blockIndex, within, &value);
            if (result != DB_StreamAliasResult::Active && result != DB_StreamAliasResult::Retained)
            {
                Com_Error(ERR_DROP, "Unsupported TEMP alias off=%08X state=%u", offset, static_cast<unsigned>(result));
                return;
            }
            *data = value; // Retained VALUE is not an address mapping to old storage.
            return;
        }
        unsigned __int8 *mapped = reinterpret_cast<unsigned __int8 *>(DB_StreamAliasSlotLookup(blockIndex, within));
        if (!mapped)
            mapped = DB_StreamExactLookup(blockIndex, within);
        if (!mapped)
        {
            mapped = DB_StreamMapLookup(blockIndex, within);
            static int s_logged;
            if (s_logged < 64)
            {
                ++s_logged;
            }
        }
        if (!mapped || blockIndex >= 7 || !g_streamBlocks[blockIndex].data || mapped < g_streamBlocks[blockIndex].data ||
            mapped + sizeof(uintptr_t) > g_streamBlocks[blockIndex].data + g_streamBlocks[blockIndex].size)
        {
            Bo1_Diag("Alias OUT OF BLOCK off=%08X block=%u within=%08X mapped=%p topType=%d segs=%s",
                     offset, blockIndex, within, (void *)mapped, g_dbTopAssetType, DB_StreamDescribe(blockIndex, within));
            *data = 0;
            return;
        }
        *data = *reinterpret_cast<uintptr_t *>(mapped);
        {
            static int s_aliasDiag;
            const unsigned int hi = static_cast<unsigned int>(*data >> 32);
            const unsigned int baseHi = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(g_streamBlocks[blockIndex].data) >> 32);
            if (hi != baseHi && s_aliasDiag++ < 40)
                Bo1_Diag("Alias suspicious off=%08X block=%u within=%08X mapped=%p value=%p segs=%s",
                         offset, blockIndex, within, (void *)mapped, (void *)*data, DB_StreamDescribe(blockIndex, within));
        }
    }
#else
    *data = *reinterpret_cast<uintptr_t *>(
        &g_streamBlocks[(offset - 1) >> 29].data[(offset - 1) & 0x1FFFFFFF]);
#endif
}

void __cdecl DB_ConvertOffsetToPointer(void *field)
{
#if defined(BO1_NATIVE_ZONE_LOAD)
    bo1::fastfile::NativeZoneValidatePointerOffset(field);
#endif
    auto *data = reinterpret_cast<uintptr_t *>(field);
    const unsigned int offset = static_cast<unsigned int>(*data);
#if BO1_NATIVE_64BIT
    {
        const unsigned int blockIndex = (offset - 1) >> 29;
        const unsigned int within = (offset - 1) & 0x1FFFFFFFu;
        unsigned __int8 *mapped = DB_StreamMapLookup(blockIndex, within);
        if (blockIndex == 0 && !mapped)
        {
            Com_Error(ERR_DROP, "Unsupported TEMP object offset=%08X", offset);
            return;
        }
        *data = reinterpret_cast<uintptr_t>(mapped);
    }
#else
    *data = reinterpret_cast<uintptr_t>(
        &g_streamBlocks[(offset - 1) >> 29].data[(offset - 1) & 0x1FFFFFFF]);
#endif
}

#if defined(__ANDROID__)
void DB_ConvertOffsetToPointerArray(void *field, unsigned count)
{
#if defined(BO1_NATIVE_ZONE_LOAD)
    bo1::fastfile::NativeZoneValidatePointerOffset(field);
#endif
    auto *data = reinterpret_cast<uintptr_t *>(field);
    const unsigned offset = static_cast<unsigned>(*data);
    unsigned __int8 *mapped = offset ? DB_StreamMapPointerArray((offset - 1) >> 29,
                                                               (offset - 1) & 0x1fffffffu, count) : nullptr;
    if (!mapped)
    {
        Com_Error(ERR_DROP, "Unsupported native pointer-array offset=%08X count=%u", offset, count);
        return;
    }
    *data = reinterpret_cast<uintptr_t>(mapped);
}
#endif

void __cdecl Load_XStringCustom(const char **str)
{
    int numBytesLoaded; // [esp+4h] [ebp-8h]

#if BO1_NATIVE_64BIT
    DB_StreamPrepareWrite();
    try
    {
#endif
    numBytesLoaded = DB_LoadXFileDataNullTerminated((unsigned __int8 *)*str);
#if BO1_NATIVE_64BIT
    }
    catch (...)
    {
        DB_StreamCancelWrite(true);
        throw;
    }
#endif
    DB_IncStreamPos(numBytesLoaded);
}

#if !defined(BO1_RAW_STRING_GATE) || defined(BO1_FRONTEND_INTERN_GATE)
void __cdecl Load_TempStringCustom(const char **str)
{
    const char *String; // [esp+0h] [ebp-4h]

    Load_XStringCustom(str);
    if ( *str )
        String = (const char *)(uintptr_t)SL_GetString((char *)*str, 4u, SCRIPTINSTANCE_SERVER);
    else
        String = 0;
    *str = String;
}


#endif
