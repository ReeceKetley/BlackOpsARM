# Original client queries and platform policy integration

2026-10-13. Full Com_Init remains incomplete.

Selected original skybox, client debug-data and compositing sources. These
provide their actual storage, debug helpers and real D3D query/fence behavior.
Original CL_IsLocalClientInGame, single-client first-active query, fatal renderer
lock error and PC_StartWithNoSounds moved outside their existing narrow selection
guards, without changing algorithm bodies. Native client selection imports the
canonical client header and existing DXVK Native SDK includes.

PC no-sound state retains one owner and the original game-tool/device-capability
checks; SD_Xaudio2CanInit's existing declaration moves into the neutral driver
contract. The actual native capability implementation remains unresolved. No
always-false capability or forced no-sound value supplied to advance boot.

First-active retains the working reference's single-client assertion and return1
policy. In-game reads actual connection state; no active state is manufactured.
Fatal lock preserves genuine diagnostic/fatal behavior. Fence polling uses the
original device GetData call when a fence exists, and its original null-fence
case when none exists; this does not certify graphics initialization.

## Evidence

ARM sources compile. Full-engine-link93 preserves missing native SDK include;
94/95 reach real unresolved services. Eight full Windows TUs pass x86/x64 in
takeover_client_policy_windows2, including the actual Windows XAudio2 driver
after declaration relocation. Windows1 missing s_nosnd forward declaration
preserved; actual single-owner forward declaration corrected before final checks.

No new tablet probe or transfer. Client state/skybox/debug/compositing execution,
audio capability and full startup remain unverified. Native renderer/audio
registration is not inferred from zero globals or successful compilation.

Continue from95: actual SD_Xaudio2CanInit/device, cinematic, event/input/network,
UI/client/game/script/physics/renderer recovery owners remain. Preserve pinned
DXVK/Vulkan and staged assets; continue authentic source/platform implementation.
