#pragma once
#include "fastfile_raw_string.h"
namespace bo1::fastfile {
enum class RawFilePayloadResult { Ok, Name, BufferLength, SourceBounds, DiskBounds, NativeBounds, CommitOverflow };
struct RawFilePayloadPlan {
    RawStringPlan name;
    bool bufferPresent{};
    std::uint64_t sourceBytes{},bufferBytes{},bufferDiskStart{},bufferNativeStart{},diskEnd{},nativeEnd{};
};
// Certifies only absent/inline names. Nonzero buffer is presence, not an offset.
// The original loader consumes exactly len+1 bytes; no buffer-NUL policy added.
RawFilePayloadResult PlanRawFilePayload(std::uint32_t nameTag,std::int32_t length,std::uint32_t bufferTag,
    const unsigned char*source,std::size_t sourceBytes,std::uint64_t diskCapacity,
    std::uint64_t nativeCapacity,std::uint64_t diskCursor,std::uint64_t nativeCursor,
    RawFilePayloadPlan&out);
const char*RawFilePayloadResultName(RawFilePayloadResult);
}
