#pragma once
#include <cstdint>

namespace bo1::android {
struct ProcessResources {
    std::uint64_t residentBytes;
    std::uint64_t peakResidentBytes;
    std::uint64_t anonymousResidentBytes;
    unsigned openFileDescriptors;
};
// Best-effort observations from this process's procfs files, not one atomic
// snapshot. Anonymous RSS is not Windows private committed memory. The FD count
// excludes the directory descriptor opened by this query. Failure leaves output
// unchanged and supplies errno.
bool ReadProcessResources(ProcessResources *output);
struct SystemCpuTicks {
    std::uint64_t idle;
    std::uint64_t total;
    unsigned ticksPerSecond;
};
// Kernel aggregate CPU accounting; procfs denial is a real unavailable sample.
bool ReadSystemCpuTicks(SystemCpuTicks *output);
}
