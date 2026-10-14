#include "process_resources.h"
#include <cerrno>
#include <charconv>
#include <climits>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <string_view>
#include <unistd.h>

namespace bo1::android {
namespace {
bool ReadKilobytes(std::string_view value, std::uint64_t *bytes)
{
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) return false;
    value.remove_prefix(first);
    std::uint64_t kilobytes;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), kilobytes);
    if (result.ec != std::errc{}) return false;
    std::string_view suffix(result.ptr, value.data() + value.size() - result.ptr);
    const auto unit = suffix.find_first_not_of(" \t");
    if (unit == std::string_view::npos) return false;
    suffix.remove_prefix(unit);
    if (suffix != "kB" || kilobytes > std::numeric_limits<std::uint64_t>::max() / 1024)
        return false;
    *bytes = kilobytes * 1024;
    return true;
}
bool ReadMemory(ProcessResources *snapshot)
{
    const int fd = open("/proc/self/status", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    char text[16384];
    std::size_t used = 0;
    bool success = true;
    while (used < sizeof(text)) {
        const ssize_t count = read(fd, text + used, sizeof(text) - used);
        if (count < 0) {
            if (errno == EINTR) continue;
            success = false;
            break;
        }
        if (!count) break;
        used += static_cast<std::size_t>(count);
    }
    if (used == sizeof(text)) { success = false; errno = EOVERFLOW; }
    const int readError = errno;
    close(fd);
    if (!success) { errno = readError; return false; }
    bool rss = false, peak = false, anonymous = false;
    std::string_view remaining(text, used);
    while (!remaining.empty()) {
        const auto end = remaining.find('\n');
        const auto line = remaining.substr(0, end);
        if (line.starts_with("VmRSS:")) rss = ReadKilobytes(line.substr(6), &snapshot->residentBytes);
        else if (line.starts_with("VmHWM:")) peak = ReadKilobytes(line.substr(6), &snapshot->peakResidentBytes);
        else if (line.starts_with("RssAnon:")) anonymous = ReadKilobytes(line.substr(8), &snapshot->anonymousResidentBytes);
        if (end == std::string_view::npos) break;
        remaining.remove_prefix(end + 1);
    }
    if (!rss || !peak || !anonymous) { errno = EPROTO; return false; }
    return true;
}
bool ReadDescriptors(unsigned *count)
{
    DIR *directory = opendir("/proc/self/fd");
    if (!directory) return false;
    const int queryFd = dirfd(directory);
    unsigned result = 0;
    errno = 0;
    while (dirent *entry = readdir(directory)) {
        unsigned number;
        const char *end = entry->d_name + std::strlen(entry->d_name);
        const auto parsed = std::from_chars(entry->d_name, end, number);
        if (parsed.ec != std::errc{} || parsed.ptr != end || number == static_cast<unsigned>(queryFd))
            continue;
        if (result == UINT_MAX) { closedir(directory); errno = EOVERFLOW; return false; }
        ++result;
    }
    const int readError = errno;
    const int closeResult = closedir(directory);
    if (readError) { errno = readError; return false; }
    if (closeResult) return false;
    *count = result;
    return true;
}
}
bool ReadProcessResources(ProcessResources *output)
{
    if (!output) { errno = EINVAL; return false; }
    ProcessResources snapshot{};
    if (!ReadMemory(&snapshot) || !ReadDescriptors(&snapshot.openFileDescriptors)) return false;
    *output = snapshot;
    return true;
}
bool ReadSystemCpuTicks(SystemCpuTicks *output)
{
    if (!output) { errno = EINVAL; return false; }
    const long frequency = sysconf(_SC_CLK_TCK);
    if (frequency <= 0 || static_cast<unsigned long>(frequency) > UINT_MAX) { errno = ERANGE; return false; }
    const int fd = open("/proc/stat", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    char buffer[1024];
    ssize_t length;
    do { length = read(fd, buffer, sizeof(buffer)); } while (length < 0 && errno == EINTR);
    const int readError = errno;
    close(fd);
    if (length < 0) { errno = readError; return false; }
    std::string_view line(buffer, static_cast<std::size_t>(length));
    const auto end = line.find('\n');
    if (end == std::string_view::npos || !line.starts_with("cpu ")) { errno = EPROTO; return false; }
    line = line.substr(4, end - 4);
    std::uint64_t ticks[8]{};
    for (auto &value : ticks) {
        const auto begin = line.find_first_not_of(" \t");
        if (begin == std::string_view::npos) { errno = EPROTO; return false; }
        line.remove_prefix(begin);
        const auto parsed = std::from_chars(line.data(), line.data() + line.size(), value);
        if (parsed.ec != std::errc{}) { errno = EPROTO; return false; }
        line.remove_prefix(parsed.ptr - line.data());
    }
    SystemCpuTicks sample{};
    for (const auto value : ticks) {
        if (value > UINT64_MAX - sample.total) { errno = EOVERFLOW; return false; }
        sample.total += value;
    }
    // Guest times are already included in user/nice. I/O wait counts as idle.
    sample.idle = ticks[3] + ticks[4];
    sample.ticksPerSecond = static_cast<unsigned>(frequency);
    *output = sample;
    return true;
}
}
