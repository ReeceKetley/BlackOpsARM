# Native debug TCP transport

The canonical debug transport section of `src/win32/win_net.cpp` is now selected
by the existing `BO1_NATIVE_UDP_ENGINE` gate. Android socket operations use
`src/platform/android/debug_tcp.{h,cpp}`; Windows socket bodies remain guarded.
Root integration owns production CMake selection and the normal Windows checks.

## Implementation

The native provider owns real IPv4 TCP descriptors with nonblocking/CLOEXEC
creation and acceptance. Engine zero means empty; a live handle is descriptor+1,
so descriptor zero is valid. Failed setup closes its descriptor while retaining
the original errno. Connect completion uses a bounded monotonic poll and
`SO_ERROR`, including refused connections. Listeners use `SO_REUSEADDR` to
support actual shutdown/rebind. Send uses `MSG_NOSIGNAL`; interrupted I/O retries
without retrying close. Read/write reject null nonempty buffers and lengths above
`INT_MAX`, preserving the exported integer result contract.

Original listen/connect commands, message caching/acknowledgement, packet
buffering, script connection and debug flags remain the engine owners. Native
branches assemble complete writes and four-byte acknowledgement frames, handle
EOF as a real error/restart, and use defined wrapping fixed32 counter arithmetic.
The original source resets `g_debugReadBytes` to null and subsequently dereferences
it; the native stream initializer supplies owned uint32 storage instead. Repeated
initialization retires existing descriptors; shutdown resets sockets, flags,
message/packet state and counters. Native string reads validate the wire length
and return failure before copying incomplete/oversized data. Android never invokes
the desktop foreground-lock system setting.

## Evidence

All logs below are under `build-android-audit/native_registry_closure/`.

- `debug-tcp-compile5.log`: production win_net ARM64/API28 TU compiles using
  the actual selected target command, with UBSan and function instrumentation.
- `debug-tcp-fixture-canonical-build.log`: same canonical TU for the fixture,
  with only function instrumentation omitted (actual debug code and UBSan retained).
- `debug-tcp-fixture-build7.log`: final isolated fixture links successfully with
  the native provider and canonical object, NDK30.0.16248370/API28, UBSan,
  no sanitizer recovery and static C++ runtime.
- `debug-tcp-honor-run5.log`: actual HONOR AKWJ9X5116G04618 execution exits0.
  It exercises descriptor-zero listener ownership, accepted NONBLOCK/CLOEXEC,
  exported argument bounds, empty nonblocking read, fragmented exact read,
  cumulative acknowledgements, 40KB constrained-buffer complete stream send,
  fragmented 1+3-byte remote acknowledgement, counter wrap, EOF closing both
  channels/listeners with restart, canonical accept/no-peer/message-cache/Ack,
  refused connect and final shutdown.
- `native-debug-tcp-tests` SHA256:
  `3c8f45670ad4c6c30c1732161731a5356dd8288ab3d0397015ff5bcaa3aeafcc`.
  Only this changed binary was transferred to
  `/data/local/tmp/blackopsarm-native-debug-tcp-tests`, chmod700.

`tools/port/tests/native_debug_tcp_tests.cpp` uses isolated stderr formatting,
assertion failure and real sleep providers; it executes real sockets and canonical
stream functions. It does not instantiate the engine or fake initialization.
Early fixture runs1/2 failed expectations: TCP availability was assumed immediate,
then an intermediate legitimate cumulative acknowledgement was assumed absent.
Those logs remain preserved; fixture polling and ordered acknowledgement checks
were corrected. Final run5 covers the final provider bounds additions.

## Limits

This is scoped shell-UID loopback transport proof, not Com_Init/Com_Frame or a
live script-debugger session. Actual command/dvar/script initialization, remote
host/device delivery, APK permissions, arbitrary concurrent lifecycle callers,
disconnect during buffered engine writes, all allocation/setup failures and
signal-driven EINTR injection remain unverified. Existing engine critical-section
and debug-thread ownership assumptions apply. The canonical blocking debug API
continues waiting for a live peer's flow-control acknowledgements; no artificial
timeout or readiness is added. Full engine linkage/execution, secure/backend/SOCKS
transport and game packet/frame consumption remain separate gates.
