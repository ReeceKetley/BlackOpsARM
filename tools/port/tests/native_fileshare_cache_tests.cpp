#include <live/live_fileshare_cache.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
extern fshCacheFileDetails_t *s_fshCache;
extern fshCacheRatingDetails_t *s_fshRatingCache;
static std::vector<unsigned char *> allocations;
static std::vector<unsigned int> sizes;
unsigned char *Hunk_Alloc(unsigned int size, const char *, int) {
    auto *memory = static_cast<unsigned char *>(std::malloc(size+16));
    if (!memory) std::abort();
    std::memset(memory, 0xA5, size+16);
    allocations.push_back(memory); sizes.push_back(size);
    return memory;
}
bool Assert_MyHandler(const char *, int, int, const char *, ...) { std::abort(); }
static void Check(bool ok) { if (!ok) std::abort(); }
int main() {
    Check(Live_FileShare_CacheInit());
    Check(sizes[0] == sizeof(fshCacheFileDetails_t)*300);
    Check(sizes[1] == sizeof(fshCacheRatingDetails_t)*300);
    const auto location = static_cast<fileShareLocation>(0);
    for (unsigned long long id : {1ULL, 0x80000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFFFFFFFFFFFFFFULL}) {
        Check(Live_FileShare_Hash(id, location) >= 0 && Live_FileShare_Hash(id, location) < 300);
        bdFileMetaData descriptor{};
        descriptor.m_fileID = id;
        descriptor.m_ownerID = id ^ 0x1122334455667788ULL;
        descriptor.m_fileSize = 987;
        Live_FileShareCache_AddDescriptor(id, location, &descriptor);
        bdFileMetaData copy{};
        Check(Live_FileShareCache_GetDescriptor(id, location, &copy));
        Check(copy.m_fileID == id && copy.m_ownerID == descriptor.m_ownerID && copy.m_fileSize == 987);
        bdVoteRankStatsInfo rating{};
        rating.m_totalVotes = 123; rating.m_fileOwnerID = id;
        Live_FileShareCache_AddRating(id, location, &rating);
        bdVoteRankStatsInfo result{};
        Check(Live_FileShareCache_GetRating(id, location, &result));
        Check(result.m_totalVotes == 123 && result.m_fileOwnerID == id);
    }
    for (unsigned int i=0;i<allocations.size();++i)
        for(unsigned int j=0;j<16;++j) Check(allocations[i][sizes[i]+j] == 0xA5);
    Live_FileShare_CacheShutdown();
    Check(!s_fshCache && !s_fshRatingCache);
    Live_FileShare_CacheShutdown();
    for(auto *memory:allocations) std::free(memory);
    std::puts("PASS actual file-share cache native allocations/construction, high-bit hash bounds, descriptor/rating copies, canaries and shutdown");
}
