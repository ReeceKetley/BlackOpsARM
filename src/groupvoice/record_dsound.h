#pragma once
#include "directsound.h"

void __cdecl DSOUNDRecord_UpdateSample(dsound_sample_t *pRecSample);
void __cdecl DSOUNDRecord_Frame();
dsound_sample_t *__cdecl DSOUNDRecord_NewSample();
int __cdecl DSOUNDRecord_DestroySample(dsound_sample_t *pRecSample);
HRESULT __cdecl DSOUNDRecord_Start(dsound_sample_t *pRecSample);
HRESULT __cdecl DSOUNDRecord_Stop(dsound_sample_t *pRecSample);
int __cdecl DSOUNDRecord_Init(bool bCallDsoundInit);
void __cdecl DSOUNDRecord_Shutdown();

#if defined(__ANDROID__) || defined(BO1_NATIVE_FAUDIO_HOST_TEST)
bool BO1_VoiceHasOwnedResources();
void BO1_VoiceRetirePlaybackSamples();
bool BO1_VoiceCaptureControls(unsigned level,bool mute);
bool BO1_VoiceCaptureRoute(char *out,unsigned capacity);
#endif
