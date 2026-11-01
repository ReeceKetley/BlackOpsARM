#pragma once
// Engine driver contract; implementations belong to the actual device backend.
// Keeping this interface independent of XAudio2 SDK types permits native ports
// of the original sound core without substituting a successful device service.
struct SndStartAliasInfo;
struct Font_s;
struct snd_voice_t;
void __cdecl SD_TruncateAudioDeviceNames(Font_s *font, float scale, int size);
bool __cdecl SD_Xaudio2CanInit();
#define SND_MAX_STREAM_VOICES 10
#define SND_MAX_VOICES 74
#define SND_PLAYBACKID_NOTPLAYED -1
enum snd_stream_status : __int32 {
    SND_STREAM_UNUSED=0, SND_STREAM_LOADING=1, SND_STREAM_OK=2,
    SND_STREAM_STARVING=3, SND_STREAM_EOF=4, SND_STREAM_ERROR=5
};
void __cdecl SD_StopVoice(int voiceIndex);
char __cdecl SD_Init();
void SD_Shutdown();
void __cdecl SD_PreUpdate();
void __cdecl SD_PauseVoice(int voiceIndex);
void __cdecl SD_UnpauseVoice(int voiceIndex);
__int64 SD_BO1SamplesPlayed(int voiceIndex);
void __cdecl SD_UpdateVoice(unsigned int voiceIndex);
int __cdecl SD_StartAlias(SndStartAliasInfo *startAliasInfo, unsigned int voice);
#if defined(__ANDROID__)
// Joined main-thread checkpoint operations; resident cursors are real buffer offsets.
bool SD_BeginCheckpoint();
bool SD_EndCheckpoint();
bool SD_RefreshCheckpointMix();
bool SD_CanRestoreResidentVoice(int voiceIndex, __int64 sampleCursor);
bool SD_RestoreResidentVoice(int voiceIndex, __int64 sampleCursor);
bool SD_CanRestoreStreamVoice(int voiceIndex, __int64 sampleCursor);
bool SD_RestoreStreamVoice(int voiceIndex, __int64 sampleCursor);
bool SD_PreflightCheckpointVoice(int voiceIndex, const snd_voice_t &voice, __int64 sampleCursor);
bool SD_RestoreCheckpointVoice(int voiceIndex, __int64 sampleCursor);
#endif
