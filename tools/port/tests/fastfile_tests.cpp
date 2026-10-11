// Bounded FastFile preamble probe (TASK-0008). Standalone gate only.
// Parses a byte-exact 48-byte fixture preamble (magic/version/XFile header)
// through the shared src/platform/fastfile_reader.h boundary and roundtrips
// it through a temp file beside this executable. No genuine .ff assets exist
// in this environment, so no retail file is read; that limit is documented,
// not hidden. No zone loading, inflate, or asset parsing is attempted.
#include <platform/fastfile_reader.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

static_assert(sizeof(Bo1FastFileRawHeader) == 12);
static_assert(offsetof(Bo1FastFileRawHeader, version) == 8);
static_assert(sizeof(Bo1FastFilePreamble) == 48);
static_assert(alignof(Bo1FastFilePreamble) == 4);
static_assert(offsetof(Bo1FastFilePreamble, version) == 8);
static_assert(offsetof(Bo1FastFilePreamble, size) == 12);
static_assert(offsetof(Bo1FastFilePreamble, externalSize) == 16);
static_assert(offsetof(Bo1FastFilePreamble, blockSize) == 20);

static void check(bool value, int line, const char *expr)
{
    if (!value)
    {
        std::fprintf(stderr, "FAIL %d: %s\n", line, expr);
        std::abort();
    }
}
#define CHECK(c) check((c), __LINE__, #c)

// Byte-exact retail-shaped preamble: magic "IWff0100", version 473,
// then a plausible XFile header. Field bytes are little-endian.
static void makeFixture(uint8_t out[BO1_FF_PREAMBLE_SIZE])
{
    memset(out, 0, BO1_FF_PREAMBLE_SIZE);
    memcpy(out, BO1_FF_MAGIC_RETAIL, 8);
    uint32_t version = BO1_FF_VERSION;
    uint32_t size = 0x12345678u;
    uint32_t external = 0x00001000u;
    memcpy(out + 8, &version, 4);
    memcpy(out + 12, &size, 4);
    memcpy(out + 16, &external, 4);
    for (int i = 0; i < BO1_FF_BLOCK_COUNT; ++i)
    {
        uint32_t blocks = 0x1000u * (uint32_t)(i + 1);
        memcpy(out + 20 + 4 * i, &blocks, 4);
    }
}

