#include <platform/android/network_addresses.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
int Sys_StringToAdr(const char *, netadr_t *);
bool Sys_IsLANAddress_IgnoreSubnet(netadr_t);
int Sys_IsLANAddress(netadr_t);

int main() {
    struct { netadr_t value; unsigned char sentinel[4]; } parsed{};
    std::memset(parsed.sentinel, 0xA5, sizeof(parsed.sentinel));
    if (Sys_StringToAdr("192.0.2.17", &parsed.value) != 1 || parsed.value.type != NA_IP ||
        parsed.value.ip[0] != 192 || parsed.value.ip[1] != 0 || parsed.value.ip[2] != 2 ||
        parsed.value.ip[3] != 17 || parsed.value.port || parsed.value.addrHandleIndex) return 1;
    for (auto byte: parsed.sentinel) if (byte != 0xA5) return 2;
    const netadr_t previous = parsed.value;
    if (Sys_StringToAdr("999.2.3.4", &parsed.value) ||
        std::memcmp(&parsed.value, &previous, sizeof(previous))) return 3;
    if (Sys_StringToAdr("", &parsed.value) ||
        std::memcmp(&parsed.value, &previous, sizeof(previous))) return 4;
    if (Sys_StringToAdr(nullptr, &parsed.value) || Sys_StringToAdr("127.0.0.1", nullptr)) return 5;
    if (Sys_StringToAdr("localhost", &parsed.value) != 1 || parsed.value.ip[0] != 127) return 6;
    struct Case { unsigned char ip[4]; bool local; } cases[] = {
        {{10,0,0,1},true}, {{127,0,0,1},true}, {{169,254,1,1},true},
        {{169,253,1,1},false}, {{172,15,0,1},false}, {{172,16,0,1},true},
        {{172,31,255,254},true}, {{172,32,0,1},false}, {{192,168,0,1},true},
        {{192,169,0,1},false}, {{8,8,8,8},false}, {{203,0,113,44},false}
    };
    for (const auto &test: cases) {
        netadr_t address{}; address.type = NA_IP;
        std::memcpy(address.ip, test.ip, 4);
        if (Sys_IsLANAddress_IgnoreSubnet(address) != test.local) return 7;
        if (test.local && Sys_IsLANAddress(address) != 1) return 8;
    }
    netadr_t loop{}; loop.type = NA_LOOPBACK;
    if (Sys_IsLANAddress(loop) != 1) return 9;
    loop.type = NA_BOT;
    if (Sys_IsLANAddress(loop) != 1) return 10;
    struct { unsigned char addresses[16][4]; unsigned char sentinel[4]; } interfaces{};
    std::memset(interfaces.sentinel, 0xA5, 4);
    int count = -1;
    if (!bo1::android::ReadLocalIPv4(interfaces.addresses, 16, &count) || count < 0 || count > 16) return 11;
    for (auto byte: interfaces.sentinel) if (byte != 0xA5) return 12;
    errno=0;
    if (bo1::android::ReadLocalIPv4(nullptr, 16, &count) || errno != EINVAL) return 13;
    const int before = count;
    errno=0;
    if (bo1::android::ReadLocalIPv4(interfaces.addresses, 0, &count) || errno != EINVAL || count != before) return 14;
    std::printf("actual network addresses: resolution/unchanged-failure/ABI-sentinel/12-LAN-boundaries/loopback-bot/kernel-interfaces passed; IPv4=%d\n", count);
}
