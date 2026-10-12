#pragma once
#include <cstddef>
#include <cstdint>
namespace bo1::fastfile {
enum class RawFileHeaderResult { Ok, UnsupportedAsset, UnsupportedTag, ShortHeader, NegativeLength, LengthOverflow, SourceBounds, DiskBounds, NativeBounds };
struct RawFileHeaderPlan {
    bool present{};
    std::int32_t length{};
    std::uint32_t nameTag{},bufferTag{};
    std::uint64_t futureBufferBytes{},diskStart{},nativeStart{},diskEnd{},nativeEnd{};
};
// Numeric header only. Child bytes and asset registration are separate gates.
RawFileHeaderResult PlanRawFileHeader(std::int32_t type,std::uint32_t tag,
    const unsigned char*source,std::size_t sourceBytes,std::uint64_t decodedRemaining,
    std::uint64_t diskCapacity,std::uint64_t nativeCapacity,std::uint64_t diskCursor,
    std::uint64_t nativeCursor,RawFileHeaderPlan&out);
const char*RawFileHeaderResultName(RawFileHeaderResult);
}
