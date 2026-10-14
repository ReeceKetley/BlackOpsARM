# Original profile, platform live, group, session and user-info integration

2026-10-13. Full executable still cannot link; actual Com_Init/Com_Frame incomplete.

Full target selects canonical live_win.cpp, live_groups_dw.cpp,
live_sessions_win.cpp, win_gamerprofile.cpp and universal/UserInfo.cpp. Existing
platform Dvar registration, local SP profile login, profile/Dvar synchronization,
group queries, session owners and profile constructors now have their original
implementations selected. Only full-target Live_LogInProfileLocally_SP and
PC_InitSigninState diagnostics are excluded where real source owners replace
them. Live_Init itself still needs its separate original owner; no replacement
or claim of complete live initialization.

Existing BO1_LIVE/BO1_LIVE_STUBS/BO1_USERINFO conditionals and accepted reference
stubs are unchanged. Original local SP profile behavior is distinct from Steam/
DemonWare sign-in; no native online authentication/readiness or successful fetch
has been invented. Native session, profile, group and backend lifecycles remain
unexecuted, including retained decompiled strides and conditional backend code.

Shared controller Dvar declarations move from the XInput-dependent win_gamepad
header to canonical platform-neutral client/cl_gamepad.h, which the Windows
header already includes. Native gamerprofile includes the declaration owner
without requiring XInput hardware types. Definitions remain in the real gamepad
source, currently requiring Android device integration; no fake Dvars created.
Native profile button-config comparison uses existing canonical I_stricmp;
Windows keeps _stricmp. Profile state and log-in checks are not forced successful.

Four native favourites adaptations retain the 42 entries and fixed saved blob
formats: six address bytes and eight UID bytes per occupied entry, 253/337 byte
buffers including spare zero bytes. Add/delete use the actual uint64 UID field
instead of hard-coded executable addresses/32-bit halves. Parse/write use memcpy
for potentially unaligned packed blobs. Parsing tests the full UID for the empty
sentinel, preserving valid IDs whose low byte is zero rather than prematurely
stopping. Packing compacts occupied entries and preserves reference callback
conditions, including refresh when adding to a full list. Windows original code
remains untouched. The parser retains the reference's existing tail handling;
these changes do not claim backend profile replacement or persistence semantics.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link341.log records unwanted XInput header
  dependency; link342 records CRT _stricmp dependency; link343 records duplicate
  local-signin diagnostics. link344 corrects diagnostics and compiles the original
  four owners. link345 compiles the required UserInfo constructor owner and is the
  latest production attempt; genuine remaining dependencies still prevent linking.
- live-profile-sessions-groups-definitions1.log verifies actual profile/local
  sign-in/favourites/group and PrivateProfileInfo constructor owners.
- takeover_live_profile_sessions_groups_windows1: five normal whole TUs (four
  selected sources plus win_gamepad header consumer) pass both Windows ABIs.
  takeover_live_profile_userinfo_windows1: normal UserInfo TU passes both ABIs.
  Final manifests and exit0 retained for all scopes.
- live-favourites-build1.log records fixture callback duplicate; build2 uses
  linker wrapping only in the fixture and identifies the genuine required
  constructor. build3 selects canonical UserInfo and passes. No dummy constructor.
- live-favourites-push1.log/run1.log: focused CMake fixture with actual live_win
  and UserInfo sources, GC/UBSan; HONOR AKWJ9X5116G04618 ADB-shell UID exit0.
  High-bit/zero-low-byte UID, exact six-byte address encoding, all 42 slots, full
  rejection, add/delete callback conditions, compact gap handling and unaligned
  full blob round trips with boundary canaries pass. The fixture wraps only the
  client refresh callback to observe invocation; production retains its original
  function. Only this new executable transferred; no repeated game asset transfer.

No real favourites client refresh/backend persistence, initialized profile/Dvars,
session tasks/voice, group membership, online identity or APK controller ownership
was executed. Canonical body selection and scoped packed-memory tests are not
whole startup proof. Renderer/audio/cinematic/Steam/network debug/Monkey/platform
and other source dependencies remain genuine blockers; see latest full link log.
