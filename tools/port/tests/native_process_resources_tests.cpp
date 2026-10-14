#include <platform/android/process_resources.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

int main()
{
    using bo1::android::ProcessResources;
    using bo1::android::ReadProcessResources;
    ProcessResources before{}, opened{}, after{}, touched{};
    if (ReadProcessResources(nullptr) || errno != EINVAL) return 1;
    if (!ReadProcessResources(&before)) { std::perror("resources"); return 2; }
    int descriptors[2];
    if (pipe(descriptors)) return 3;
    if (!ReadProcessResources(&opened)) return 4;
    if (opened.openFileDescriptors != before.openFileDescriptors + 2) return 5;
    close(descriptors[0]);
    close(descriptors[1]);
    if (!ReadProcessResources(&after) || after.openFileDescriptors != before.openFileDescriptors) return 6;
    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) return 7;
    constexpr std::size_t bytes = 16 * 1024 * 1024;
    auto *allocation = static_cast<volatile unsigned char *>(std::malloc(bytes));
    if (!allocation) return 8;
    for (std::size_t i = 0; i < bytes; i += static_cast<std::size_t>(pageSize)) allocation[i] = 1;
    if (!ReadProcessResources(&touched)) return 9;
    std::printf("PASS native resources: fd %u -> %u -> %u; RSS %llu -> %llu; anon %llu -> %llu; peak %llu bytes\n",
        before.openFileDescriptors, opened.openFileDescriptors, after.openFileDescriptors,
        static_cast<unsigned long long>(before.residentBytes), static_cast<unsigned long long>(touched.residentBytes),
        static_cast<unsigned long long>(before.anonymousResidentBytes), static_cast<unsigned long long>(touched.anonymousResidentBytes),
        static_cast<unsigned long long>(touched.peakResidentBytes));
    std::free(const_cast<unsigned char *>(allocation));
    bo1::android::SystemCpuTicks cpu{}, next{};
    if (bo1::android::ReadSystemCpuTicks(&cpu)) {
        if (!bo1::android::ReadSystemCpuTicks(&next) || !cpu.ticksPerSecond
            || next.ticksPerSecond != cpu.ticksPerSecond || cpu.idle > cpu.total
            || next.idle > next.total || next.total < cpu.total || next.idle < cpu.idle)
            return 10;
        std::printf("PASS system CPU: total %llu -> %llu, idle %llu -> %llu, frequency %u Hz\n",
            static_cast<unsigned long long>(cpu.total), static_cast<unsigned long long>(next.total),
            static_cast<unsigned long long>(cpu.idle), static_cast<unsigned long long>(next.idle), cpu.ticksPerSecond);
    } else {
        std::printf("System CPU unavailable: errno %d (explicit missing sample)\n", errno);
    }
    return 0;
}
