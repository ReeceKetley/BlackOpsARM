#include <qcommon/msg_mp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int GetMinBitCountForNum(unsigned int value);
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
int main() {
    require(GetMinBitCountForNum(0) == 0, "zero bit width");
    for (unsigned bit = 0; bit < 32; ++bit) {
        require(GetMinBitCountForNum(1u << bit) == static_cast<int>(bit + 1), "single bit width");
        require(GetMinBitCountForNum((1u << bit) - 1) == static_cast<int>(bit), "preceding value width");
    }
    require(GetMinBitCountForNum(0xffffffffu) == 32, "full fixed32 width");
    alignas(8) unsigned char storage[32];
    std::memset(storage, 0x5a, sizeof(storage));
    msg_t message{}; message.data = storage + 2; message.maxsize = 11;
    MSG_WriteByte(&message, 0xa5); MSG_WriteShort(&message, 0xbeef);
    MSG_WriteLong(&message, 0x78563412); MSG_WriteFloat(&message, 1.5f);
    const unsigned char expected[] = {0xa5,0xef,0xbe,0x12,0x34,0x56,0x78,0,0,0xc0,0x3f};
    require(!message.overflowed && message.cursize == 11, "combined primitive length");
    require(!std::memcmp(message.data, expected, sizeof(expected)), "exact little-endian wire bytes");
    MSG_WriteLong(&message, 99);
    require(message.overflowed && message.cursize == 11, "overflow preserves cursor");
    for (unsigned i = 13; i < sizeof(storage); ++i) require(storage[i] == 0x5a, "overflow adjacent bounds");
    require(storage[0] == 0x5a && storage[1] == 0x5a, "prefix boundary");
    msg_t reader{}; reader.data = message.data; reader.cursize = 5;
    reader.splitData = message.data + 5; reader.splitSize = 6; reader.readOnly = 1;
    require(MSG_ReadByte(&reader) == 0xa5, "read byte");
    require(MSG_ReadShort(&reader) == static_cast<short>(0xbeef), "read signed short");
    require(MSG_ReadLong(&reader) == 0x78563412, "long crossing split boundary");
    require(MSG_ReadLong(&reader) == 0x3fc00000, "float representation read");
    require(reader.readcount == 11 && !reader.overflowed, "read cursor");
    require(MSG_ReadByte(&reader) == -1 && reader.overflowed && reader.readcount == 11, "read past end");
    std::puts("PASS original message fixed32 bit widths, unaligned wire bytes, split reads and overflow bounds");
}
