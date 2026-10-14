#pragma once
#include <qcommon/net_address.h>
#include <cstddef>
#include <cstdint>
namespace bo1::android {
bool OpenUdp(const char *interfaceName, std::uint16_t hostPort);
bool CloseUdp();
std::uint16_t BoundUdpPort();
bool SendUdp(const void *data, std::size_t length, const netadr_t &destination);
// 1 packet, 0 would-block, -1 real error. Metadata commits only for full packets.
int ReceiveUdp(void *data, std::size_t capacity, netadr_t *source, std::size_t *length);
}
