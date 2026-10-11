#pragma once
// Bounded FastFile preamble reader (TASK-0008).
//
// Covers the FastFile head in two production-accurate layers, mirroring
// DB_LoadXFileInternal order: the 12-byte raw file head (8-byte magic
// "IWff0100" retail or "IWffu100" unsigned, little-endian uint32 version
// 473, verified before inflate) and the logical 48-byte stream head (raw
// head + 36-byte XFile header: size, externalSize, blockSize[7]; see
// src/database/db_file_load.h). All engine targets are little-endian; the
// parser copies bytes with memcpy so unaligned buffers are safe, but
// cross-endian hosts are not supported.
//
// The file-I/O boundary is synchronous stdio: production opens zones with
// overlapped CreateFileA + ReadFileEx/SleepEx APC pump and inflates the
// stream with zlib. That async pipeline, zone block allocation and all asset
// parsing are explicitly future work (TASK-0009/0010). Errors are explicit;
// there are no success stubs.
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BO1_FF_MAGIC_RETAIL "IWff0100"
#define BO1_FF_MAGIC_UNSIGNED "IWffu100"
#define BO1_FF_VERSION 473u
#define BO1_FF_BLOCK_COUNT 7
#define BO1_FF_RAW_HEADER_SIZE 12
#define BO1_FF_PREAMBLE_SIZE 48

// Raw file head, verified by production DB_LoadXFileInternal before inflate
// init: 8-byte magic plus little-endian uint32 version. The 36-byte XFile
// header is NOT stored contiguously after it; it is the first 36 bytes of
// the zlib-deflated stream (retail files carry compressed bytes at raw
// offset 12, e.g. code_pre_gfx.ff). Bo1FastFilePreamble below models the
// logical stream head: raw head + first 36 decompressed bytes.
struct Bo1FastFileRawHeader
{
    char magic[8];
    uint32_t version;
};

static_assert(sizeof(Bo1FastFileRawHeader) == BO1_FF_RAW_HEADER_SIZE,
    "Raw FastFile head must stay 12 bytes: 8 magic + 4 version");
static_assert(offsetof(Bo1FastFileRawHeader, version) == 8,
    "version follows magic");

struct Bo1FastFilePreamble
{
    char magic[8];
    uint32_t version;
    uint32_t size;
    uint32_t externalSize;
    uint32_t blockSize[BO1_FF_BLOCK_COUNT];
};

static_assert(sizeof(Bo1FastFilePreamble) == BO1_FF_PREAMBLE_SIZE,
    "FastFile preamble must stay 48 bytes: 8 magic + 4 version + 36 XFile");
static_assert(offsetof(Bo1FastFilePreamble, version) == 8,
    "version follows magic");
static_assert(offsetof(Bo1FastFilePreamble, size) == 12,
    "XFile.size offset must match production db_file_load.h");
static_assert(offsetof(Bo1FastFilePreamble, externalSize) == 16,
    "XFile.externalSize offset must match production db_file_load.h");
static_assert(offsetof(Bo1FastFilePreamble, blockSize) == 20,
    "XFile.blockSize offset must match production db_file_load.h");

enum class Bo1FastFileParseResult
{
    Ok,
    ShortBuffer,
    BadMagic,
    BadVersion,
};

inline Bo1FastFileParseResult Bo1_ParseFastFileRawHeader(
    const uint8_t *data, size_t length, Bo1FastFileRawHeader *out)
{
    if (!data || !out)
        return Bo1FastFileParseResult::ShortBuffer;
    if (length < BO1_FF_RAW_HEADER_SIZE)
        return Bo1FastFileParseResult::ShortBuffer;
    memcpy(out, data, BO1_FF_RAW_HEADER_SIZE);
    if (memcmp(out->magic, BO1_FF_MAGIC_RETAIL, 8) != 0
        && memcmp(out->magic, BO1_FF_MAGIC_UNSIGNED, 8) != 0)
        return Bo1FastFileParseResult::BadMagic;
    if (out->version != BO1_FF_VERSION)
        return Bo1FastFileParseResult::BadVersion;
    return Bo1FastFileParseResult::Ok;
}

inline Bo1FastFileParseResult Bo1_ParseFastFilePreamble(
    const uint8_t *data, size_t length, Bo1FastFilePreamble *out)
{
    if (!data || !out)
        return Bo1FastFileParseResult::ShortBuffer;
    if (length < BO1_FF_PREAMBLE_SIZE)
        return Bo1FastFileParseResult::ShortBuffer;
    memcpy(out, data, BO1_FF_PREAMBLE_SIZE);
    if (memcmp(out->magic, BO1_FF_MAGIC_RETAIL, 8) != 0
        && memcmp(out->magic, BO1_FF_MAGIC_UNSIGNED, 8) != 0)
        return Bo1FastFileParseResult::BadMagic;
    if (out->version != BO1_FF_VERSION)
        return Bo1FastFileParseResult::BadVersion;
    return Bo1FastFileParseResult::Ok;
}

enum class Bo1FileIoResult
{
    Ok,
    NotFound,
    ShortRead,
    IoError,
};

// fopen_s on MSVC (C4996), plain fopen elsewhere (Bionic/POSIX).
inline FILE *Bo1_OpenFile(const char *path, const char *mode)
{
#if defined(_WIN32)
    FILE *f = nullptr;
    return fopen_s(&f, path, mode) == 0 ? f : nullptr;
#else
    return fopen(path, mode);
#endif
}

// Reads exactly `wanted` bytes from the start of `path`. `received` reports
// how many bytes were placed in `buffer` on ShortRead.
inline Bo1FileIoResult Bo1_ReadFilePrefix(
    const char *path, uint8_t *buffer, size_t wanted, size_t *received)
{
    if (received)
        *received = 0;
    if (!path || !buffer)
        return Bo1FileIoResult::IoError;
    FILE *f = Bo1_OpenFile(path, "rb");
    if (!f)
        return Bo1FileIoResult::NotFound;
    size_t got = fread(buffer, 1, wanted, f);
    if (received)
        *received = got;
    if (ferror(f))
    {
        fclose(f);
        return Bo1FileIoResult::IoError;
    }
    fclose(f);
    return got == wanted ? Bo1FileIoResult::Ok : Bo1FileIoResult::ShortRead;
}

inline Bo1FileIoResult Bo1_WriteFileWhole(
    const char *path, const uint8_t *data, size_t length)
{
    if (!path || (!data && length > 0))
        return Bo1FileIoResult::IoError;
    FILE *f = Bo1_OpenFile(path, "wb");
    if (!f)
        return Bo1FileIoResult::IoError;
    size_t wrote = length > 0 ? fwrite(data, 1, length, f) : 0;
    bool failed = ferror(f) != 0;
    if (fclose(f) != 0)
        failed = true;
    if (failed || wrote != length)
        return Bo1FileIoResult::IoError;
    return Bo1FileIoResult::Ok;
}

inline Bo1FileIoResult Bo1_FileSize(const char *path, long *sizeOut)
{
    if (sizeOut)
        *sizeOut = 0;
    if (!path || !sizeOut)
        return Bo1FileIoResult::IoError;
    FILE *f = Bo1_OpenFile(path, "rb");
    if (!f)
        return Bo1FileIoResult::NotFound;
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return Bo1FileIoResult::IoError;
    }
    long size = ftell(f);
    fclose(f);
    if (size < 0)
        return Bo1FileIoResult::IoError;
    *sizeOut = size;
    return Bo1FileIoResult::Ok;
}
