# Original message, config strings, client state and ragdoll update

2026-10-13. Full-engine-link117 compiles selected sources but still fails to link;
actual Com_Init remains incomplete.

Selected whole original msg_mp, huffman, com_clients, com_constantconfigstrings
and ragdoll_update. Original message serialization tables/Huffman algorithm,
clientGameStates and configuration/checksum policies remain canonical. Four FULL
diagnostics are excluded where com_clients now supplies real Com_InitClientGameStates,
Com_LocalClient_GetControllerIndex, Com_LocalClient_SetControllerIndex and
Com_LocalClient_SetPrimary. OFF diagnostic ownership remains as before. No new
placeholders, readiness values or assertion bypasses were introduced.

Native GetMinBitCountForNum uses the existing four-byte BitScanReverse output,
instead of casting an int address to LP64 unsigned long*. Zero handling and
algorithm remain unchanged. Forty-five signed field-code case literals use their
negative decimal representation, preserving original dispatch and bit patterns.
Source CP1252 encoding is preserved.

Native short/long/int64 byte-buffer writes use memcpy for potentially unaligned
positions; Windows retains original stores. Two/four/eight-byte widths remain;
int64 retains its existing LittleLong64 conversion call. This does not implement
or verify that endian service: its non-startup q_shared definition is recursive,
and the selected startup branch omits it. Resolve its genuine owner before any
retained 64-bit message path runs; do not claim its current functionality.

HONOR UBSan message-wire-run1 (build2) runs actual production msg_mp primitive
functions: zero/all32 bit widths, exact combined byte/short/long/float little-endian
wire bytes with deliberately unaligned stores, signed short read, long crossing
the split-buffer boundary, float representation, read/write overflow/cursor and
adjacent bounds. The fixture initializes a raw msg_t buffer; it does not execute
MSG_Init or Huffman initialization, compression, string conversion, int64 IO or
full snapshots. It links existing engine unavailable_services only for fail-on-use
assertion handling; an assertion would exit78, never bypass failure. No test-only
success service was added. Draft build1's buffer offset was corrected before
execution to ensure stores were actually misaligned; final build2 passed run1.
Only this test binary was transferred; game assets were reused.

All five whole normal Windows TUs pass x86/x64 compile checks in
takeover_message_config_windows1. Final memcpy additions are Android-only; Windows
branches retain the tested code. No additional Windows runtime parity claim.

Logs114..117 preserve BitScan/signed-case compilation corrections, four duplicate
diagnostic corrections and final link failure. Broader unresolved graph still
includes audio device/capability, cinematic IO, dynamic pieces, client-game assets,
UI/string-table services, memory, renderer/game/input/network. Continue selecting
actual source owners and implementing real platform boundaries. Com_Init/Com_Frame
completion requires actual production execution; source/probe success is narrower.
