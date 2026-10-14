# Real native UDP ownership and canonical packet/configuration integration

2026-10-13. Full Com_Init/Com_Frame and networking startup remain incomplete.

Android udp_socket owns one real IPv4 datagram descriptor behind a mutex. Socket
created nonblocking/CLOEXEC, actual SO_BROADCAST enabled, bind failure closes
unpublished fd preserving errno; duplicate open fails EALREADY retaining active
owner. Bind default/localhost (case-insensitive) remains reference wildcard policy;
explicit interface uses real resolver. Bound port obtained from kernel. Close
serializes with IO, clears ownership and does not retry Linux close/EINTR.

Real sendto accepts only actual complete datagram sends; absent socket/would-block/
invalid request fails rather than reporting success as reference legacy did for
some absent/transient paths. User no-fake-success direction honored; Windows stays
unchanged. Receive recvmsg MSG_TRUNC distinguishes empty queue from real failure,
rejects truncation and exact-capacity packets as reference policy, commits source/
length only for accepted packets. Failed truncated receives may modify payload
buffer prefix; metadata is not published as a valid packet. Port remains network
order in netadr_t; no secure router handle or network-ready state fabricated.

Eight canonical win_net owners selected in full entry-state: Sys_SendPacket,
Sys_GetPacket, NET_OpenIP, NET_Config, NET_GetDvars, NET_Restart,
NET_SocketPool_Init, NET_Init. Configuration retains original enable/disable/modified
start-stop flow, Dvar names/defaults,10-port retry policy; native stop closes actual
owner, open publishes actual kernel port and guards retry beyond65535. Native IPv4
enumeration already reads current interfaces on LAN queries; old cached Windows
hostname log not used as native interface readiness. NET_Init uses real config and
still calls required NET_InitDebug (unresolved), not a dummy completed debug init.

SOCKS requested on native fails through actual ERR_FATAL and aborts if unexpectedly
returned; it never silently falls back to raw UDP. Secure/DemonWare/router/SOCKS/
NAT-pool transport not implemented by this raw UDP service. Existing socket-pool
init remains original Dvar-only policy, not fabricated allocated pool/socket state.
FULL replaces inherited NET_Init diagnostic only with actual body; OFF unchanged.

Evidence under build-android-audit/native_registry_closure:

- full-engine-link326..327 compile packet/open boundary; initial CMake source was
  briefly inserted into address-test list, corrected to actual database owner
  before build. Actual owners verified; no claim based on unused fixture compile.
- link328 catches NET_Init diagnostic duplicate; corrected FULL exclusion.
  link329 compiles config/init and remains truly unresolved NET_InitDebug/other
  dependencies; link330 final explicit no-SOCKS-fallback branch, still fails links.
- native-udp-packet-open-definitions1/2 logs confirm packet/open/config/init owners
  and five actual OS socket APIs in production archives.
- udp-socket-build1..4/run1..4 preserve scoped fixture history. First two runs reach
  exec test but fail its incorrect assumption FD0 stays absent after exec. Actual
  observation: non-socket FD0 target /dev/null; runtime replaced standard input.
  Corrected fixture directly verifies owned socket at FD0/wildcard port and
  FD_CLOEXEC before exec, then absence of socket (EBADF or exact /dev/null replacement).
  This fixes harness expectation, does not weaken owned-socket closure invariant.
- Final run4 HONOR UBSan passes actual bind conflict/no prior owner, duplicate-open
  preservation, nonblocking receive, bidirectional loopback datagrams/network port,
  truncation/exact-capacity metadata and buffer bounds, closure/no-socket failure,
  valid FD0+CLOEXEC and actual canonical Sys_GetPacket publication/no-packet state.
  Fixture uses actual providers/adapter, no successful transport/error stubs.
- takeover_native_udp_packet_open_windows1/config_init_final_windows1/
  no_socks_fallback_windows1: normal whole WinNet TU x86/x64 exit0 at changes;
  last is final source manifest. No broad prior runtime checks repeated.

Transfers only changed fixture binaries (~1.2-1.33MiB), no game assets/production
entry. Kernel send success is acceptance, not remote delivery acknowledgment.
No broadcast traffic sent; SO_BROADCAST/broadcast send branch not full runtime proof.
Fixture does not execute canonical Sys_SendPacket error branch or Dvar-driven
NET_Init/Config/OpenIP lifecycle. Actual Sys_GetPacket/no-packet metadata exercised.

Full startup still unlinked. NET_InitDebug/read/write/debug restart and wider
socket/network/lifecycle callback closure, app-UID permissions, concurrent close/
restart stress, packet/frame/server consumers and secure/optional transports remain
work. Mutex serializes descriptor IO/close, but fixture is not full game thread
ownership proof. Dvar networkingEnabled is original configuration intent, not a
socket readiness guarantee. No complete network init, Com_Init return or Com_Frame.
