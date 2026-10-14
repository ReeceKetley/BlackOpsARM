#include "live_fileshare_cache.h"
#if defined(__ANDROID__)
#include <new>
#include <cstdint>
#endif
#include <universal/com_memory.h>

fshCacheFileDetails_t *s_fshCache;
fshCacheRatingDetails_t *s_fshRatingCache;

int s_fshCacheFileCount;
int s_fshCacheRatingCount;

bool __cdecl Live_FileShare_CacheInit()
{
    if ( s_fshCache
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_fileshare_cache.cpp",
                    37,
                    0,
                    "%s",
                    "s_fshCache == NULL") )
    {
        __debugbreak();
    }
    if ( s_fshRatingCache
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_fileshare_cache.cpp",
                    38,
                    0,
                    "%s",
                    "s_fshRatingCache == NULL") )
    {
        __debugbreak();
    }
#if defined(__ANDROID__)
    auto *files = reinterpret_cast<fshCacheFileDetails_t *>(
        Hunk_Alloc(sizeof(fshCacheFileDetails_t) * 300, "fileShareCache", 36));
    auto *ratings = reinterpret_cast<fshCacheRatingDetails_t *>(
        Hunk_Alloc(sizeof(fshCacheRatingDetails_t) * 300, "fileShareCache", 36));
    if (!files || !ratings) return false;
    for (unsigned int i = 0; i < 300; ++i)
    {
        new (&files[i]) fshCacheFileDetails_t{};
        new (&ratings[i]) fshCacheRatingDetails_t{};
    }
    s_fshCache = files;
    s_fshRatingCache = ratings;
    s_fshCacheFileCount = s_fshCacheRatingCount = 0;
    return true;
#else
    s_fshCache = (fshCacheFileDetails_t *)Hunk_Alloc(0x9AB00u, "fileShareCache", 36);
    s_fshCacheFileCount = 0;
    s_fshRatingCache = (fshCacheRatingDetails_t *)Hunk_Alloc(0xFD20u, "fileShareCache", 36);
    s_fshCacheRatingCount = 0;
    return s_fshCache && s_fshRatingCache;
#endif
}

void __cdecl Live_FileShare_CacheShutdown()
{
#if defined(__ANDROID__)
    for (unsigned int i = 0; i < 300; ++i)
    {
        if (s_fshCache) s_fshCache[i].~fshCacheFileDetails_t();
        if (s_fshRatingCache) s_fshRatingCache[i].~fshCacheRatingDetails_t();
    }
#endif
    s_fshCache = 0;
    s_fshCacheFileCount = 0;
    s_fshRatingCache = 0;
    s_fshCacheRatingCount = 0;
}

int __cdecl Live_FileShare_Hash(unsigned __int64 fileID, fileShareLocation location)
{
#if defined(__ANDROID__)
    const auto key = static_cast<std::uint32_t>(location) ^ static_cast<std::uint32_t>(fileID) ^ static_cast<std::uint32_t>(fileID >> 32);
    return static_cast<int>(key % 300);
#else
    return (int)(location ^ fileID ^ HIDWORD(fileID)) % 300;
#endif
}

char __cdecl Live_FileShare_IsCached(unsigned __int64 fileID, fileShareLocation location, int *index)
{
    int idx; // [esp+8h] [ebp-4h]

    if ( !s_fshCache )
        return 0;
    idx = Live_FileShare_Hash(fileID, location);
    if ( LODWORD(s_fshCache[idx].descriptor.m_fileID) != (unsigned int)fileID
        || __PAIR64__(s_fshCache[idx].location, HIDWORD(s_fshCache[idx].descriptor.m_fileID)) != __PAIR64__(
                                                                                                                                                                                             location,
                                                                                                                                                                                             HIDWORD(fileID)) )
    {
        return 0;
    }
    *index = idx;
    return 1;
}

void __cdecl Live_FileShareCache_AddDescriptor(
                unsigned __int64 fileID,
                fileShareLocation location,
                bdFileMetaData *descriptor)
{
    int v3; // ecx

    if ( s_fshCache || Live_FileShare_CacheInit() )
    {
        v3 = Live_FileShare_Hash(fileID, location);
        s_fshCache[v3].location = location;
#if defined(__ANDROID__)
        s_fshCache[v3].descriptor = *descriptor;
#else
        memcpy(&s_fshCache[v3], descriptor, 0x838u);
#endif
    }
}

char __cdecl Live_FileShareCache_GetDescriptor(
                unsigned __int64 fileID,
                fileShareLocation location,
                bdFileMetaData *descriptor)
{
    int index; // [esp+8h] [ebp-4h] BYREF

    index = 0;
    if ( !Live_FileShare_IsCached(fileID, location, &index) )
        return 0;
#if defined(__ANDROID__)
    *descriptor = s_fshCache[index].descriptor;
#else
    memcpy(descriptor, &s_fshCache[index], sizeof(bdFileMetaData));
#endif
    return 1;
}

char __cdecl Live_FileShare_IsRatingCached(unsigned __int64 fileID, fileShareLocation location, int *index)
{
    int idx; // [esp+8h] [ebp-4h]

    if ( !s_fshRatingCache )
        return 0;
    idx = Live_FileShare_Hash(fileID, location);
    if ( LODWORD(s_fshRatingCache[idx].fileID) != (unsigned int)fileID
        || __PAIR64__(s_fshRatingCache[idx].location, HIDWORD(s_fshRatingCache[idx].fileID)) != __PAIR64__(
                                                                                                                                                                                            location,
                                                                                                                                                                                            HIDWORD(fileID)) )
    {
        return 0;
    }
    *index = idx;
    return 1;
}

void __cdecl Live_FileShareCache_AddRating(
                unsigned __int64 fileID,
                fileShareLocation location,
                bdVoteRankStatsInfo *rating)
{
    int index; // [esp+8h] [ebp-4h]

    if ( s_fshRatingCache || Live_FileShare_CacheInit() )
    {
        index = Live_FileShare_Hash(fileID, location);
        s_fshRatingCache[index].location = location;
        s_fshRatingCache[index].fileID = fileID;
#if defined(__ANDROID__)
        s_fshRatingCache[index].rating = *rating;
#else
        memcpy(&s_fshRatingCache[index].rating, rating, sizeof(s_fshRatingCache[index].rating));
#endif
    }
}

char __cdecl Live_FileShareCache_GetRating(
                unsigned __int64 fileID,
                fileShareLocation location,
                bdVoteRankStatsInfo *rating)
{
    int index; // [esp+8h] [ebp-4h] BYREF

    index = 0;
    if ( !Live_FileShare_IsRatingCached(fileID, location, &index) )
        return 0;
#if defined(__ANDROID__)
    *rating = s_fshRatingCache[index].rating;
#else
    memcpy(rating, &s_fshRatingCache[index].rating, sizeof(bdVoteRankStatsInfo));
#endif
    return 1;
}

