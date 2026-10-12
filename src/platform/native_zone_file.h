#pragma once
#include "fastfile_inflate.h"
#include "fastfile_zone.h"
#include <array>

namespace bo1::fastfile {
enum class NativeZoneResult { Ok, Occupied, InvalidArgument, DecodeFailed,
    PlanFailed, WorkingSetLimit, AllocationFailed, StreamsAttached };
// Serial engine component. Main thread/Hunk (Android) or PMem (Windows) must
// already be initialized. This synchronous source does not replace DB workers.
// Native Windows refuses a third same-prim owner until legacy PMem record stride
// is verified. Android uses named Hunk ownership and has no such record limit.
// The object cannot move: PMem retains the address of its name and streams retain
// the address of its block array. Registered headers borrow its block children;
// remove/free those headers before Close, after detaching DB streams.
class NativeZoneFile final {
public:
    NativeZoneFile() = default;
    ~NativeZoneFile();
    NativeZoneFile(const NativeZoneFile &) = delete;
    NativeZoneFile &operator=(const NativeZoneFile &) = delete;
    NativeZoneResult Open(const char *path, const char *ownerName,
        const Bo1InflateLimits &, const ZoneLimits &, std::size_t pageBytes,
        std::uint64_t workingSetLimit);
    // workingSetLimit covers decoded/codec buffers then decoded+planned owner
    // capacity and the fixed name. Allocator metadata/process RSS are separate.
    NativeZoneResult Close();
    bool Read(unsigned char *destination, std::size_t capacity, int bytes);
    bool ReadCString(unsigned char *destination, std::size_t capacity, int &bytes);
    bool IsOpen() const { return open_; }
    std::size_t Position() const { return position_; }
    std::size_t Remaining() const { return decoded_.decoded.size() - position_; }
    const unsigned char *Current() const;
    const Bo1InflatedFastFile &Decoded() const { return decoded_; }
    const ZonePlan &Plan() const { return plan_; }
    XBlock (&Blocks())[7] { return blocks_; }
    Bo1InflateResult DecodeStatus() const { return decodeStatus_; }
    ZoneResult AllocationStatus() const { return zoneStatus_; }
private:
    static bool Begin(void *, const ZonePlan &);
    static unsigned char *Allocate(void *, unsigned, unsigned);
    static void End(void *);
    static void Release(void *);
    std::array<char, 64> name_{};
    Bo1InflatedFastFile decoded_;
    ZonePlan plan_{};
    XBlock blocks_[7]{};
    std::size_t position_{};
    bool open_{}, ownsAllocation_{};
    Bo1InflateResult decodeStatus_ = Bo1InflateResult::Ok;
    ZoneResult zoneStatus_ = ZoneResult::Ok;
};
}
