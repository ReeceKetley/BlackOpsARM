#include <qcommon/common.h>
#include <live/live_storage.h>
#include <live/live_storage_win.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>

extern int s_UTCOffset;
extern bool s_UTCSynced;

// Isolated fixture error boundary. Successful clock calls must never reach it.
// Production continues to link the canonical Com_Error implementation.
void Com_Error(errorParm_t, const char *, ...) { std::abort(); }

static int PackedUtc(std::time_t stamp) {
    std::tm utc{};
    if (!::gmtime_r(&stamp, &utc)) std::abort();
    return ((utc.tm_year + 1900) << 16) | ((utc.tm_mon + 1) << 8) | utc.tm_mday;
}

int main() {
    for (const char *zone : {"UTC0", "EST5", "JST-9"}) {
        if (::setenv("TZ", zone, 1)) return 1;
        ::tzset();
        const auto before = std::time(nullptr);
        const int packed = SystemTimeToInt();
        const auto after = std::time(nullptr);
        if (packed != PackedUtc(before) && packed != PackedUtc(after)) return 2;
        for (int offset : {0, INT_MAX, INT_MIN}) {
            s_UTCOffset = offset;
            const auto low = static_cast<std::int64_t>(std::time(nullptr)) + offset;
            const auto actual = LiveStorage_GetUTC();
            const auto high = static_cast<std::int64_t>(std::time(nullptr)) + offset;
            if (actual < low || actual > high) return 3;
            if (s_UTCSynced) return 4;
        }
    }
    s_UTCOffset = 0;
    std::puts("PASS actual storage UTC date across three TZ settings; signed offset extremes; no fabricated server sync");
}
