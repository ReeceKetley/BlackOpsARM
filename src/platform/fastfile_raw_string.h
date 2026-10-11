#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>
#include <cstdint>

namespace bo1::fastfile {
enum class RawStringResult { Ok, UnsupportedTag, InvalidSource, Unterminated, CommitOverflow, DiskBounds, NativeBounds };
struct RawStringPlan {
    bool present{};
    std::uint64_t bytes{},diskStart{},nativeStart{},diskEnd{},nativeEnd{},nativePadding{};
};
// Only inline(-1) and absent(0) tags are certified here. Offset conversion is
// a separate boundary: allocated capacity does not prove initialized characters.
RawStringResult PlanRawString(std::uint32_t tag,const unsigned char*source,std::size_t remaining,
    std::uint64_t diskCapacity,std::uint64_t nativeCapacity,std::uint64_t diskCursor,
    std::uint64_t nativeCursor,RawStringPlan&out);
const char *RawStringResultName(RawStringResult);

// Borrowed validated decoded bytes; does not own or duplicate a decoded buffer.
// Callers provide the actual remaining destination capacity. Every failed read
// leaves the destination and source position untouched. No async/refill claim.
class DecodedSource {
public:
    DecodedSource(const unsigned char*data,std::size_t size):data_(data),size_(size){}
    bool Read(unsigned char*destination,std::size_t capacity,int bytes);
    RawStringResult ReadCString(unsigned char*destination,std::size_t capacity,int&bytes);
    std::size_t Position()const{return position_;}
    std::size_t Remaining()const{return size_-position_;}
    const unsigned char*Current()const{return data_?data_+position_:nullptr;}
private:
    const unsigned char*data_{};
    std::size_t size_{},position_{};
};
}
