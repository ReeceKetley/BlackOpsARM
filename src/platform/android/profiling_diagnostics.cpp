#include "profiling_diagnostics.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <unistd.h>

bool bo1::android::ProcessVirtualBytes(std::uint64_t *bytes) {
    if (!bytes) { errno = EINVAL; return false; }
    FILE *file = std::fopen("/proc/self/statm", "r");
    if (!file) return false;
    unsigned long long pages = 0;
    const int parsed = std::fscanf(file, "%llu", &pages);
    const int closed = std::fclose(file);
    if (parsed != 1) { errno = EIO; return false; }
    if (closed != 0) return false;
    const long pageSize = ::sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) { errno = EIO; return false; }
    if (pages > std::numeric_limits<std::uint64_t>::max() / static_cast<std::uint64_t>(pageSize)) {
        errno = EOVERFLOW; return false;
    }
    *bytes = pages * static_cast<std::uint64_t>(pageSize);
    return true;
}

bool bo1::android::ProfileSymbolName(const void *address, char *name, std::size_t capacity) {
    if (!address || !name || !capacity) { errno = EINVAL; return false; }
    Dl_info info{};
    if (!::dladdr(address, &info)) return false;
    int written;
    if (info.dli_sname && *info.dli_sname) {
        written = std::snprintf(name, capacity, "%s", info.dli_sname);
    } else if (info.dli_fname && info.dli_fbase) {
        const char *base = std::strrchr(info.dli_fname, '/');
        const auto offset = reinterpret_cast<std::uintptr_t>(address)
            - reinterpret_cast<std::uintptr_t>(info.dli_fbase);
        written = std::snprintf(name, capacity, "%s+0x%llx", base ? base + 1 : info.dli_fname,
                                static_cast<unsigned long long>(offset));
    } else return false;
    if (written < 0 || static_cast<std::size_t>(written) >= capacity) {
        errno = ERANGE; return false;
    }
    return true;
}
