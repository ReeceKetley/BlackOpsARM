# Native IPv4 resolution and original LAN address policy

2026-10-13. Genuine Com_Init/Com_Frame and socket transport incomplete.

Original win_net owner now selects Sys_StringToAdr, Sys_IsLANAddress_IgnoreSubnet,
Sys_IsLANAddress in full entry-state build through source-scoped
BO1_NATIVE_NET_ADDRESS_POLICY. Windows bodies unchanged. Existing LAN private/
loopback/bot classification and first-three-octet local-prefix policy retained;
native path reads actual current Android UP IPv4 interfaces rather than fabricated
Windows localIP state. This preserves the reference /24-style comparison, not an
interface-netmask redesign. Interface enumeration failure returns non-match rather
than inventing interfaces or LAN status; OS errno remains available.

Android boundary network_addresses.cpp owns actual getaddrinfo/freeaddrinfo and
getifaddrs/freeifaddrs lifetimes. Resolver constrained to AF_INET, copies address
bytes/network port into original16-byte netadr_t and commits output only on success.
Handle index defaults0 for raw address, no fabricated secure router handle. Numeric
input uses AI_NUMERICHOST, so invalid numeric test does not trigger DNS. Named hosts
use real OS resolver. Null/empty/oversized host or invalid output fails; returned
address is not readiness evidence. IPv6/port-string parsing outside this engine API
not added; original netadr is IPv4 and higher-level engine handles port separately.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link325 compiles canonical adapter and native
  providers, full link still fails genuine remaining engine/native dependencies.
- network-address-definitions1.log records three actual engine address owners and
  two OS boundary owners in entry-state/database archives.
- takeover_native_network_address_windows1: one normal whole WinNet TU x86/x64
  exit0 with source manifests.
- network-address-build1/run1: actual canonical win_net adapters plus providers in
  bo1_native_network_address_tests, HONOR UBSan passes numeric/localhost resolution,
  unchanged failed output,16-byte ABI sentinel,12 distinct LAN range boundaries,
  loopback/bot policy, kernel interface enumeration/output bounds/invalid arguments.
  Actual shell namespace reports2 IPv4 interfaces. Only changed49544-byte fixture
  transferred, no game assets or production executable run.

Test covers address services under authorized ADB shell UID, not APK UID/network
permissions, remote DNS availability, interface-change races or public local-prefix
classification during live reconfiguration. Fixed interface-list capacity16 matches
reference policy; enumeration beyond capacity is bounded, not a full inventory.
No UDP send/receive/socket bind or secure transport initialized. Sys_SendPacket,
NET_Init/config/debug streams and network lifecycle remain work; existing real
NET_Sleep wait/provider evidence unchanged. Full engine unlinked and Com_Init has
not returned. Address parsing/IF data does not indicate networking initialized.
