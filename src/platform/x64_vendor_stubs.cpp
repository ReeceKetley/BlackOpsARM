// x64 link stubs for vendor libs that only ship as Win32 (Bink, Steam, libvpx).
// NvAPI uses nvapi64.lib from CMake. Cinematics / Steam / VPX encode are
// non-essential for the Zombies gameplay bring-up path.

#include <cstdint>

#if defined(BO1_PORT_X64) || defined(_M_X64) || defined(_WIN64)

extern "C" {

// ---- Bink (RADEXPLINK == __stdcall on Win32; ignored on x64) ----
using HBINK = void *;
using S32 = int;
using U32 = unsigned int;

char *__stdcall BinkGetError(void) { return const_cast<char *>("bink stub"); }
HBINK __stdcall BinkOpen(const char *, U32) { return nullptr; }
void __stdcall BinkGetFrameBuffersInfo(HBINK, void *) {}
void __stdcall BinkRegisterFrameBuffers(HBINK, void *) {}
S32 __stdcall BinkDoFrame(HBINK) { return 0; }
void __stdcall BinkNextFrame(HBINK) {}
S32 __stdcall BinkWait(HBINK) { return 0; }
void __stdcall BinkClose(HBINK) {}
S32 __stdcall BinkPause(HBINK, S32) { return 0; }
void __stdcall BinkSetVolume(HBINK, U32, S32) {}
S32 __stdcall BinkShouldSkip(HBINK) { return 0; }
S32 __stdcall BinkControlBackgroundIO(HBINK, U32) { return 0; }
void __stdcall BinkGetRealtime(HBINK, void *, U32) {}
void __stdcall BinkSetSoundTrack(U32, U32 *) {}
void __stdcall BinkSetIOSize(U32) {}
S32 __stdcall BinkSetSoundSystem(void *, void *) { return 0; }
void *__stdcall BinkOpenDirectSound(U32) { return nullptr; }
void __stdcall BinkSetMemory(void *(__stdcall *)(U32), void(__stdcall *)(void *)) {}

// ---- Steam API ----
using HSteamUser = int;

HSteamUser SteamAPI_GetHSteamUser() { return 0; }
void SteamAPI_RunCallbacks() {}
void SteamAPI_Shutdown() {}
void SteamAPI_RegisterCallback(void *, int) {}
void SteamAPI_UnregisterCallback(void *) {}
void SteamAPI_RegisterCallResult(void *, std::uint64_t) {}
void SteamAPI_UnregisterCallResult(void *, std::uint64_t) {}
void SteamGameServer_RunCallbacks() {}
HSteamUser SteamGameServer_GetHSteamUser() { return 0; }
// The SDK's SteamFriends()/SteamUser()/... inlines read interface pointers out of the CSteamAPIContext this returns; a
// zeroed context makes every accessor return null (callers already handle "Steam not running") instead of faulting.
void *SteamInternal_ContextInit(void *) { static alignas(16) unsigned char s_zeroContext[1024]; return s_zeroContext; }
void *SteamInternal_CreateInterface(const char *) { return nullptr; }
void *SteamInternal_FindOrCreateUserInterface(HSteamUser, const char *) { return nullptr; }
void *SteamInternal_FindOrCreateGameServerInterface(HSteamUser, const char *) { return nullptr; }
int SteamInternal_SteamAPI_Init(const char *, void *) { return 1; } // FailedGeneric

// ---- libvpx encode path ----
int vpx_img_alloc(void *, int, unsigned, unsigned, unsigned) { return 0; }
void vpx_img_free(void *) {}
int vpx_codec_destroy(void *) { return 0; }
int vpx_codec_enc_init_ver(void *, void *, const void *, long, int) { return -1; }
int vpx_codec_enc_config_default(void *, void *, unsigned) { return -1; }
int vpx_codec_encode(void *, const void *, std::int64_t, unsigned long, long, unsigned long)
{
    return -1;
}
const void *vpx_codec_get_cx_data(void *, const void **) { return nullptr; }
// Taken as an interface pointer; never dereferenced if encode init fails.
char vpx_codec_vp8_cx_algo;

} // extern "C"

#endif
