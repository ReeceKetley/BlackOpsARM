#include "fastfile_inflate.h"
#include <zlib/zlib.h> // T5's matching custom stream ABI; never system libz.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace {
using Result = Bo1InflateResult;
using Reader = bool (*)(void *, std::uint8_t *, std::size_t, std::size_t *);
struct Memory { const std::uint8_t *data; std::size_t left; };
bool memoryRead(void *context, std::uint8_t *buffer, std::size_t count, std::size_t *got) {
    auto &m = *static_cast<Memory *>(context);
    *got = (std::min)(count, m.left);
    if (*got) { std::memcpy(buffer, m.data, *got); m.data += *got; m.left -= *got; }
    return true;
}
bool fileRead(void *context, std::uint8_t *buffer, std::size_t count, std::size_t *got) {
    auto *file = static_cast<FILE *>(context);
    *got = std::fread(buffer, 1, count, file);
    return std::ferror(file) == 0;
}
voidpf codecAlloc(voidpf, uInt items, uInt size) {
    if (items && size > (std::numeric_limits<std::size_t>::max)() / items) return nullptr;
    return std::calloc(items, size);
}
void codecFree(voidpf, voidpf allocation) { std::free(allocation); }
struct Stream {
    z_stream value{};
    bool initialized{};
    ~Stream() { if (initialized) inflateEnd(&value); }
};
bool valid(const Bo1InflateLimits &l) {
    return l.chunkBytes && l.chunkBytes <= 32768 && l.maxZeroPadding <= 31;
}
Result decode(Reader reader, void *context, const Bo1InflateLimits &limits, Bo1InflatedFastFile &result) {
    std::uint64_t readBytes = 0;
    auto read = [&](std::uint8_t *p, std::size_t count, std::size_t &got) {
        auto remaining = limits.maxFileBytes - readBytes;
        // Probe at most one byte beyond the budget so exact EOF at the limit passes.
        if (remaining < count) count = static_cast<std::size_t>(remaining) + 1;
        if (!reader(context, p, count, &got)) return Result::IoError;
        if (got > remaining) return Result::InputLimit;
        readBytes += got;
        return Result::Ok;
    };
    std::array<std::uint8_t, 32768> input{}, output{};
    std::size_t got = 0;
    auto status = read(input.data(), BO1_FF_RAW_HEADER_SIZE, got);
    if (status != Result::Ok) return status;
    if (got != BO1_FF_RAW_HEADER_SIZE) return Result::ShortRawHeader;
    auto parsed = Bo1_ParseFastFileRawHeader(input.data(), got, &result.raw);
    if (parsed == Bo1FastFileParseResult::BadMagic) return Result::BadMagic;
    if (parsed == Bo1FastFileParseResult::BadVersion) return Result::BadVersion;
    if (std::memcmp(result.raw.magic, BO1_FF_MAGIC_UNSIGNED, 8)) return Result::UnsupportedAuthenticated;
    Stream stream;
    auto &z = stream.value;
    z.zalloc = codecAlloc; z.zfree = codecFree;
    int code = inflateInit(&z); // Wrapped zlib/Adler32, default window15, native sizeof.
    if (code != Z_OK) return code == Z_MEM_ERROR ? Result::MemoryError : Result::CodecError;
    stream.initialized = true;
    bool eof = false, header = false;
    std::uint64_t consumed = 0, produced = 0;
    for (;;) {
        if (!z.avail_in && !eof) {
            status = read(input.data(), limits.chunkBytes, got);
            if (status != Result::Ok) return status;
            z.next_in = input.data(); z.avail_in = static_cast<uInt>(got);
            eof = !got;
        }
        z.next_out = output.data(); z.avail_out = static_cast<uInt>(limits.chunkBytes);
        uInt before = z.avail_in;
        code = inflate(&z, Z_SYNC_FLUSH); // Same mode as the production DB_Auth wrapper.
        auto used = before - z.avail_in;
        auto count = limits.chunkBytes - z.avail_out;
        consumed += used;
        if (count > limits.maxDecodedBytes - produced) return Result::OutputLimit;
        produced += count;
        result.decoded.insert(result.decoded.end(), output.begin(), output.begin() + count);
        if (!header && produced >= sizeof(XFile)) {
            std::memcpy(&result.file, result.decoded.data(), sizeof(XFile));
            header = true;
            if (std::uint64_t(sizeof(XFile)) + result.file.size > limits.maxDecodedBytes) return Result::OutputLimit;
        }
        if (code == Z_STREAM_END) break;
        if (code == Z_NEED_DICT) return Result::DictionaryRequired;
        if (code == Z_DATA_ERROR) return Result::CorruptStream;
        if (code == Z_MEM_ERROR) return Result::MemoryError;
        if (code != Z_OK && code != Z_BUF_ERROR) return Result::CodecError;
        if (!used && !count) {
            if (eof) return Result::TruncatedStream;
            if (z.avail_in) return Result::CorruptStream; // No-progress guard.
        }
    }
    if (!header) return Result::ShortXFile;
    if (produced != std::uint64_t(sizeof(XFile)) + result.file.size) return Result::SizeMismatch;
    std::uint64_t padding = 0;
    auto checkPadding = [&](const std::uint8_t *p, std::size_t size) {
        if (size > limits.maxZeroPadding - padding) return Result::TrailingData;
        for (std::size_t i = 0; i < size; ++i) if (p[i]) return Result::TrailingData;
        padding += size;
        return Result::Ok;
    };
    status = checkPadding(z.next_in, z.avail_in);
    if (status != Result::Ok) return status;
    do {
        status = read(input.data(), limits.chunkBytes, got);
        if (status != Result::Ok) return status;
        status = checkPadding(input.data(), got);
        if (status != Result::Ok) return status;
    } while (got);
    result.fileBytes = readBytes; result.compressedBytes = consumed; result.paddingBytes = padding;
    return Result::Ok;
}
Result run(Reader reader, void *context, const Bo1InflateLimits &limits, Bo1InflatedFastFile *out) {
    if (!out) return Result::InvalidArgument;
    *out = {};
    if (!valid(limits)) return Result::InvalidArgument;
    Bo1InflatedFastFile candidate;
    try {
        auto status = decode(reader, context, limits, candidate);
        if (status == Result::Ok) *out = std::move(candidate);
        return status;
    } catch (const std::bad_alloc &) { return Result::MemoryError; }
      catch (const std::length_error &) { return Result::OutputLimit; }
}
}
Bo1InflateResult Bo1_InflateFastFileMemory(const std::uint8_t *data, std::size_t length,
    const Bo1InflateLimits &limits, Bo1InflatedFastFile *out) {
    if (out && data && length && !out->decoded.empty()) {
        auto input = reinterpret_cast<std::uintptr_t>(data);
        auto stored = reinterpret_cast<std::uintptr_t>(out->decoded.data());
        if (input <= stored ? stored - input < length : input - stored < out->decoded.size()) {
            *out = {}; return Result::InvalidArgument;
        }
    }
    if (!data && length) { if (out) *out = {}; return Result::InvalidArgument; }
    Memory memory{data, length};
    return run(memoryRead, &memory, limits, out);
}
Bo1InflateResult Bo1_InflateFastFilePath(const char *path, const Bo1InflateLimits &limits, Bo1InflatedFastFile *out) {
    if (out) *out = {};
    if (!path || !out || !valid(limits)) return Result::InvalidArgument;
    errno = 0;
    auto *file = Bo1_OpenFile(path, "rb");
    if (!file) return errno == ENOENT || errno == ENOTDIR ? Result::NotFound : Result::IoError;
    auto status = run(fileRead, file, limits, out);
    if (std::fclose(file) && status == Result::Ok) { *out = {}; return Result::IoError; }
    return status;
}
const char *Bo1_InflateResultName(Bo1InflateResult result) {
    switch (result) {
#define NAME(x) case Result::x: return #x
    NAME(Ok); NAME(InvalidArgument); NAME(NotFound); NAME(IoError); NAME(ShortRawHeader);
    NAME(BadMagic); NAME(BadVersion); NAME(UnsupportedAuthenticated); NAME(InputLimit);
    NAME(OutputLimit); NAME(MemoryError); NAME(CodecError); NAME(DictionaryRequired);
    NAME(CorruptStream); NAME(TruncatedStream); NAME(ShortXFile); NAME(SizeMismatch); NAME(TrailingData);
#undef NAME
    }
    return "Unknown";
}
