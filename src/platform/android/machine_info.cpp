#include <qcommon/sys_info.h>
#include <sys/sysinfo.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>

SysInfo sys_info;
extern unsigned int s_cpuCount;
void Sys_SetAutoConfigureGHz(SysInfo *);
namespace {
[[noreturn]] void failed(const char *what) {
    std::fprintf(stderr, "Android machine information unavailable: %s\n", what);
    std::abort();
}
long readNumber(int cpu, const char *leaf) {
    char path[160];
    std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/%s", cpu, leaf);
    FILE *file = std::fopen(path, "r");
    if (!file) failed(path);
    long value;
    const bool ok = std::fscanf(file, "%ld", &value) == 1;
    std::fclose(file);
    if (!ok) failed(path);
    return value;
}
void property(const char *name, char *out, std::size_t capacity) {
    char value[PROP_VALUE_MAX]{};
    if (__system_property_get(name, value) <= 0) failed(name);
    std::snprintf(out, capacity, "%s", value);
}
}
void Sys_GetPhysicalCpuCount(SysInfo *info) {
    std::set<std::pair<long, long>> cores;
    for (int cpu = 0; cpu < info->logicalCpuCount; ++cpu) {
        cores.emplace(readNumber(cpu, "topology/physical_package_id"),
                      readNumber(cpu, "topology/core_id"));
    }
    info->physicalCpuCount = static_cast<int>(cores.size());
}
void Sys_FindInfo() {
    const long count = sysconf(_SC_NPROCESSORS_CONF);
    if (count < 1 || count > std::numeric_limits<int>::max()) failed("CPU count");
    sys_info.logicalCpuCount = static_cast<int>(count);
    s_cpuCount = static_cast<unsigned int>(count);
    long maxKHz = 0;
    for (int cpu = 0; cpu < count; ++cpu) {
        const long value = readNumber(cpu, "cpufreq/cpuinfo_max_freq");
        if (value <= 0) failed("CPU maximum frequency");
        if (value > maxKHz) maxKHz = value;
    }
    sys_info.cpuGHz = static_cast<long double>(maxKHz) / 1000000.0L;
    struct sysinfo memory{};
    if (sysinfo(&memory)) failed("sysinfo");
    const auto total = static_cast<std::uint64_t>(memory.totalram) * memory.mem_unit;
    // Preserve the reference engine's 1 GiB configuration cap; report real
    // physical RAM separately rather than using that cap as measured RAM.
    const auto megabytes = (total + 524288) / 1048576;
    sys_info.sysMB = static_cast<int>(megabytes > 1024 ? 1024 : megabytes);
    sys_info.SSE = false; // ARM64 does not implement x86 SSE.
    property("ro.soc.manufacturer", sys_info.cpuVendor, sizeof(sys_info.cpuVendor));
    property("ro.soc.model", sys_info.cpuName, sizeof(sys_info.cpuName));
    std::snprintf(sys_info.gpuDescription, sizeof(sys_info.gpuDescription),
                  "Headless: graphics device not initialized");
    // This calls the original reference configuration policy, including its
    // existing fixed benchmark estimate. It is not a measured ARM benchmark.
    Sys_SetAutoConfigureGHz(&sys_info);
    std::fprintf(stderr, "ANDROID INFO CPUs=%d cores=%d maxGHz=%.4Lf RAMMiB=%llu engineRAMMiB=%d SSE=0 SoC=%s %s referenceConfigureGHz=%.2Lf\n",
                 sys_info.logicalCpuCount, sys_info.physicalCpuCount, sys_info.cpuGHz,
                 static_cast<unsigned long long>(megabytes), sys_info.sysMB,
                 sys_info.cpuVendor, sys_info.cpuName, sys_info.configureGHz);
}
