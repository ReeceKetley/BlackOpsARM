# Original network channel, geometry, camera, light and game-state sources

2026-10-13. Full-engine-link192 compiles added sources and fails linking real
remaining services. Actual Com_Init/Com_Frame unexecuted.

Selected whole com_convexhull, net_chan_mp, DynEntity_gamestate, rope_gamestate,
cg_camera, cg_sp_client_ext and r_pointlights. These supply original geometry/
rotation, network-channel initialization/state, dynamic-entity/rope serialization,
camera/SP visual policy and hero light-grid helpers. Existing algorithms,
assertions, disk/network metadata and state ownership retained; no artificial
world/channel/visual readiness or serialization output supplied.

Native network-channel adaptation uses actual existing legacy CRT formatting,
lowercase hexadecimal XUID formatting via unsigned64 std::sprintf and hexadecimal
std::strtoull parsing. Original caller's minimum17-byte output capacity remains
required, as with original _ui64toa; no new truncated identity contract. Windows
calls unchanged. Decimal substitution was considered before verifying source but
not applied: exact source shows radix16, and implementation preserves that radix.
Int64ToString/StringToInt64 legacy word-order helpers remain original/unverified.

FULL inherited Netchan_Init diagnostic excluded because the real canonical body
now supplies it. It registers real network/message compression Dvars, qport and
commands; it has not been executed on Android. Diagnostic OFF preserves its
endpoint. No new successful/unsupported placeholder introduced to force a link.

Evidence:

-190 preserves native underscore formatter/conversion errors;191 corrects them
  and reaches inherited network initializer duplicate.192 resolves duplicate and
  reaches remaining genuine native events/input/network sleep/audio/cinematic
  and further game/client/renderer dependencies.
- takeover_network_geom_camera_windows1 passes all seven whole normal sources
  on Windows x86/x64 exit0 with commands/manifests/build-result.json.

No new runtime probe or device/asset transfers. Native camera/convex-hull/light,
SP visual state, dynamic entity/rope serialization, XUID conversion and actual
network initialization/execution remain unverified. Existing decompiled runtime
pointer/stride paths require actual evidence, not blanket certification from
compilation. Next real platform gates are Sys_GetEvent, Platform_InputFrame and
NET_Sleep, with actual SDL input consumer/event queue integration required; do
not use empty polling or synthetic events as successful platform implementation.