int main(int argc, char **argv)
{
    // In-memory parse paths first; no filesystem needed.
    uint8_t fixture[BO1_FF_PREAMBLE_SIZE];
    makeFixture(fixture);
    Bo1FastFilePreamble parsed{};
    CHECK(Bo1_ParseFastFilePreamble(fixture, sizeof(fixture), &parsed)
        == Bo1FastFileParseResult::Ok);
    CHECK(memcmp(parsed.magic, BO1_FF_MAGIC_RETAIL, 8) == 0);
    CHECK(parsed.version == 473u);
    CHECK(parsed.size == 0x12345678u);
    CHECK(parsed.externalSize == 0x1000u);
    for (int i = 0; i < BO1_FF_BLOCK_COUNT; ++i)
        CHECK(parsed.blockSize[i] == 0x1000u * (uint32_t)(i + 1));

    uint8_t dev[BO1_FF_PREAMBLE_SIZE];
    makeFixture(dev);
    memcpy(dev, BO1_FF_MAGIC_UNSIGNED, 8);
    CHECK(Bo1_ParseFastFilePreamble(dev, sizeof(dev), &parsed)
        == Bo1FastFileParseResult::Ok);
    CHECK(parsed.version == 473u);

    uint8_t bad[BO1_FF_PREAMBLE_SIZE];
    makeFixture(bad);
    memcpy(bad, "NOTaFF!!", 8);
    CHECK(Bo1_ParseFastFilePreamble(bad, sizeof(bad), &parsed)
        == Bo1FastFileParseResult::BadMagic);
    makeFixture(bad);
    uint32_t oldVersion = 472u;
    memcpy(bad + 8, &oldVersion, 4);
    CHECK(Bo1_ParseFastFilePreamble(bad, sizeof(bad), &parsed)
        == Bo1FastFileParseResult::BadVersion);
    makeFixture(bad);
    uint32_t newVersion = 474u;
    memcpy(bad + 8, &newVersion, 4);
    CHECK(Bo1_ParseFastFilePreamble(bad, sizeof(bad), &parsed)
        == Bo1FastFileParseResult::BadVersion);
    CHECK(Bo1_ParseFastFilePreamble(fixture, 47, &parsed)
        == Bo1FastFileParseResult::ShortBuffer);
    CHECK(Bo1_ParseFastFilePreamble(fixture, 0, &parsed)
        == Bo1FastFileParseResult::ShortBuffer);

    // File-I/O boundary: fixture roundtrip beside this executable, plus
    // explicit missing-file and short-file errors.
    CHECK(argc > 0 && argv[0]);
    std::string self = argv[0];
    size_t slash = self.find_last_of("/\\");
    std::string dir = slash == std::string::npos ? "." : self.substr(0, slash);
    std::string path = dir + "/bo1_fastfile_fixture.ff";
    std::string shortPath = dir + "/bo1_fastfile_short.ff";
    CHECK(Bo1_WriteFileWhole(path.c_str(), fixture, sizeof(fixture))
        == Bo1FileIoResult::Ok);
    long size = 0;
    CHECK(Bo1_FileSize(path.c_str(), &size) == Bo1FileIoResult::Ok);
    CHECK(size == (long)BO1_FF_PREAMBLE_SIZE);
    uint8_t roundtrip[BO1_FF_PREAMBLE_SIZE];
    memset(roundtrip, 0xA5, sizeof(roundtrip));
    size_t received = 0;
    CHECK(Bo1_ReadFilePrefix(path.c_str(), roundtrip, sizeof(roundtrip), &received)
        == Bo1FileIoResult::Ok);
    CHECK(received == sizeof(roundtrip));
    CHECK(memcmp(roundtrip, fixture, sizeof(fixture)) == 0);
    CHECK(Bo1_ParseFastFilePreamble(roundtrip, sizeof(roundtrip), &parsed)
        == Bo1FastFileParseResult::Ok);
    CHECK(parsed.size == 0x12345678u && parsed.blockSize[6] == 0x7000u);

    uint8_t ten[10] = {'I', 'W', 'f', 'f', '0', '1', '0', '0', 0, 0};
    CHECK(Bo1_WriteFileWhole(shortPath.c_str(), ten, sizeof(ten))
        == Bo1FileIoResult::Ok);
    uint8_t readback[BO1_FF_PREAMBLE_SIZE];
    received = 0;
    CHECK(Bo1_ReadFilePrefix(shortPath.c_str(), readback, sizeof(readback), &received)
        == Bo1FileIoResult::ShortRead);
    CHECK(received == sizeof(ten));
    CHECK(Bo1_ParseFastFilePreamble(readback, received, &parsed)
        == Bo1FastFileParseResult::ShortBuffer);

    std::string missing = dir + "/bo1_fastfile_missing.ff";
    std::remove(missing.c_str());
    CHECK(Bo1_ReadFilePrefix(missing.c_str(), readback, sizeof(readback), &received)
        == Bo1FileIoResult::NotFound);
    CHECK(Bo1_FileSize(missing.c_str(), &size) == Bo1FileIoResult::NotFound);

    // Raw-head parser on the same fixture bytes (production verifies the
    // 12-byte raw head before inflate init).
    Bo1FastFileRawHeader raw{};
    CHECK(Bo1_ParseFastFileRawHeader(fixture, sizeof(fixture), &raw)
        == Bo1FastFileParseResult::Ok);
    CHECK(memcmp(raw.magic, BO1_FF_MAGIC_RETAIL, 8) == 0);
    CHECK(raw.version == 473u);
    CHECK(Bo1_ParseFastFileRawHeader(fixture, 11, &raw)
        == Bo1FastFileParseResult::ShortBuffer);
    uint8_t rawBad[BO1_FF_RAW_HEADER_SIZE];
    memcpy(rawBad, fixture, sizeof(rawBad));
    memcpy(rawBad, "BADMAGIC", 8);
    CHECK(Bo1_ParseFastFileRawHeader(rawBad, sizeof(rawBad), &raw)
        == Bo1FastFileParseResult::BadMagic);
    memcpy(rawBad, fixture, sizeof(rawBad));
    uint32_t rawOld = 472u;
    memcpy(rawBad + 8, &rawOld, 4);
    CHECK(Bo1_ParseFastFileRawHeader(rawBad, sizeof(rawBad), &raw)
        == Bo1FastFileParseResult::BadVersion);

    // Optional genuine-file mode: argv[1] names a retail .ff whose 12-byte
    // raw head is verified through the same file-I/O boundary. Beyond byte
    // 12 retail files carry deflate bytes (XFile comes from the inflate
    // stream, TASK-0010), so only the raw head is checked here.
    if (argc > 1 && argv[1] && argv[1][0])
    {
        uint8_t head[BO1_FF_RAW_HEADER_SIZE];
        size_t got = 0;
        CHECK(Bo1_ReadFilePrefix(argv[1], head, sizeof(head), &got)
            == Bo1FileIoResult::Ok);
        CHECK(got == sizeof(head));
        CHECK(Bo1_ParseFastFileRawHeader(head, sizeof(head), &raw)
            == Bo1FastFileParseResult::Ok);
        long genuineSize = 0;
        CHECK(Bo1_FileSize(argv[1], &genuineSize) == Bo1FileIoResult::Ok);
        CHECK(genuineSize > (long)sizeof(head));
        std::printf("GENUINE ok: '%s' magic=%.8s version=%u size=%ld\n",
            argv[1], raw.magic, raw.version, genuineSize);
    }

    std::remove(path.c_str());
    std::remove(shortPath.c_str());
    std::puts("PASS bounded FastFile preamble: retail/unsigned magic, version 473, XFile header fields, fixture roundtrip, explicit missing/short/bad errors");
    return 0;
}
