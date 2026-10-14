#include <platform/native_pmem_api.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <limits>
#include <map>
#include <mutex>

namespace {
std::mutex mappingMutex;
std::map<void *, std::size_t> mappings;
[[noreturn]] void unsupported() {
    std::fputs("Android PMem: unsupported virtual-memory request\n", stderr);
    std::abort();
}
}

// PMem's Windows null-address MEM_COMMIT call reserves and commits a new
// zero-filled read/write region. mmap provides that backing without touching
// the entire pool. Do not emulate unrelated Windows allocation modes.
void *VirtualAlloc(void *address, std::size_t size, unsigned int allocationType,
                   unsigned int protection) {
    if (address || allocationType != 0x1000u || protection != 4u) unsupported();
    const long page = sysconf(_SC_PAGESIZE);
    if (page <= 0 || !size || size > std::numeric_limits<std::size_t>::max() -
                                      static_cast<std::size_t>(page - 1)) {
        errno = EINVAL;
        return nullptr;
    }
    const auto bytes = ((size + page - 1) / page) * page;
    void *memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) return nullptr;
    try {
        std::lock_guard<std::mutex> guard(mappingMutex);
        mappings.emplace(memory, bytes);
    } catch (...) {
        munmap(memory, bytes);
        errno = ENOMEM;
        return nullptr;
    }
    return memory;
}

int VirtualFree(void *address, std::size_t size, unsigned int freeType) {
    if (!address || size || freeType != 0x8000u) unsupported();
    std::lock_guard<std::mutex> guard(mappingMutex);
    const auto found = mappings.find(address);
    if (found == mappings.end()) { errno = EINVAL; return 0; }
    if (munmap(address, found->second)) return 0;
    mappings.erase(found);
    return 1;
}
