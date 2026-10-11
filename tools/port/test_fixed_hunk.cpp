// Standalone regression for the fixed allocator, which the Pentagon route does not use.
#include <intrin.h>
#include "universal/mem_fixed.h"
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static int reportedAssertions;
bool Assert_MyHandler(const char *, int, int, const char *, ...)
{
    ++reportedAssertions;
    return true;
}
static void Check(bool ok, const char *message)
{
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main()
{
    const size_t bufferSize = 4096;
    char *buffer = static_cast<char *>(VirtualAlloc(nullptr, bufferSize + 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(buffer != nullptr, "allocate backing buffer");
#if defined(_WIN64)
    Check(reinterpret_cast<uintptr_t>(buffer) > 0x80000000ULL, "exercise pointers above the old 2 GB cap");
#endif
    std::memset(buffer + bufferSize, 0xA5, 4096);
    for (int blockSize : { 4, 32, 256 })
    {
        HunkUser *user = Hunk_FixedInit(buffer, bufferSize, HU_SCHEME_FIXED, 2, &blockSize, "test", 0);
        Check(user != nullptr, "initialize allocator");
        std::vector<void *> blocks;
        while (void *p = Hunk_FixedAlloc(user, blockSize, 1, "test"))
        {
            for (void *old : blocks) Check(old != p, "allocations are distinct");
            std::memset(p, 0x5A, blockSize);
            blocks.push_back(p);
        }
        Check(blocks.size() > 1, "free list reaches more than the first block");
        const size_t capacity = blocks.size();
        for (void *p : blocks)
            for (int i = 0; i < blockSize; ++i)
                Check(static_cast<unsigned char *>(p)[i] == 0x5A, "allocated blocks do not overlap");
        for (void *p : blocks) Hunk_FixedFree(user, p);
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it)
            Check(Hunk_FixedAlloc(user, blockSize, 1, "test") == *it, "free restores complete native pointers");
        Check(Hunk_FixedAlloc(user, blockSize, 1, "test") == nullptr, "exhausted allocator returns null");
        Hunk_FixedReset(user);
        size_t resetCount = 0;
        while (Hunk_FixedAlloc(user, blockSize, 1, "test")) ++resetCount;
        Check(resetCount == capacity, "reset restores capacity");
        const int before = reportedAssertions;
        Hunk_FixedFree(user, static_cast<char *>(blocks[0]) + 1);
        Check(reportedAssertions == before + 1, "interior pointer is rejected");
        Check(Hunk_FixedAlloc(user, blockSize, 1, "test") == nullptr, "invalid free leaves exhausted list intact");
        Hunk_FixedDestroy(user);
        for (size_t i = 0; i < 4096; ++i)
            Check(static_cast<unsigned char>(buffer[bufferSize + i]) == 0xA5, "allocator stays inside the buffer");
    }
    std::printf("PASS fixed allocator at %p (%zu-bit pointers)\n", buffer, sizeof(void *) * 8);
    VirtualFree(buffer, 0, MEM_RELEASE);
}
