// Real Windows virtual-memory boundary for standalone probes inside BlackOpsARM.
#include <Windows.h>
#include <universal/memory_services.h>
#include <map>
#include <mutex>
#include <cstdlib>
static std::mutex memoryMutex;
static std::map<void *, int> memoryBlocks;
std::size_t HunkTestReservationCount()
{
    std::lock_guard<std::mutex> lock(memoryMutex);
    return memoryBlocks.size();
}
void *Z_VirtualReserve(int size)
{
    void *p = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    if (!p) std::abort();
    std::lock_guard<std::mutex> lock(memoryMutex);
    memoryBlocks.emplace(p, size);
    return p;
}
void Z_VirtualCommit(char *p, int size, int)
{
    if (!VirtualAlloc(p, size, MEM_COMMIT, PAGE_READWRITE)) std::abort();
}
void Z_VirtualDecommit(char *p, int size, int)
{
    if (!VirtualFree(p, size, MEM_DECOMMIT)) std::abort();
}
void Z_VirtualFree(void *p, int)
{
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (!memoryBlocks.count(p) || !VirtualFree(p, 0, MEM_RELEASE)) std::abort();
    memoryBlocks.erase(p);
}
