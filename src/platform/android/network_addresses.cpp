#include "network_addresses.h"
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>
#include <climits>

static_assert(sizeof(netadr_t) == 16 && sizeof(netadr_t::port) == 2);
namespace bo1::android {
bool ResolveIPv4Host(const char *host, netadr_t *address) {
    if (!host || !address || !*host || std::strlen(host) > 255) { errno = EINVAL; return false; }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    bool numeric = true;
    for (const char *p = host; *p; ++p) if ((*p < '0' || *p > '9') && *p != '.') numeric = false;
    if (numeric) hints.ai_flags = AI_NUMERICHOST;
    addrinfo *result = nullptr;
    const int error = ::getaddrinfo(host, nullptr, &hints, &result);
    if (error) { if (error != EAI_SYSTEM) errno = EADDRNOTAVAIL; return false; }
    bool found = false;
    netadr_t resolved{};
    for (auto *entry = result; entry; entry = entry->ai_next) {
        if (entry->ai_family == AF_INET && entry->ai_addr && entry->ai_addrlen >= sizeof(sockaddr_in)) {
            const auto *ipv4 = reinterpret_cast<const sockaddr_in *>(entry->ai_addr);
            resolved.type = NA_IP;
            std::memcpy(resolved.ip, &ipv4->sin_addr, sizeof(resolved.ip));
            resolved.port = ipv4->sin_port;
            found = true;
            break;
        }
    }
    ::freeaddrinfo(result);
    if (!found) { errno = EADDRNOTAVAIL; return false; }
    *address = resolved;
    return true;
}
bool ReadLocalIPv4(unsigned char (*addresses)[4], std::size_t capacity, int *count) {
    if (!addresses || !count || !capacity || capacity > INT_MAX) { errno = EINVAL; return false; }
    ifaddrs *interfaces = nullptr;
    if (::getifaddrs(&interfaces)) return false;
    int copied = 0;
    for (auto *entry = interfaces; entry && static_cast<std::size_t>(copied) < capacity; entry = entry->ifa_next) {
        if (!entry->ifa_addr || entry->ifa_addr->sa_family != AF_INET || !(entry->ifa_flags & IFF_UP)) continue;
        const auto *ipv4 = reinterpret_cast<const sockaddr_in *>(entry->ifa_addr);
        std::memcpy(addresses[copied++], &ipv4->sin_addr, 4);
    }
    ::freeifaddrs(interfaces);
    *count = copied;
    return true;
}
}
