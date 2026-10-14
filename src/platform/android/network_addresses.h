#pragma once
#include <qcommon/net_address.h>
#include <cstddef>
namespace bo1::android {
bool ResolveIPv4Host(const char *host, netadr_t *address);
bool ReadLocalIPv4(unsigned char (*addresses)[4], std::size_t capacity, int *count);
}
