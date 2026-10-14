#include <qcommon/sv_msg_write.h>
#include <qcommon/msg.h>
#include <platform/android/thread_services.h>
#include <platform/critical_section_services.h>
#include <bit>
#include <climits>
#include <cstdint>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

namespace {
int checks;
struct Wire {
    std::uint64_t before[2];
    unsigned char bytes[64];
    std::uint64_t after[2];
};
std::uint32_t field(std::uint32_t bits, unsigned size)
{
    if (size == 4) return bits;
    const unsigned count = 8 * size;
    std::uint32_t result = 0;
    for (unsigned i = 0; i < count; ++i)
        if ((bits >> i) & 1u) result |= std::uint32_t{1} << i;
    if ((bits >> (count - 1)) & 1u)
        for (unsigned i = count; i < 32; ++i) result |= std::uint32_t{1} << i;
    return result;
}
bool run(int bits, unsigned size, std::uint32_t fromBits, std::uint32_t toBits, unsigned prefix, bool noXor)
{
    const unsigned count = bits < 0 ? static_cast<unsigned>(-bits) : static_cast<unsigned>(bits);
    const int from = std::bit_cast<int>(fromBits);
    const int to = std::bit_cast<int>(toBits);
    const std::uint32_t fromField = field(fromBits, size);
    const std::uint32_t toField = field(toBits, size);
    const std::uint32_t delta = noXor ? toBits : fromField ^ toField;
    Wire wire;
    std::memset(&wire, 0xA5, sizeof(wire));
    msg_t msg{};
    msg.data = wire.bytes;
    msg.maxsize = sizeof(wire.bytes);
    if (prefix) MSG_WriteBits(&msg, 5, prefix);
    const SnapshotInfo_s snapshot{};
    if (noXor) MSG_WriteValueNoXor(&snapshot, &msg, to, bits);
    else MSG_WriteValue(&snapshot, &msg, &from, &to, bits, size);

    // Independent wire construction: partial bits share the prefix's byte;
    // each complete byte follows at the byte cursor, matching the field format.
    unsigned char expected[64]{};
    unsigned length = prefix ? 1 : 0;
    unsigned bitCursor = prefix;
    if (prefix) expected[0] = 5;
    const unsigned partial = count % 8;
    for (unsigned i = 0; i < partial; ++i) {
        if (bitCursor % 8 == 0) {
            bitCursor = length * 8;
            expected[length++] = 0;
        }
        if ((delta >> i) & 1u) expected[bitCursor / 8] |= 1u << (bitCursor % 8);
        ++bitCursor;
    }
    for (unsigned offset = partial; offset < count; offset += 8) {
        unsigned byte = 0;
        for (unsigned bit = 0; bit < 8; ++bit)
            if ((delta >> (offset + bit)) & 1u) byte |= 1u << bit;
        expected[length++] = static_cast<unsigned char>(byte);
    }
    if (msg.overflowed || msg.cursize != static_cast<int>(length) || msg.bit != static_cast<int>(bitCursor)
        || std::memcmp(msg.data, expected, length) != 0) {
        std::printf("FAIL wire width=%d size=%u prefix=%u noXor=%d from=%08x to=%08x\n", bits, size, prefix, noXor, fromBits, toBits);
        return false;
    }
    ++checks;
    for (unsigned i = length; i < sizeof(wire.bytes); ++i)
        if (wire.bytes[i] != 0xA5) return false;
    for (unsigned i = 0; i < 2; ++i)
        if (wire.before[i] != UINT64_C(0xA5A5A5A5A5A5A5A5) || wire.after[i] != UINT64_C(0xA5A5A5A5A5A5A5A5)) return false;
    ++checks;
    msg_t reader = msg;
    reader.readOnly = 1;
    MSG_BeginReading(&reader);
    if (prefix && MSG_ReadBits(&reader, prefix) != 5) return false;
    std::uint32_t resultExpected = 0;
    const std::uint32_t resultSource = noXor ? delta : toField;
    for (unsigned i = 0; i < count; ++i)
        if ((resultSource >> i) & 1u) resultExpected |= std::uint32_t{1} << i;
    if (!noXor && bits < 0 && ((resultSource >> (count - 1)) & 1u))
        for (unsigned i = count; i < 32; ++i) resultExpected |= std::uint32_t{1} << i;
    int output = 0x12345678;
    const int result = noXor ? MSG_ReadValueNoXor(&reader, count) : MSG_ReadValue(&reader, &from, &output, bits, size);
    if (std::bit_cast<std::uint32_t>(result) != resultExpected || reader.overflowed || reader.readcount != msg.cursize
        || output != 0x12345678 || std::bit_cast<std::uint32_t>(from) != fromBits || std::bit_cast<std::uint32_t>(to) != toBits) {
        std::printf("FAIL read width=%d size=%u prefix=%u noXor=%d actual=%08x expected=%08x\n", bits, size, prefix, noXor, std::bit_cast<std::uint32_t>(result), resultExpected);
        return false;
    }
    ++checks;
    return true;
}
}
int main(int argc, char **argv)
{
    Sys_InitMainThread();
    Sys_InitializeCriticalSections();
    if (argc > 1) {
        static msg_t msg{};
        static unsigned char storage[64]{};
        msg.data = storage;
        msg.maxsize = sizeof(storage);
        // Exercise the actual main-thread error recovery owner, rather than
        // leaving Com_Error's genuine nonlocal destination uninitialized.
        if (setjmp(*static_cast<jmp_buf *>(Sys_GetValue(2))) != 0) {
            if (msg.cursize || msg.bit) {
                std::puts("FAIL invalid width wrote data before error recovery");
                return 1;
            }
            for (unsigned char byte : storage)
                if (byte != 0) {
                    std::puts("FAIL invalid width changed wire bytes before error recovery");
                    return 1;
                }
            std::puts("EXPECTED canonical Com_Error nonlocal rejection; wire bytes/cursors unchanged");
            return 2;
        }
        const int bits = std::strcmp(argv[1], "min") == 0 ? INT_MIN : std::atoi(argv[1]);
        if (argc > 2) MSG_WriteValueNoXor(nullptr, &msg, -1, bits);
        else { const int from = 0, to = -1; MSG_WriteValue(nullptr, &msg, &from, &to, bits, 4); }
        std::puts("FAIL invalid width accepted");
        return 1;
    }
    const int widths[] = {1,-1,8,-8,16,-16,31,-31,32,-32};
    const std::uint32_t values[][2] = {
        {0,0}, {0,0xffffffffu}, {0xffffffffu,0},
        {0xa5a57f00u,0x8000f0fdu}, {0x7fff8001u,0xdeadbeefu}
    };
    for (int width : widths)
        for (const auto &value : values)
            for (unsigned prefix : {0u,3u}) {
                for (unsigned size : {1u,2u,4u})
                    if (!run(width, size, value[0], value[1], prefix, false)) return 1;
                if (!run(width, 4, value[0], value[1], prefix, true)) return 1;
            }
    // Width zero remains an existing empty-field operation, rather than clamping.
    if (!run(0, 4, 0, 0, 0, true)) return 1;
    std::printf("PASS %d actual signed-width writer/reader wire/canary checks\n", checks);
    return 0;
}
