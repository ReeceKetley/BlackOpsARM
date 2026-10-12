#pragma once
#include <database/db_zone_types.h>
#include "fastfile_reader.h"
#include <cstddef>
#include <cstdint>
#include <vector>

// TASK-0010 synchronous isolation. Not the engine's async loader or asset parser.
enum class Bo1InflateResult {
    Ok, InvalidArgument, NotFound, IoError, ShortRawHeader, BadMagic, BadVersion,
    UnsupportedAuthenticated, InputLimit, OutputLimit, MemoryError, CodecError,
    DictionaryRequired, CorruptStream, TruncatedStream, ShortXFile, SizeMismatch,
    TrailingData
};
struct Bo1InflateLimits {
    std::uint64_t maxFileBytes = 256ull * 1024 * 1024; // Includes raw header/padding.
    std::uint64_t maxDecodedBytes = 512ull * 1024 * 1024; // Output length incl. XFile, not total heap/capacity.
    std::size_t chunkBytes = 32768; // 1..32768; never narrow a whole file to uInt.
    std::size_t maxZeroPadding = 31; // Explicit policy; 0 selects strict end-of-file.
    // Zero preserves historical behavior. Nonzero caps decoded allocations
    // (including old+new growth), codec allocations and fixed 64KB buffers.
    std::uint64_t maxWorkingBytes = 0;
};
struct Bo1InflatedFastFile {
    Bo1FastFileRawHeader raw{};
    XFile file{};
    std::vector<std::uint8_t> decoded; // Exact disk XFile then file.size payload bytes.
    std::uint64_t fileBytes{}, compressedBytes{}, paddingBytes{};
    std::uint64_t peakWorkingBytes{}; // Allocation bytes, not process RSS.
};
// Output is cleared on every failure; no partial header is published as validated.
// Input overlapping out->decoded is rejected before clearing output.
Bo1InflateResult Bo1_InflateFastFileMemory(const std::uint8_t *data, std::size_t length,
    const Bo1InflateLimits &limits, Bo1InflatedFastFile *out);
Bo1InflateResult Bo1_InflateFastFilePath(const char *path,
    const Bo1InflateLimits &limits, Bo1InflatedFastFile *out);
const char *Bo1_InflateResultName(Bo1InflateResult result);
