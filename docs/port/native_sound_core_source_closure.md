# Original sound core and neutral driver contract

2026-10-13. Full Com_Init remains incomplete; no audio initialization/playback proof.

Full native library now selects original snd.cpp and snd_occlusion.cpp. The core
owns actual g_snd, initialization/policy, voice/fader/control logic and shutdown.
Inherited unavailable SND_Init is excluded only from FULL_DATABASE builds now
that the original initializer is selected. OFF diagnostics remain explicit.

Moved the existing nine backend declarations, stream status enum and three voice/
playback constants into canonical snd_driver_api.h, included by both core/stream
and the XAudio2-specific header. Original declarations are removed from the latter
to keep one owner. No driver function implemented as a stand-in; actual SD_*/
stream/device owners remain unresolved. This allows original engine policy to
compile independently of Windows COM/DSP types, preserving backend contracts.

Native CRT formatting/comparison adapted. Original shutdown delay uses the real
non-alertable native SleepEx. Occlusion control reads/stores use native four-byte
atomics; callback launch uses a correctly typed adapter invoking the original
no-argument callback. Windows state accesses/operations retain original behavior.

## Verification and limits

Native sources compile; full-engine-link87 fails on actual remaining production
services, including audio driver, stream, occlusion events/worker and broader
client/physics/cinematic/input/network/recovery. Logs83..85 preserve SDK-header/
constant/Sleep compile errors,86 preserves duplicate SND_Init,87 corrected.
Whole core/occlusion/XAudio2 implementation TUs pass normal Windows x86/x64 in
takeover_sound_core_windows2, verifying the neutral declaration relocation still
compiles the real Windows backend. Initial run1 preserved as earlier evidence.

No new tablet test or transfer. Active sound path, worker retirement, streaming
and device initialization remain unverified. The original shutdown sleep does
not prove occlusion-worker join or safe retirement; native worker integration
must establish that lifecycle before exercising active sound shutdown.

Actual SND_ShouldInit preserves PC_StartWithNoSounds and dedicated/FastFile policy.
Actual SND_Init clears/initializes state and sets g_snd.init from SD_Init only
within its permitted active path. No forced true init or unconditional event
publication was added to advance boot. Source selection is not successful startup.

Continue real closure from87: stream/worker/device backend and cinematic IO, plus
remaining engine owners. Preserve pinned DXVK/Vulkan requirements; no game asset
changes or fake hardware information.
